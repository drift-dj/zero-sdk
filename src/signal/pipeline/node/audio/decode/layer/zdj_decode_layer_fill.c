#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>

#include <libavformat/avformat.h>
#include <libavcodec/packet.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavcodec/codec_id.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

static void _fill( zdj_decode_layer_t * layer, zdj_pipeline_node_t * node );
static void _seek_and_decode( 
    zdj_decode_layer_t * layer, 
    zdj_pipeline_node_t * node, 
    zdj_decode_profile_t * profile
);

// Addr helpers
static bool _layer_end_gt_win_end( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_end 
);

static bool _layer_end_lt_win_end( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_end 
);
static bool _layer_start_inside_win( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_start, 
    zdj_decode_addr_t * win_end 
);
static bool _layer_end_inside_win( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_start, 
    zdj_decode_addr_t * win_end 
);
static bool _layer_last_packet_gt_win_end( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_end 
);

static bool _layer_last_packet_lt_win_end( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_end 
);
static bool _layer_last_packet_lt_layer_end( zdj_decode_layer_t * layer );
static bool _layer_last_packet_gt_layer_end( zdj_decode_layer_t * layer );
static bool _layer_last_packet_inside_win( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_start, 
    zdj_decode_addr_t * win_end 
);
static bool _layer_start_lt_win_start( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_start 
);
static bool _layer_start_gt_win_start( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_start 
);
static bool _layer_first_packet_gt_layer_start( zdj_decode_layer_t * layer );
static bool _layer_first_packet_lt_layer_start( zdj_decode_layer_t * layer );
static bool _layer_first_packet_gt_win_start( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_start 
);
static bool _layer_first_packet_lt_win_start( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_start 
);
static bool _layer_first_packet_lt_win_end( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_end 
);

void zdj_decode_layer_init_fill_api( zdj_decode_layer_t * layer ) {
    layer->fill = &_fill;
    layer->seek_and_decode = &_seek_and_decode;
}

static void _fill( zdj_decode_layer_t * layer, zdj_pipeline_node_t * node ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    zdj_deck_control_loop_state_t * loop_state = (zdj_deck_control_loop_state_t*)state->loop_state;
    zdj_deck_control_skip_state_t * skip_state = (zdj_deck_control_skip_state_t*)state->skip_state;

    bool debug = false;

    // Capture current node win addresses
    zdj_decode_addr_t win_start; state->get_win_start_addr( node, &win_start );
    zdj_decode_addr_t win_end; state->get_win_end_addr( node, &win_end );

    // Any discontinuous jump in seek may require garbage packets from the decoder
    bool seek_has_discontinuity = false;

    // First remove any packets outside the window
    while( layer->can_remove_packet( layer, node, layer->first_packet ) ) {
        layer->remove_packet( layer, layer->first_packet );
    }
    while( layer->can_remove_packet( layer, node, layer->last_packet ) ) {
        layer->remove_packet( layer, layer->last_packet );
    }
    if( layer->is_empty( layer ) ) {
    }
    //Decode Profile holds the params for the seek and decode call
    zdj_decode_profile_t profile;

    /////////////////////////////
    // EMPTY/NEW LAYER CASES:
    if( layer->is_empty( layer ) ) {
        // Layer Lead In Start < Win Start &&
        // Layer Lead Out End > Win End
        // Quantize Seek Targ. fwd from 0 by est.pkt.size to include win start coord
        // Packet Count to cover entire window
        // Can be a Contig. or Loop layer
        // --v
        // __|________________|__
        //   |________________|
        if( _layer_start_lt_win_start( layer, &win_start ) &&
            _layer_end_gt_win_end( layer, &win_end )
        ) {
            double raw_seek_target = floor( win_start.origin_d / state->estimated_packet_sample_count ) * state->estimated_packet_sample_count;
            profile.seek_target = (int64_t)raw_seek_target;
            profile.packet_count = ceil( ( win_end.origin_d - raw_seek_target ) / state->estimated_packet_sample_count );
            profile.dir = ZDJ_DECODE_DIR_FWD;
            profile.is_valid = true;

            if( debug ) { printf( "%p _[%d>___]_\n", layer, profile.packet_count ); }
        }
        // Layer Lead In Start in Win
        // Seek Targ. = Loop Start
        // Packet Count to cover remainder of win.
        //     v
        //     *_____________|_
        //  |________________|
        // THIS FILLS A NEW LAYER WHEN PLAYING FORWARD
        // Can be a Contig., Loop, or Skip layer
        else if( _layer_start_inside_win( layer, &win_start, &win_end ) ) { 
            if( layer->back_discon_type == ZDJ_DECODE_LAYER_DISCON_TYPE_LOOP ) {
                profile.seek_target = floor( loop_state->start_origin_d/state->estimated_packet_sample_count ) * state->estimated_packet_sample_count;
                profile.packet_count = ceil( (win_end.origin_d-loop_state->start_origin_d) / state->estimated_packet_sample_count );
            } else if( layer->back_discon_type == ZDJ_DECODE_LAYER_DISCON_TYPE_SKIP ) {
                profile.seek_target = floor( layer->lead_in_start.origin_d/state->estimated_packet_sample_count ) * state->estimated_packet_sample_count;
                profile.packet_count = ceil( (win_end.origin_d-layer->lead_in_start.origin_d) / state->estimated_packet_sample_count );
            } else if( layer->back_discon_type == ZDJ_DECODE_LAYER_DISCON_TYPE_NONE ) {
                profile.seek_target = 0;
                profile.packet_count = ceil( win_end.origin_d / state->estimated_packet_sample_count );
            }
            profile.dir = ZDJ_DECODE_DIR_FWD;
            profile.is_valid = true;

            if( debug ) { printf( "%p  [ |>%d_]_\n", layer, profile.packet_count ); }
        }
        // Layer Lead Out End in Win
        // Quantize Seek Targ fwd from 0 by est.pkt.size to include win start coord
        // Packet Count to cover to end of layer
        // --v
        // __|__________*
        //   |________________|
        // THIS FILLS A NEW LOOP LAYER WHEN SCRUBBING BACKWARD
        // Should only be a Loop layer
        else if( _layer_end_inside_win( layer, &win_start, &win_end ) ) {
            double raw_seek_target = floor( win_start.origin_d / state->estimated_packet_sample_count ) * state->estimated_packet_sample_count;
            profile.seek_target = (int64_t)raw_seek_target;
            profile.packet_count = ceil( ( layer->lead_out_end.origin_d - win_start.origin_d ) / state->estimated_packet_sample_count );
            profile.dir = ZDJ_DECODE_DIR_FWD;
            profile.is_valid = true;

            if( debug ) { printf( "%p _[%d>_| ]\n", layer, profile.packet_count ); }

        }

        // DELETE CASES
        else if( _layer_start_lt_win_start( layer, &win_start ) &&
                 !_layer_end_inside_win( layer, &win_start, &win_end )
        ) {
            if( debug ) { printf( "%p _[     ]\n", layer ); }

            profile.is_valid = false; 
        }
        else if( !_layer_start_inside_win( layer, &win_start, &win_end ) &&
                 _layer_end_gt_win_end( layer, &win_end )
        ) {
            if( debug ) { printf( "%p  [     ]_\n", layer ); }

            profile.is_valid = false; 
        }
        // UNDEFINED CASES
        else { 

            printf( "%p (empty) UNDEFINED!!! %1.0f[ ]%1.0f %1.0f|_|%1.0f\n", 
                layer,
                win_start.transport_d,
                win_end.transport_d,
                layer->lead_in_start.transport_d,
                layer->lead_out_end.transport_d
            ); 

            profile.is_valid = false; 
        }
        /////////////////////////////
      

    /////////////////////////////
    // NON EMPTY LAYER CASES:
    } else {
        // Layer Lead Out End in Win &&
        // Last Packet End < Layer End
        // Seek Targ = Last Packet End
        // Count to cover rest of layer
        //        v
        // __=====|______*
        //   |________________|
        if( _layer_end_inside_win( layer, &win_start, &win_end ) &&
            _layer_start_lt_win_start( layer, &win_start ) &&
            _layer_last_packet_inside_win( layer, &win_start, &win_end ) &&
            _layer_last_packet_lt_layer_end( layer )
        ) {
            profile.seek_target = layer->last_packet->end_addr.origin_i;
            profile.packet_count = ceil( ( layer->lead_out_end.transport_d - layer->last_packet->end_addr.transport_d ) / state->estimated_packet_sample_count );
            profile.dir = ZDJ_DECODE_DIR_FWD;
            profile.is_valid = true;

            if( debug ) { printf( "%p _[=%d>| ]\n", layer, profile.packet_count ); }
            if( debug && profile.packet_count == 0 ) {
                printf( "%1.0f[ l:%1.0f/p:%1.0f == p:%1.0f/l:%1.0f ]%1.0f\n", 
                    win_start.transport_d,
                    layer->lead_in_start.transport_d,
                    layer->first_packet->start_addr.transport_d,
                    layer->last_packet->end_addr.transport_d,
                    layer->lead_out_end.transport_d,
                    win_end.transport_d
                ); 
            }
        }
        // Layer Lead Out End > Win End &&
        // Last Packet End in Win &&
        // First Packet Start < Win Start
        // Seek Targ = Last Packet End
        // Count to cover rest of layer
        //       V
        // __====|________________
        //   |________________|
        else if( _layer_end_gt_win_end( layer, &win_end ) &&
                 _layer_start_lt_win_start( layer, &win_start ) &&
                 _layer_last_packet_lt_win_end( layer, &win_end )
        ) {
            profile.seek_target = layer->last_packet->end_addr.origin_i;
            profile.packet_count = ceil( ( win_end.transport_d - layer->last_packet->end_addr.transport_d ) / state->estimated_packet_sample_count );
            profile.dir = ZDJ_DECODE_DIR_FWD;
            profile.is_valid = true;

            if( debug ) { printf( "%p _[===%d>]_\n", layer, profile.packet_count ); }
        }
        // Layer Lead In Start within Win &&
        // Last Packet End in Win
        // Seek Targ = Last Packet End
        // Count to cover rest of layer
        //             V
        //       *=====|_________
        //   |________________|
        else if( _layer_start_inside_win( layer, &win_start, &win_end ) &&
                 _layer_last_packet_lt_win_end( layer, &win_end )
        ) {
            profile.seek_target = layer->last_packet->end_addr.origin_i;
            profile.packet_count = ceil( ( win_end.transport_d - layer->last_packet->end_addr.transport_d ) / state->estimated_packet_sample_count );
            profile.dir = ZDJ_DECODE_DIR_FWD;
            profile.is_valid = true;

            if( debug ) { printf( "%p  [ |=%d>]_\n", layer, profile.packet_count ); }
        }
        // Layer Lead In Start < Win Start
        // quantize back from first_packet by est.pkt.size
        // to include win start coord
        //   v------
        // *________===========__
        //   |________________|
        else if( _layer_start_lt_win_start( layer, &win_start )  &&
                //  _layer_start_gt_win_start( layer, &win_start ) &&
                 _layer_first_packet_lt_win_end( layer, &win_end ) &&
                 _layer_first_packet_gt_win_start( layer, &win_start )
        ) {
            int seek_offset = layer->first_packet->start_addr.transport_d - win_start.transport_d;
            profile.packet_count = ceil( (double)seek_offset / state->estimated_packet_sample_count );
            double raw_seek_target = layer->first_packet->start_addr.origin_d - ( profile.packet_count * state->estimated_packet_sample_count );
            profile.seek_target = (int64_t)fmax( 0.0, raw_seek_target );
            profile.dir = ZDJ_DECODE_DIR_BACK;
            profile.is_valid = true;

            if( debug ) { printf( "%p _[<%d===]_\n", layer, profile.packet_count ); }
        }
        // Layer Lead In Start within Win &&
        // First Packet < Win End &&
        // First Packet > Layer Start
        // quantize back from first_packet by est.pkt.size
        // to include lead-in start coord
        //      v------
        //      *______========___
        //   |________________|
        else if( _layer_start_inside_win( layer, &win_start, &win_end ) &&
                 _layer_first_packet_lt_win_end( layer, &win_end ) &&
                 _layer_first_packet_gt_layer_start( layer )
        ) {
            int seek_offset = layer->first_packet->start_addr.transport_d - layer->core_start.transport_d;
            profile.packet_count = ceil( (double)seek_offset / state->estimated_packet_sample_count );
            double raw_seek_target = layer->first_packet->start_addr.origin_d - ( profile.packet_count * state->estimated_packet_sample_count );
            profile.seek_target = (int64_t)fmax( 0.0, raw_seek_target );
            profile.dir = ZDJ_DECODE_DIR_BACK;
            profile.is_valid = true;

            if( debug ) { printf( "%p  [ |<%d=]_\n", layer, profile.packet_count ); }

        }
        // Win inside Layer &&
        // Layer already filled
        // _==================_
        //  |________________|
        else if( _layer_start_lt_win_start( layer, &win_start ) &&
                 _layer_end_gt_win_end( layer, &win_end ) &&
                 _layer_first_packet_lt_win_start( layer, &win_start ) &&
                 _layer_last_packet_gt_win_end( layer, &win_end )
        ) {
            if( debug ) { printf( "%p _[=====]_\n", layer ); }
        }
        // Layer Start inside Win &&
        // Layer already filled
        //     *==============___
        //  |________________|
        else if( _layer_start_gt_win_start( layer, &win_start ) &&
                 _layer_end_gt_win_end( layer, &win_end ) &&
                 _layer_first_packet_lt_layer_start( layer ) &&
                 _layer_last_packet_gt_win_end( layer, &win_end )
        ) {
            if( debug ) { printf( "%p  [ |===]_\n", layer ); }
        }
        // Layer End inside Win &&
        // Layer already filled
        // _===========*
        //  |________________|
        else if( _layer_start_lt_win_start( layer, &win_start ) &&
                 _layer_end_lt_win_end( layer, &win_end ) &&
                 _layer_first_packet_lt_win_start( layer, &win_start ) &&
                 _layer_last_packet_gt_layer_end( layer )
        ) {
            if( debug ) { printf( "%p _[===| ]\n", layer ); }
        }  
        /// UNDEFINED CASES
        else { 
            if( debug ) { 
                printf( "%p (non-empty) UNDEFINED!!! %1.0f[ l:%1.0f/p:%1.0f == p:%1.0f/l:%1.0f ]%1.0f\n", 
                    layer,
                    win_start.transport_d,
                    layer->lead_in_start.transport_d,
                    layer->first_packet->start_addr.transport_d,
                    layer->last_packet->end_addr.transport_d,
                    layer->lead_out_end.transport_d,
                    win_end.transport_d
                ); 
            };
            profile.is_valid = false; 
        }
        /////////////////////////////
    }

    if( profile.is_valid &&
        profile.packet_count > 0 ) {
        // This doesn't work.  Need to find a way to flag this
        profile.seek_has_discontinuity = true;

        profile.av_seek_flag = AVSEEK_FLAG_FRAME;
        profile.requires_garbage = state->requires_garbage;
        layer->seek_and_decode( layer, node, &profile );
    }
}

static void _seek_and_decode( 
    zdj_decode_layer_t * layer, 
    zdj_pipeline_node_t * node, 
    zdj_decode_profile_t * profile
) {
    // printf( "seek_and_decode pkts:%d tgt:%lu\n", profile->packet_count, profile->seek_target );
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;

    // Note that we handle the garbage packet decode internally here
    if( profile->requires_garbage ) { 
        profile->seek_target -= (node_state->estimated_packet_sample_count * 3);
        if( profile->seek_target < 0 ) { profile->seek_target = 0; } 
    }

    av_seek_frame( node_state->fmt_ctx, 0, profile->seek_target*node_state->av_timebase_factor, profile->av_seek_flag );

    if ( profile->requires_garbage && profile->seek_target > 0 ) {
        // printf( "decoding 3 garbage packets\n" ); 
        // Decode and discard 3 frames
        zdj_decode_garbage_packet( 
            node, layer, node_state->fmt_ctx, node_state->codec_ctx 
        );
        zdj_decode_garbage_packet( 
            node, layer, node_state->fmt_ctx, node_state->codec_ctx 
        );
        zdj_decode_garbage_packet( 
            node, layer, node_state->fmt_ctx, node_state->codec_ctx 
        );
    }

    if( profile->dir == ZDJ_DECODE_DIR_FWD ) {
        for( int i=0; i<profile->packet_count; i++ ) {  
            zdj_decode_packet_t * packet = zdj_decode_packet( 
                node, layer, node_state->fmt_ctx, node_state->codec_ctx, node_state->av_timebase_factor,
                (i==0) ? ZDJ_DECODE_JUSTIFY_RIGHT : ZDJ_DECODE_JUSTIFY_LEFT
            );
            if( packet->has_decode_error ) {
                // Packets w/decode error need to have addrs manually set.
                // They will be silent during accum phase, but addrs still
                // need to be correct.
                int64_t start_origin_i_coord;
                int64_t end_origin_i_coord;
                if( !layer->is_empty( layer ) ) { 
                    // If there's a last_packet in layer, pull addrs from there.
                    start_origin_i_coord = layer->last_packet->end_addr.origin_i;
                    end_origin_i_coord = start_origin_i_coord + node_state->estimated_packet_sample_count;
                } else { 
                    // If layer is empty, back up from layer core_end
                    end_origin_i_coord = layer->core_end.origin_i;
                    start_origin_i_coord = end_origin_i_coord - node_state->estimated_packet_sample_count;
                }

                node_state->addr_for_origin_i_coord_in_layer( 
                    node, layer, &packet->start_addr, start_origin_i_coord
                );
                node_state->addr_for_origin_i_coord_in_layer( 
                    node, layer, &packet->end_addr, end_origin_i_coord 
                );
            }
            layer->append_packet( layer, packet ); 
        }

    } else if ( profile->dir == ZDJ_DECODE_DIR_BACK ) {

        for( int i=0; i<profile->packet_count; i++ ) {  
            
            zdj_decode_packet_t * packet = zdj_decode_packet( 
                node, layer, node_state->fmt_ctx, node_state->codec_ctx, node_state->av_timebase_factor,
                (i==0) ? ZDJ_DECODE_JUSTIFY_RIGHT : ZDJ_DECODE_JUSTIFY_LEFT
            );
            if( packet->has_decode_error ) {
                printf( "BACK_FILL PACKET HAS DECODE ERROR!!!\n" );
                // This is bad.  I don't know how to handle this.
                // This will corrupt the layer addressing, I think.
            }
            if( i == 0 ) {
                layer->prepend_packet( layer, packet );
            } else {
                layer->insert_packet_after( layer, packet, layer->first_packet );
            }
        }
    } 

    // printf( "layer seek_and_decode done\n" );
}

// NOTE: These are all in TRANSPORT_D
static bool _layer_end_gt_win_end( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_end 
) {
    return layer->lead_out_end.transport_d > win_end->transport_d;
}

static bool _layer_end_lt_win_end( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_end 
) {
    return layer->lead_out_end.transport_d < win_end->transport_d;
}

static bool _layer_start_inside_win( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_start, 
    zdj_decode_addr_t * win_end 
) {
    return layer->lead_in_start.transport_d > win_start->transport_d &&
           layer->lead_in_start.transport_d < win_end->transport_d;
}

static bool _layer_end_inside_win( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_start, 
    zdj_decode_addr_t * win_end 
) { 
    return layer->lead_out_end.transport_d > win_start->transport_d &&
           layer->lead_out_end.transport_d < win_end->transport_d;
}

static bool _layer_last_packet_inside_win( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_start, 
    zdj_decode_addr_t * win_end 
) {
    if( !layer->last_packet || layer->last_packet->has_decode_error ) {
        printf( "_layer_last_packet_inside_win UNDEFINED!!!\n" );
        return false;
    }
    return layer->last_packet->end_addr.transport_d > win_start->transport_d &&
           layer->last_packet->start_addr.transport_d < win_end->transport_d;
}

static bool _layer_last_packet_gt_win_end( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_end 
) {
    if( !layer->last_packet || layer->last_packet->has_decode_error ) {
        printf( "_layer_last_packet_inside_win UNDEFINED!!!\n" );
        return false;
    }
    return layer->last_packet->end_addr.transport_d > win_end->transport_d;
}

static bool _layer_last_packet_lt_win_end( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_end 
) {
    if( !layer->last_packet || layer->last_packet->has_decode_error ) {
        printf( "_layer_last_packet_inside_win UNDEFINED!!!\n" );
        return false;
    }
    return layer->last_packet->end_addr.transport_d < win_end->transport_d;
}

static bool _layer_last_packet_lt_layer_end( zdj_decode_layer_t * layer ) {
    if( !layer->last_packet || layer->last_packet->has_decode_error ) {
        printf( "_layer_last_packet_lt_layer_end UNDEFINED!!!\n" );
        return false;
    }
    // double offset = layer->lead_out_end.transport_d - layer->last_packet->end_addr.transport_d;
    // return offset > ZDJ_SOUNDCARD_BUF_LEN / 2; // This is totally arbitrary.
    return layer->last_packet->end_addr.transport_d < layer->lead_out_end.transport_d;
}

static bool _layer_last_packet_gt_layer_end( zdj_decode_layer_t * layer ) {
    if( !layer->last_packet || layer->last_packet->has_decode_error ) {
        printf( "_layer_last_packet_lt_layer_end UNDEFINED!!!\n" );
        return false;
    }
    // double offset = layer->last_packet->end_addr.transport_d - layer->lead_out_end.transport_d;
    // return offset > ZDJ_SOUNDCARD_BUF_LEN / 2; // This is totally arbitrary.
    return layer->last_packet->end_addr.transport_d > layer->lead_out_end.transport_d;
}


static bool _layer_start_lt_win_start( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_start 
) {
    return layer->lead_in_start.transport_d < win_start->transport_d;
}

static bool _layer_start_gt_win_start( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_start 
) {
    return layer->lead_in_start.transport_d > win_start->transport_d;
}

static bool _layer_first_packet_gt_layer_start( zdj_decode_layer_t * layer ) {
    if( !layer->first_packet || layer->first_packet->has_decode_error ) {
        printf( "_layer_first_packet_gt_layer_start UNDEFINED!!!\n" );
        return false;
    }

    // double offset = layer->first_packet->start_addr.transport_d - layer->lead_in_start.transport_d;
    // return offset > ZDJ_SOUNDCARD_BUF_LEN / 2; // This is totally arbitrary.
    return layer->first_packet->start_addr.transport_d > layer->lead_in_start.transport_d;
}

static bool _layer_first_packet_lt_layer_start( zdj_decode_layer_t * layer ) {
    if( !layer->first_packet || layer->first_packet->has_decode_error ) {
        printf( "_layer_first_packet_gt_layer_start UNDEFINED!!!\n" );
        return false;
    }

    // double offset = layer->lead_in_start.transport_d - layer->first_packet->start_addr.transport_d;
    // return offset > ZDJ_SOUNDCARD_BUF_LEN / 2; // This is totally arbitrary.
    return layer->first_packet->start_addr.transport_d < layer->lead_in_start.transport_d;
}

static bool _layer_first_packet_gt_win_start( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_start 
) {
    if( !layer->first_packet || layer->first_packet->has_decode_error ) {
        printf( "_layer_first_packet_gt_win_start UNDEFINED!!!\n" );
        return false;
    }
    return layer->first_packet->start_addr.transport_d > win_start->transport_d;
}

static bool _layer_first_packet_lt_win_start( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_start 
) {
    if( !layer->first_packet || layer->first_packet->has_decode_error ) {
        printf( "_layer_first_packet_lt_win_start UNDEFINED!!!\n" );
        return false;
    }
    return layer->first_packet->start_addr.transport_d < win_start->transport_d;
}

static bool _layer_first_packet_lt_win_end( 
    zdj_decode_layer_t * layer, 
    zdj_decode_addr_t * win_end 
) {
    if( !layer->first_packet || layer->first_packet->has_decode_error ) {
        printf( "_layer_first_packet_lt_win_end UNDEFINED!!!\n" );
        return false;
    }
    return layer->first_packet->start_addr.transport_d < win_end->transport_d;
}
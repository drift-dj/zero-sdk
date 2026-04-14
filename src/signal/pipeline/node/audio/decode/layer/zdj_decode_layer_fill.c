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

static void _fill( zdj_decode_layer_t * layer, zdj_pipeline_node_t * node );
static bool _can_fill( zdj_decode_layer_t * layer, zdj_pipeline_node_t * node, zdj_decode_dir_t dir );
static int64_t _fill_packet_count( 
    zdj_decode_layer_t * layer, zdj_pipeline_node_t * node, zdj_decode_dir_t dir
);
static int64_t _seek_target( 
    zdj_decode_layer_t * layer, zdj_pipeline_node_t * node, int packet_count, zdj_decode_dir_t dir 
);
static void _seek_and_decode( 
    zdj_decode_layer_t * layer,
    zdj_pipeline_node_t * node, 
    int64_t seek_target, 
    int seek_flag,
    zdj_decode_dir_t dir,
    zdj_decode_packet_justify_t first_packet_justify,
    int packet_count, 
    bool predecode_garbage 
);

void zdj_decode_layer_init_fill_api( zdj_decode_layer_t * layer ) {
    layer->fill = &_fill;
    layer->can_fill = &_can_fill;
    layer->fill_packet_count = &_fill_packet_count;
    layer->seek_target = &_seek_target;
    layer->seek_and_decode = &_seek_and_decode;
}

static void _fill( zdj_decode_layer_t * layer, zdj_pipeline_node_t * node ) {
    // printf( "\nlayer %p _fill node:%p\n", layer, node );
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;

    // Capture current node win addresses
    zdj_decode_addr_t win_start; node_state->get_win_start_addr( node, &win_start );
    zdj_decode_addr_t win_end; node_state->get_win_end_addr( node, &win_end );

    // Any discontinuous jump in seek may require garbage packets from the decoder
    bool seek_has_discontinuity = false;
    int64_t seek_target;
    int64_t packet_count;

    // Add packet to empty layer
    if( layer->is_empty( layer ) ) {
        // printf( "\n[%p] empty layer add [t:%1.0f->%1.0f - o:%1.0f->%1.0f] head:%1.0f\n\n",
        //     layer,
        //     layer->lead_in_start.transport_d, layer->lead_out_end.transport_d,
        //     layer->lead_in_start.origin_d, layer->lead_out_end.origin_d,
        //     node_state->head.transport_d
        // );
        
        double p_start = zdj_perf_time( );

        packet_count = 1;
        // If layer start intersects window
        if( win_start.less_than( &win_start, &layer->lead_in_start, ZDJ_ADDR_COORD_TRANSPORT ) &&
            win_end.greater_than( &win_end, &layer->lead_in_start, ZDJ_ADDR_COORD_TRANSPORT )
        ) {
            seek_target = layer->lead_in_start.origin_i;
            // printf( "%p empty layer at start: o:%ld t:%ld\n", layer, seek_target, layer->lead_in_start.transport_i );
        
        // Else if layer end intersects window
        } else if( win_start.less_than( &win_start, &layer->lead_out_end, ZDJ_ADDR_COORD_TRANSPORT ) &&
            win_end.greater_than( &win_end, &layer->lead_out_end, ZDJ_ADDR_COORD_TRANSPORT )
        ) {
            // Get end origin addr
            seek_target = layer->lead_out_end.origin_i - node_state->estimated_packet_sample_count;
            // printf( "empty layer at end: %ld\n", seek_target );


        // Else if window is inside layer (new skip layer + needledrop go here)
        } else if( win_start.greater_than( &win_start, &layer->lead_in_start, ZDJ_ADDR_COORD_TRANSPORT ) &&
            win_end.less_than( &win_end, &layer->lead_out_end, ZDJ_ADDR_COORD_TRANSPORT )
        ) {
            // Get origin coord 
            seek_target = node_state->head.origin_i - node_state->estimated_packet_sample_count;
        }
        
        // Layer nay be empty because playhead is outside song file.
        // Don't attempt to decode first packet in that case.
        zdj_decode_addr_t * seek_addr = calloc( 1, sizeof( zdj_decode_addr_t ) );
        zdj_decode_init_addr( seek_addr );
        seek_addr->origin_d = (double)seek_target;
        seek_addr->origin_i = seek_target;
        if( layer->contains_addr( layer, seek_addr, ZDJ_ADDR_COORD_ORIGIN ) ) {
            layer->seek_and_decode( 
                layer, node, seek_target, AVSEEK_FLAG_BACKWARD, ZDJ_DECODE_DIR_FWD, ZDJ_DECODE_JUSTIFY_RIGHT, packet_count, node_state->requires_garbage 
            );
        } else {
            // printf( "no empty decode for song outside window\n" );
            // printf( "s - o:%1.0f l - s:%1.0f e:%1.0f\n", 
            //     seek_addr->origin_d, 
            //     layer->core_start.origin_d,
            //     layer->core_end.origin_d 
            // );
        }

        // printf( "%p layer s:%lu e:%lu 1st pac st: %ld\n", 
        //     layer,
        //     layer->lead_in_start.transport_i,
        //     layer->lead_out_end.transport_i,
        //     layer->first_packet->start_addr.transport_i 
        // );
    }

    // If window doesn't contain any song origin coords, we're done.
    if( win_start.origin_d > node_state->song_pcm_duration ||
        win_end.origin_d < 0
    ) {
        // printf( "no song samples in window - done\n" );
        return;
    }

    // Fill backward
    if( layer->can_fill( layer, node, ZDJ_DECODE_DIR_BACK ) ) {
        // printf( "\nlayer can fill back hd:%ld fs:%ld le:%ld\n\n",
        //     node_state->head.origin_i,
        //     layer->first_packet->start_addr.origin_i,
        //     layer->last_packet->end_addr.origin_i
        // );

        double p_start = zdj_perf_time( );

        packet_count = layer->fill_packet_count( layer, node, ZDJ_DECODE_DIR_BACK );
        seek_target = layer->seek_target( layer, node, packet_count, ZDJ_DECODE_DIR_BACK );
        // printf( "fill back packet count: %ld\n", packet_count );
        // layer->seek_and_decode( 
        //     layer, node, seek_target, AVSEEK_FLAG_BACKWARD, ZDJ_DECODE_DIR_BACK, ZDJ_DECODE_JUSTIFY_LEFT, packet_count, node_state->requires_garbage 
        // );
        layer->seek_and_decode( 
            layer, node, seek_target, AVSEEK_FLAG_FRAME, ZDJ_DECODE_DIR_BACK, ZDJ_DECODE_JUSTIFY_LEFT, packet_count, node_state->requires_garbage 
        );
        seek_has_discontinuity = true;

        double p_end = zdj_perf_time( );
        double p_tot = ( p_end - p_start ) / 1000000.0;
        // if( p_tot > 4.0 ) { 
        //     printf( "bak fill:%1.3f p_cnt:%ld targ:%ld\n", p_tot, packet_count, seek_target ); 
        // }
    }
    

    // printf( "fill 2\n" );
    // Fill forward
    if( layer->can_fill( layer, node, ZDJ_DECODE_DIR_FWD ) ) {
        // printf( "\nlayer %p can fill forward hd:%ld fs:%ld le:%ld\n\n",
        //     layer,
        //     node_state->head.origin_i,
        //     layer->first_packet->start_addr.origin_i,
        //     layer->last_packet->end_addr.origin_i
        // );
        double p_start = zdj_perf_time( );

        packet_count = layer->fill_packet_count( layer, node, ZDJ_DECODE_DIR_FWD );
        seek_target = layer->seek_target( layer, node, packet_count, ZDJ_DECODE_DIR_FWD );
        layer->seek_and_decode( 
            layer, node, seek_target, AVSEEK_FLAG_FRAME, ZDJ_DECODE_DIR_FWD, ZDJ_DECODE_JUSTIFY_LEFT, packet_count, seek_has_discontinuity 
        );
        // printf( "layer fwd fill done\n" );
        double p_end = zdj_perf_time( );
        double p_tot = ( p_end - p_start ) / 1000000.0;
        // if( p_tot > 4.0 ) { 
        //     printf( "%p fwd fill:%1.3f p_cnt:%ld targ:%ld\n", layer, p_tot, packet_count, seek_target ); 
        // }
    }

    
    // printf( "fill 3\n" );
    // Trim any packets outside the window
    while( layer->can_remove_packet( layer, node, layer->first_packet ) ) {
        layer->remove_packet( layer, layer->first_packet );
    }
    while( layer->can_remove_packet( layer, node, layer->last_packet ) ) {
        layer->remove_packet( layer, layer->last_packet );
    }

    // printf( "layer fill last packet: %ld\n", layer->last_packet->end_addr.transport_i );
    // printf( "layer _fill done pkts: %d\n", layer->debug_packet_counter );
}

static bool _can_fill( zdj_decode_layer_t * layer, zdj_pipeline_node_t * node, zdj_decode_dir_t dir ) {
    // printf( "layer can_fill: %s %p\n", (dir==ZDJ_DECODE_DIR_FWD) ? "fwd" : "back", node );
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    // Check current first packet in layer against node's win_start and layer's lead_in start
    zdj_decode_addr_t win_start; state->get_win_start_addr( node, &win_start );
    zdj_decode_addr_t win_end; state->get_win_end_addr( node, &win_end );

    if( !layer->last_packet || !layer->first_packet ) {
        // printf( "_can_fill called on empty layer!: %p\n", layer );
        return false;
    }
    
    if( dir == ZDJ_DECODE_DIR_FWD ) {
        // printf( "fill fwd: %1.0f-%1.0f/%1.0f -> %1.0f | %1.0f -> %1.0f\n", 
        //     layer->last_packet->start_addr.transport_d,
        //     layer->last_packet->end_addr.transport_d, 
        //     layer->last_packet->end_addr.origin_d, 
        //     layer->lead_out_end.transport_d,
        //     layer->last_packet->end_addr.transport_d,
        //     win_end.transport_d
        // );
        if( layer->last_packet->end_addr.less_than(
                &layer->last_packet->end_addr, &layer->lead_out_end, ZDJ_ADDR_COORD_TRANSPORT
            ) &&
            layer->last_packet->end_addr.less_than(
                &layer->last_packet->end_addr, &win_end, ZDJ_ADDR_COORD_TRANSPORT
            )
        ) {
            // printf( "===>Can Fill Fwd\n" );
            return true;
        }
        
    } else if( dir == ZDJ_DECODE_DIR_BACK ) {
        // printf( "[%p] can fill back: l[%1.0f-%1.0f] p[%1.0f-%1.0f]\n",
        //     layer,
        //     layer->core_start.transport_d, layer->core_end.transport_d,
        //     layer->first_packet->start_addr.transport_d, layer->first_packet->end_addr.transport_d 
        // );
        if( layer->back_discon_type == ZDJ_DECODE_DISCON_LOOP ) {
            // First packet > layer start && first_packet > win_start
            if( layer->first_packet->start_addr.greater_than( 
                    &layer->first_packet->start_addr, &layer->lead_in_start, ZDJ_ADDR_COORD_TRANSPORT
                ) &&
                layer->first_packet->start_addr.greater_than(
                    &layer->first_packet->start_addr, &win_start, ZDJ_ADDR_COORD_TRANSPORT
                )
            ) {
                // printf( "===>Can Fill Back\n" );
                return true;
            }
        } else if( layer->back_discon_type == ZDJ_DECODE_DISCON_NONE ) {
            if( layer->first_packet &&
                layer->first_packet->start_addr.greater_than( 
                    &layer->first_packet->start_addr, &layer->lead_in_start, ZDJ_ADDR_COORD_TRANSPORT
                ) &&
                layer->first_packet->start_addr.greater_than(
                    &layer->first_packet->start_addr, &win_start, ZDJ_ADDR_COORD_TRANSPORT
                ) &&
                layer->first_packet->start_addr.origin_d > 0.0
            ) {
                // printf( "===>Can Fill Back\n" );
                return true;
            }
        }
    }
    return false;
    // printf( "layer can_fill done\n" );
}

static int64_t _fill_packet_count( 
    zdj_decode_layer_t * layer, zdj_pipeline_node_t * node, zdj_decode_dir_t dir
) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    zdj_deck_control_loop_state_t * loop_state = (zdj_deck_control_loop_state_t*)layer->_loop_state;
    int sample_count = 0;
    zdj_decode_addr_t win_start; 
    state->get_win_start_addr( node, &win_start );
    zdj_decode_addr_t win_end; 
    state->get_win_end_addr( node, &win_end );

    if( dir == ZDJ_DECODE_DIR_BACK ) {
        // TODO: Bring this up to the fwd fill
        if( win_start.greater_than( &win_start, &layer->lead_in_start, ZDJ_ADDR_COORD_TRANSPORT ) ) {
            // Layer start is beyond left edge of window
            sample_count = layer->first_packet->start_addr.transport_i - win_start.transport_i;
            // printf( "left edge\n" );
        } else if ( 
            // Layer start is within window
            win_start.less_than( &win_start, &layer->lead_in_start, ZDJ_ADDR_COORD_TRANSPORT ) &&
            win_end.greater_than( &win_end, &layer->lead_in_start, ZDJ_ADDR_COORD_TRANSPORT )
        ) {
            sample_count = layer->first_packet->start_addr.transport_i - layer->lead_in_start.transport_i;
            // printf( "inside %p -- p:%lu st:%lu win s:%lu e:%lu\n", 
            //     layer,
            //     layer->first_packet->start_addr.transport_i, 
            //     layer->lead_in_start.transport_i,
            //     win_start.transport_i,
            //     win_end.transport_i
            // );
        }
        // printf( "back sample_count: %d\n", sample_count );

    } else if( dir == ZDJ_DECODE_DIR_FWD ) {
        if( win_end.less_than( &win_end, &layer->lead_out_end, ZDJ_ADDR_COORD_TRANSPORT ) ) {
            // Layer end is beyond right edge of window
            sample_count = win_end.transport_i - layer->last_packet->end_addr.transport_i;
        } else if (
            // Layer start is within window
            win_end.greater_than( &win_end, &layer->lead_in_start, ZDJ_ADDR_COORD_TRANSPORT ) &&
            win_start.less_than( &win_start, &layer->lead_in_start, ZDJ_ADDR_COORD_TRANSPORT )
        ) {
            sample_count = win_end.transport_i - layer->lead_in_start.transport_i;
        } else if (
            // Layer end is within window
            win_end.greater_than( &win_end, &layer->lead_out_end, ZDJ_ADDR_COORD_TRANSPORT ) &&
            win_start.less_than( &win_end, &layer->lead_out_end, ZDJ_ADDR_COORD_TRANSPORT )
        ) {
            sample_count = layer->lead_out_end.transport_i - layer->last_packet->end_addr.transport_i;
        }
        // printf( "end: %ld %ld\n", win_end.transport_i, layer->last_packet->end_addr.transport_i );
    }

    return ceil( (double)sample_count / state->estimated_packet_sample_count );
}

static int64_t _seek_target( 
    zdj_decode_layer_t * layer, zdj_pipeline_node_t * node, int packet_count, zdj_decode_dir_t dir 
) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    if( dir == ZDJ_DECODE_DIR_BACK ) {
        return layer->first_packet->start_addr.origin_i - ( packet_count * state->estimated_packet_sample_count );

    } else if ( dir == ZDJ_DECODE_DIR_FWD ) {
        return layer->last_packet->end_addr.origin_i;
    }
}

static void _seek_and_decode( 
    zdj_decode_layer_t * layer, 
    zdj_pipeline_node_t * node, 
    int64_t seek_target, 
    int seek_flag,
    zdj_decode_dir_t dir,
    zdj_decode_packet_justify_t first_packet_justify,
    int packet_count, 
    bool predecode_garbage
) {
    // printf( "[%p] seek_and_decode %d packets @:%ld dir:%d garb:%d\n", layer, packet_count, seek_target, dir, predecode_garbage );
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;

    // Note that we handle the garbage packet decode internally here
    if( predecode_garbage ) { 
        seek_target -= (node_state->estimated_packet_sample_count * 2);
        if( seek_target < 0 ) { seek_target = 0; } 
    }

    // printf( "seek_target: %ld\n", seek_target );

    av_seek_frame( node_state->fmt_ctx, 0, seek_target*node_state->av_timebase_factor, seek_flag );

    if ( predecode_garbage && seek_target > 0 ) {
        // printf( "decoding 2 garbage packets\n" ); 
        // Decode and discard 2 frames
        zdj_decode_garbage_packet( 
            node, layer, node_state->fmt_ctx, node_state->codec_ctx 
        );
        zdj_decode_garbage_packet( 
            node, layer, node_state->fmt_ctx, node_state->codec_ctx 
        );
        // zdj_decode_garbage_packet( 
        //     node, layer, node_state->fmt_ctx, node_state->codec_ctx 
        // );
    }

    if( dir == ZDJ_DECODE_DIR_FWD ) {
        // printf( "decode fwd: %d\n", packet_count );
        for( int i=0; i<packet_count; i++ ) {  
            // printf( "fwd\n" );
            zdj_decode_packet_t * packet = zdj_decode_packet( 
                node, layer, node_state->fmt_ctx, node_state->codec_ctx, node_state->av_timebase_factor,
                (i==0) ? first_packet_justify : ZDJ_DECODE_JUSTIFY_LEFT
            );
            // printf( "packet: [t:%1.0f/o:%1.0f - t:%1.0f/o:%1.0f]\n", 
            //     packet->start_addr.transport_d, packet->start_addr.origin_d, 
            //     packet->end_addr.transport_d, packet->end_addr.origin_d
            // );
            layer->append_packet( layer, packet ); 
        }

    } else if ( dir == ZDJ_DECODE_DIR_BACK ) {

        for( int i=0; i<packet_count; i++ ) {  
            
            zdj_decode_packet_t * packet = zdj_decode_packet( 
                node, layer, node_state->fmt_ctx, node_state->codec_ctx, node_state->av_timebase_factor,
                (i==0) ? first_packet_justify : ZDJ_DECODE_JUSTIFY_LEFT
            );
            if( i == 0 ) {
                layer->prepend_packet( layer, packet );
            } else {
                layer->insert_packet_after( layer, packet, layer->first_packet );
            }
        }
    } 

    // printf( "layer seek_and_decode done\n" );
}
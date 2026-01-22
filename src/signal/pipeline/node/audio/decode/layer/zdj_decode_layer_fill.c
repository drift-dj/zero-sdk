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
    // printf( "\nlayer %p _fill empty:%d\n", layer, layer->is_empty( layer ) );
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;

    // Capture current node win addresses
    zdj_decode_addr_t win_start; node_state->get_win_start_addr( node, &win_start );
    zdj_decode_addr_t win_end; node_state->get_win_end_addr( node, &win_end );

    // Any discontinuous jump in seek may require garbage packets from the decoder
    bool seek_has_discontinuity = false;
    int64_t seek_target;
    int64_t packet_count;

    // printf( "fill 0 %d\n", layer->is_empty( layer ) );

    



    // Add packet to empty layer
    if( layer->is_empty( layer ) ) {

    
        // I don't think the below is correct - why would we check if it containst the head?
        // If it's a new layer, @ origin 0, it would, but if it's a new loop/skip layer, it won't
        // What if we change it to window_contains?
        // layer->contains_addr( layer, &node_state->head, ZDJ_ADDR_COORD_ORIGIN ) 
        // if ( ( node_state->win_contains_addr( node, &layer->lead_in_start, ZDJ_ADDR_COORD_TRANSPORT ) ||
        //     node_state->win_contains_addr( node, &layer->lead_out_end, ZDJ_ADDR_COORD_TRANSPORT ) ) ||
        //     ( win_start.greater_than( &win_start, &layer->lead_in_start, ZDJ_ADDR_COORD_ORIGIN ) &&
        //     win_end.less_than( &win_end, &layer->lead_out_end, ZDJ_ADDR_COORD_ORIGIN ) )
        // ) { 
            printf( "\n[%p] empty layer add [t:%1.0f/o:%1.0f - t:%1.0f/o:%1.0f] head:%1.0f\n\n",
                layer,
                layer->lead_in_start.transport_d, layer->lead_out_end.transport_d,
                layer->lead_in_start.origin_d, layer->lead_out_end.origin_d,
                node_state->head.transport_d
            );
            
            double p_start = zdj_perf_time( );

            // packet_count = 1;


            // THis is the problem.
            // seek_target = layer->lead_in_start.origin_i;
            // An empty layer needs to start near the window, or can_fill
            // will find a bazillion packets to decode.


            packet_count = 1;
            // If layer start intersects window
            if( win_start.less_than( &win_start, &layer->lead_in_start, ZDJ_ADDR_COORD_TRANSPORT ) &&
                win_end.greater_than( &win_end, &layer->lead_in_start, ZDJ_ADDR_COORD_TRANSPORT )
            ) {
                seek_target = layer->lead_in_start.origin_i;
                printf( "empty layer at start\n" );
            
            // Else if layer end intersects window
            } else if( win_start.less_than( &win_start, &layer->lead_out_end, ZDJ_ADDR_COORD_TRANSPORT ) &&
                win_end.greater_than( &win_end, &layer->lead_out_end, ZDJ_ADDR_COORD_TRANSPORT )
            ) {
                // Get end origin addr
                seek_target = layer->lead_out_end.origin_i - node_state->estimated_packet_sample_count;
                // seek_target = layer->lead_out_end.origin_i 
                    // quantized to packet start containing coord
                printf( "empty layer at end: %ld\n", seek_target );

            // Else if window is inside layer
            } else if( layer->lead_in_start.less_than( &layer->lead_in_start, &win_start, ZDJ_ADDR_COORD_TRANSPORT ) &&
                layer->lead_out_end.greater_than( &layer->lead_out_end, &win_end, ZDJ_ADDR_COORD_TRANSPORT )
            ) {
                // seek_target = origin_i for win_start in layer
                    // quantized to packet start containing coord
                printf( "CREATED EMPTY LAYER AWAY FROM START/END\n" );
            } 
            
                


            layer->seek_and_decode( 
                layer, node, seek_target, AVSEEK_FLAG_FRAME, ZDJ_DECODE_DIR_FWD, ZDJ_DECODE_JUSTIFY_RIGHT, packet_count, node_state->requires_garbage 
            );


            // double p_end = zdj_perf_time( );
            // double p_tot = ( p_end - p_start ) / 1000000.0;
            // if( p_tot > 4.0 ) { 
            //     printf( "fill:%1.3f p_cnt:%ld targ:%ld\n", p_tot, packet_count, seek_target ); 
            // }
        // }
    }

    // printf( "fill 1\n" );
    // Fill backward
    if( layer->can_fill( layer, node, ZDJ_DECODE_DIR_BACK ) ) {
        printf( "\nlayer can fill back hd:%ld fs:%ld le:%ld\n\n",
            node_state->head.origin_i,
            layer->first_packet->start_addr.origin_i,
            layer->last_packet->end_addr.origin_i
        );
        // printf( "fill 1.1\n" );

        double p_start = zdj_perf_time( );


        packet_count = layer->fill_packet_count( layer, node, ZDJ_DECODE_DIR_BACK );
        seek_target = layer->seek_target( layer, node, packet_count, ZDJ_DECODE_DIR_BACK );
        printf( "fill back packet count: %ld\n", packet_count );
        layer->seek_and_decode( 
            layer, node, seek_target, AVSEEK_FLAG_BACKWARD, ZDJ_DECODE_DIR_BACK, ZDJ_DECODE_JUSTIFY_LEFT, packet_count, node_state->requires_garbage 
        );
        // printf( "fill 1.2\n" );
        seek_has_discontinuity = true;
        // printf(  "layer back fill done\n");


        double p_end = zdj_perf_time( );
        double p_tot = ( p_end - p_start ) / 1000000.0;
        if( p_tot > 4.0 ) { 
            printf( "fill:%1.3f p_cnt:%ld targ:%ld\n", p_tot, packet_count, seek_target ); 
        }
    }
    

    // printf( "fill 2\n" );
    // Fill forward
    if( layer->can_fill( layer, node, ZDJ_DECODE_DIR_FWD ) ) {
        // printf( "\nlayer can fill forward hd:%ld fs:%ld le:%ld\n\n",
        //     node_state->head.origin_i,
        //     layer->first_packet->start_addr.origin_i,
        //     layer->last_packet->end_addr.origin_i
        // );
        // double p_start = zdj_perf_time( );

        packet_count = layer->fill_packet_count( layer, node, ZDJ_DECODE_DIR_FWD );
        seek_target = layer->seek_target( layer, node, packet_count, ZDJ_DECODE_DIR_FWD );
        layer->seek_and_decode( 
            layer, node, seek_target, AVSEEK_FLAG_FRAME, ZDJ_DECODE_DIR_FWD, ZDJ_DECODE_JUSTIFY_LEFT, packet_count, seek_has_discontinuity 
        );
        // printf( "layer fwd fill done\n" );
        // double p_end = zdj_perf_time( );
        // double p_tot = ( p_end - p_start ) / 1000000.0;
        // if( p_tot > 4.0 ) { 
        //     printf( "fill:%1.3f p_cnt:%ld targ:%ld\n", p_tot, packet_count, seek_target ); 
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
    // printf( "layer _fill done\n" );
}

static bool _can_fill( zdj_decode_layer_t * layer, zdj_pipeline_node_t * node, zdj_decode_dir_t dir ) {
    // printf( "layer can_fill: %s\n", (dir==ZDJ_DECODE_DIR_FWD) ? "fwd" : "back" );
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    // Check current first packet in layer against node's win_start and layer's lead_in start
    zdj_decode_addr_t win_start; state->get_win_start_addr( node, &win_start );
    zdj_decode_addr_t win_end; state->get_win_end_addr( node, &win_end );
    
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
        printf( "[%p] can fill back: l[%1.0f-%1.0f] p[%1.0f-%1.0f]\n",
            layer,
            layer->core_start.transport_d, layer->core_end.transport_d,
            layer->first_packet->start_addr.transport_d, layer->first_packet->end_addr.transport_d 
        );
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
        
        if( win_start.greater_than( &win_start, &layer->lead_in_start, ZDJ_ADDR_COORD_TRANSPORT ) ) {
            // Layer start is beyond left edge of window
            sample_count = layer->first_packet->start_addr.transport_i - win_start.transport_i;
        } else if ( 
            // Layer start is within window
            win_start.less_than( &win_start, &layer->lead_in_start, ZDJ_ADDR_COORD_TRANSPORT ) &&
            win_end.greater_than( &win_end, &layer->lead_in_start, ZDJ_ADDR_COORD_TRANSPORT )
        ) {
            sample_count = layer->first_packet->start_addr.transport_i - layer->lead_in_start.transport_i;
        }
        
    } else if( dir == ZDJ_DECODE_DIR_FWD ) {
        if( win_end.less_than( &win_end, &layer->lead_out_end, ZDJ_ADDR_COORD_TRANSPORT ) ) {
            // Layer end is beyond right edge of window
            sample_count = win_end.transport_i - layer->last_packet->end_addr.transport_i;
            // printf( "end off right edge: %d\n", sample_count );
        } else if (
            // Layer start is within window
            win_end.greater_than( &win_end, &layer->lead_in_start, ZDJ_ADDR_COORD_TRANSPORT ) &&
            win_start.less_than( &win_start, &layer->lead_in_start, ZDJ_ADDR_COORD_TRANSPORT )
        ) {
            sample_count = win_end.transport_i - layer->lead_in_start.transport_i;
            // WRONG //
            // sample_count = layer->lead_out_end.transport_i - layer->last_packet->end_addr.transport_i;
            // WRONG //
            printf( "start in win: sc:%d h:%1.1f l_s:%ld l_ps:%ld l_e:%ld l_pe:%ld\n",
                sample_count,
                state->head.transport_d,
                layer->lead_in_start.transport_i,
                layer->first_packet->start_addr.transport_i,
                layer->lead_out_end.transport_i,
                layer->last_packet->end_addr.transport_i 
            );
        } else if (
            // Layer end is within window
            win_end.greater_than( &win_end, &layer->lead_out_end, ZDJ_ADDR_COORD_TRANSPORT ) &&
            win_start.less_than( &win_end, &layer->lead_out_end, ZDJ_ADDR_COORD_TRANSPORT )
        ) {
            // WRONG //
            sample_count = layer->lead_out_end.transport_i - layer->last_packet->end_addr.transport_i;
            // WRONG //
            printf( "end in win: sc:%d h:%1.1f l_s:%ld l_ps:%ld l_e:%ld l_pe:%ld\n",
                sample_count,
                state->head.transport_d,
                layer->lead_in_start.transport_i,
                layer->first_packet->start_addr.transport_i,
                layer->lead_out_end.transport_i,
                layer->last_packet->end_addr.transport_i 
            );
        }

        
        // printf( "end: %ld %ld\n", win_end.transport_i, layer->last_packet->end_addr.transport_i );
    }
    // if( sample_count > 2000 ) {
    //     printf( "%s fill_packet_count: %1.0f / %1.0f = %f\n", 
    //         ( dir == ZDJ_DECODE_DIR_FWD ) ? "fwd" : "back",
    //         (double)sample_count,
    //         state->estimated_packet_sample_count,
    //         ceil( (double)sample_count / state->estimated_packet_sample_count )
    //     );
    // }
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
    printf( "[%p] seek_and_decode %d packets @:%ld dir:%d garb:%d\n", layer, packet_count, seek_target, dir, predecode_garbage );
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;

    // Note that we handle the garbage packet decode internally here
    if( predecode_garbage ) { 
        seek_target -= (node_state->estimated_packet_sample_count * 2);
        if( seek_target < 0 ) { seek_target = 0; } 
    }

    printf( "seek_target: %ld\n", seek_target );

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
            printf( "fwd\n" );
            zdj_decode_packet_t * packet = zdj_decode_packet( 
                node, layer, node_state->fmt_ctx, node_state->codec_ctx, node_state->av_timebase_factor,
                (i==0) ? first_packet_justify : ZDJ_DECODE_JUSTIFY_LEFT
            );
            printf( "packet: [t:%1.0f/o:%1.0f - t:%1.0f/o:%1.0f]\n", 
                packet->start_addr.transport_d, packet->start_addr.origin_d, 
                packet->end_addr.transport_d, packet->end_addr.origin_d
            );
            layer->append_packet( layer, packet ); 
        }

    } else if ( dir == ZDJ_DECODE_DIR_BACK ) {

        for( int i=0; i<packet_count; i++ ) {  
            
            zdj_decode_packet_t * packet = zdj_decode_packet( 
                node, layer, node_state->fmt_ctx, node_state->codec_ctx, node_state->av_timebase_factor,
                (i==0) ? first_packet_justify : ZDJ_DECODE_JUSTIFY_LEFT
            );
            printf( "back\n" );
            if( i == 0 ) {
                layer->prepend_packet( layer, packet );
            } else {
                layer->insert_packet_after( layer, packet, layer->first_packet );
            }
        }
    } 

    // printf( "layer seek_and_decode done\n" );
}
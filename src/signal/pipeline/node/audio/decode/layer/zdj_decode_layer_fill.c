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
    // printf( "layer _fill\n" );
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;

    // Capture current node win addresses
    zdj_decode_addr_t win_start; node_state->get_win_start_addr( node, &win_start );
    zdj_decode_addr_t win_end; node_state->get_win_end_addr( node, &win_end );

    // Any discontinuous jump in seek may require garbage packets from the decoder
    bool seek_has_discontinuity = false;
    int64_t seek_target;
    int64_t packet_count;

    // printf( "fill 0\n" );

    // Add packet to empty layer
    if( layer->is_empty( layer ) ) { 
        packet_count = 1;
        seek_target = node_state->head.origin_i;
        layer->seek_and_decode( 
            layer, node, seek_target, AVSEEK_FLAG_FRAME, ZDJ_DECODE_DIR_FWD, packet_count, node_state->requires_garbage 
        );
    }

    // printf( "fill 1\n" );
    // Fill backward
    if( layer->can_fill( layer, node, ZDJ_DECODE_DIR_BACK ) ) {
        // printf( "fill 1.1\n" );
        packet_count = layer->fill_packet_count( layer, node, ZDJ_DECODE_DIR_BACK );
        seek_target = layer->seek_target( layer, node, packet_count, ZDJ_DECODE_DIR_BACK );
        layer->seek_and_decode( 
            layer, node, seek_target, AVSEEK_FLAG_BACKWARD, ZDJ_DECODE_DIR_BACK, packet_count, node_state->requires_garbage 
        );
        // printf( "fill 1.2\n" );
        seek_has_discontinuity = true;
    }

    // printf( "fill 2\n" );
    // Fill forward
    if( layer->can_fill( layer, node, ZDJ_DECODE_DIR_FWD ) ) {
        packet_count = layer->fill_packet_count( layer, node, ZDJ_DECODE_DIR_FWD );
        seek_target = layer->seek_target( layer, node, packet_count, ZDJ_DECODE_DIR_FWD );
        layer->seek_and_decode( 
            layer, node, seek_target, AVSEEK_FLAG_FRAME, ZDJ_DECODE_DIR_FWD, packet_count, seek_has_discontinuity 
        );
    }

    // printf( "fill 3\n" );
    // Trim any packets outside the window
    while( layer->can_remove_packet( layer, node, layer->first_packet ) ) {
        layer->remove_packet( layer, layer->first_packet );
    }
    while( layer->can_remove_packet( layer, node, layer->last_packet ) ) {
        layer->remove_packet( layer, layer->last_packet );
    }
    // printf( "layer _fill done\n" );
}

static bool _can_fill( zdj_decode_layer_t * layer, zdj_pipeline_node_t * node, zdj_decode_dir_t dir ) {
    // printf( "layer can_fill: %d\n", dir );
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
}

static int64_t _fill_packet_count( 
    zdj_decode_layer_t * layer, zdj_pipeline_node_t * node, zdj_decode_dir_t dir
) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    int sample_count;

    if( dir == ZDJ_DECODE_DIR_BACK ) {
        zdj_decode_addr_t win_start; 
        state->get_win_start_addr( node, &win_start );
        sample_count = layer->first_packet->start_addr.transport_i - win_start.transport_i;

    } else if( dir == ZDJ_DECODE_DIR_FWD ) {
        zdj_decode_addr_t win_end; 
        state->get_win_end_addr( node, &win_end );
        sample_count = win_end.transport_i - layer->last_packet->end_addr.transport_i;
        printf( "end: %ld %ld\n", win_end.transport_i, layer->last_packet->end_addr.transport_i );
    }
    printf( "fill_packet_count: %1.0f / %1.0f = %f\n", 
        (double)sample_count,
        state->estimated_packet_sample_count,
        ceil( (double)sample_count / state->estimated_packet_sample_count )
    );
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
    int packet_count, 
    bool predecode_garbage
) {
    printf( "layer seek_and_decode %d packets @:%ld dir:%d garb:%d\n", packet_count, seek_target, dir, predecode_garbage );
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;

    // Note that we handle the garbage packet decode internally here
    if( predecode_garbage ) { 
        seek_target -= (node_state->estimated_packet_sample_count * 2);
        if( seek_target < 0 ) { seek_target = 0; } 
    }

    av_seek_frame( node_state->fmt_ctx, 0, seek_target*node_state->av_timebase_factor, seek_flag );

    if ( predecode_garbage && seek_target > 0 ) {
        printf( "decoding 2 garbage packets\n" ); 
        // Decode and discard 2 frames
        zdj_decode_garbage_packet( 
            node, layer, node_state->fmt_ctx, node_state->codec_ctx 
        );
        zdj_decode_garbage_packet( 
            node, layer, node_state->fmt_ctx, node_state->codec_ctx 
        );
    }

    if( dir == ZDJ_DECODE_DIR_FWD ) {
        printf( "decode fwd: %d\n", packet_count );
        for( int i=0; i<packet_count; i++ ) {  
            zdj_decode_packet_t * packet = zdj_decode_packet( 
                node, layer, node_state->fmt_ctx, node_state->codec_ctx, node_state->av_timebase_factor 
            );
            layer->append_packet( layer, packet );
        }

    } else if ( dir == ZDJ_DECODE_DIR_BACK ) {

        for( int i=0; i<packet_count; i++ ) {  
            zdj_decode_packet_t * packet = zdj_decode_packet( 
                node, layer, node_state->fmt_ctx, node_state->codec_ctx, node_state->av_timebase_factor
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
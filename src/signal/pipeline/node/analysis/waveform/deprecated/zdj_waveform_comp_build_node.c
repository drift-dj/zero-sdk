#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/analysis/waveform/zdj_waveform_comp_build_node.h>

#define ZDJ_WAVEFORM_COMP_POINTS_PER_CYCLE 10

static void _zdj_waveform_comp_build_update_wait( zdj_pipeline_node_t * node );
static void _zdj_waveform_comp_build_deinit_state( zdj_pipeline_node_t * node );

zdj_pipeline_node_t * zdj_new_waveform_comp_build_node( zdj_pipeline_node_t * decode_node ) {
    // Make node
    zdj_pipeline_node_t * node = zdj_new_pipeline_node( );
    node->deinit_state = &_zdj_waveform_comp_build_deinit_state;
    node->update_wait = &_zdj_waveform_comp_build_update_wait;
    
    // Make state
    zdj_waveform_comp_build_node_state_t * state = calloc( 1, sizeof( zdj_waveform_comp_build_node_state_t ) );
    node->state = state;
    state->decode_node = decode_node;

    // Make Gaussian scaling state
    state->kernel = zdj_new_gaussian( ZDJ_PLAYBACK_WAVEFORM_SAMPLE_STRIDE, 1.0 );

    // Setup point window - (as byte_buf)
    // zdj_pipeline_window_resize( node->window, 0, ZDJ_WAVEFORM_COMP_POINTS_PER_CYCLE );

    // Setup decode_node's window based on the number of samples per cycle required.
    // uint32_t decode_width = ZDJ_WAVEFORM_COMP_POINTS_PER_CYCLE * ZDJ_PLAYBACK_WAVEFORM_SAMPLE_STRIDE;
    // decode_node->resize_window( decode_node, 0, decode_width );

    return node;
}

void _zdj_waveform_comp_build_update_wait( zdj_pipeline_node_t * node ) {
    // zdj_waveform_comp_build_node_state_t * state = (zdj_waveform_comp_build_node_state_t*)node->state;
    // // state->phase = ZDJ_WAVEFORM_COMP_BUILD_PHASE_RUNNING;

    // // Tell decode node to fill its sample window
    // decode_node->update_wait( decode_node );

    // int point_width = ZDJ_PLAYBACK_WAVEFORM_SAMPLE_STRIDE;
    // int decode_width = point_width * ZDJ_WAVEFORM_COMP_POINTS_PER_CYCLE;

    // // Fill infill window with data from decode node.
    // double val;
    // for( int i=0; i<ZDJ_WAVEFORM_COMP_POINTS_PER_CYCLE; i++ ) {
    //     // Build source_buf ref into decode_node's window.
    //     float * source_buf_addr = &decode_node->window->float_buf[ point_width * i ];
    //     val = zdj_gaussian_convolve_rectified( state->kernel, source_buf_addr );
        
    //     // scale val to fit within 1 byte
    //     uint8_t scaled_val = uint8_t(val * 0.001);

    //     // Put scaled val into byte_buf
    //     node->window->byte_buf[ i ] = scaled_val;
    // }

    // // Shift decode_node's window for the next cycle
    // decode_node->move_window( decode_node, decode_width );
}

void _zdj_waveform_comp_build_deinit_state( zdj_pipeline_node_t * node ) {
    if( node ) {
        zdj_waveform_comp_build_node_state_t * state = (zdj_waveform_comp_build_node_state_t*)node->state;

        if( state ) {
            zdj_gaussian_free( state->kernel );
            free( state->kernel );
            free( state );
            state = NULL;
        }
    }
}


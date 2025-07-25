#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/zdj_audio_node.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/signal/pipeline/node/render/zdj_waveform_display_node.h>

static void _zdj_waveform_display_update_wait( zdj_pipeline_node_t * node );
static void _zdj_waveform_display_deinit_state( zdj_pipeline_node_t * node );

zdj_pipeline_node_t * zdj_new_waveform_display_node( zdj_pipeline_node_t * pcm_node, double scale, int point_count ) {
    // Make node
    zdj_pipeline_node_t * node = zdj_new_pipeline_node( );
    node->deinit_state = &_zdj_waveform_display_deinit_state;
    node->update_wait = &_zdj_waveform_display_update_wait;
    
    // Make state
    zdj_waveform_display_node_state_t * state = calloc( 1, sizeof( zdj_waveform_display_node_state_t ) );
    node->state = state;
    state->pcm_node = pcm_node;
    state->point_count = point_count;

    // Make Gaussian scaling state
    zdj_gaussian_t * g = zdj_new_gaussian( scale );
    state->gaussian = g;

    // Resize window to point_count
    // zdj_pipeline_window_resize( node->window, point_count / 2, point_count / 2 );

    return node;
}

zdj_error_type_t zdj_waveform_display_node_set_scale( zdj_pipeline_node_t * node, double scale ) {
    zdj_waveform_display_node_state_t * state = (zdj_waveform_display_node_state_t *)node->state;

    // Set Gaussian scale and re-build lut

    // Calculate new pcm_node window size required to satisfy 
    // point_count mapped to gaussian scale and lut size.

    // Resize pcm_node's window
    // zdj_pipeline_window_resize( state->pcm_node->window,  );
}

void _zdj_waveform_display_update_wait( zdj_pipeline_node_t * node ) {
    // Fill infill window with any available data from PCM node.

    // Update PCM node's infill window based on current infill window.
    
    // If PCM update cycle is needed, and PCM node isn't running, start it.
}

void _zdj_waveform_display_deinit_state( zdj_pipeline_node_t * node ) {

}
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/buffer/zdj_audio_buffer_node.h>


static float * _adj_audio_buffer_node_get_data( zdj_pipeline_node_t * node );
static void _zdj_audio_buffer_node_deinit_state( zdj_pipeline_node_t * node );

zdj_pipeline_node_t * zdj_new_audio_buffer_node( int buffer_sample_count, zdj_audio_buffer_stereo_mode_t stereo_mode ) {
    // printf( "zdj_new_audio_buffer_node channels: %d\n", stereo_mode );
    zdj_pipeline_node_t * node = zdj_new_pipeline_node( );
    node->get_data = &_adj_audio_buffer_node_get_data;
    node->deinit_state = &_zdj_audio_buffer_node_deinit_state;

    // Set up state.
    zdj_audio_buffer_node_state_t * state = calloc( 1, sizeof( zdj_audio_buffer_node_state_t ) );
    node->state = state;
    state->sample_count = buffer_sample_count;
    state->stereo_mode = stereo_mode;
    // Hardcode stereo buffers, because at this point we don't know if this node is
    // half of a stereo pair.
    state->buffer = calloc( buffer_sample_count * 2, sizeof( float ) );
    memset( state->buffer, 0, sizeof( float ) * buffer_sample_count * stereo_mode );

    return node;
}

float * _adj_audio_buffer_node_get_data( zdj_pipeline_node_t * node ) {
    zdj_audio_buffer_node_state_t * state = (zdj_audio_buffer_node_state_t*)node->state;
    return state->buffer;
}

void _zdj_audio_buffer_node_deinit_state( zdj_pipeline_node_t * node ) {
    zdj_audio_buffer_node_state_t * state = (zdj_audio_buffer_node_state_t *)node->state;
    free( state->buffer );
    free( state );
    node->state = NULL;
}
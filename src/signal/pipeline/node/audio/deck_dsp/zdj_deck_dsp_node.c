#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/deck_dsp/zdj_deck_dsp_node.h>
#include <zerodj/signal/pipeline/node/audio/deck_dsp/eq/zdj_eq_node.h>
#include <zerodj/signal/pipeline/node/audio/deck_dsp/filter/zdj_filter_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

zdj_pipeline_node_t * zdj_new_deck_dsp_node( void ) {
    zdj_pipeline_node_t * node = calloc( 1, sizeof( zdj_pipeline_node_t ) );
    
    zdj_deck_dsp_node_state_t * state = calloc( 1, sizeof( zdj_deck_dsp_node_state_t ) );
    node->state = state;
    state->eq_node = zdj_new_basic_eq_node( );
    
    return node;
}

void zdj_deck_dsp_node_process_buffer( 
    zdj_pipeline_node_t * node, 
    float * in, 
    float * out, 
    int channel_count, 
    int sample_count 
) {
    // printf( "zdj_deck_dsp_node_process_buffer\n" );
    memcpy( out, in, ZDJ_SOUNDCARD_BUF_LEN * channel_count * sizeof( float ) );
    // printf( "zdj_deck_dsp_node_process_buffer done\n" );
}

void zdj_deck_dsp_node_push_buffer( 
    zdj_pipeline_node_t * node, 
    float * in, 
    int channel_count, 
    int sample_count 
) {
    // printf( "zdj_deck_dsp_node_push_buffer\n" );
    zdj_deck_dsp_node_state_t * state = (zdj_deck_dsp_node_state_t*)node->state;
    memcpy( state->in_buf, in, ZDJ_SOUNDCARD_BUF_LEN * channel_count * sizeof( float ) );
    // printf( "zdj_deck_dsp_node_push_buffer done\n" );
}

void zdj_deck_dsp_node_pull_buffer( 
    zdj_pipeline_node_t * node, 
    float * out, 
    int channel_count, 
    int sample_count 
) {
    // printf( "zdj_deck_dsp_node_pull_buffer\n" );
    zdj_deck_dsp_node_state_t * state = (zdj_deck_dsp_node_state_t*)node->state;
    memcpy( out, state->in_buf, ZDJ_SOUNDCARD_BUF_LEN * channel_count * sizeof( float ) );
    // printf( "zdj_deck_dsp_node_pull_buffer done\n" );
}
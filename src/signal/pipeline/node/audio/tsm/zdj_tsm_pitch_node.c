#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/signal/pipeline/node/audio/tsm/zdj_tsm_pitch_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

static void _update_wait( zdj_pipeline_node_t * node );
static void _deinit_state( zdj_pipeline_node_t * node );

zdj_pipeline_node_t * zdj_new_tsm_pitch_node( 
    bool stereo, 
    int sample_count,
    zdj_pipeline_node_t * decode_node
) {
    // Make node
    zdj_pipeline_node_t * node = zdj_new_pipeline_node( );
    node->deinit_state = &_deinit_state;
    node->update_wait = &_update_wait;

    // Add state
    zdj_tsm_pitch_node_state_t * state = calloc( 1, sizeof( zdj_tsm_pitch_node_state_t ) );
    node->state = state;
    state->stereo = stereo;
    state->channel_count = stereo + 1;
    state->sample_count = sample_count;
    state->decode_node = decode_node;

    // Capture starting state of decode out_buf indexes
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)decode_node->state;
    state->decode_buf_ref_coord = decode_state->head_decode_addr - decode_state->head_win_start;
    state->decode_start_coord = 0;

    // Alloc output buffer
    state->out_buffer = calloc( state->sample_count * state->channel_count, sizeof( float ) );

    return node;
}

// Calculate this buffer's start/stop address and interpolate decode samples into out_buffer
static void _update_wait( zdj_pipeline_node_t * node ) {
    zdj_tsm_pitch_node_state_t * state = (zdj_tsm_pitch_node_state_t*)node->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)state->decode_node->state;

    // printf( "tsm update_wait decd %ld>%ld>%ld ref: %1.1f st:%1.1f en:%1.1f\n", 
    //     decode_state->head_win_start,
    //     decode_state->head_decode_addr,
    //     decode_state->head_win_end,
    //     state->decode_buf_ref_coord, 
    //     state->decode_start_coord,
    //     state->decode_end_coord
    // );

    // printf( "tsm: %1.1f -> %1.1f\n", state->decode_start_coord, state->decode_end_coord );

    // Interp from rate-based decode buf coords to full width of tsm buf.
    zdj_signal_naive_resample_audio( 
        decode_state->out_buffer,
        state->decode_start_coord,
        state->decode_end_coord,
        state->decode_buf_ref_coord,
        decode_state->channel_count,
        state->out_buffer,
        ZDJ_SOUNDCARD_BUF_LEN,
        state->channel_count
    );

    // Naïve copy
    // printf( "--- TSM naive copy:\n" );
    // for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
    //     state->out_buffer[ i ] = decode_state->out_buffer[ i ];
    //     // printf( "%1.1f -> %1.1f\n", decode_state->out_buffer[ i ], state->out_buffer[ i ] ); 
    // }

    // Start next interp from end of current interp
    state->decode_start_coord = state->decode_end_coord;
}

static void _deinit_state( zdj_pipeline_node_t * node ) {
    zdj_tsm_pitch_node_state_t * state = (zdj_tsm_pitch_node_state_t*)node->state;
    if( state->out_buffer ) { free( state->out_buffer ); }
    if( state ) { node->state = NULL; free( state );  }
}
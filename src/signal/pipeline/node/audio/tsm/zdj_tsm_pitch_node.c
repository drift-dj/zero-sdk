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
    // zdj_decode_node_capture_mono_addr( decode_node, &state->decode_mono_addr_snapshot );

    // Alloc output buffer
    state->out_buffer = calloc( state->sample_count * state->channel_count, sizeof( float ) );

    return node;
}

// Calculate this buffer's start/stop address and interpolate decode samples into out_buffer
static void _update_wait( zdj_pipeline_node_t * node ) {
    zdj_tsm_pitch_node_state_t * state = (zdj_tsm_pitch_node_state_t*)node->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)state->decode_node->state;

    int tsm_buf_len = ZDJ_SOUNDCARD_BUF_LEN;
    // Figure out the distance (in decode out_buf index space) covered
    // by this buffer.  Based on current playback rate.
    double interp_distance = state->rate * (double)tsm_buf_len;

    // Ask the decode node to transform our last set of interp coords 
    // into the new set of coords based on how far decode window has moved
    // since our last tsm node update_wait() call.
    // zdj_decode_node_xform_tsm_coords_for_captured_mono_addr( 
    //     state->decode_node,
    //     &state->decode_start_coord,
    //     &state->decode_end_coord,
    //     &state->decode_mono_addr_snapshot
    // );

    // Start interp from end of last interp
    state->decode_start_coord = state->decode_end_coord;
    // End interp by arithmetic of start coord + distance
    state->decode_end_coord = state->decode_start_coord + interp_distance;

    // Interp from rate-based decode buf coords to full width of tsm buf.
    zdj_signal_resample_audio( 
        decode_state->out_buffer,
        state->decode_start_coord,
        state->decode_end_coord,
        decode_state->channel_count,
        state->out_buffer,
        0.0,
        (double)tsm_buf_len,
        state->channel_count
    );

    // Naïve copy
    // printf( "--- TSM naive copy:\n" );
    for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
        state->out_buffer[ i ] = decode_state->out_buffer[ i ];
        // printf( "%1.1f -> %1.1f\n", decode_state->out_buffer[ i ], state->out_buffer[ i ] ); 
    }
}

static void _deinit_state( zdj_pipeline_node_t * node ) {
    zdj_tsm_pitch_node_state_t * state = (zdj_tsm_pitch_node_state_t*)node->state;
    if( state->out_buffer ) { free( state->out_buffer ); }
    if( state ) { node->state = NULL; free( state );  }
}
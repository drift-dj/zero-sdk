#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/analysis/meter/zdj_meter_node.h>
#include <zerodj/signal/pipeline/node/audio/buffer/zdj_audio_buffer_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

static void _zdj_audio_meter_node_update( zdj_pipeline_node_t * node );
static void _zdj_clock_meter_node_update( zdj_pipeline_node_t * node );

zdj_pipeline_node_t * zdj_new_meter_node( 
    zdj_meter_node_type_t type, 
    zdj_pipeline_node_t * signal_node 
) {
    zdj_pipeline_node_t * node = zdj_new_pipeline_node( );
    zdj_meter_node_state_t * state = calloc( 1, sizeof( zdj_meter_node_state_t ) );
    node->state = state;
    state->type = type;

    zdj_audio_buffer_node_state_t * signal_node_state = (zdj_audio_buffer_node_state_t*)signal_node->state;
    state->signal_node = signal_node;
    
    switch ( type ) {
    case ZDJ_METER_NODE_TYPE_AUDIO:
        node->update_wait = &_zdj_audio_meter_node_update;
        state->channel_count = signal_node_state->stereo_mode;
        break;
    case ZDJ_METER_NODE_TYPE_CLOCK:
        node->update_wait = &_zdj_clock_meter_node_update;
        break;
    case ZDJ_METER_NODE_TYPE_CV:
    case ZDJ_METER_NODE_TYPE_MIDI:
        break;
    }
    return node;
}

// Synchronous update calls to be run in soundcard fast-cycle thread
static float audio_ol_0_thresh = 0.85;
static float audio_ol_1_thresh = 0.9; 
static float audio_clip_thresh = 0.95;
void _zdj_audio_meter_node_update( zdj_pipeline_node_t * node ) {
    zdj_meter_node_state_t * state = (zdj_meter_node_state_t*)node->state;
    zdj_audio_buffer_node_state_t * signal_node_state = (zdj_audio_buffer_node_state_t*)state->signal_node->state;
    float * sig_buf = state->signal_node->get_data( state->signal_node );
    // Accumulate samples in signal_buffer into audio values
    if( state->channel_count == 1 ) {
        float avg = 0;
        float max = 0;
        for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
            avg += fabs( sig_buf[ i ] );
            max = fmax( max, fabs( sig_buf[ i ] ) );
        }
        state->buffer_avg_0 = avg / (float)ZDJ_SOUNDCARD_BUF_LEN;
        if( avg > audio_ol_0_thresh ) { state->has_ol_0_0 = true; }
        if( avg > audio_ol_1_thresh ) { state->has_ol_1_0 = true; }
        if( avg > audio_clip_thresh ) { state->has_clip_0 = true; }
    } else if ( state->channel_count == 2 ) {
        float avg_0 = 0;
        float max_0 = 0;
        float avg_1 = 0;
        float max_1 = 0;
        for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
            avg_0 += fabs( sig_buf[ i*2 ] );
            max_0 = fmax( max_0, fabs( sig_buf[ i*2 ] ) );
            avg_1 += fabs( sig_buf[ i*2+1 ] );
            max_1 = fmax( max_1, fabs( sig_buf[ i*2+1 ] ) );
        }
        state->buffer_avg_0 = avg_0 / (float)ZDJ_SOUNDCARD_BUF_LEN;
        if( avg_0 > audio_ol_0_thresh ) { state->has_ol_0_0 = true; }
        if( avg_0 > audio_ol_1_thresh ) { state->has_ol_1_0 = true; }
        if( avg_0 > audio_clip_thresh ) { state->has_clip_0 = true; }
        state->buffer_avg_1 = avg_1 / (float)ZDJ_SOUNDCARD_BUF_LEN;
        if( avg_1 > audio_ol_0_thresh ) { state->has_ol_0_1 = true; }
        if( avg_1 > audio_ol_1_thresh ) { state->has_ol_1_1 = true; }
        if( avg_1 > audio_clip_thresh ) { state->has_clip_1 = true; }
    }
    // Update OL/clip lamp timers/vals
    int clip_duration = 100;
    if( state->has_ol_0_0 && state->timer_ol_0_0++ < clip_duration ) {
        state->has_ol_0_0 = false; state->timer_ol_0_0 = 0;
    }
    if( state->has_ol_1_0 && state->timer_ol_1_0++ < clip_duration ) {
        state->has_ol_1_0 = false; state->timer_ol_1_0 = 0;
    }
    if( state->has_clip_0 && state->timer_clip_0++ < clip_duration ) {
        state->has_clip_0 = false; state->timer_clip_0 = 0;
    }
    if( state->has_ol_0_1 && state->timer_ol_0_1++ < clip_duration ) {
        state->has_ol_0_1 = false; state->timer_ol_0_1 = 0;
    }
    if( state->has_ol_1_1 && state->timer_ol_1_1++ < clip_duration ) {
        state->has_ol_1_1 = false; state->timer_ol_1_1 = 0;
    }
    if( state->has_clip_1 && state->timer_clip_1++ < clip_duration ) {
        state->has_clip_1 = false; state->timer_clip_1 = 0;
    }
}

void _zdj_clock_meter_node_update( zdj_pipeline_node_t * node ) {
    // Accumulate samples in signal_buffer into audio values
}
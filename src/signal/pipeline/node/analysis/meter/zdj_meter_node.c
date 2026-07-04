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

static void _zdj_audio_meter_add_frame( zdj_pipeline_node_t * meter_node, float val_0, float val_1 );
static void _zdj_clock_meter_add_frame( zdj_pipeline_node_t * meter_node, float val_0, float val_1 );
static void _zdj_cv_meter_add_frame( zdj_pipeline_node_t * meter_node, float val_0, float val_1 );

zdj_pipeline_node_t * zdj_new_meter_node( zdj_meter_node_type_t type, int channel_count ) {
    zdj_pipeline_node_t * node = zdj_new_pipeline_node( );
    zdj_meter_node_state_t * state = calloc( 1, sizeof( zdj_meter_node_state_t ) );
    node->state = state;
    state->type = type;
    
    switch ( type ) {
    case ZDJ_METER_NODE_TYPE_AUDIO:
        state->add_frame = &_zdj_audio_meter_add_frame;
        state->channel_count = channel_count;
        break;
    case ZDJ_METER_NODE_TYPE_CLOCK:
        state->add_frame = &_zdj_clock_meter_add_frame;
        break;
    case ZDJ_METER_NODE_TYPE_CV:
        state->add_frame = &_zdj_cv_meter_add_frame;
        break;
    case ZDJ_METER_NODE_TYPE_MIDI:
        break;
    }
    return node;
}

// Reset all state values to zero
zdj_error_type_t zdj_meter_node_reset( zdj_pipeline_node_t * meter_node ) {
    zdj_meter_node_state_t * state = (zdj_meter_node_state_t*)meter_node->state;
    state->instant_val_0 = 0;
    state->instant_val_1 = 0;
    state->has_ol_0_0 = false;
    state->has_ol_1_0 = false;
    state->has_clip_0 = false;
    state->has_ol_0_1 = false;
    state->has_ol_1_1 = false;
    state->has_clip_1 = false;
}

// Synchronous update calls to be run in soundcard fast-cycle thread
static float audio_ol_0_thresh = 0.85;
static float audio_ol_1_thresh = 0.9; 
static float audio_clip_thresh = 0.95;
static int clip_duration = 100;
static void _zdj_audio_meter_add_frame( zdj_pipeline_node_t * meter_node, float val_0, float val_1 ) {
    zdj_meter_node_state_t * state = (zdj_meter_node_state_t*)meter_node->state;
    
    // Very slight lowpass on the instant val, to keep peaking sane.
    val_0 *= 0.96;
    if( val_0 > state->instant_val_0 ) {
        state->instant_val_0 = val_0;
    } else {
        state->instant_val_0 *= 0.99;
    }
    if( val_0 > audio_ol_0_thresh ) { state->has_ol_0_0 = true; }
    if( val_0 > audio_ol_1_thresh ) { state->has_ol_1_0 = true; }
    if( val_0 > audio_clip_thresh ) { state->has_clip_0 = true; }
    if( state->channel_count > 1 ) {
        val_1 *= 0.96;
        if( val_1 > state->instant_val_1 ) {
            state->instant_val_1 = val_1;
        } else {
            state->instant_val_1 *= 0.99;
        }
        if( val_1 > audio_ol_0_thresh ) { state->has_ol_0_1 = true; }
        if( val_1 > audio_ol_1_thresh ) { state->has_ol_1_1 = true; }
        if( val_1 > audio_clip_thresh ) { state->has_clip_1 = true; }
    }
    
    // Update OL/clip lamp timers/vals
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

void _zdj_clock_meter_add_frame( zdj_pipeline_node_t * meter_node, float val_0, float val_1 ) {
    // Accumulate samples in signal_buffer into audio values
}

void _zdj_cv_meter_add_frame( zdj_pipeline_node_t * meter_node, float val_0, float val_1 ) {
    zdj_meter_node_state_t * state = (zdj_meter_node_state_t*)meter_node->state;
    // Accumulate samples in signal_buffer into cv values
    state->instant_val_0 = val_0;
}
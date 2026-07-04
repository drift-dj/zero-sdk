#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/zdj_deck_manager.h>
#include <zerodj/signal/deck/xport/zdj_deck_xport.h>
#include <zerodj/signal/math/zdj_signal_math.h>
// #include <zerodj/signal/soundcard/zdj_soundcard.h>
// #include <zerodj/signal/pipeline/zdj_pipeline.h>

static void _set_sync_bpm( zdj_deck_t * deck, double bpm );
static void _offset_sync_bpm( zdj_deck_t * deck, double offset );
static void _offset_pitch_setting( zdj_deck_t * deck, double offset );
static void _request_sync_mult( zdj_deck_t * deck, float factor );

void zdj_deck_xport_init_sync( zdj_deck_t * deck ) {
    zdj_xport_deck_state_t * state = (zdj_xport_deck_state_t*)deck->state;

    // Setup sync state
    deck->set_sync_bpm = &_set_sync_bpm;
    deck->offset_sync_bpm = &_offset_sync_bpm;
    deck->offset_pitch_setting = &_offset_pitch_setting;
    deck->request_sync_mult = &_request_sync_mult;

    deck->can_sync = true;
    deck->sync_factor = 1.0;

    if( zdj_deck_manager( )->sync.preferred &&
        zdj_deck_manager_can_activate_sync( )
    ) {
        if( zdj_deck_manager( )->sync.active && zdj_deck_manager( )->sync.locked ) {
            // Adopt the root tempo if it's been set already.
            deck->set_sync_bpm( deck, zdj_deck_manager( )->sync.set_bpm );
        }
    }
}

static void _set_sync_bpm( zdj_deck_t * deck, double bpm ) {
    zdj_xport_deck_state_t * deck_state = (zdj_xport_deck_state_t*)deck->state;
    deck_state->set_bpm = bpm;
    deck->controls.platter.motor.pitch_setting = deck_state->set_bpm / 120.0;
}

static void _offset_sync_bpm( zdj_deck_t * deck, double offset ) {
    zdj_xport_deck_state_t * deck_state = (zdj_xport_deck_state_t*)deck->state;
    deck_state->set_bpm += offset;
    deck->controls.platter.motor.pitch_setting = deck_state->set_bpm / 120.0;
}

static void _offset_pitch_setting( zdj_deck_t * deck, double offset ) {
    deck->controls.platter.motor.pitch_setting += offset;
}

static void _request_sync_mult( zdj_deck_t * deck, float factor ) {
    zdj_xport_deck_state_t * deck_state = (zdj_xport_deck_state_t*)deck->state;
    deck->sync_factor = factor;
    deck->controls.platter.motor.pitch_setting = (deck_state->set_bpm * deck->sync_factor) / 120.0;
}
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/zdj_deck_manager.h>
#include <zerodj/signal/deck/dj/zdj_deck_dj.h>
#include <zerodj/signal/math/zdj_signal_math.h>
// #include <zerodj/signal/soundcard/zdj_soundcard.h>
// #include <zerodj/signal/pipeline/zdj_pipeline.h>

static void _set_sync_bpm( zdj_deck_t * deck, double bpm );
static void _offset_sync_bpm( zdj_deck_t * deck, double offset );

void zdj_deck_dj_init_sync( zdj_deck_t * deck ) {
    // printf( "zdj_deck_dj_init_sync\n" );
    zdj_dj_deck_state_t * state = (zdj_dj_deck_state_t*)deck->state;

    // Setup sync state
    deck->set_sync_bpm = &_set_sync_bpm;
    deck->offset_sync_bpm = &_offset_sync_bpm;
    deck->can_sync = true;

    // Ignore sync state if song BPM ~= 0.0.
    if( state->song->performance && fabs( state->song->performance->bpm ) > zdj_eps ) { 
        if( zdj_deck_manager( )->sync.enabled ) {
            if( zdj_deck_manager( )->sync.locked ) {
                // Adopt the root tempo if it's been set already.
                deck->set_sync_bpm( deck, zdj_deck_manager( )->sync.set_bpm );
            } else {
                // Else, lock the root tempo to the deck's song's tempo.
                deck->set_sync_bpm( deck, state->song->performance->bpm );
                zdj_deck_manager( )->sync.locked = true;
                zdj_deck_manager( )->sync.set_bpm = state->song->performance->bpm;
            }
        } else {
            deck->set_sync_bpm( deck, state->song->performance->bpm );
        }
    }
    // printf( "zdj_deck_dj_init_sync done\n" );
}

static void _set_sync_bpm( zdj_deck_t * deck, double bpm ) {

    // printf( "dj deck %d set bpm: %1.2f\n", deck->station, bpm );

    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    deck_state->set_bpm = bpm;

    if( deck_state->song->performance && (fabs( deck_state->song->performance->bpm ) > zdj_eps) ) {
        deck->controls.platter.motor.pitch_setting = deck_state->set_bpm / deck_state->song->performance->bpm;
        // printf( "pitch set: %1.2f\n", deck->controls.platter.motor.pitch_setting );
    }
}

static void _offset_sync_bpm( zdj_deck_t * deck, double offset ) {

    // printf( "dj deck offset bpm: %1.4f\n", offset );

    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    deck_state->set_bpm += offset;

    if( deck_state->song->performance && (fabs( deck_state->song->performance->bpm ) > zdj_eps) ) {
        deck->controls.platter.motor.pitch_setting = deck_state->set_bpm / deck_state->song->performance->bpm;
        // printf( "pitch set: %1.2f\n", deck->controls.platter.motor.pitch_setting );
    }
}
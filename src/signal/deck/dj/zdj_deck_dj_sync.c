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
static void _offset_pitch_setting( zdj_deck_t * deck, double offset );
static void _request_sync_mult( zdj_deck_t * deck, float factor );

void zdj_deck_dj_init_sync( zdj_deck_t * deck ) {
    zdj_dj_deck_state_t * state = (zdj_dj_deck_state_t*)deck->state;

    // Setup sync state
    deck->set_sync_bpm = &_set_sync_bpm;
    deck->offset_sync_bpm = &_offset_sync_bpm;
    deck->offset_pitch_setting = &_offset_pitch_setting;
    deck->request_sync_mult = &_request_sync_mult;

    state->sync_mult_ui_counter = 0;
    
    if( state->song->performance && 
        fabs( state->song->performance->bpm ) > zdj_eps
    ) { deck->can_sync = true; } else { deck->can_sync = false; }

    deck->sync_factor = 1.0;

    // printf( "===> zdj_deck_dj_init_sync can_sync:%d\n", deck->can_sync );

    if( deck->can_sync &&
        zdj_deck_manager( )->sync.preferred &&
        zdj_deck_manager_can_activate_sync( )
    ) {
        if( zdj_deck_manager( )->sync.active && zdj_deck_manager( )->sync.locked ) {
            // printf( "===> New DJ Deck syncing to: %1.1f\n", zdj_deck_manager( )->sync.set_bpm );
            // Adopt the root tempo if it's been set already.
            deck->set_sync_bpm( deck, zdj_deck_manager( )->sync.set_bpm );
        } else {
            // printf( "===> New DJ Deck setting sync tempo to: %1.1f\n", state->song->performance->bpm );
            // Else, lock the root tempo to the deck's song's tempo.
            zdj_deck_manager_set_sync( state->song->performance->bpm );
            // zdj_deck_manager( )->sync.active = true;
            // zdj_deck_manager( )->sync.locked = true;
            // zdj_deck_manager( )->sync.set_bpm = state->song->performance->bpm;
        }
    } else if( !deck->can_sync ) {
        zdj_deck_manager( )->sync.active = false;
    }
    // printf( "zdj_deck_dj_init_sync done\n" );
}

static void _set_sync_bpm( zdj_deck_t * deck, double bpm ) {

    // printf( "dj deck %d set bpm: %1.2f\n", deck->station, bpm );

    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    deck_state->set_bpm = bpm;

    if( deck_state->song->performance && (fabs( deck_state->song->performance->bpm ) > zdj_eps) ) {
        // deck->controls.platter.motor.pitch_setting = deck_state->set_bpm / deck_state->song->performance->bpm;
        deck->controls.platter.motor.pitch_setting = (deck_state->set_bpm * deck->sync_factor) / deck_state->song->performance->bpm;
        // printf( "pitch set: %1.2f\n", deck->controls.platter.motor.pitch_setting );
    }
}

static void _offset_sync_bpm( zdj_deck_t * deck, double offset ) {
    // printf( "dj deck offset bpm: %1.4f\n", offset );

    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    if( deck_state->set_bpm < 0.01 ) { return; }
    deck_state->set_bpm += offset;

    if( deck_state->song->performance && (fabs( deck_state->song->performance->bpm ) > zdj_eps) ) {
        deck->controls.platter.motor.pitch_setting = deck_state->set_bpm / deck_state->song->performance->bpm;
        deck->controls.platter.motor.pitch_setting = (deck_state->set_bpm * deck->sync_factor) / deck_state->song->performance->bpm;
        // printf( "pitch set: %1.2f\n", deck->controls.platter.motor.pitch_setting );
    }
}

static void _offset_pitch_setting( zdj_deck_t * deck, double offset ) {
    // printf( "dj deck offset pitch setting: %1.4f\n", offset );
    deck->controls.platter.motor.pitch_setting += offset;
    // printf( "pitch set: %1.2f\n", deck->controls.platter.motor.pitch_setting );
}

static void _request_sync_mult( zdj_deck_t * deck, float factor ) {
    // printf( "dj deck request_sync_mult: %1.1f\n", factor );
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    
    deck->sync_factor = factor;

    if( deck_state->song->performance && (fabs( deck_state->song->performance->bpm ) > zdj_eps) ) {
        deck->controls.platter.motor.pitch_setting = (deck_state->set_bpm * deck->sync_factor) / deck_state->song->performance->bpm;
    }
}
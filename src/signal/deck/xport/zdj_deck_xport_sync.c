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

void zdj_deck_xport_init_sync( zdj_deck_t * deck ) {
    zdj_xport_deck_state_t * state = (zdj_xport_deck_state_t*)deck->state;

    // Setup sync state
    deck->set_sync_bpm = &_set_sync_bpm;
    deck->offset_sync_bpm = &_offset_sync_bpm;
    deck->offset_pitch_setting = &_offset_pitch_setting;

    // // printf( "===> zdj_deck_dj_init_sync can_sync:%d\n", deck->can_sync );

    if( zdj_deck_manager( )->sync.preferred &&
        zdj_deck_manager_can_activate_sync( )
    ) {
        if( zdj_deck_manager( )->sync.active && zdj_deck_manager( )->sync.locked ) {
            // printf( "===> New DJ Deck syncing to: %1.1f\n", zdj_deck_manager( )->sync.set_bpm );
            // Adopt the root tempo if it's been set already.
            deck->set_sync_bpm( deck, zdj_deck_manager( )->sync.set_bpm );
        } else {
            // printf( "===> New DJ Deck setting sync tempo to: %1.1f\n", state->song->performance->bpm );
            // Else, lock the root tempo to the deck's song's tempo.
            // deck->set_sync_bpm( deck, state->set_bpm );
            // zdj_deck_manager( )->sync.active = true;
            // zdj_deck_manager( )->sync.locked = true;
            // zdj_deck_manager( )->sync.set_bpm = state->set_bpm;
        }
    }
    // printf( "zdj_deck_dj_init_sync done\n" );
}

static void _set_sync_bpm( zdj_deck_t * deck, double bpm ) {

    // printf( "dj deck %d set bpm: %1.2f\n", deck->station, bpm );

    zdj_xport_deck_state_t * deck_state = (zdj_xport_deck_state_t*)deck->state;
    deck_state->set_bpm = bpm;

    deck->controls.platter.motor.pitch_setting = deck_state->set_bpm / 120.0;
    // printf( "pitch set: %1.2f\n", deck->controls.platter.motor.pitch_setting );
}

static void _offset_sync_bpm( zdj_deck_t * deck, double offset ) {
    printf( "clock deck offset bpm: %1.4f\n", offset );

    zdj_xport_deck_state_t * deck_state = (zdj_xport_deck_state_t*)deck->state;
    deck_state->set_bpm += offset;

    deck->controls.platter.motor.pitch_setting = deck_state->set_bpm / 120.0;
    // printf( "pitch set: %1.2f\n", deck->controls.platter.motor.pitch_setting );
}

static void _offset_pitch_setting( zdj_deck_t * deck, double offset ) {
    // printf( "dj deck offset pitch setting: %1.4f\n", offset );
    deck->controls.platter.motor.pitch_setting += offset;
    // printf( "pitch set: %1.2f\n", deck->controls.platter.motor.pitch_setting );
}
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/xport/zdj_deck_xport.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/signal/pipeline/node/audio/tsm/zdj_tsm_pitch_node.h>
#include <zerodj/signal/pipeline/node/audio/tsm/zdj_tsm_tempo_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

static void _update_platter_model_inputs( zdj_deck_t * deck );

void zdj_deck_xport_init_transport( zdj_deck_t * deck ) {
    deck->update_platter_model = &_update_platter_model_inputs;
}

void zdj_xport_deck_reset_platter( 
    zdj_deck_platter_t * platter, 
    double addr 
) {
    // printf( "zdj_dj_deck_reset_platter: %1.1f\n", addr );
    // platter->motor.head = addr;
    // platter->slip.offset = 0;
    // platter->slip.set_val = addr;
    // platter->slip.instant_val = addr;
    // platter->needle.head = addr;
}

////////////////////////////////
// WARNING !!!!!
// Clock synth is currently wired
// directly to motor.set_rate.
// Need to update to take output of
// platter model
////////////////////////////////

//////////////////////////////
// Platter Model
// Updated from control thread
// ~900 Hz
//////////////////////////////

static void _update_platter_model_inputs( zdj_deck_t * deck ) {
    // printf( "_update_platter_model_inputs %p\n", deck );
    zdj_xport_deck_state_t * deck_state = (zdj_xport_deck_state_t*)deck->state;
    if( deck->status < ZDJ_DECK_STATUS_RUNNING ){ return; }

    // Get platter models
    zdj_deck_platter_t * platter = &deck->controls.platter;
    zdj_deck_platter_motor_t * motor = &platter->motor;
    zdj_deck_platter_slip_t * slip = &platter->slip;

    // Motor
    // -----

    // Move the motor based on set rate.
    // Set motor instant rate based on any active spin_up/down events
    float spin_mod;
    switch( motor->state ) {
        case ZDJ_PLATTER_MOTOR_IDLE:
            // printf( "ZDJ_PLATTER_MOTOR_IDLE\n" );
            motor->instant_rate = 0.0f;
            break;
    
        case ZDJ_PLATTER_MOTOR_RUN:
            // printf( "ZDJ_PLATTER_MOTOR_RUN\n" );
            motor->set_rate = motor->pitch_setting;
            // motor->instant_rate = motor->set_rate;
            // This is a hack since clock synth directly reads motor.set_rate
            motor->instant_rate = motor->set_rate + platter->slip.instant_val;
            // printf( "motor rt:%1.3f slip val:%1.5f\n", motor->instant_rate, platter->slip.instant_val );
            platter->slip.instant_val *= 0.7;
            break;
    }
}
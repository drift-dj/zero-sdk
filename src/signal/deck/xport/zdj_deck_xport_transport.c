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

// static void _update_platter_model( zdj_deck_t * deck );
static void _update_platter_model_inputs( zdj_deck_t * deck );
static void _update_platter_model_outputs( zdj_deck_t * deck );
static void _reset_tsm_tempo_node_to_decode_addr( zdj_pipeline_node_t * node, double addr );

void zdj_deck_xport_init_transport( zdj_deck_t * deck ) {
    deck->update_transport_inputs = &_update_platter_model_inputs; // Control thread @900Hz
    deck->update_transport_outputs = &_update_platter_model_outputs; // Soundcard thread @110Hz
}

void zdj_xport_deck_reset_platter( 
    zdj_deck_platter_t * platter, 
    double addr 
) {
    // printf( "zdj_dj_deck_reset_platter: %1.1f\n", addr );
    platter->motor.head = addr;
    platter->slip.offset = 0;
    platter->slip.set_val = addr;
    platter->slip.instant_val = addr;
    platter->needle.head = addr;
}

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
            motor->instant_rate = motor->set_rate;
            break;
    }
}

static void _update_platter_model_outputs( zdj_deck_t * deck ) {
    // printf( "_update_platter_model_outputs %p\n", deck );
    zdj_xport_deck_state_t * deck_state = (zdj_xport_deck_state_t*)deck->state;

    // Get platter models
    zdj_deck_platter_t * platter = &deck->controls.platter;
    zdj_deck_platter_motor_t * motor = &platter->motor;
    zdj_deck_platter_slip_t * slip = &platter->slip;

    double motor_factor = motor->instant_rate * ZDJ_SOUNDCARD_BUF_LEN;

    // Slipmat + Needle Head
    // ---------------------

    // Update the slip map sim.
    if( slip->state == ZDJ_PLATTER_SLIP_LAMINAR_PITCH ) {
        // If we're not slipping, don't adjust slip offset
        motor->head += motor_factor;
        slip->set_val += motor_factor;
        slip->instant_val = slip->set_val;

        // Update the needle head
        platter->needle.head = slip->offset + slip->instant_val;

    } else if( slip->state == ZDJ_PLATTER_SLIP_NUDGE_PITCH ) {
        // If we're slipping, offset the slipmat angle from the platter angle
        motor->head += motor_factor;
        slip->offset += motor_factor;
        // If we're slipping, update damped spring simulation on slipmat
        double displacement = slip->set_val - slip->instant_val;
        // Calculate acceleration using the damped spring equation
        double acceleration = (-slip->spring_k * displacement - slip->damp_c * slip->velocity) / slip->mass;
        // Using Euler here... switch to RK 2/4 if needed.
        double step_val = (1.0/slip->sim_duration);
        slip->velocity += acceleration * step_val;
        displacement += slip->velocity * step_val;
        
        // Update outputs
        slip->instant_val = slip->set_val - displacement;

        // Keep the sim running while jog is settling
        if( fabs( displacement ) > 10 ) { slip->sim_counter = 0; }

        // Once the slip dwell expires, return to laminar mode
        if( slip->sim_counter++ > slip->slip_dwell ) {
            slip->state = ZDJ_PLATTER_SLIP_TO_LAM;
        } 
         
        // Update the needle head
        platter->needle.head = slip->offset + slip->instant_val;
        // printf( "nudge pitch needle.head: %1.3f\n", platter->needle.head );

    } else if( slip->state == ZDJ_PLATTER_SLIP_TO_LAM ) {
        slip->state = ZDJ_PLATTER_SLIP_LAMINAR_PITCH;
        // printf( "slip to lam needle.head: %1.3f\n", platter->needle.head );
    }
    
    // printf( "needle.head: %1.3f\n", platter->needle.head );

    // if( deck->status == ZDJ_DECK_STATUS_WAIT_SPOOLDOWN && slip->instant_val < 0.001 ) {
    if( deck->status == ZDJ_DECK_STATUS_WAIT_SPOOLDOWN ) {
        // printf( "spooldown i_rate: %1.4f needle.head: %1.3f\n", motor->instant_rate, platter->needle.head );
        if( motor->instant_rate < 0.001 ) { deck->safe_to_deinit = true; }
    }

    // printf( "%1.1f\n", deck->controls.platter.motor.set_rate );
    // printf( "needle.head: %1.1f\n" );
}
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <math.h>

#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/lib/zdj_deck_lib.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

zdj_deck_t * zdj_new_deck( zdj_deck_type_t type, zdj_deck_station_t station, void * resource ) {
    zdj_deck_t * deck = calloc( 1, sizeof( zdj_deck_t ) );
    deck->type = type;
    deck->station = station;
    deck->status = ZDJ_DECK_STATUS_NEW;

    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_DECK_INIT;
    switch ( type ) {
        case ZDJ_DECK_TYPE_LIB: zdj_new_lib_deck( deck, resource ); return deck;
        case ZDJ_DECK_TYPE_DJ: zdj_new_dj_deck( deck, resource ); return deck;
        case ZDJ_DECK_TYPE_EXTERNAL: zdj_new_extern_deck( deck ); return deck;
        case ZDJ_DECK_TYPE_TEST:
            // deck->get_edge_data = &_zdj_deck_get_test_data;
            // zdj_test_deck_state_t * state = calloc( 1, sizeof( zdj_test_deck_state_t ) );
            // deck->state = state;
            // state->s1_p = 0;
            // state->s1_f = 945;
            // state->s2_p = 0;
            // state->s2_f = 7;
            // state->s3_p = 0;
            // state->s3_f = 1021;
            // state->s4_p = 0;
            // state->s4_f = 3;
            break;
    }
    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_UNCLAIMED;
    return deck;
}

// Set the common params for a playback deck
void zdj_deck_init_controls( zdj_deck_t * deck ) {
    memset( &deck->controls, 0, sizeof( zdj_deck_control_state_t ) );
    deck->controls.platter.motor.pitch_setting = 1.0f;
    deck->controls.platter.motor.ramp_rate = 0.4f;

    deck->controls.platter.slip.slip_dwell = 20;
    deck->controls.platter.slip.set_val = 0.0f;
    deck->controls.platter.slip.instant_val = 0.0f;

    // Sim constants - adjust to taste
    deck->controls.platter.slip.sim_duration = (double)deck->controls.platter.slip.slip_dwell / 4.0;
    deck->controls.platter.slip.mass = 10.0;
    deck->controls.platter.slip.spring_k = 13.0;
    deck->controls.platter.slip.damp_c = 6.0;

    // Scratch/Nudge constants
    // These define the number of song PCM samples covered by a single step of the jog encoder.
    // Scratch should be orders of magnitude higher than nudge.
    // deck->controls.platter.nudge_coeff = 10;
    // deck->controls.platter.scratch_coeff = 700.0;
    deck->controls.platter.nudge_coeff = 0.1;
    deck->controls.platter.scratch_coeff = 3.0;

    // deck->controls.loop_state.phase = ZDJ_DECK_LOOP_PHASE_INACTIVE;
}

void zdj_clear_deck_control_flags( zdj_deck_t * deck ) {
    memset( deck->controls.control_change_flags, 0, ZDJ_CONTROL_ID_COUNT * sizeof( uint8_t ) );
}


// This is invoked during the deck manager control update cycle.
// Called on the soundcard fast audio cycle before mixdown.
void zdj_deck_handle_control( zdj_deck_t * deck, zdj_control_event_t * event ) {
    // printf( "zdj_deck_handle_control: %d\n", event->id );
    zdj_deck_platter_t * platter = &deck->controls.platter;

    switch ( event->id ) {
    case ZDJ_DECK_1_CONTROL_PLAY_PAUSE:
        if( platter->motor.set_rate < 0.01 ) {
            platter->motor.set_rate = platter->motor.pitch_setting;
            platter->motor.state = ZDJ_PLATTER_MOTOR_RUN;
        } else {
            platter->motor.set_rate = 0.0;
            platter->motor.state = ZDJ_PLATTER_MOTOR_IDLE;
        }
        printf( "lib deck toggle play/pause: %1.3f\n", platter->motor.set_rate );
        break;
    case ZDJ_DECK_1_CONTROL_HOTCUE_START:
        printf( "lib deck hotcue start\n" );
        platter->motor.set_rate = platter->motor.pitch_setting;
        break;
    case ZDJ_DECK_1_CONTROL_HOTCUE_END:
        printf( "lib deck hotcue end\n" );
        platter->motor.set_rate = 0.0;
        break;
    case ZDJ_DECK_1_CONTROL_SCRUB:
        // printf( "lib deck scrub\n" );
        if( platter->motor.state == ZDJ_PLATTER_MOTOR_RUN && !platter->nudge_override
        ) {
            // Nudge a running platter
            platter->slip.set_val += event->i_val * platter->nudge_coeff;
            platter->slip.state = ZDJ_PLATTER_SLIP_NUDGE;
        } else {
            // Scratch a non-running platter (or if user overrides nudge)
            platter->slip.set_val += event->i_val * platter->scratch_coeff;
            platter->slip.state = ZDJ_PLATTER_SLIP_SCRATCH;
        }
        platter->slip.sim_counter = 0;
        break;
    case ZDJ_DECK_1_CONTROL_TEMPO:
        platter->motor.pitch_setting += event->i_val * 0.02;
    case ZDJ_DECK_1_CONTROL_TEMPO_FINE:
        platter->motor.pitch_setting += event->i_val * 0.001;
        break;
    default:
        break;
    }
}

void zdj_deck_update_controls ( zdj_deck_t * deck ) {
    // printf( "zdj_deck_update_controls\n" );
    zdj_lib_deck_state_t * deck_state = (zdj_lib_deck_state_t*)deck->state;
    if( !deck_state->decode_node ) { return; }
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_platter_t * platter = &deck->controls.platter;
    zdj_deck_platter_motor_t * motor = &platter->motor;
    zdj_deck_platter_slip_t * slip = &platter->slip;

    // Move the motor based on set rate.
    motor->instant_rate = zdj_signal_lowpass( motor->instant_rate, motor->set_rate, motor->ramp_rate );
    double motor_factor = motor->instant_rate * ZDJ_SOUNDCARD_BUF_LEN;
    motor->head += motor_factor;

    // Update the slip map sim.
    if( slip->state == ZDJ_PLATTER_SLIP_LAMINAR ) {
        // If we're not slipping, don't adjust slip offset
        slip->set_val += motor_factor;
        slip->instant_val = slip->set_val;

    } else if( slip->state == ZDJ_PLATTER_SLIP_SCRATCH ) {
        // If we're slipping, offset the slipmat angle from the platter angle
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
        if( slip->sim_counter++ > slip->slip_dwell ) {
            slip->state = ZDJ_PLATTER_SLIP_TO_LAM;
        } 

    } else if( slip->state == ZDJ_PLATTER_SLIP_NUDGE ) {
        // If we're slipping, offset the slipmat angle from the platter angle
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
        if( slip->sim_counter++ > slip->slip_dwell ) {
            slip->state = ZDJ_PLATTER_SLIP_TO_LAM;
        } 
           
    } else if( slip->state == ZDJ_PLATTER_SLIP_TO_LAM ) {
        // Move the 
        slip->state = ZDJ_PLATTER_SLIP_LAMINAR;
    }

    platter->needle.head = slip->offset + slip->instant_val;

    // printf( "head: %1.3f off: %1.3f sl_i %1.3f sl_s %1.3f\n", 
    //     platter->needle.head,
    //     slip->offset,
    //     slip->instant_val,
    //     slip->set_val
    // );

    int window_move = round( platter->needle.head - (double)decode_state->head_decode_addr );
    // int window_move = platter->needle.head - (double)decode_state->head_decode_addr;

    // if( window_move != 0 ) {
    //     printf( "needle head: %1.3f move:%d\n", platter->needle.head, window_move );
    //     deck_state->decode_node->move_window( 
    //         deck_state->decode_node, 
    //         window_move
    //     );
    // }
}

int64_t zdj_deck_get_pcm_addr_for_decode_addr( zdj_deck_t * deck, int64_t decode_addr ) {
    return 0;
}
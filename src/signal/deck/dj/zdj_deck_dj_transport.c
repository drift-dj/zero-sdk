#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/dj/zdj_deck_dj.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/signal/pipeline/node/audio/tsm/zdj_tsm_pitch_node.h>
#include <zerodj/signal/pipeline/node/audio/tsm/zdj_tsm_tempo_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

static void _update_platter_model ( zdj_deck_t * deck );
static void _reset_tsm_tempo_node_to_decode_addr( zdj_pipeline_node_t * node, double addr );

void zdj_deck_dj_init_transport( zdj_deck_t * deck ) {
    deck->update_transport = &_update_platter_model;
}

void zdj_dj_deck_reset_platter( 
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
//////////////////////////////

static void _update_platter_model ( zdj_deck_t * deck ) {
    // printf( "zdj_deck_update_controls %p\n", deck );
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    if( !deck_state->decode_node ) { return; }
    
    // Get node states
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_tsm_pitch_node_state_t * tsm_pitch_state = (zdj_tsm_pitch_node_state_t*)deck_state->tsm_pitch_node->state;
    zdj_tsm_tempo_node_state_t * tsm_tempo_state = (zdj_tsm_tempo_node_state_t*)deck_state->tsm_tempo_node->state;

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
        case ZDJ_PLATTER_MOTOR_SPIN_UP:
            if( motor->spin_up_cycle_count ) { 
                spin_mod = (float)motor->cur_spin_up_cycle / (float)motor->spin_up_cycle_count;
                motor->cur_spin_up_cycle++;
                motor->set_rate = motor->pitch_setting;
                motor->instant_rate = motor->set_rate * spin_mod;
            } else {
                motor->instant_rate = motor->set_rate;
            }
            if( motor->cur_spin_up_cycle >= motor->spin_up_cycle_count ) { 
                motor->state = ZDJ_PLATTER_MOTOR_RUN; 
                
                if( deck_state->tempo_tsm_enabled ) { 
                    // After spinup, we need to enter tempo tsm mode if enabled.
                    slip->state = ZDJ_PLATTER_SLIP_LAMINAR_TEMPO; 
                    // Sync tempo decode to needle head
                    tsm_tempo_state->rate = tsm_tempo_state->rate = deck->controls.platter.motor.set_rate;
                    _reset_tsm_tempo_node_to_decode_addr( deck_state->tsm_tempo_node, platter->needle.head );
                }
            }
            // printf( "ZDJ_PLATTER_MOTOR_SPIN_UP: %1.3f\n", motor->instant_rate );
            break;
        case ZDJ_PLATTER_MOTOR_RUN:
            // printf( "ZDJ_PLATTER_MOTOR_RUN\n" );
            motor->set_rate = motor->pitch_setting;
            motor->instant_rate = motor->set_rate;
            break;
        case ZDJ_PLATTER_MOTOR_SPIN_DOWN:
            // printf( "ZDJ_PLATTER_MOTOR_SPIN_DOWN: %1.3f\n", motor->instant_rate );
            if( motor->spin_down_cycle_count ) { 
                spin_mod = 1.0 - ((float)motor->cur_spin_down_cycle / (float)motor->spin_down_cycle_count);
                motor->cur_spin_down_cycle++;
                motor->instant_rate = motor->set_rate * spin_mod;
            } else {
                motor->instant_rate = motor->set_rate;
            }
            if( motor->cur_spin_down_cycle >= motor->spin_down_cycle_count ) { 
                motor->set_rate = 0.0;
                motor->state = ZDJ_PLATTER_MOTOR_IDLE; 
            }
            break;
    }

    double motor_factor = motor->instant_rate * ZDJ_SOUNDCARD_BUF_LEN;

    // Slipmat + Needle Head
    // ---------------------

    // Update the slip map sim.
    if( slip->state == ZDJ_PLATTER_SLIP_LAMINAR_PITCH ) {
        deck_state->tsm_source = ZDJ_DECK_TSM_SOURCE_PITCH;
        // If we're not slipping, don't adjust slip offset
        motor->head += motor_factor;
        slip->set_val += motor_factor;
        slip->instant_val = slip->set_val;

        // Update the needle head
        platter->needle.head = slip->offset + slip->instant_val;
        // printf( "lam pitch needle.head: %1.3f\n", platter->needle.head );

    } else if( slip->state == ZDJ_PLATTER_SLIP_LAMINAR_TEMPO ) {
        // In tempo mode, needle head is not driven by slip value.
        // Instead, time-stretch engine is driven by motor's instant rate.
        // When transitioning from time-stretch to pitch, we just 
        // reset the needle simulation to time-stretch's last read
        // sample.  This is only accurate to a couple hundred samples.
        // If there is unacceptable artifacting, re-factor.
        // printf( "ZDJ_PLATTER_SLIP_LAMINAR_TEMPO\n" );
        deck_state->tsm_source = ZDJ_DECK_TSM_SOURCE_TEMPO;
        
        zdj_dj_deck_reset_platter( platter, tsm_tempo_state->decode_coord );
        // printf( "lam tempo needle.head: %1.3f\n", platter->needle.head );

    } else if( slip->state == ZDJ_PLATTER_SLIP_SCRATCH ) {
        deck_state->tsm_source = ZDJ_DECK_TSM_SOURCE_PITCH;
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
        if( slip->sim_counter++ > slip->slip_dwell ) {
            slip->state = ZDJ_PLATTER_SLIP_TO_LAM;
        } 

        // Update the needle head
        platter->needle.head = slip->offset + slip->instant_val;
        // printf( "scratch needle.head: %1.3f rate: %1.3f\n", platter->needle.head, motor->instant_rate );
        // printf( "scratch plat m_h:%1.3f s_s:%1.3f s_i:%1.3f s_o:%1.3f n_h: %1.3f\n", 
        //     motor->head,
        //     slip->set_val,
        //     slip->instant_val,
        //     slip->offset,
        //     platter->needle.head );

    } else if( slip->state == ZDJ_PLATTER_SLIP_NUDGE_PITCH ) {
        deck_state->tsm_source = ZDJ_DECK_TSM_SOURCE_PITCH;
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

    } else if( slip->state == ZDJ_PLATTER_SLIP_NUDGE_TEMPO ) {
        deck_state->tsm_source = ZDJ_DECK_TSM_SOURCE_TEMPO;

        // Use slip.offset to build a simplified rate calculation
        // since tempo mode doesn't need full platter simulation data.
        slip->tempo_nudge_rate *= 0.9;

        // Once the slip dwell expires, return to laminar mode
        if( slip->sim_counter++ > slip->slip_dwell ) {
            slip->tempo_nudge_rate = 0.0;
            slip->state = ZDJ_PLATTER_SLIP_LAMINAR_TEMPO;
        } 

        // Update needle head to follow tsm node
        zdj_dj_deck_reset_platter( platter, tsm_tempo_state->decode_coord );
        // printf( "nudge tempo rate: %1.3f needle.head: %1.3f\n", tsm_tempo_state->rate, platter->needle.head );

    } else if( slip->state == ZDJ_PLATTER_SLIP_TO_LAM ) {
        deck_state->tsm_source = ZDJ_DECK_TSM_SOURCE_PITCH;
        if( motor->enabled && deck_state->tempo_tsm_enabled ) {
            slip->state = ZDJ_PLATTER_SLIP_LAMINAR_TEMPO;
            // Set tempo tsm head;
            tsm_tempo_state->rate = tsm_tempo_state->rate = deck->controls.platter.motor.set_rate;
            _reset_tsm_tempo_node_to_decode_addr( deck_state->tsm_tempo_node, platter->needle.head );
        } else {
            slip->state = ZDJ_PLATTER_SLIP_LAMINAR_PITCH;
        }
        // printf( "slip to lam needle.head: %1.3f\n", platter->needle.head );
    }
    
    // printf( "needle.head: %1.3f\n", platter->needle.head );

    // if( deck->status == ZDJ_DECK_STATUS_WAIT_SPOOLDOWN && slip->instant_val < 0.001 ) {
    if( deck->status == ZDJ_DECK_STATUS_WAIT_SPOOLDOWN ) {
        // printf( "spooldown i_rate: %1.4f needle.head: %1.3f\n", motor->instant_rate, platter->needle.head );
        if( motor->instant_rate < 0.001 ) { deck->safe_to_deinit = true; }
    }

    // printf( "zdj_deck_update_controls done\n" );
}


static void _reset_tsm_tempo_node_to_decode_addr( 
    zdj_pipeline_node_t * node, 
    double addr 
) {
    // printf( "_reset_tsm_tempo_node_to_decode_addr: %1.1f\n", addr );
    zdj_tsm_tempo_node_state_t * tsm_tempo_state = (zdj_tsm_tempo_node_state_t*)node->state;
    tsm_tempo_state->decode_coord = addr;
    zdj_reset_tsm_tempo_node( node );
}
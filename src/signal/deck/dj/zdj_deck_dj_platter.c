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

static void _update_platter_req( zdj_deck_t * deck );
static void _update_platter_model( zdj_deck_t * deck );

void zdj_deck_dj_init_platter( zdj_deck_t * deck ) {
    // Note: platter_req and platter_model are totally asynchronous:
    // Request faces the hmi input thread (~800Hz).
    // Model faces the soundcard thread (@buffer rate).
    // Model pulls state from Request on every soundcard buffer boundary.
    deck->update_platter_req = &_update_platter_req;
    deck->update_platter_model = &_update_platter_model;
}

zdj_deck_control_platter_request_t * zdj_dj_deck_new_platter_request( zdj_deck_t * deck ) {
    zdj_deck_control_platter_request_t * req = &deck->platter_reqs[ deck->platter_req_write_index ];
    memset( req, 0, sizeof( zdj_deck_control_platter_request_t ) );
    req->phase = ZDJ_DECK_PLATTER_REQUEST_PHASE_NEW;
    deck->platter_req_write_index++;
    deck->platter_req_write_index %= 8;
    return req;
}


/////////////////////////////////////////////////////////////
// NOT THREAD SAFE!                                        //
static void _update_platter_req( zdj_deck_t * deck ) {
// Platter Request is updated from the HMI input thread.   //
// It must not directly set anything on the Platter Model. //
/////////////////////////////////////////////////////////////
    if( deck && deck->state ) {
        zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
        if( !deck_state->decode_node ) { return; }
        
        zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
        
        // Update platter req is generally no-op.
        // Use this if architecture demands platter reqs take longer
        // than a single cycle to update.

        //////////////////////////////////////////////
        // If we've played off the end of the song, //
        // request a motor stop in next cycle.      //
        //////////////////////////////////////////////
        if( decode_state->head.origin_d > decode_state->song_pcm_duration &&
            zdj_dj_deck_platter_is_playing( deck ) &&
            !zdj_dj_deck_loop_is_enabled( deck )
        ) {
            zdj_deck_control_platter_request_t * req = NULL;
            if( (req = zdj_dj_deck_new_platter_request( deck )) ) {
                req->type = ZDJ_DECK_PLATTER_REQUEST_STOP_MOTOR;
                req->spin_up = false;
                req->spin_down = true;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////
// NOT THREAD SAFE!                                                       //
static void _update_platter_model( zdj_deck_t * deck ) {
// Platter Model is updated from the soundcard thread on buffer boundary. //
// If Platter Request model declares an update, pull state and            //
// declare the update to be received.                                     //
////////////////////////////////////////////////////////////////////////////
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_platter_t * platter = &deck->controls.platter;
    zdj_deck_platter_motor_t * motor = &platter->motor;
    zdj_deck_platter_slip_t * slip = &platter->slip;

    double scrub_rate = 0.0;

    ///////////////////////////////////////
    // Update the platter request system //
    ///////////////////////////////////////
    for( int i=0; i<8; i++ ) {
        zdj_deck_control_platter_request_t * platter_req = &deck->platter_reqs[ i ];
        if( platter_req->phase == ZDJ_DECK_PLATTER_REQUEST_PHASE_NEW ) {
            switch ( platter_req->type ) {
                case ZDJ_DECK_PLATTER_REQUEST_START_MOTOR:
                    if( zdj_dj_deck_platter_can_play( deck ) ) { 
                        zdj_dj_deck_platter_start_motor( deck, platter_req->spin_up ); 
                    }
                    break;

                case ZDJ_DECK_PLATTER_REQUEST_STOP_MOTOR:
                    if( zdj_dj_deck_platter_is_playing( deck ) ) { 
                        zdj_dj_deck_platter_stop_motor( deck, platter_req->spin_down ); 
                    }
                    break;

                case ZDJ_DECK_PLATTER_REQUEST_TOGGLE_MOTOR:
                    // printf( "toggle motor - is playing:%d can play:%d\n", 
                    //     zdj_dj_deck_platter_is_playing( deck ), 
                    //     zdj_dj_deck_platter_can_play( deck ) 
                    // );
                    if( zdj_dj_deck_platter_is_playing( deck ) ) { 
                        zdj_dj_deck_platter_stop_motor( deck, platter_req->spin_down ); 
                    } else if( zdj_dj_deck_platter_can_play( deck ) ) { 
                        zdj_dj_deck_platter_start_motor( deck, platter_req->spin_up ); 
                    }
                    break;

                case ZDJ_DECK_PLATTER_REQUEST_SCRUB:
                    
                    // Nudge a running platter
                    if( zdj_dj_deck_platter_can_nudge( deck ) &&
                        !zdj_dj_deck_platter_can_scrub( deck, platter_req->event_i_val ) 
                    ) { 
                        // Use tempo nudge rate to bypass the platter sim and only change rate val.
                        platter->slip.tempo_nudge_rate += platter_req->event_i_val * platter->nudge_coeff;
                        // Use set_val in pitch mode so the full platter sim is employed.
                        // platter->slip.set_val += platter_req->event_i_val * platter->nudge_coeff;
                        platter->slip_input_samps += (double)platter_req->event_i_val * platter->nudge_coeff;
                        platter->slip.state = ZDJ_PLATTER_MODE_SLIP;
                    

                    // Scratch a stopped/scratch_override platter
                    } else { 
                        // Break out early if we can't scrub right now
                        if( !zdj_dj_deck_platter_can_scrub( deck, platter_req->event_i_val ) ) { 
                            // platter->slip.sim_counter = 0;
                            break;
                        }

                        // While scratching, we don't want to scrub faster than the buffer can play,
                        // so we need to limit the scrub rate to prevent buffer underrun.
                        platter->slip_input_samps += (double)platter_req->event_i_val * platter->scratch_coeff;
                        // We need to limit to something below exit_thresh because there's a spring sim
                        // involved.  During a violent movement, the spring will try to catch up 
                        // to the platter, and that may require more movement.
                        // double max_scrub_rate = platter->hyperscrub.exit_thresh * 0.8;
                        double max_scrub_rate = platter->hyperscrub.exit_thresh;
                        platter->slip_input_samps = fmin( platter->slip_input_samps, max_scrub_rate );
                        platter->slip_input_samps = fmax( platter->slip_input_samps, max_scrub_rate*-1 );
                        platter->slip.state = ZDJ_PLATTER_MODE_SLIP;
                        platter->slip.sim_counter = 0;

                        // If we're in tempo TSM mode, and slo-coder isn't enabled,
                        // request immediate switch to pitch TSM mode.
                        if( zdj_dj_deck_platter_is_playing( deck ) &&
                            zdj_dj_deck_is_in_tempo_tsm_mode( deck ) &&
                            !deck_state->slo_coder 
                        ) {
                            deck_state->tsm_tx_req = ZDJ_DECK_TSM_TX_TO_PITCH;
                        }
                    }
                    break;

                case ZDJ_DECK_PLATTER_REQUEST_SCRUB_ALT_0:
                    // Always scratch, regardless of playback state
                    // Break out early if we can't scratch right now
                    if( !zdj_dj_deck_platter_can_scrub( deck, platter_req->event_i_val ) ) { break; }

                    // While scratching, we don't want to scrub faster than the buffer can play,
                    // so we need to limit the scrub rate to prevent buffer underrun.
                    platter->slip_input_samps += (double)platter_req->event_i_val * platter->scratch_coeff;
                    double max_scrub_rate = platter->hyperscrub.exit_thresh * 0.8;
                    platter->slip_input_samps = fmin( platter->slip_input_samps, max_scrub_rate );
                    platter->slip_input_samps = fmax( platter->slip_input_samps, max_scrub_rate*-1 );
                    platter->slip.state = ZDJ_PLATTER_MODE_SLIP;
                    platter->slip.sim_counter = 0;

                    // If we're in tempo TSM mode, and slo-coder isn't enabled,
                    // request immediate switch to pitch TSM mode.
                    if( zdj_dj_deck_platter_is_playing( deck ) &&
                        zdj_dj_deck_is_in_tempo_tsm_mode( deck ) &&
                        !deck_state->slo_coder 
                    ) {
                        deck_state->tsm_tx_req = ZDJ_DECK_TSM_TX_TO_PITCH;
                    }
                    break;

                case ZDJ_DECK_PLATTER_REQUEST_SCRUB_ALT_1:
                    // Always scrub, with increased sensitivity, and enter 
                    // hyperscrub if scrubbing too fast for decode.

                    // Alt-1 scrub allows us to scrub way faster than the buffer can play.
                    // Don't limit the scrub rate here, and let the slip sim below detect
                    // a hyperscrub event if scrub rate crosses the hyperscrube threshold.
                    
                    if( !zdj_dj_deck_loop_is_enabled( deck ) ) {
                        platter->slip_input_samps = (double)platter_req->event_i_val * platter->hyperscrub_coeff;
                        // Hand hyperscrub rate to UI
                        platter->hyperscrub.ui_rate = fabs( platter->slip_input_samps );
                    } else {
                        platter->slip_input_samps += (double)platter_req->event_i_val * platter->scratch_coeff;
                        double max_scrub_rate = platter->hyperscrub.exit_thresh * 0.8;
                        platter->slip_input_samps = fmin( platter->slip_input_samps, max_scrub_rate );
                        platter->slip_input_samps = fmax( platter->slip_input_samps, max_scrub_rate*-1 );
                    }

                    platter->slip.state = ZDJ_PLATTER_MODE_SLIP;
                    platter->slip.sim_counter = 0;
                    
                    // If we're in tempo TSM mode, and slo-coder isn't enabled,
                    // request immediate switch to pitch TSM mode.
                    if( zdj_dj_deck_platter_is_playing( deck ) &&
                        zdj_dj_deck_is_in_tempo_tsm_mode( deck ) &&
                        !deck_state->slo_coder 
                    ) {
                        deck_state->tsm_tx_req = ZDJ_DECK_TSM_TX_TO_PITCH;
                    }
                    break;
            }

            platter_req->phase = ZDJ_DECK_PLATTER_REQUEST_PHASE_RECEIVED;
        }

    }



    //////////////////////////////////
    // Update the Platter Motor Sim //
    //////////////////////////////////
    if( motor->state == ZDJ_PLATTER_MOTOR_IDLE ) {
        // printf( "ZDJ_PLATTER_MOTOR_IDLE\n" );
        motor->instant_rate = 0.0f;

    } else if( motor->state == ZDJ_PLATTER_MOTOR_SPIN_UP ) {
        if( motor->spin_up_cycle_count ) { 
            float spin_mod = (float)motor->cur_spin_up_cycle / (float)motor->spin_up_cycle_count;
            motor->cur_spin_up_cycle++;
            motor->set_rate = motor->pitch_setting;
            motor->instant_rate = motor->set_rate * spin_mod;
        } else {
            motor->instant_rate = motor->set_rate;
        }
        if( motor->cur_spin_up_cycle >= motor->spin_up_cycle_count ) { 
            motor->state = ZDJ_PLATTER_MOTOR_RUN; 

            // After spinup, if tempo TSM is available, req tx to tempo TSM
            if( deck_state->tempo_tsm_enabled ) { deck_state->tsm_tx_req = ZDJ_DECK_TSM_TX_TO_TEMPO; }
        }

    } else if( motor->state == ZDJ_PLATTER_MOTOR_RUN ) {
        // printf( "ZDJ_PLATTER_MOTOR_RUN\n" );
        motor->set_rate = motor->pitch_setting;
        motor->instant_rate = motor->set_rate;

    } else if( motor->state == ZDJ_PLATTER_MOTOR_SPIN_DOWN ) {
        // printf( "ZDJ_PLATTER_MOTOR_SPIN_DOWN: %1.3f\n", motor->instant_rate );
        if( motor->spin_down_cycle_count ) { 
            float spin_mod = 1.0 - ((float)motor->cur_spin_down_cycle / (float)motor->spin_down_cycle_count);
            motor->cur_spin_down_cycle++;
            motor->instant_rate = motor->set_rate * spin_mod;
        } else {
            motor->instant_rate = motor->set_rate;
        }
        if( motor->cur_spin_down_cycle >= motor->spin_down_cycle_count ) { 
            motor->set_rate = 0.0;
            motor->state = ZDJ_PLATTER_MOTOR_IDLE; 
        }
    }
    motor->instant_val = motor->instant_rate * ZDJ_SOUNDCARD_BUF_LEN;

    /////////////////////////////////////
    // Update the Slip Mat physics sim //
    /////////////////////////////////////
    if( slip->state == ZDJ_PLATTER_MODE_LAMINAR ) {
        // slip->instant_val = slip->set_val;
        slip->tempo_nudge_rate = 0.0;
        platter->head_move_samps = platter->motor.instant_val;
        platter->head_move_rate = platter->head_move_samps / ZDJ_SOUNDCARD_BUF_LEN;
        platter->internal_instant_head += platter->head_move_samps;
        platter->slip_input_head += platter->head_move_samps;

    } else if( slip->state == ZDJ_PLATTER_MODE_SLIP ) {
        // Integrate any scrub inputs and motor movement
        // TODO: for scratching while motor engaged, cancel out the motor input
        platter->last_head_move_rate = platter->head_move_rate;

        platter->slip_input_head += platter->slip_input_samps;
        platter->slip_input_samps = 0;
        platter->slip_input_head += platter->motor.instant_val;
        platter->internal_instant_head_prev = platter->internal_instant_head;
        // If we're scrubbing/nudging, update damped spring simulation on slipmat
        double displacement = platter->slip_input_head - platter->internal_instant_head_prev;
        // Calculate acceleration using the damped spring equation
        double acceleration = (-slip->spring_k * displacement - slip->damp_c * slip->velocity) / slip->mass;
        // Using Euler here... switch to RK 2/4 if needed.
        double step_val = (1.0/slip->sim_duration);
        slip->velocity += acceleration * step_val;
        displacement += slip->velocity * step_val;

        // Update the nudge input
        slip->tempo_nudge_rate *= 0.9;
        double tempo_nudge_samps = slip->tempo_nudge_rate;
        
        // Update outputs
        platter->internal_instant_head = platter->slip_input_head - displacement;
        platter->head_move_samps = platter->internal_instant_head - platter->internal_instant_head_prev;
        platter->head_move_samps += tempo_nudge_samps;
        platter->head_move_rate = platter->head_move_samps / ZDJ_SOUNDCARD_BUF_LEN;

        // Keep the sim running while jog is settling
        double rate_offset = fabs( platter->last_head_move_rate - platter->head_move_rate );
        if( rate_offset > 4.0 ) { slip->sim_counter = 0; }

        // Once the slip dwell expires, return to laminar mode
        if( slip->sim_counter++ > slip->slip_dwell ) {
            slip->state = ZDJ_PLATTER_MODE_LAMINAR;
            // After slip, if tempo TSM is available and not already running, req tx to tempo TSM
            if( zdj_dj_deck_platter_is_playing( deck ) && 
                deck_state->tempo_tsm_enabled &&
                !zdj_dj_deck_is_in_tempo_tsm_mode( deck )
            ) { 
                deck_state->tsm_tx_req = ZDJ_DECK_TSM_TX_TO_TEMPO; 
            }
        } 
    } 

    //////////////////////////////////////////////////
    // Sum the motor and slipmat sim values to get  //
    // the PCM sample count  & rate at which the    //
    // playhead should move during this cycle.      //
    //////////////////////////////////////////////////
    // platter->internal_instant_head += platter->slip.instant_val + platter->motor.instant_val;
    // platter->head_move_samps = platter->slip.instant_val + platter->motor.instant_val;
    // platter->head_move_rate = platter->head_move_samps / ZDJ_SOUNDCARD_BUF_LEN;

    ////////////////////////////////////////////
    // Manage exit to/reentry from Hyperscrub //
    ////////////////////////////////////////////
    if( platter->hyperscrub.state == ZDJ_DECK_HYPERSCRUB_ACTIVE ) {
        // printf( "Hyper: %1.1f\n", head_move_val );
        if( fabs( platter->head_move_samps ) < platter->hyperscrub.reentry_thresh ) {
            platter->hyperscrub.state = ZDJ_DECK_HYPERSCRUB_INACTIVE;
        }
    } else if( platter->hyperscrub.state == ZDJ_DECK_HYPERSCRUB_INACTIVE && 
               fabs( platter->head_move_samps ) > platter->hyperscrub.exit_thresh 
    ) {
        platter->hyperscrub.state = ZDJ_DECK_HYPERSCRUB_ACTIVE;
    }

    //////////////////////////////////////////////////////////////////
    // Detect a completed spooldown to release the deinit interlock //
    // TODO: Surely this can be moved somewhere else?               //
    //////////////////////////////////////////////////////////////////
    if( deck->status == ZDJ_DECK_STATUS_WAIT_SPOOLDOWN ) {
        if( fabs( platter->head_move_samps ) < 1.0 ) { deck->safe_to_deinit = true; }
    }
}

bool zdj_dj_deck_platter_can_play( zdj_deck_t * deck ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    return decode_state->head.origin_d < decode_state->song_pcm_duration;
}

bool zdj_dj_deck_platter_is_playing( zdj_deck_t * deck ) {
    return deck->controls.platter.motor.enabled;
}

// Allow scrubbing under the following conditions:
//  - Platter motor is not running
//  - Scratch override is enabled
//  - decode_head is within song PCM bounds
bool zdj_dj_deck_platter_can_scrub( zdj_deck_t * deck, int dir ) {
    if( !zdj_dj_deck_platter_is_playing( deck ) ||
        deck->controls.platter.scratch_override
    ) {
        zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
        zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;

        if( (dir > 0) && (decode_state->head.origin_d > decode_state->song_pcm_duration) ) {
            return false; // Playhead is after end of track
        } else if( (dir < 0) && (decode_state->head.origin_d < 0) ) {
            return false; // Playhead is before start of track
        } else {
            return true; // Playhead is within track
        }
    } else {
        return false; // Platter is running and scratch_override isn't enabled
    }
}

bool zdj_dj_deck_platter_can_nudge( zdj_deck_t * deck ) {
    if( zdj_dj_deck_platter_is_playing( deck ) ) {
        return true;
    } else {
        return false;
    }
}

bool zdj_dj_deck_platter_is_hyperscrubbing( zdj_deck_t * deck ){ 
    zdj_deck_platter_t * platter = &deck->controls.platter;
    if( platter->slip.state != ZDJ_PLATTER_MODE_SLIP ) { return false; }
    else { return platter->hyperscrub.state == ZDJ_DECK_HYPERSCRUB_ACTIVE; }
}

void zdj_dj_deck_platter_update_scrub_fade( zdj_deck_t * deck ) {
    zdj_deck_platter_t * platter = &deck->controls.platter;

    // bug out early if we're not scrubbing
    if( platter->slip.state != ZDJ_PLATTER_MODE_SLIP ) {
        platter->scrub_fade.start_fade_val = 1.0f;
        platter->scrub_fade.end_fade_val = 1.0f;
        return;
    }

    // No fade below a certain scrub rate
    if( fabs( platter->head_move_samps ) < platter->scrub_fade.fade_start_rate ) {
        platter->scrub_fade.start_fade_val = 1.0f;
        platter->scrub_fade.end_fade_val = 1.0f;

    // Totally mute above a certain scrub rate
    } else if( fabs( platter->head_move_samps ) > platter->scrub_fade.fade_complete_rate ) {
        platter->scrub_fade.start_fade_val = 0.0f;
        platter->scrub_fade.end_fade_val = 0.0f;

    // Interpolate fade value when within active scrub rate range
    } else {
        float num = fabs( platter->head_move_samps ) - platter->scrub_fade.fade_start_rate;
        float den = platter->scrub_fade.fade_complete_rate - platter->scrub_fade.fade_start_rate;
        platter->scrub_fade.start_fade_val = platter->scrub_fade.end_fade_val;
        platter->scrub_fade.end_fade_val = 1.0 - ( num / den );
    }
}


void zdj_dj_deck_platter_reset_antipop( zdj_deck_t * deck ) {
    // Zero the anti-pop at the current decode head.
    zdj_deck_platter_antipop_t * antipop = &deck->controls.platter.antipop;
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_pipeline_node_t * decode_node = deck_state->decode_node;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;

    antipop->tracking_val = decode_state->out_buffer[ decode_state->head.buf_i * decode_state->channel_count ];
    antipop->start_fade_val = 0.0;
    antipop->end_fade_val = 0.0;
}

void zdj_dj_deck_platter_update_antipop( zdj_deck_t * deck ) {
    zdj_deck_platter_t * platter = &deck->controls.platter;
    zdj_deck_platter_antipop_t * antipop = &platter->antipop;

    // Step forward
    antipop->start_fade_val = antipop->end_fade_val;

    if( fabs( platter->head_move_samps ) > antipop->engage_thresh ) {
        // Lowpass tracking val back toward 0.0 if playback rate > thresh
        antipop->tracking_val *= antipop->lowpass_val;
        antipop->end_fade_val = 1.0 - antipop->tracking_val;
    } else {
        // Playback rate is slow enough: move tracking val toward buf val.
        zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
        zdj_pipeline_node_t * decode_node = deck_state->decode_node;
        zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
        // Calculate avg val:
        // get sample @head, get sample @scrub_rate
        int buf_start = decode_state->head.buf_i;
        int buf_end = decode_state->head.buf_i + platter->slip.instant_val;
        int chan_count = decode_state->channel_count;
        double start_samp = decode_state->out_buffer[ buf_start * chan_count ];
        double end_samp = decode_state->out_buffer[ buf_end * chan_count ];
        // TODO: fmax the right channel if present
        float buf_avg = fabs((start_samp + end_samp) / 2.0);
        antipop->tracking_val += (buf_avg - antipop->tracking_val) * antipop->lowpass_val;

        antipop->end_fade_val = buf_avg - antipop->tracking_val;
    }    
}

void zdj_dj_deck_platter_start_motor( zdj_deck_t * deck, bool spin_up ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_deck_platter_t * platter = &deck->controls.platter;
    platter->motor.enabled = true;
    platter->motor.set_rate = platter->motor.pitch_setting;
    platter->motor.state = ZDJ_PLATTER_MOTOR_SPIN_UP;
    if( spin_up ) {
        platter->motor.cur_spin_up_cycle = 0;
    } else {
        platter->motor.cur_spin_up_cycle = platter->motor.spin_up_cycle_count;
    }
    deck_state->tsm_tx_req = ZDJ_DECK_TSM_TX_TO_PITCH;
}

void zdj_dj_deck_platter_stop_motor( zdj_deck_t * deck, bool spin_down ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_tsm_pitch_node_state_t * tsm_pitch_state = (zdj_tsm_pitch_node_state_t*)deck_state->tsm_pitch_node->state;
    zdj_tsm_tempo_node_state_t * tsm_tempo_state = (zdj_tsm_tempo_node_state_t*)deck_state->tsm_tempo_node->state;
    zdj_deck_platter_t * platter = &deck->controls.platter;
    if( platter->slip.state == ZDJ_PLATTER_MODE_LAMINAR ) {
        // We also need to set the pitch node's sample addresses to something reasonable
        tsm_pitch_state->decode_start_coord = tsm_tempo_state->decode_coord - ZDJ_SOUNDCARD_BUF_LEN;
    }
    platter->motor.enabled = false;
    platter->motor.state = ZDJ_PLATTER_MOTOR_SPIN_DOWN;
    if( spin_down ) {
        platter->motor.cur_spin_down_cycle = 0;
    } else {
        platter->motor.cur_spin_down_cycle = platter->motor.spin_down_cycle_count;
    }
    // Ensure slip is in pitch mode so we hear spin down.
    platter->slip.state = ZDJ_PLATTER_MODE_LAMINAR;

    deck_state->tsm_tx_req = ZDJ_DECK_TSM_TX_TO_PITCH;
}
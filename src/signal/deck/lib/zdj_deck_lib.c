#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <math.h>

#include <zerodj/controls/zdj_controls.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/signal/pipeline/node/audio/record/zdj_audio_record_node.h>
#include <zerodj/signal/pipeline/node/audio/tsm/zdj_tsm_pitch_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/system/thread/zdj_thread.h>

// External data/event entry points
static void _update_state ( zdj_deck_t * deck );
static void _deinit( zdj_deck_t * deck );
static void _get_edge_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo );
static void _handle_control( zdj_deck_t * deck, zdj_control_event_t * event );
static void _update_control_model ( zdj_deck_t * deck );

static double _max_scrub_rate_for_deck( zdj_deck_t * deck );
static double _min_scrub_rate_for_deck( zdj_deck_t * deck );

// Pipeline update thread
static void * _pipeline_thread_main( void * arg );

zdj_error_type_t zdj_new_lib_deck( zdj_deck_t * deck, void * resource ) {
    // printf( "zdj_new_lib_deck\n" );
    deck->update_state = &_update_state;
    deck->deinit = &_deinit;
    deck->get_edge_data = &_get_edge_data;
    deck->update_controls = &_update_control_model;
    deck->handle_control_event = NULL; // Ignore control events until we're playable.
   
    zdj_lib_deck_state_t * state = calloc( 1, sizeof( zdj_lib_deck_state_t ) );
    deck->state = state;
    state->song = (zdj_library_song_t *)resource;
    sem_init( &state->start_cycle, 0, 0 );

    // Reset deck controls
    zdj_deck_init_controls( deck );

    // printf( "zdj_new_lib_deck done\n" );
    return ZDJ_ERROR_OKAY;
}

static void _deinit( zdj_deck_t * deck ) {
    zdj_lib_deck_state_t * deck_state = (zdj_lib_deck_state_t*)deck->state;
    // Teardown decode + tsm pipeline.
    deck_state->decode_node->deinit( deck_state->decode_node );
    deck_state->tsm_node->deinit( deck_state->tsm_node );
}

// Called from the slow deck update thread (~.25Hz).
static void _update_state ( zdj_deck_t * deck ) {
    // printf( "lib deck _update_state\n" );

    // Early exit if we're up and running.
    if( deck->status == ZDJ_DECK_STATUS_RUNNING ) { return; }
    
    zdj_lib_deck_state_t * deck_state = (zdj_lib_deck_state_t*)deck->state;

    switch ( deck->status ) {

        case ZDJ_DECK_STATUS_NEW:
            printf( "ZDJ_DECK_STATUS_NEW\n" );
            deck->status = ZDJ_DECK_STATUS_MAKE_PIPELINE;
            break;

        // Stand up pipeline nodes.
        case ZDJ_DECK_STATUS_MAKE_PIPELINE:
            printf( "ZDJ_DECK_STATUS_MAKE_PIPELINE\n" );
            deck_state->decode_node = zdj_new_decode_node( 
                deck_state->song, 0, ZDJ_SOUNDCARD_BUF_LEN*20, ZDJ_SOUNDCARD_BUF_LEN*20 
            );
            deck_state->tsm_node = zdj_new_tsm_pitch_node( 
                deck_state->song->audio->av_channel_count,
                ZDJ_SOUNDCARD_BUF_LEN,
                deck_state->decode_node
            );
            deck->status = ZDJ_DECK_STATUS_WAIT_THREAD_AVAIL;
            break;
            
        // Make sure station 1 doesn't already have a thread running.
        // If it does, assume it's in the process of exiting,
        // and keep polling here until it becomes available.
        case ZDJ_DECK_STATUS_WAIT_THREAD_AVAIL:
            printf( "ZDJ_DECK_STATUS_WAIT_THREAD_AVAIL\n" );
            if( zdj_thread_deck_1_station_available ) {
                zdj_thread_launch_deck_station_1_cycle( _pipeline_thread_main, deck );
                deck->status = ZDJ_DECK_STATUS_WAIT_THREAD_READY;
            }
            break;

        // Wait for new deck thread to finish filling its buffers.
        case ZDJ_DECK_STATUS_WAIT_THREAD_READY:
            printf( "LIB ZDJ_DECK_STATUS_WAIT_THREAD_READY\n" );
            if( deck_state->thread_ready ) {                
                // Start serving deck samples to soundcard.
                zdj_soundcard_link_deck( zdj_soundcard, deck );
                // Accept control events when we're ready for playback
                deck->handle_control_event = &_handle_control;

                deck->controls.loop_state.quantize = true;
                deck->controls.loop_state.beatgrid_len = 1.0f;
                deck->controls.skip_state.skip_unit = 0.125f;
                deck->controls.skip_state.phase = ZDJ_DECK_SKIP_PHASE_INACTIVE;

                // Advance state to running
                deck->status = ZDJ_DECK_STATUS_RUNNING; 
            }
            break;

        // Ignore new control events and start spooldown of deck drive/transport model.
        case ZDJ_DECK_STATUS_STOP_TRANSPORT:
            printf( "ZDJ_DECK_STATUS_STOP_TRANSPORT\n" );
            // Immediately stop accepting control events.
            deck->handle_control_event = NULL;
            // Stop deck playback if running.
            if( deck->controls.platter.motor.enabled ) {
                deck->safe_to_deinit = false;
                // if( deck->controls.platter.slip.state == ZDJ_PLATTER_SLIP_LAMINAR_TEMPO ) {
                //     // If we're in tempo mode, we need to sync the needle head with tempo node.
                //     zdj_dj_deck_reset_platter( &deck->controls.platter, tsm_tempo_state->decode_coord );
                //     // We also need to set the pitch node's sample addresses to something reasonable
                //     zdj_tsm_pitch_node_state_t * tsm_pitch_state = (zdj_tsm_pitch_node_state_t*)deck_state->tsm_pitch_node->state;
                //     tsm_pitch_state->decode_start_coord = tsm_tempo_state->decode_coord - ZDJ_SOUNDCARD_BUF_LEN;
                // }
                deck->controls.platter.motor.enabled = false;
                deck->controls.platter.motor.state = ZDJ_PLATTER_MOTOR_SPIN_DOWN;
                deck->controls.platter.motor.cur_spin_down_cycle = 0;
                // Ensure slip is in pitch mode so we hear spin down.
                deck->controls.platter.slip.state = ZDJ_PLATTER_SLIP_LAMINAR_PITCH;
            } else {
                deck->safe_to_deinit = true;
            }
            deck->status = ZDJ_DECK_STATUS_WAIT_SPOOLDOWN; 
            break;

        // Wait while deck transport fades out or slows to rate=0.
        // Allow this to take several audio buffer cycles.
        case ZDJ_DECK_STATUS_WAIT_SPOOLDOWN:
            printf( "ZDJ_DECK_STATUS_WAIT_SPOOLDOWN\n" );
            if( deck->safe_to_deinit ) {
                // Stop sending deck samples to soundcard
                zdj_soundcard_unlink_deck( zdj_soundcard, deck );
                // Tell the deck's fast-audio pipeline thread to exit.
                // Manually post the sem since we have unlinked the deck.
                deck_state->exit_thread = true;
                sem_post( &deck_state->start_cycle );
                // Tell the deck_manager we are ready for deinit()
                deck->status = ZDJ_DECK_STATUS_IDLE;
            }
            break;
        default: break;
    }

    // printf( "lib deck _update_state done\n" );
}

static void _handle_control( zdj_deck_t * deck, zdj_control_event_t * event ) {
    // printf( "lib deck _handle_control: %p %d %d\n", deck, deck->station, event->id );
    zdj_deck_platter_t * platter = &deck->controls.platter;

    zdj_soundcard_node_t * node;

    switch ( event->id ) {
    case ZDJ_DECK_CONTROL_LR_VOL:
        node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS );
        node->gain += event->i_val * 2;
        if( node->gain > 255 ) { node->gain = 255; }
        else if( node->gain < 0 ) { node->gain = 0; }
        event->blocked = true;
        break;
    case ZDJ_DECK_1_CONTROL_PLAY_PAUSE:
        // if( platter->motor.set_rate < 0.01 ) {
        //     platter->motor.set_rate = platter->motor.pitch_setting;
        //     platter->motor.state = ZDJ_PLATTER_MOTOR_RUN;
        // } else {
        //     platter->motor.set_rate = 0.0;
        //     platter->motor.state = ZDJ_PLATTER_MOTOR_IDLE;
        // }
        // printf( "lib deck toggle play/pause: %1.3f\n", platter->motor.set_rate );
        // break;
        // Play
        if( !platter->motor.enabled ) {
            platter->motor.enabled = true;
            platter->motor.set_rate = platter->motor.pitch_setting;
            platter->motor.state = ZDJ_PLATTER_MOTOR_SPIN_UP;
            platter->motor.cur_spin_up_cycle = 0;
        // Pause
        } else {
            platter->motor.enabled = false;
            platter->motor.state = ZDJ_PLATTER_MOTOR_SPIN_DOWN;
            platter->motor.cur_spin_down_cycle = 0;
            // Ensure slip is in pitch mode so we hear spin down.
            platter->slip.state = ZDJ_PLATTER_SLIP_LAMINAR_PITCH;
        }
        printf( "dj deck toggle play/pause: %1.3f\n", platter->motor.set_rate );
        break;
    case ZDJ_DECK_1_CONTROL_SCRUB:
        // printf( "lib deck scrub\n" );
        if( platter->motor.state == ZDJ_PLATTER_MOTOR_RUN && !platter->scratch_override
        ) {
            // Nudge a running platter
            platter->slip.set_val += event->i_val * platter->nudge_coeff;
            platter->slip.state = ZDJ_PLATTER_SLIP_NUDGE_PITCH;
        } else {
            // Scratch a non-running platter (or if user overrides nudge)
            double scrub_rate = (double)event->i_val * platter->scratch_coeff;
            if( scrub_rate > _max_scrub_rate_for_deck( deck ) ){ scrub_rate = _max_scrub_rate_for_deck( deck ); }
            if( scrub_rate < _min_scrub_rate_for_deck( deck ) ){ scrub_rate = _min_scrub_rate_for_deck( deck ); }
            platter->slip.set_val += scrub_rate;
            platter->slip.state = ZDJ_PLATTER_SLIP_SCRATCH;
        }
        platter->slip.sim_counter = 0;
        break;
    default:
        break;
    }
    // printf( "lib deck _handle_control done\n" );
}

static void _update_control_model ( zdj_deck_t * deck ) {
    // printf( "deck lib _update_control_model\n" );
    zdj_lib_deck_state_t * deck_state = (zdj_lib_deck_state_t*)deck->state;
    if( !deck_state->decode_node ) { return; }
    
    // Get node states
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_tsm_pitch_node_state_t * tsm_pitch_state = (zdj_tsm_pitch_node_state_t*)deck_state->tsm_node->state;

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
    
    // motor->instant_rate = zdj_signal_lowpass( motor->instant_rate, motor->set_rate, motor->ramp_rate );
    double motor_factor = motor->instant_rate * ZDJ_SOUNDCARD_BUF_LEN;
    // motor->head += motor_factor;

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
        // printf( "lam pitch needle.head: %1.3f\n", platter->needle.head );

    } else if( slip->state == ZDJ_PLATTER_SLIP_SCRATCH ) {
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
    
    // platter->needle.head = slip->offset + slip->instant_val;
    // printf( "needle.head: %1.3f\n", platter->needle.head );

    if( deck->status == ZDJ_DECK_STATUS_WAIT_SPOOLDOWN && slip->instant_val < 0.001 ) {
        // printf( "spooldown needle.head: %1.3f\n", platter->needle.head );
        deck->safe_to_deinit = true;
    }
}

static void _get_edge_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo ) {
    // printf( "lib get edge data\n" );
    zdj_deck_t * deck = (zdj_deck_t*)_deck;
    zdj_lib_deck_state_t * deck_state = (zdj_lib_deck_state_t*)deck->state;
    zdj_tsm_pitch_node_state_t * tsm_state = (zdj_tsm_pitch_node_state_t*)deck_state->tsm_node->state;

    float * soundcard_buf = data_pipe->get_data( data_pipe );
    
    // copy TSM out_buffer to data_pipe
    if( stereo && tsm_state->channel_count == 2 ) {
        // stereo->stereo
        memcpy( soundcard_buf, tsm_state->out_buffer, ZDJ_SOUNDCARD_BUF_LEN * 2 * sizeof( float ) );
    } else if( !stereo && tsm_state->channel_count == 1 ) {
        // mono->mono
        memcpy( soundcard_buf, tsm_state->out_buffer, ZDJ_SOUNDCARD_BUF_LEN * sizeof( float ) );
    }
    // stereo->mono + mono->stereo not handled yet.

    // printf( "lib get edge data done\n" );
    // Release another cycle of the pipeline_thread_main below.
    sem_post( &deck_state->start_cycle );
}

// Thread for processing soundcard fast-cycle requests.
// get_edge_data posts the start_cycle semaphore below.
static void * _pipeline_thread_main( void * arg ) {
    printf( "_pipeline_thread_main: %p\n", arg );
    zdj_deck_t * deck = (zdj_deck_t*)arg;
    zdj_lib_deck_state_t * deck_state = (zdj_lib_deck_state_t*)deck->state;
    zdj_tsm_pitch_node_state_t * tsm_state = (zdj_tsm_pitch_node_state_t*)deck_state->tsm_node->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;

    // Claim the deck station thread
    zdj_thread_deck_1_station_available = false;

    double head_move_val;
    double head_move_rate;
    // double last_head_val = deck->controls.platter.needle.head;

    while( !deck_state->exit_thread ) {
        // printf( "lib deck thread\n" );

        // Process any new loop/skip requests
        if( deck->controls.loop_state.phase == ZDJ_DECK_LOOP_PHASE_ACTIVATE ) {
            zdj_deck_new_loop( deck );
        } else if( deck->controls.loop_state.phase == ZDJ_DECK_LOOP_PHASE_DEACTIVATE ) {
            zdj_deck_disable_loop( deck );
        }

        if( deck->controls.skip_state.phase == ZDJ_DECK_SKIP_PHASE_ACTIVATE ) {
            zdj_deck_stage_skip( deck );
        } else if( deck->controls.skip_state.phase == ZDJ_DECK_SKIP_PHASE_STAGED ) {
            zdj_deck_update_skip( deck );
        }
        
        // Build a window-move value for the decode_node to keep head in the window.
        head_move_val = deck->controls.platter.needle.head - decode_state->head_decode_addr;
        if( fabs( head_move_val ) > 64 ) {
            // Move the decode window and keep the tsm head in sync.
            deck_state->decode_node->move_window( deck_state->decode_node, head_move_val );
            // Re-charge the decode node buffer after the move
            deck_state->decode_node->update_wait( deck_state->decode_node );
        }
        // printf( "tsm pitch update\n" );
        // Update the pitch tsm node only during transitions/pitch playback
        tsm_state->decode_end_coord = deck->controls.platter.needle.head;
        tsm_state->decode_buf_ref_coord = decode_state->head_win_start;       
        deck_state->tsm_node->update_wait( deck_state->tsm_node );
        tsm_state->decode_start_coord = tsm_state->decode_end_coord;


        // // Gather transport control model values.
        // // Build a window-move value for the decode_node to keep head in the window.
        // head_move_val = deck->controls.platter.needle.head - decode_state->head_decode_addr;

        // // Apply some hysteresis to window moves
        // if( fabs( head_move_val ) > 4 ) {
        //     // Move the decode window and keep the tsm head in sync.
        //     deck_state->decode_node->move_window( deck_state->decode_node, head_move_val );
        //     // Re-charge the decode node buffer after the move
        //     deck_state->decode_node->update_wait( deck_state->decode_node );
        // }

        // // Feed new needle head to tsm node state
        // tsm_state->decode_end_coord = deck->controls.platter.needle.head;
        // tsm_state->decode_buf_ref_coord = decode_state->head_win_start;
        // // Pitch-interpolate samples from decode window into tsm buffer        
        // deck_state->tsm_node->update_wait( deck_state->tsm_node );

        // This doesn't mean anything after the first run thru the loop.
        // Can we clean that up a bit?
        // thread_ready tells the lib deck's init sequence to enter 'running' state.
        deck_state->thread_ready = true;

        // printf( "lib deck thread done\n" );
        // Wait for signal from soundcard fast cycle to process another buffer
        sem_wait( &deck_state->start_cycle );
    }

    // Thread cleanup before exit.

    // Give back the deck station thread
    zdj_thread_deck_1_station_available = true;

    // Detach so kernel cleans up mem
    pthread_detach( pthread_self( ) );
    return NULL;
}

static double _max_scrub_rate_for_deck( zdj_deck_t * deck ) {

    return 1400.0f;
}

static double _min_scrub_rate_for_deck( zdj_deck_t * deck ) {
    return -1400.0f;
}
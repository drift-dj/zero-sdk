#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <math.h>
#include <sys/syscall.h>

#include <zerodj/controls/zdj_controls.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/dj/zdj_deck_dj.h>
#include <zerodj/signal/deck/zdj_deck_manager.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/signal/pipeline/node/audio/tsm/zdj_tsm_pitch_node.h>
#include <zerodj/signal/pipeline/node/audio/tsm/zdj_tsm_tempo_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

static void _handle_controls( zdj_deck_t * deck, zdj_control_event_t * event );
// static double _max_scrub_rate_for_deck( zdj_deck_t * deck );
// static double _min_scrub_rate_for_deck( zdj_deck_t * deck );

void zdj_deck_dj_init_controls( zdj_deck_t * deck ) {
    deck->handle_control_event = &_handle_controls;
}

// This is invoked during the deck manager control update cycle.
// Called on the soundcard fast audio cycle before mixdown.
static void _handle_controls( zdj_deck_t * deck, zdj_control_event_t * event ) {
    // printf( "zdj_dj_deck_handle_controls: %p %d %d\n", deck, deck->station, event->id );
    zdj_deck_platter_t * platter = &deck->controls.platter;
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_tsm_pitch_node_state_t * tsm_pitch_state = (zdj_tsm_pitch_node_state_t*)deck_state->tsm_pitch_node->state;
    zdj_tsm_tempo_node_state_t * tsm_tempo_state = (zdj_tsm_tempo_node_state_t*)deck_state->tsm_tempo_node->state;
    zdj_soundcard_node_t * soundcard_node;
    zdj_soundcard_node_t * node;
    zdj_soundcard_dsp_dto_t * dsp_dto;
    zdj_soundcard_dsp_stage_dto_t * dsp_stage;
    double sample_target;
    double val;
    int dir;

    switch ( event->id ) {

    //////////////////
    // Play / Pause //
    //////////////////  
     
    case ZDJ_DECK_1_CONTROL_PLAY_PAUSE:
    case ZDJ_DECK_2_CONTROL_PLAY_PAUSE:
    
        // Play
        if( !platter->motor.enabled &&
            decode_state->head.origin_d < decode_state->song_pcm_duration 
        ) {
            platter->motor.enabled = true;
            platter->motor.set_rate = platter->motor.pitch_setting;
            platter->motor.state = ZDJ_PLATTER_MOTOR_SPIN_UP;
            platter->motor.cur_spin_up_cycle = 0;
            deck_state->tsm_source = ZDJ_DECK_TSM_SOURCE_PITCH;
        
        // Pause
        } else {
            if( platter->slip.state == ZDJ_PLATTER_SLIP_LAMINAR_TEMPO ) {
                // If we're in tempo mode, we need to sync the needle head with tempo node.
                zdj_dj_deck_reset_platter( platter, tsm_tempo_state->decode_coord );
                // We also need to set the pitch node's sample addresses to something reasonable
                tsm_pitch_state->decode_start_coord = tsm_tempo_state->decode_coord - ZDJ_SOUNDCARD_BUF_LEN;
            }
            platter->motor.enabled = false;
            platter->motor.state = ZDJ_PLATTER_MOTOR_SPIN_DOWN;
            platter->motor.cur_spin_down_cycle = 0;
            // Ensure slip is in pitch mode so we hear spin down.
            platter->slip.state = ZDJ_PLATTER_SLIP_LAMINAR_PITCH;
        }
        printf( "dj deck toggle play/pause: %1.3f\n", platter->motor.set_rate );
        break;


    ////////////
    // Hotcue //
    ////////////  

    case ZDJ_DECK_1_CONTROL_HOTCUE_START:
    case ZDJ_DECK_2_CONTROL_HOTCUE_START:

        // TODO: If a loop is enabled, jump to loop start.
        if( deck->controls.loop_state.is_enabled ) { break; }
        //  Requires working skip-in-loop before implementation.

        // Get current cuepoint or beatgrid start or song start.
        sample_target = 0;
        if( deck_state->song->performance ) {
            if( deck_state->song->performance->cuepoint_count > 0 ) {
                sample_target = deck_state->song->performance->cuepoints[ deck_state->song->performance->current_cuepoint ]->sample;
            } else if( deck_state->song->performance->has_beat_grid ) {
                sample_target = deck_state->song->performance->beat_grid_start_sample;
            }
        }

        // printf( "dj deck hotcue start: %1.0f\n", sample_target );
        
        // We are not playing
        if( !platter->motor.enabled ) {
            // // TEMPORARY
            // // If loop is enabled, set target to loop start
            // if( deck->controls.loop_state.is_enabled ) {
            //     sample_target = deck->controls.loop_state.start_origin_d;
            // }
            // //

            // If we're not playing and head is near current cuepoint, just play
            // printf( "head origin:%1.0f\n", decode_state->head.origin_d );
            if( fabs( decode_state->head.origin_d - sample_target ) < decode_state->estimated_packet_sample_count ) {
                printf( "hotcue play\n" );
                platter->motor.enabled = true;
                platter->motor.set_rate = platter->motor.pitch_setting;
                platter->motor.state = ZDJ_PLATTER_MOTOR_SPIN_UP;
                platter->motor.cur_spin_up_cycle = 0;
                deck_state->tsm_source = ZDJ_DECK_TSM_SOURCE_PITCH;
            
            // If we're not playing and head is away from current cuepoint, needledrop
            } else {
                // printf( "hotcue needledrop\n" );
                deck->new_needledrop( deck, sample_target );
            }
            
        // We are playing
        } else {
            // printf( "hotcue skip\n" );

            // Skip to current cuepoint immediately -- no quantize
            double sample_offset = sample_target - decode_state->head.origin_d;
            deck->new_skip( 
                deck, 
                sample_offset,
                // (deck->controls.discon_quantize) ? ZDJ_DECK_SKIP_TYPE_QUANT : ZDJ_DECK_SKIP_TYPE_UNQUANT
                ZDJ_DECK_SKIP_TYPE_UNQUANT
            );
        }

        break;

    case ZDJ_DECK_1_CONTROL_HOTCUE_END:
    case ZDJ_DECK_2_CONTROL_HOTCUE_END:
        // TODO: implement in-loop hotcue
        if( deck->controls.loop_state.is_enabled ){ break; }
        // printf( "dj deck hotcue end\n" );
        if( platter->slip.state == ZDJ_PLATTER_SLIP_LAMINAR_TEMPO ) {
            // If we're in tempo mode, we need to sync the needle head with tempo node.
            zdj_dj_deck_reset_platter( platter, tsm_tempo_state->decode_coord );
            // We also need to set the pitch node's sample addresses to something reasonable
            tsm_pitch_state->decode_start_coord = tsm_tempo_state->decode_coord - ZDJ_SOUNDCARD_BUF_LEN;
        }
        platter->motor.enabled = false;
        platter->motor.state = ZDJ_PLATTER_MOTOR_SPIN_DOWN;
        platter->motor.cur_spin_down_cycle = deck->controls.platter.motor.spin_down_cycle_count - 2;
        // Ensure slip is in pitch mode so we hear spin down.
        platter->slip.state = ZDJ_PLATTER_SLIP_LAMINAR_PITCH;

        // Stage a needledrop to the current cuepoint
        // printf( "hotcue needledrop\n" );
        deck->new_needledrop( deck, deck->controls.hotcue_state.current_target_d );
        break;

    case ZDJ_DECK_1_CONTROL_HOTCUE_NEXT:
    case ZDJ_DECK_2_CONTROL_HOTCUE_NEXT:
        printf( "hotcue next\n" );
        if( deck_state->song->performance &&
            deck_state->song->performance->cuepoint_count > 0 
        ){
            deck_state->song->performance->current_cuepoint++;
            deck_state->song->performance->current_cuepoint %= deck_state->song->performance->cuepoint_count;
            zdj_library_cuepoint_t * cuepoint = deck_state->song->performance->cuepoints[ deck_state->song->performance->current_cuepoint ];
            deck->controls.hotcue_state.current_target_d = (double)cuepoint->sample;

            if( platter->slip.state == ZDJ_PLATTER_SLIP_LAMINAR_TEMPO ) {
                // If we're in tempo mode, we need to sync the needle head with tempo node.
                zdj_dj_deck_reset_platter( platter, tsm_tempo_state->decode_coord );
                // We also need to set the pitch node's sample addresses to something reasonable
                tsm_pitch_state->decode_start_coord = tsm_tempo_state->decode_coord - ZDJ_SOUNDCARD_BUF_LEN;
            }
            platter->motor.enabled = false;
            platter->motor.state = ZDJ_PLATTER_MOTOR_SPIN_DOWN;
            platter->motor.cur_spin_down_cycle = deck->controls.platter.motor.spin_down_cycle_count - 2;
            // Ensure slip is in pitch mode so we hear spin down.
            platter->slip.state = ZDJ_PLATTER_SLIP_LAMINAR_PITCH;

            // Stage a needledrop to the current cuepoint
            deck->new_needledrop( deck, deck->controls.hotcue_state.current_target_d );
        }
        break;

    ///////////
    // Scrub //
    ///////////  

    case ZDJ_DECK_1_CONTROL_SCRUB:
    case ZDJ_DECK_2_CONTROL_SCRUB:
        // printf( "dj deck scrub\n" );
        if( platter->motor.enabled && !platter->scratch_override
        ) {
            // Nudge a running platter
            if( deck_state->tempo_tsm_enabled ) {
                // Use tempo nudge rate to bypass the platter sim and only change rate val.
                platter->slip.tempo_nudge_rate += event->i_val * 0.001 * platter->nudge_coeff;
                platter->slip.state = ZDJ_PLATTER_SLIP_NUDGE_TEMPO;
            } else {
                // Use set_val in pitch mode so the full platter sim is employed.
                platter->slip.set_val += event->i_val * platter->nudge_coeff;
                platter->slip.state = ZDJ_PLATTER_SLIP_NUDGE_PITCH;
            }
            
        } else {
            // Scratch a non-running or scratch-override platter
            if( !platter->motor.enabled || platter->scratch_override ) {
                // Bug out early if we're outside the song origin coords.
                if( event->i_val < 0 && 
                    (decode_state->head.origin_d < -1000.0)
                ) {
                    platter->slip.sim_counter = 0;
                    break;
                } else if( event->i_val > 0 && 
                           (decode_state->head.origin_d > decode_state->song_pcm_duration + 1000.0)
                ) {
                    platter->slip.sim_counter = 0;
                    break;
                }

                if( platter->slip.state == ZDJ_PLATTER_SLIP_LAMINAR_TEMPO ) {
                    // If we're in tempo drive mode, we need to sync the needle head with tempo node.
                    zdj_dj_deck_reset_platter( platter, tsm_tempo_state->decode_coord );
                    // We also need to set the pitch node's sample addresses to something reasonable
                    tsm_pitch_state->decode_start_coord = tsm_tempo_state->decode_coord - ZDJ_SOUNDCARD_BUF_LEN;
                }

                // Build scrub rate from event input
                double scrub_rate = (double)event->i_val * platter->scratch_coeff;  
                double hyperscrub_offset;              

                double max_hyperscrub_offset = decode_state->win_sample_count;
                // double max_scrub_rate = decode_state->win_fwd_sample_count / 2;
                double max_scrub_rate = decode_state->win_fwd_sample_count;

                // Hyperscrub behavior:
                // When scrubbing faster than the pitch-stretch algo will allow based 
                // on decode window, "catch up" to the scrub rate by inserting periodic skips.
                if( scrub_rate > max_scrub_rate ) {
                    // printf( "//// limiting fwd rate ////\n" );
                    hyperscrub_offset = fmin( scrub_rate, max_hyperscrub_offset );
                    deck->controls.hyperscrub_state.req_offset += hyperscrub_offset;
                    
                    scrub_rate = max_scrub_rate;

                } else if( scrub_rate < (max_scrub_rate * -1) ) {
                    // printf( "//// limiting rev rate ////\n" );
                    hyperscrub_offset = fmax( scrub_rate, max_hyperscrub_offset*-1 );
                    deck->controls.hyperscrub_state.req_offset += hyperscrub_offset;

                    scrub_rate = max_scrub_rate * -1;
                }

                // printf( "scrub rate: %1.1f\n", scrub_rate );
                platter->slip.set_val += scrub_rate;
                platter->slip.state = ZDJ_PLATTER_SLIP_SCRATCH;
            }
        }
        platter->slip.sim_counter = 0;
        break;


    ///////////
    // Tempo //
    ///////////  

    case ZDJ_DECK_1_CONTROL_TEMPO:
    case ZDJ_DECK_2_CONTROL_TEMPO:
        // printf( "tempo: %1.2f\n", platter->motor.pitch_setting );
        
        // if( clock_is_output( ) ) {
        if( zdj_deck_manager( )->sync.active ) {
            zdj_deck_manager_update_sync_bpm( event->i_val * 1 );
        } else if( deck->can_sync ) {
            deck->offset_sync_bpm( deck, event->i_val * 1 );
        } else {
            deck->offset_pitch_setting( deck, (double)event->i_val * 0.01 );
        }
        // } else {
            // Clock is input - flash tempo warning
        // }
        break;

    case ZDJ_DECK_1_CONTROL_TEMPO_FINE:
    case ZDJ_DECK_2_CONTROL_TEMPO_FINE:
        // printf( "tempo: fine %1.2f\n", platter->motor.pitch_setting );
        // if( clock_is_output( ) ) {
        if( zdj_deck_manager( )->sync.active ) {
            zdj_deck_manager_update_sync_bpm( event->i_val * 0.01 );
        } else if( deck->can_sync ) {
            deck->offset_sync_bpm( deck, event->i_val * 0.01 );
        } else {
            deck->offset_pitch_setting( deck, (double)event->i_val * 0.001 );
        }
        // } else {
            // Clock is input - flash tempo warning
        // }
        break;


    /////////////////
    // EQ + Filter //
    /////////////////

    case ZDJ_DECK_1_CONTROL_EQ_LO:
        soundcard_node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE );
        dsp_dto = soundcard_node->dsp_dto;
        dsp_stage = dsp_dto->get_stage_for_type( soundcard_node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ );
        if( dsp_stage ) { dsp_stage->adjust_knob( dsp_stage, 0, event->i_val ); }
        // printf( "dj deck 1 eq lo\n" );
        break;
    case ZDJ_DECK_2_CONTROL_EQ_LO:
        soundcard_node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE );
        dsp_dto = soundcard_node->dsp_dto;
        dsp_stage = dsp_dto->get_stage_for_type( soundcard_node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ );
        if( dsp_stage ) { dsp_stage->adjust_knob( dsp_stage, 0, event->i_val ); }
        // printf( "dj deck 2 eq lo\n" );
        // Set bus node's EQ lo val
        break;
    
    case ZDJ_DECK_1_CONTROL_EQ_MID:
        soundcard_node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE );
        dsp_dto = soundcard_node->dsp_dto;
        dsp_stage = dsp_dto->get_stage_for_type( soundcard_node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ );
        if( dsp_stage ) { dsp_stage->adjust_knob( dsp_stage, 1, event->i_val ); }
        // printf( "dj deck 1 eq mid\n" );
        break;
    case ZDJ_DECK_2_CONTROL_EQ_MID:
        soundcard_node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE );
        dsp_dto = soundcard_node->dsp_dto;
        dsp_stage = dsp_dto->get_stage_for_type( soundcard_node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ );
        if( dsp_stage ) { dsp_stage->adjust_knob( dsp_stage, 1, event->i_val ); }
        // printf( "dj deck 2 eq mid\n" );
        break;

    case ZDJ_DECK_1_CONTROL_EQ_HI:
        soundcard_node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE );
        dsp_dto = soundcard_node->dsp_dto;
        dsp_stage = dsp_dto->get_stage_for_type( soundcard_node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ );
        if( dsp_stage ) { dsp_stage->adjust_knob( dsp_stage, 2, event->i_val ); }
        // printf( "dj deck 1 eq hi\n" );
        break;
    case ZDJ_DECK_2_CONTROL_EQ_HI:
        soundcard_node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE );
        dsp_dto = soundcard_node->dsp_dto;
        dsp_stage = dsp_dto->get_stage_for_type( soundcard_node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ );
        if( dsp_stage ) { dsp_stage->adjust_knob( dsp_stage, 2, event->i_val ); }
        // printf( "dj deck 2 eq hi\n" );
        break;

    case ZDJ_DECK_1_2_BASS_SWAP:
        printf( "dj deck bass swap\n" );
        // Invert event input to deck 1 knob
        soundcard_node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE );
        dsp_dto = soundcard_node->dsp_dto;
        dsp_stage = dsp_dto->get_stage_for_type( soundcard_node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ );
        if( dsp_stage ) { dsp_stage->adjust_knob( dsp_stage, 0, event->i_val * -1 ); }

        // Send event input to deck 2 knob
        soundcard_node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE );
        dsp_dto = soundcard_node->dsp_dto;
        dsp_stage = dsp_dto->get_stage_for_type( soundcard_node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ );
        if( dsp_stage ) { dsp_stage->adjust_knob( dsp_stage, 0, event->i_val ); }
        break;

    case ZDJ_DECK_1_CONTROL_FILTER_0:
        // printf( "dj deck filter 0\n" );
        soundcard_node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE );
        dsp_dto = soundcard_node->dsp_dto;
        dsp_stage = dsp_dto->get_stage_for_type( soundcard_node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_FILT );
        if( dsp_stage ) { 
            dsp_stage->adjust_knob( dsp_stage, 0, event->i_val ); 
        }
        break;
    case ZDJ_DECK_2_CONTROL_FILTER_0:
        // printf( "dj deck filter 0\n" );
        soundcard_node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE );
        dsp_dto = soundcard_node->dsp_dto;
        dsp_stage = dsp_dto->get_stage_for_type( soundcard_node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_FILT );
        if( dsp_stage ) { 
            dsp_stage->adjust_knob( dsp_stage, 0, event->i_val ); 
        }
        break;
    
    case ZDJ_DECK_1_CONTROL_FILTER_1:
        // printf( "dj deck filter 1\n" );
        soundcard_node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE );
        dsp_dto = soundcard_node->dsp_dto;
        dsp_stage = dsp_dto->get_stage_for_type( soundcard_node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_FILT );
        if( dsp_stage ) { dsp_stage->adjust_knob( dsp_stage, 1, event->i_val ); }
        break;
    case ZDJ_DECK_2_CONTROL_FILTER_1:
        // printf( "dj deck filter 1\n" );
        soundcard_node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE );
        dsp_dto = soundcard_node->dsp_dto;
        dsp_stage = dsp_dto->get_stage_for_type( soundcard_node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_FILT );
        if( dsp_stage ) { dsp_stage->adjust_knob( dsp_stage, 1, event->i_val ); }
        break;
    
    case ZDJ_DECK_1_CONTROL_FILTER_2:
        // printf( "dj deck filter 1\n" );
        soundcard_node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE );
        dsp_dto = soundcard_node->dsp_dto;
        dsp_stage = dsp_dto->get_stage_for_type( soundcard_node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_FILT );
        if( dsp_stage ) { dsp_stage->adjust_knob( dsp_stage, 2, event->i_val ); }
        break;
    case ZDJ_DECK_2_CONTROL_FILTER_2:
        // printf( "dj deck filter 2\n" );
        soundcard_node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE );
        dsp_dto = soundcard_node->dsp_dto;
        dsp_stage = dsp_dto->get_stage_for_type( soundcard_node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_FILT );
        if( dsp_stage ) { dsp_stage->adjust_knob( dsp_stage, 2, event->i_val ); }
        break;

    case ZDJ_DECK_1_CONTROL_FILTER_RESET:
        // printf( "dj deck filter reset\n" );
        soundcard_node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE );
        dsp_dto = soundcard_node->dsp_dto;
        dsp_stage = dsp_dto->get_stage_for_type( soundcard_node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_FILT );
        if( dsp_stage ) { dsp_stage->set_knob( dsp_stage, 0, 0.0 ); }
        break;
    case ZDJ_DECK_2_CONTROL_FILTER_RESET:
        // printf( "dj deck filter reset\n" );
        soundcard_node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE );
        dsp_dto = soundcard_node->dsp_dto;
        dsp_stage = dsp_dto->get_stage_for_type( soundcard_node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_FILT );
        if( dsp_stage ) { dsp_stage->set_knob( dsp_stage, 0, 0.0 ); }
        break;


    ///////////////////////
    // Fade / Trim / Cue //
    ///////////////////////  

    case ZDJ_DECK_1_CONTROL_FADE:
        // printf( "dj deck 1 fade\n" );
        node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_POSTFADE );
        node->dsp_dto->set_gain( node, 255 - event->i_val );
        event->blocked = true;
        break;
    case ZDJ_DECK_2_CONTROL_FADE:
        // printf( "dj deck 2 fade\n" );
        node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_POSTFADE );
        node->dsp_dto->set_gain( node, 255 - event->i_val );
        event->blocked = true;
        break;
    
    case ZDJ_DECK_1_CONTROL_TRIM:
        node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE );
        node->dsp_dto->adjust_gain( node, event->i_val );
        event->blocked = true;
        break;
    case ZDJ_DECK_2_CONTROL_TRIM:
        node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE );
        node->dsp_dto->adjust_gain( node, event->i_val );
        event->blocked = true;
        break;

    case ZDJ_DECK_1_CONTROL_PFL_TRIM:
        node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_CUE );
        node->dsp_dto->adjust_gain( node, event->i_val );
        event->blocked = true;
        break;
    case ZDJ_DECK_2_CONTROL_PFL_TRIM:
        node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_CUE );
        node->dsp_dto->adjust_gain( node, event->i_val );
        event->blocked = true;
        break;
    
    case ZDJ_DECK_1_CONTROL_PFL_TOGGLE_MUTE:
        node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_CUE );
        node->dsp_dto->toggle_mute( node );
        event->blocked = true;
        break;
    case ZDJ_DECK_2_CONTROL_PFL_TOGGLE_MUTE:
        node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_CUE );
        node->dsp_dto->toggle_mute( node );
        event->blocked = true;
        break;


    /////////////////
    // Loop / Skip //
    /////////////////  
        
    case ZDJ_DECK_1_CONTROL_LOOP_TOGGLE:
        // printf( "toggle loop: %1.1f/%1.2f\n", deck->controls.loop_state.pcm_len, deck->controls.loop_state.beatgrid_len );

        // Toggle loop on/off
        if( !deck->controls.loop_state.is_enabled ) {
            deck->new_loop( deck, deck->controls.loop_state.pcm_len, deck->controls.discon_quantize );
            // Takeover hotcue button for loop reset
            // zdj_activate_control( ZDJ_DECK_1_CONTROL_LOOP_RESET_TO_START );
            // zdj_deactivate_control( ZDJ_DECK_1_CONTROL_HOTCUE_END );
            zdj_activate_control_map( ZDJ_CONTROL_MAP_STATION_1_LOOP_ON );
        } else {
            // zdj_deck_disable_loop_req( deck );
            deck->disable_loop( deck );
            // Release hotcue button for loop reset
            // zdj_deactivate_control( ZDJ_DECK_1_CONTROL_LOOP_RESET_TO_START );
            // zdj_activate_control( ZDJ_DECK_1_CONTROL_HOTCUE_END );
            zdj_activate_control_map( ZDJ_CONTROL_MAP_STATION_1_LOOP_OFF );
        }
        break;
    case ZDJ_DECK_2_CONTROL_LOOP_TOGGLE:
        // printf( "toggle loop: %1.1f/%1.2f\n", deck->controls.loop_state.pcm_len, deck->controls.loop_state.beatgrid_len );

        // Toggle loop on/off
        if( !deck->controls.loop_state.is_enabled ) {
            deck->new_loop( deck, deck->controls.loop_state.pcm_len, deck->controls.discon_quantize );
            // Takeover hotcue button for loop reset
            // zdj_activate_control( ZDJ_DECK_2_CONTROL_LOOP_RESET_TO_START );
            // zdj_deactivate_control( ZDJ_DECK_2_CONTROL_HOTCUE_END );
            zdj_activate_control_map( ZDJ_CONTROL_MAP_STATION_2_LOOP_ON );
        } else {
            // zdj_deck_disable_loop_req( deck );
            deck->disable_loop( deck );
            // Release hotcue button for loop reset
            // zdj_deactivate_control( ZDJ_DECK_2_CONTROL_LOOP_RESET_TO_START );
            // zdj_activate_control( ZDJ_DECK_2_CONTROL_HOTCUE_END );
            zdj_activate_control_map( ZDJ_CONTROL_MAP_STATION_2_LOOP_OFF );
        }
        break;

    case ZDJ_DECK_1_CONTROL_LOOP_START:
    case ZDJ_DECK_2_CONTROL_LOOP_START:
        // printf( "loop start\n" );
        // deck->controls.loop_state.c_count_start /= 4;
        if( deck->controls.loop_state.is_enabled ) {
            if( deck->controls.discon_quantize ) {
                // printf( "quantize move\n" );
                deck->controls.loop_state.c_count_start += event->i_val;
                if( abs( deck->controls.loop_state.c_count_start ) > 6 ) {
                    double bg_offset = (double)deck->controls.loop_state.c_count_start * deck->controls.discon_quantize_val;
                    val = decode_state->get_d_offset_for_beatgrid_dist( 
                        deck_state->decode_node, bg_offset 
                    );
                    deck->controls.loop_state.c_count_start = 0;
                }
            } else {
                // printf( "unquantize move\n" );
                val = (double)event->i_val * 100.0;
            }
            deck->move_loop( deck, val );
        }
        
        break;


    case ZDJ_DECK_1_CONTROL_LOOP_START_ALT:
    case ZDJ_DECK_2_CONTROL_LOOP_START_ALT:
        // printf( "loop start\n" );
        // deck->controls.loop_state.c_count_start /= 4;
        if( deck->controls.loop_state.is_enabled ) {
            // printf( "quantize move alt\n" );
            if( deck->controls.discon_quantize ) {
                deck->controls.loop_state.c_count_start += event->i_val;
                if( abs( deck->controls.loop_state.c_count_start ) > 6 ) {
                    double bg_offset = (double)deck->controls.loop_state.c_count_start * deck->controls.discon_quantize_val * 4;
                    val = decode_state->get_d_offset_for_beatgrid_dist( 
                        deck_state->decode_node, bg_offset 
                    );
                    deck->controls.loop_state.c_count_start = 0;
                }
            } else {
                // printf( "unquantize move alt\n" );
                val = (double)event->i_val * 2000.0;
            }
            deck->move_loop( deck, val );
        }
        
        break;

    case ZDJ_DECK_1_CONTROL_LOOP_LENGTH:
    case ZDJ_DECK_2_CONTROL_LOOP_LENGTH:
        if( deck->controls.discon_quantize ) {
            deck->controls.loop_state.c_count_len += event->i_val;
            if( abs( deck->controls.loop_state.c_count_len ) > 8 ) {
                double bg_offset;
                if( deck->controls.loop_state.c_count_len > 0 ) {
                    bg_offset = deck->controls.discon_quantize_val;
                } else {
                    bg_offset = deck->controls.discon_quantize_val * -1.0;
                }
                
                val = decode_state->get_d_offset_for_beatgrid_dist( 
                    deck_state->decode_node, bg_offset 
                );
                deck->controls.loop_state.c_count_len = 0;
                deck->resize_loop( deck, val );
            }
        } else {
            val = (double)event->i_val * 100.0;
            deck->controls.loop_state.c_count_len = 0;
            deck->resize_loop( deck, val );
        }
        
        break;

    case ZDJ_DECK_1_CONTROL_LOOP_LENGTH_ALT:
    case ZDJ_DECK_2_CONTROL_LOOP_LENGTH_ALT:
        printf( "loop len alt\n" );
        if( deck->controls.discon_quantize ) {
            deck->controls.loop_state.c_count_len += event->i_val;
            if( abs( deck->controls.loop_state.c_count_len ) > 8 ) {
                printf( "cc_len: %d %1.3f\n", 
                    deck->controls.loop_state.c_count_len,
                    deck->controls.discon_quantize_val
                );
                double bg_offset = deck->controls.discon_quantize_val;
                val = decode_state->get_d_offset_for_beatgrid_dist( 
                    deck_state->decode_node, bg_offset 
                );
                deck->controls.loop_state.c_count_len = 0;
                deck->resize_loop( deck, val );
            }
        } else {
            val = (double)event->i_val * 2000.0;
            deck->controls.loop_state.c_count_len = 0;
            deck->resize_loop( deck, val );
        }
        
        break;

    case ZDJ_DECK_1_CONTROL_LOOP_RESET_TO_START:
    case ZDJ_DECK_2_CONTROL_LOOP_RESET_TO_START:
        // printf( "reset to loop start\n" );
        if( deck->controls.platter.motor.enabled || deck->controls.platter.motor.instant_rate > zdj_eps ) {

        } else {
            // Needledrop if deck is not playing
            double bg_offset = deck->controls.discon_quantize_val * event->i_val;
            double sample_offset = decode_state->get_d_offset_for_beatgrid_dist( 
                deck_state->decode_node, bg_offset 
            );
            double origin_d_coord = decode_state->head.origin_d + sample_offset;
            if( origin_d_coord > 0.0 && origin_d_coord < decode_state->song_pcm_duration ) {
                deck->new_needledrop( deck, origin_d_coord );
            }
        }
        break;

    case ZDJ_DECK_1_CONTROL_SKIP:
    case ZDJ_DECK_2_CONTROL_SKIP:
        // printf( "skip\n" );

        // TODO: implement in-loop hotcue
        if( deck->controls.loop_state.is_enabled ){ break; }

        // Temporarily disable loops for songs w/o beatgrids
        if( !deck_state->song->performance->has_beat_grid ) { break; }

        deck->controls.skip_state.c_count_skip += event->i_val;
        if( abs( deck->controls.skip_state.c_count_skip ) > 4 ) {
            if( deck->controls.platter.motor.enabled || deck->controls.platter.motor.instant_rate > zdj_eps ) {
                // Skip if deck is playing
                double bg_offset = deck->controls.discon_quantize_val * event->i_val;
                double sample_offset = decode_state->get_d_offset_for_beatgrid_dist( 
                    deck_state->decode_node, bg_offset 
                );
                deck->new_skip( 
                    deck, 
                    sample_offset,
                    (deck->controls.discon_quantize) ? ZDJ_DECK_SKIP_TYPE_QUANT : ZDJ_DECK_SKIP_TYPE_UNQUANT
                );
            } else {
                // Needledrop if deck is not playing
                double bg_offset = deck->controls.discon_quantize_val * event->i_val;
                double sample_offset = decode_state->get_d_offset_for_beatgrid_dist( 
                    deck_state->decode_node, bg_offset 
                );
                double origin_d_coord = decode_state->head.origin_d + sample_offset;
                if( origin_d_coord > 0.0 && origin_d_coord < decode_state->song_pcm_duration ) {
                    deck->new_needledrop( deck, origin_d_coord );
                }
            }

            deck->controls.skip_state.c_count_skip = 0;
        }
        break;

    case ZDJ_DECK_1_CONTROL_SKIP_LENGTH:
    case ZDJ_DECK_2_CONTROL_SKIP_LENGTH:

        // Temporarily disable loops for songs w/o beatgrids
        if( !deck_state->song->performance->has_beat_grid ) { break; }

        deck->controls.c_count_q_val += event->i_val;
        if( abs( deck->controls.c_count_q_val ) > 4 ) {
            dir = (event->i_val > 0) ? 1 : -1;
            if( dir > 0 ) {
                if( fabs( deck->controls.discon_quantize_val - 0.0625 ) < zdj_eps ) { // Sixteenth note
                    deck->controls.discon_quantize_val = 0.125;
                } else if( fabs( deck->controls.discon_quantize_val - 0.125 ) < zdj_eps ) { // Half note
                    deck->controls.discon_quantize_val = 0.25;
                }
            } else {
                if( fabs( deck->controls.discon_quantize_val - 0.125 ) < zdj_eps ) { // Sixteenth note
                    deck->controls.discon_quantize_val = 0.0625;
                } else if( fabs( deck->controls.discon_quantize_val - 0.250 ) < zdj_eps ) { // Half note
                    deck->controls.discon_quantize_val = 0.125;
                }
            }
            
            deck->controls.c_count_q_val = 0;
        }
        break;

    case ZDJ_DECK_1_CONTROL_SKIP_SET_ORIGIN:
    case ZDJ_DECK_2_CONTROL_SKIP_SET_ORIGIN:
        printf( "setting skip origin\n" );
        deck->controls.skip_state.current_offset = 0.0;
        break;

    case ZDJ_DECK_1_CONTROL_SKIP_RESET_TO_ORIGIN:
    case ZDJ_DECK_2_CONTROL_SKIP_RESET_TO_ORIGIN:

        // Temporarily disable loops for songs w/o beatgrids
        if( !deck_state->song->performance->has_beat_grid ) { break; }


        // printf( "reset skip to origin: %1.3f\n", deck->controls.skip_state.current_offset );
        if( deck->controls.platter.motor.enabled || deck->controls.platter.motor.instant_rate > zdj_eps ) {
            // Skip if deck is playing
            double sample_offset = deck->controls.skip_state.current_offset * -1;
            deck->new_skip( 
                deck, 
                sample_offset,
                (deck->controls.discon_quantize) ? ZDJ_DECK_SKIP_TYPE_QUANT : ZDJ_DECK_SKIP_TYPE_UNQUANT
            );
        } else {
            // Needledrop if deck is not playing
            double sample_offset = deck->controls.skip_state.current_offset * -1;
            double origin_d_coord = decode_state->head.origin_d + sample_offset;
            if( origin_d_coord > 0.0 && origin_d_coord < decode_state->song_pcm_duration ) {
                deck->new_needledrop( deck, origin_d_coord );
            }
        }
        // printf( "reset skip to origin done\n" );
        break;


    //////////
    // Sync //
    //////////  

    case ZDJ_DECK_CONTROL_SYNC_TOGGLE:
        printf( "sync toggle\n" );
        if( zdj_deck_manager( )->sync.preferred ) {
            zdj_deck_manager_set_prefer_sync( false );
        } else {
            zdj_deck_manager_set_prefer_sync( true );
        }
        break;

    case ZDJ_DECK_1_CONTROL_SYNC_MULT:
    case ZDJ_DECK_2_CONTROL_SYNC_MULT:
        if( deck->can_sync ) { 
            // printf( "sync mult\n" );
            deck_state->sync_mult_ui_counter += event->i_val;
            if( abs( deck_state->sync_mult_ui_counter ) > 4 ) {
                deck_state->sync_mult_ui_counter = 0;
                if( fabs( 1.0 - deck->sync_factor ) < zdj_eps ) {
                    if( event->i_val > 0 ) {
                        deck->request_sync_mult( deck, 2.0 ); 
                    } else {
                        deck->request_sync_mult( deck, 0.5 ); 
                    }
                } else if( fabs( 2.0 - deck->sync_factor ) < zdj_eps ) {
                    if( event->i_val < 0 ) {
                        deck->request_sync_mult( deck, 1.0 ); 
                    }
                } else if( fabs( 0.5 - deck->sync_factor ) < zdj_eps ) {
                    if( event->i_val > 0 ) {
                        deck->request_sync_mult( deck, 1.0 ); 
                    }
                }
            }
        }
        break;


    ////////
    // FX //
    ////////  

    case ZDJ_DECK_1_CONTROL_FX_SELECT:
    case ZDJ_DECK_2_CONTROL_FX_SELECT:
        break;

    case ZDJ_DECK_1_CONTROL_FX_0:
    case ZDJ_DECK_2_CONTROL_FX_0:
        break;
    
    case ZDJ_DECK_1_CONTROL_FX_1:
    case ZDJ_DECK_2_CONTROL_FX_1:
        break;

    case ZDJ_DECK_1_CONTROL_FX_2:
    case ZDJ_DECK_2_CONTROL_FX_2:
        break;

    case ZDJ_DECK_1_CONTROL_FX_3:
    case ZDJ_DECK_2_CONTROL_FX_3:
        break;

    case ZDJ_DECK_1_CONTROL_FX_4:
    case ZDJ_DECK_2_CONTROL_FX_4:
        break;

    case ZDJ_DECK_1_CONTROL_FX_5:
    case ZDJ_DECK_2_CONTROL_FX_5:
        break;

    default:
        break;
    }
}

// static double _max_scrub_rate_for_deck( zdj_deck_t * deck ) {

//     return 1400.0f;
// }

// static double _min_scrub_rate_for_deck( zdj_deck_t * deck ) {
//     return -1400.0f;
// }
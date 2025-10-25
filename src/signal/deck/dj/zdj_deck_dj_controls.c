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
#include <zerodj/signal/pipeline/node/audio/tsm/zdj_tsm_pitch_node.h>
#include <zerodj/signal/pipeline/node/audio/tsm/zdj_tsm_tempo_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

static void _handle_controls( zdj_deck_t * deck, zdj_control_event_t * event );
static double _max_scrub_rate_for_deck( zdj_deck_t * deck );
static double _min_scrub_rate_for_deck( zdj_deck_t * deck );

void zdj_deck_dj_init_controls( zdj_deck_t * deck ) {
    deck->handle_control_event = &_handle_controls;
}

// This is invoked during the deck manager control update cycle.
// Called on the soundcard fast audio cycle before mixdown.
static void _handle_controls( zdj_deck_t * deck, zdj_control_event_t * event ) {
    // printf( "zdj_dj_deck_handle_controls: %p %d %d\n", deck, deck->station, event->id );
    zdj_deck_platter_t * platter = &deck->controls.platter;
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_tsm_pitch_node_state_t * tsm_pitch_state = (zdj_tsm_pitch_node_state_t*)deck_state->tsm_pitch_node->state;
    zdj_tsm_tempo_node_state_t * tsm_tempo_state = (zdj_tsm_tempo_node_state_t*)deck_state->tsm_tempo_node->state;

    zdj_soundcard_node_t * node;
    int dir;

    switch ( event->id ) {


    /////////////
    // Out Vol //
    /////////////  

    case ZDJ_DECK_CONTROL_LR_VOL:
        node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS );
        node->gain += event->i_val * 2;
        if( node->gain > 255 ) { node->gain = 255; }
        else if( node->gain < 0 ) { node->gain = 0; }
        event->blocked = true;
        break;

    case ZDJ_DECK_CONTROL_CUE_VOL:
        node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS );
        node->gain += event->i_val * 2;
        if( node->gain > 255 ) { node->gain = 255; }
        else if( node->gain < 0 ) { node->gain = 0; }
        event->blocked = true;
        break;


    //////////////////
    // Play / Pause //
    //////////////////  
     
    case ZDJ_DECK_1_CONTROL_PLAY_PAUSE:
    case ZDJ_DECK_2_CONTROL_PLAY_PAUSE:
        // Play
        if( !platter->motor.enabled ) {
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
        printf( "dj deck hotcue start\n" );
        platter->motor.set_rate = platter->motor.pitch_setting;
        platter->motor.enabled = true;
        // Start playback w/no rate ramp
        break;

    case ZDJ_DECK_1_CONTROL_HOTCUE_END:
    case ZDJ_DECK_2_CONTROL_HOTCUE_END:
        printf( "dj deck hotcue end\n" );
        platter->motor.set_rate = 0.0;
        platter->motor.enabled = false;
        // Skip back to hotcue point
        // Recharge tempo tsm buffer
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
                if( platter->slip.state == ZDJ_PLATTER_SLIP_LAMINAR_TEMPO ) {
                    // If we're in tempo drive mode, we need to sync the needle head with tempo node.
                    zdj_dj_deck_reset_platter( platter, tsm_tempo_state->decode_coord );
                    // We also need to set the pitch node's sample addresses to something reasonable
                    tsm_pitch_state->decode_start_coord = tsm_tempo_state->decode_coord - ZDJ_SOUNDCARD_BUF_LEN;
                }
                double scrub_rate = (double)event->i_val * platter->scratch_coeff;
                if( scrub_rate > _max_scrub_rate_for_deck( deck ) ){ scrub_rate = _max_scrub_rate_for_deck( deck ); }
                if( scrub_rate < _min_scrub_rate_for_deck( deck ) ){ scrub_rate = _min_scrub_rate_for_deck( deck ); }
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
        // platter->motor.pitch_setting += event->i_val * 0.02;
        // deck->update_sync_tempo( deck, event->i_val * 0.02 );
        if( zdj_deck_manager( )->sync.enabled ) {
            zdj_deck_manager_update_sync_bpm( event->i_val * 1 );
        } else {
            deck->offset_sync_bpm( deck, event->i_val * 1 );
        }
        break;

    case ZDJ_DECK_1_CONTROL_TEMPO_FINE:
    case ZDJ_DECK_2_CONTROL_TEMPO_FINE:
        // printf( "tempo: fine %1.2f\n", platter->motor.pitch_setting );
        // platter->motor.pitch_setting += event->i_val * 0.001;
        // deck->update_sync_tempo( deck, event->i_val * 0.001 );
        if( zdj_deck_manager( )->sync.enabled ) {
            zdj_deck_manager_update_sync_bpm( event->i_val * 0.01 );
        } else {
            deck->offset_sync_bpm( deck, event->i_val * 0.01 );
        }
        break;


    /////////////////
    // EQ + Filter //
    /////////////////

    case ZDJ_DECK_1_CONTROL_EQ_LO:
    case ZDJ_DECK_2_CONTROL_EQ_LO:
        printf( "dj deck eq lo\n" );
        break;
    
    case ZDJ_DECK_1_CONTROL_EQ_MID:
    case ZDJ_DECK_2_CONTROL_EQ_MID:
        printf( "dj deck eq mid\n" );
        break;

    case ZDJ_DECK_1_CONTROL_EQ_HI:
    case ZDJ_DECK_2_CONTROL_EQ_HI:
        printf( "dj deck eq hi\n" );
        break;

    case ZDJ_DECK_1_2_BASS_SWAP:
        printf( "dj deck bass swap\n" );
        break;

    case ZDJ_DECK_1_CONTROL_FILTER_0:
    case ZDJ_DECK_2_CONTROL_FILTER_0:
        printf( "dj deck filter\n" );
        break;


    ///////////////////////
    // Fade / Trim / Cue //
    ///////////////////////  

    case ZDJ_DECK_CONTROL_XFADE:
        printf( "dj deck xfad\n" );
        break;

    case ZDJ_DECK_1_CONTROL_FADE:
    case ZDJ_DECK_2_CONTROL_FADE:
        printf( "dj deck fade\n" );
        break;
    
    case ZDJ_DECK_1_CONTROL_TRIM:
    case ZDJ_DECK_2_CONTROL_TRIM:
        printf( "dj deck trim\n" );
        break;

    case ZDJ_DECK_1_CONTROL_PFL_TRIM:
    case ZDJ_DECK_2_CONTROL_PFL_TRIM:
        printf( "dj deck pfl trim\n" );
        break;
    
    case ZDJ_DECK_1_CONTROL_PFL_TOGGLE_MUTE:
    case ZDJ_DECK_2_CONTROL_PFL_TOGGLE_MUTE:
        printf( "dj deck pfl mute\n" );
        break;


    /////////////////
    // Loop / Skip //
    /////////////////  
        
    case ZDJ_DECK_1_CONTROL_LOOP_TOGGLE:
    case ZDJ_DECK_2_CONTROL_LOOP_TOGGLE:
        // Toggle loop on/off
        if( !deck->controls.loop_state.is_enabled ) {
            // zdj_deck_new_loop_req( deck, 20003, deck->controls.loop_state.quantize );
            deck->new_loop( deck, 20003, deck->controls.loop_state.quantize );
        } else {
            // zdj_deck_disable_loop_req( deck );
            deck->disable_loop( deck );
        }
        break;

    case ZDJ_DECK_1_CONTROL_LOOP_START:
    case ZDJ_DECK_2_CONTROL_LOOP_START:
        break;

    case ZDJ_DECK_1_CONTROL_LOOP_LENGTH:
    case ZDJ_DECK_2_CONTROL_LOOP_LENGTH:
        printf( "loop length\n" );
        dir = (event->i_val > 0) ? 1 : -1;
        if( deck->controls.loop_state.quantize ) {
            double len = deck->controls.loop_state.beatgrid_len;
            printf( "quant len: %1.3f\n", deck->controls.loop_state.beatgrid_len );
            if( dir == 1 ) {
                if( fabs( len - 0.25 ) < zdj_eps ) { // Quarter note
                    deck->controls.loop_state.beatgrid_len = 0.5;
                } else if( fabs( len - 0.5 ) < zdj_eps ) { // Half note
                    deck->controls.loop_state.beatgrid_len = 1.0;
                } else if( fabs( len - 1.0 ) < zdj_eps ) { // 1-Bar
                    deck->controls.loop_state.beatgrid_len = 4.0;
                } else if( fabs( len - 4.0 ) < zdj_eps ) { // 4-Bar
                    deck->controls.loop_state.beatgrid_len = 8.0;
                } else if( fabs( len - 8.0 ) < zdj_eps ) { // 16-Bar
                    deck->controls.loop_state.beatgrid_len = 16.0;
                } else if( fabs( len - 16.0 ) < zdj_eps ) { // 32-Bar
                    deck->controls.loop_state.beatgrid_len = 32.0;
                } else if( fabs( len - 32.0 ) < zdj_eps ) { // 64-Bar
                    deck->controls.loop_state.beatgrid_len = 64.0;
                }
            } else {
                if( fabs( len - 0.5 ) < zdj_eps ) { // Quarter note
                    deck->controls.loop_state.beatgrid_len = 0.25;
                } else if( fabs( len - 1.0 ) < zdj_eps ) { // Half note
                    deck->controls.loop_state.beatgrid_len = 0.5;
                } else if( fabs( len - 4.0 ) < zdj_eps ) { // 1-Bar
                    deck->controls.loop_state.beatgrid_len = 1.0;
                } else if( fabs( len - 8.0 ) < zdj_eps ) { // 4-Bar
                    deck->controls.loop_state.beatgrid_len = 4.0;
                } else if( fabs( len - 16.0 ) < zdj_eps ) { // 16-Bar
                    deck->controls.loop_state.beatgrid_len = 8.0;
                } else if( fabs( len - 32.0 ) < zdj_eps ) { // 32-Bar
                    deck->controls.loop_state.beatgrid_len = 16.0;
                } else if( fabs( len - 64.0 ) < zdj_eps ) { // 32-Bar
                    deck->controls.loop_state.beatgrid_len = 32.0;
                }
            }
        } else {
            
        }
        break;

    case ZDJ_DECK_1_CONTROL_SKIP:
    case ZDJ_DECK_2_CONTROL_SKIP:
        // printf( "skip\n" );
        // zdj_deck_new_skip_req( deck, deck->controls.skip_state.skip_unit * event->i_val );
        deck->new_skip( deck, deck->controls.skip_state.skip_unit * event->i_val );
        break;

    case ZDJ_DECK_1_CONTROL_SKIP_LENGTH:
    case ZDJ_DECK_2_CONTROL_SKIP_LENGTH:
        dir = (event->i_val > 0) ? 1 : -1;
        if( dir > 0 ) {
            if( fabs( deck->controls.skip_state.skip_unit - 0.0625 ) < zdj_eps ) { // Sixteenth note
                deck->controls.skip_state.skip_unit = 0.125;
            } else if( fabs( deck->controls.skip_state.skip_unit - 0.125 ) < zdj_eps ) { // Half note
                deck->controls.skip_state.skip_unit = 0.25;
            }
        } else {
            if( fabs( deck->controls.skip_state.skip_unit - 0.125 ) < zdj_eps ) { // Sixteenth note
                deck->controls.skip_state.skip_unit = 0.0625;
            } else if( fabs( deck->controls.skip_state.skip_unit - 0.250 ) < zdj_eps ) { // Half note
                deck->controls.skip_state.skip_unit = 0.125;
            }
        }
        break;

    case ZDJ_DECK_1_CONTROL_SKIP_SET_ORIGIN:
    case ZDJ_DECK_2_CONTROL_SKIP_SET_ORIGIN:
        break;

    case ZDJ_DECK_1_CONTROL_SKIP_RESET_TO_ORIGIN:
    case ZDJ_DECK_2_CONTROL_SKIP_RESET_TO_ORIGIN:
        break;


    //////////
    // Sync //
    //////////  

    case ZDJ_DECK_CONTROL_SYNC_TOGGLE:
        printf( "sync toggle\n" );
        break;

    case ZDJ_DECK_1_CONTROL_SYNC_MULT:
    case ZDJ_DECK_2_CONTROL_SYNC_MULT:
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

static double _max_scrub_rate_for_deck( zdj_deck_t * deck ) {

    return 1400.0f;
}

static double _min_scrub_rate_for_deck( zdj_deck_t * deck ) {
    return -1400.0f;
}
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
    zdj_deck_control_platter_request_t * req;

    switch ( event->id ) {

    //////////////////
    // Play / Pause //
    //////////////////  
     
    case ZDJ_DECK_1_CONTROL_PLAY_PAUSE:
    case ZDJ_DECK_2_CONTROL_PLAY_PAUSE:
        if( (req = zdj_dj_deck_new_platter_request( deck )) ) {
            req->type = ZDJ_DECK_PLATTER_REQUEST_TOGGLE_MOTOR;
            req->spin_up = false;
            req->spin_down = true;
        }
        // printf( "dj deck toggle play/pause: %1.3f\n", platter->motor.set_rate );
        break;


    ///////////
    // Scrub //
    ///////////  

    case ZDJ_DECK_1_CONTROL_SCRUB:
    case ZDJ_DECK_2_CONTROL_SCRUB:
        if( (req = zdj_dj_deck_new_platter_request( deck )) ) {
            req->type = ZDJ_DECK_PLATTER_REQUEST_SCRUB;
            req->event_i_val = event->i_val;
        }
        break;
    case ZDJ_DECK_1_CONTROL_SCRUB_ALT_0:
    case ZDJ_DECK_2_CONTROL_SCRUB_ALT_0:
        if( (req = zdj_dj_deck_new_platter_request( deck )) ) {
            req->type = ZDJ_DECK_PLATTER_REQUEST_SCRUB_ALT_0;
            req->event_i_val = event->i_val;
        }
        break;
    case ZDJ_DECK_1_CONTROL_SCRUB_ALT_1:
    case ZDJ_DECK_2_CONTROL_SCRUB_ALT_1:
        if( (req = zdj_dj_deck_new_platter_request( deck )) ) {
            req->type = ZDJ_DECK_PLATTER_REQUEST_SCRUB_ALT_1;
            req->event_i_val = event->i_val;
        }
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


    ////////////
    // Hotcue //
    ////////////  

    case ZDJ_DECK_1_CONTROL_CUE_START:
    case ZDJ_DECK_2_CONTROL_CUE_START:
        // printf( "cue_start\n" );
        if( (req = zdj_dj_deck_new_platter_request( deck )) ) {
            req->type = ZDJ_DECK_PLATTER_REQUEST_START_MOTOR;
            req->spin_up = false;
            req->spin_down = false;
            zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_CUE_START );
        }
        break;

    case ZDJ_DECK_1_CONTROL_CUE_END:
    case ZDJ_DECK_2_CONTROL_CUE_END:
        zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_CUE_END );
        break;

    case ZDJ_DECK_1_CONTROL_CUE_NEXT:
    case ZDJ_DECK_2_CONTROL_CUE_NEXT:
        // printf( "hotcue next\n" );
        if( (req = zdj_dj_deck_new_platter_request( deck )) ) {
            req->type = ZDJ_DECK_PLATTER_REQUEST_STOP_MOTOR;
            req->spin_up = false;
            req->spin_down = false;
            zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_NEXT_CUEPOINT );
        }
        break;

    case ZDJ_DECK_1_CONTROL_CUE_SET:
    case ZDJ_DECK_2_CONTROL_CUE_SET:
        // printf( "set cuepoint\n" );
        zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_SET_CUEPOINT );
        break;


    ////////////
    // Hotcue //
    ////////////
    case ZDJ_DECK_1_CONTROL_HOTCUE_0:
    case ZDJ_DECK_2_CONTROL_HOTCUE_0:
        if( (req = zdj_dj_deck_new_platter_request( deck )) ) {
            req->type = ZDJ_DECK_PLATTER_REQUEST_START_MOTOR;
            req->spin_up = false;
            req->spin_down = false;
            if( zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_HOTCUE ) ) {
                deck->command_req.hotcue_num = 0;
            }
        }
        break;
    case ZDJ_DECK_1_CONTROL_HOTCUE_1:
    case ZDJ_DECK_2_CONTROL_HOTCUE_1:
        if( (req = zdj_dj_deck_new_platter_request( deck )) ) {
            req->type = ZDJ_DECK_PLATTER_REQUEST_START_MOTOR;
            req->spin_up = false;
            req->spin_down = false;
            if( zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_HOTCUE ) ) {
                deck->command_req.hotcue_num = 1;
            }
        }
        break;
    case ZDJ_DECK_1_CONTROL_HOTCUE_2:
    case ZDJ_DECK_2_CONTROL_HOTCUE_2:
        // if( zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_HOTCUE ) ) {
        //     deck->command_req.hotcue_num = 2;
        // }
        if( (req = zdj_dj_deck_new_platter_request( deck )) ) {
            req->type = ZDJ_DECK_PLATTER_REQUEST_START_MOTOR;
            req->spin_up = false;
            req->spin_down = false;
            if( zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_HOTCUE ) ) {
                deck->command_req.hotcue_num = 2;
            }
        }
        break;
    case ZDJ_DECK_1_CONTROL_HOTCUE_3:
    case ZDJ_DECK_2_CONTROL_HOTCUE_3:
        if( (req = zdj_dj_deck_new_platter_request( deck )) ) {
            req->type = ZDJ_DECK_PLATTER_REQUEST_START_MOTOR;
            req->spin_up = false;
            req->spin_down = false;
            if( zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_HOTCUE ) ) {
                deck->command_req.hotcue_num = 3;
            }
        }
        break;
    case ZDJ_DECK_1_CONTROL_HOTCUE_4:
    case ZDJ_DECK_2_CONTROL_HOTCUE_4:
        if( (req = zdj_dj_deck_new_platter_request( deck )) ) {
            req->type = ZDJ_DECK_PLATTER_REQUEST_START_MOTOR;
            req->spin_up = false;
            req->spin_down = false;
            if( zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_HOTCUE ) ) {
                deck->command_req.hotcue_num = 4;
            }
        }
        break;
    case ZDJ_DECK_1_CONTROL_HOTCUE_5:
    case ZDJ_DECK_2_CONTROL_HOTCUE_5:
        if( (req = zdj_dj_deck_new_platter_request( deck )) ) {
            req->type = ZDJ_DECK_PLATTER_REQUEST_START_MOTOR;
            req->spin_up = false;
            req->spin_down = false;
            if( zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_HOTCUE ) ) {
                deck->command_req.hotcue_num = 5;
            }
        }
        break;
    case ZDJ_DECK_1_CONTROL_HOTCUE_6:
    case ZDJ_DECK_2_CONTROL_HOTCUE_6:
        if( (req = zdj_dj_deck_new_platter_request( deck )) ) {
            req->type = ZDJ_DECK_PLATTER_REQUEST_START_MOTOR;
            req->spin_up = false;
            req->spin_down = false;
            if( zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_HOTCUE ) ) {
                deck->command_req.hotcue_num = 6;
            }
        }
        break;
    case ZDJ_DECK_1_CONTROL_HOTCUE_7:
    case ZDJ_DECK_2_CONTROL_HOTCUE_7:
        if( (req = zdj_dj_deck_new_platter_request( deck )) ) {
            req->type = ZDJ_DECK_PLATTER_REQUEST_START_MOTOR;
            req->spin_up = false;
            req->spin_down = false;
            if( zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_HOTCUE ) ) {
                deck->command_req.hotcue_num = 7;
            }
        }
        break;


    /////////////////
    // Loop / Skip //
    /////////////////  
        
    case ZDJ_DECK_1_CONTROL_LOOP_TOGGLE:
    case ZDJ_DECK_2_CONTROL_LOOP_TOGGLE:
        zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_TOGGLE_LOOP );
        break;

    case ZDJ_DECK_1_CONTROL_LOOP_START:
    case ZDJ_DECK_2_CONTROL_LOOP_START:
        // printf( "loop start\n" );
        if( zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_MOVE_LOOP ) ) {
            deck->command_req.event_i_val = event->i_val;
        }        
        break;


    case ZDJ_DECK_1_CONTROL_LOOP_START_ALT:
    case ZDJ_DECK_2_CONTROL_LOOP_START_ALT:
        // printf( "loop start alt\n" );
        if( zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_MOVE_LOOP ) ) {
            deck->command_req.event_i_val = event->i_val * 10;
        }
        break;

    case ZDJ_DECK_1_CONTROL_LOOP_LENGTH:
    case ZDJ_DECK_2_CONTROL_LOOP_LENGTH:
        if( zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_RESIZE_LOOP ) ) {
            deck->command_req.event_i_val = event->i_val;
        }        
        break;

    case ZDJ_DECK_1_CONTROL_LOOP_LENGTH_ALT:
    case ZDJ_DECK_2_CONTROL_LOOP_LENGTH_ALT:
        // printf( "loop len alt\n" );
        if( zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_RESIZE_LOOP ) ) {
            deck->command_req.event_i_val = event->i_val * 10;
        }        
        break;

    case ZDJ_DECK_1_CONTROL_LOOP_RESET_TO_START:
    case ZDJ_DECK_2_CONTROL_LOOP_RESET_TO_START:
        // printf( "reset to loop start\n" );
        zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_JUMP_TO_LOOP_START );
        break;

    case ZDJ_DECK_1_CONTROL_SKIP:
    case ZDJ_DECK_2_CONTROL_SKIP:
        if( zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_SKIP ) ) {
            deck->command_req.event_i_val = event->i_val;
        }
        break;

    case ZDJ_DECK_1_CONTROL_SKIP_ALT:
    case ZDJ_DECK_2_CONTROL_SKIP_ALT:
        // printf( "skip\n" );
        if( zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_SKIP ) ) {
            deck->command_req.event_i_val = event->i_val * 10;
        }
        break;

    case ZDJ_DECK_1_CONTROL_SKIP_LENGTH:
    case ZDJ_DECK_2_CONTROL_SKIP_LENGTH:
        printf( "skip len\n" );
        if( zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_CHANGE_SKIP_LENGTH ) ) {
            deck->command_req.event_i_val = event->i_val;
        }
        break;

    case ZDJ_DECK_1_CONTROL_SKIP_SET_ORIGIN:
    case ZDJ_DECK_2_CONTROL_SKIP_SET_ORIGIN:
        printf( "setting skip origin\n" );
        zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_SET_SKIP_ORIGIN );
        break;

    case ZDJ_DECK_1_CONTROL_SKIP_RESET_TO_ORIGIN:
    case ZDJ_DECK_2_CONTROL_SKIP_RESET_TO_ORIGIN:
        zdj_dj_deck_command_request( deck, ZDJ_DECK_COMMAND_REQUEST_JUMP_TO_SKIP_ORIGIN );
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
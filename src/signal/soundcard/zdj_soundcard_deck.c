#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <zerodj/controls/zdj_controls.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/xport/zdj_deck_xport.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/record/zdj_audio_record_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

zdj_error_type_t zdj_soundcard_link_deck( 
    zdj_soundcard_t * soundcard, 
    zdj_deck_t * deck 
) {
    if( !soundcard ) { return ZDJ_ERROR_SYS_ERROR; }
    
    // Link soundcard node's pipeline_nodes to deck's get_data fn. 
    // Used during fast-cycle mix flow to bring deck's audio data pipeline 
    // into the soundcard graph at the specified deck edge node.
    zdj_soundcard_node_t * edge_node;
    zdj_soundcard_node_t * edge_input_node;
    zdj_soundcard_node_t * edge_output_node;
    zdj_soundcard_node_t * deck_input_node;
    zdj_soundcard_node_t * deck_prefade_node;
    zdj_soundcard_node_t * deck_postfade_node;
    zdj_soundcard_node_t * deck_cue_node;
    switch ( deck->station ) {
        case ZDJ_DECK_STATION_1:
            edge_node = zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_EDGE );
            edge_node->get_edge_input_data = deck->get_edge_data;
            edge_node->edge_input_link = deck;
            break;
        case ZDJ_DECK_STATION_2:
            edge_node = zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_EDGE );
            edge_node->get_edge_input_data = deck->get_edge_data;
            edge_node->edge_input_link = deck;
            break;
        case ZDJ_DECK_STATION_EXT:
            edge_input_node = zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT );
            edge_input_node->push_edge_output_data = deck->push_edge_data;
            edge_input_node->edge_output_link = deck;

            edge_output_node = zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_EDGE );
            edge_output_node->get_edge_input_data = deck->get_edge_data;
            edge_output_node->edge_input_link = deck;
            break;
        case ZDJ_DECK_STATION_XPORT:
            edge_input_node = zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_XPORT_0 );
            edge_input_node->push_edge_output_data = deck->push_edge_data;
            edge_input_node->edge_output_link = deck;

            edge_output_node = zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_XPORT_1 );
            edge_output_node->get_edge_input_data = deck->get_edge_data;
            edge_output_node->edge_input_link = deck;
            break;
    }

    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t zdj_soundcard_unlink_deck( 
    zdj_soundcard_t * soundcard, 
    zdj_deck_t * deck 
) {
    if( !soundcard ) { return ZDJ_ERROR_SYS_ERROR; }
    
    // Unlink soundcard node's pipeline_nodes from deck's get_data fn.
    zdj_soundcard_node_t * edge_node;
    switch ( deck->station ) {
        case ZDJ_DECK_STATION_1:
            edge_node = zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_EDGE );
            edge_node->get_edge_input_data = NULL;
            edge_node->edge_input_link = NULL;
            break;
        case ZDJ_DECK_STATION_2:
            edge_node = zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_EDGE );
            edge_node->get_edge_input_data = NULL;
            edge_node->edge_input_link = NULL;
            break;
        case ZDJ_DECK_STATION_EXT:
            edge_node = zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_EDGE );
            edge_node->get_edge_input_data = NULL;
            edge_node->edge_input_link = NULL;
            break;
    }

    return ZDJ_ERROR_OKAY;
}


// case ZDJ_DECK_CONTROL_XFADE:
//         printf( "dj deck xfad\n" );
//         // Set dsp gain for XFade A/B nodes
//         node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_XFADE_A );
//         node->dsp_dto->set_gain( node, event->i_val );
//         node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_XFADE_B );
//         node->dsp_dto->set_gain( node, 255 - event->i_val );
//         event->blocked = true;
//         break;


void zdj_soundcard_handle_deck_event(
    zdj_soundcard_t * soundcard, 
    zdj_control_event_t * event
) {
    // printf( "zdj_soundcard_handle_deck_event %p %d\n", soundcard, event->id );
    if( !soundcard ) { return; }
    if( event->id == ZDJ_DECK_CONTROL_LR_VOL ) {
        // printf( "soundcard lr vol event\n" );
        zdj_soundcard_node_t * node = zdj_soundcard_get_node_for_name( 
            zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS 
        );
        node->dsp_dto->adjust_gain( node, event->i_val );
        // printf( "LR Vol Adjust: %d\n", node->dsp_dto->gain );
    } else if( event->id == ZDJ_DECK_CONTROL_XFADE ) {
        zdj_soundcard_node_t * node_a = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_XFADE_A );
        double a_curve_pow = node_a->dsp_dto->stages[ 0 ].knob_0 * 5.5;
        double a_input_val = (255.0 - (double)event->i_val) / 255.0;
        double a_coeff = 1.0 - pow( a_input_val, a_curve_pow );

        zdj_soundcard_node_t * node_b = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_XFADE_B );
        double b_curve_pow = node_b->dsp_dto->stages[ 0 ].knob_0 * 5.5;
        double b_input_val = (double)event->i_val / 255.0;
        double b_coeff = 1.0 - pow( b_input_val, b_curve_pow );

        // FIXME - This is backwards
        node_a->dsp_dto->set_gain( node_a, round(b_coeff*255.0) );
        node_b->dsp_dto->set_gain( node_b, round(a_coeff*255.0) );

        // printf( "XFade Adjust A:%1.2f/%1.2f/%1.2f B:%1.2f/%1.2f/%1.2f\n", a_curve_pow, a_input_val, a_coeff, b_curve_pow, b_input_val, b_coeff );
    } else if( event->id == ZDJ_DECK_CONTROL_RECORD_VOL ) {
        // printf( "soundcard record vol event\n" );
        zdj_soundcard_node_t * node = zdj_soundcard_get_node_for_name( 
            zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS 
        );
        node->dsp_dto->adjust_gain( node, event->i_val );
        // printf( "Record Vol Adjust: %d\n", node->dsp_dto->gain );
    } else if( event->id == ZDJ_DECK_CONTROL_TOGGLE_RECORD ) {
        // printf( "toggling record\n" );
        // Get soundcard record node
        zdj_audio_record_node_state_t * recording_node_state = (zdj_audio_record_node_state_t*)zdj_soundcard->recording_node->state;
        
        if( recording_node_state->status == ZDJ_AUDIO_RECORD_ACTIVE ) {
            // Stop recording if currently running
            zdj_finish_audio_record_capture( zdj_soundcard->recording_node, true );
        } else {
            // Start a recording if not running
            zdj_new_audio_record_capture( zdj_soundcard->recording_node );
        }
    } else if( event->id == ZDJ_DECK_1_2_BASS_SWAP ) {
        // printf( "dj deck bass swap\n" );
        zdj_soundcard_node_t * soundcard_node;
        zdj_soundcard_dsp_dto_t * dsp_dto;
        zdj_soundcard_dsp_stage_dto_t * dsp_stage;
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
    }
}


void zdj_soundcard_init_clock_deck( zdj_soundcard_t * soundcard ) {
    zdj_deck_t * deck = soundcard->clock_deck;
    zdj_xport_deck_state_t * deck_state = (zdj_xport_deck_state_t*)deck->state;
    // deck_state->ppqn = soundcard->dto.clock_1_val;
    // state->set_bpm = zdj_soundcard_get_set_bpm_for_clock( &zdj_soundcard->dto );
    // state->sync_mode = zdj_soundcard_get_sync_mode_for_clock( &zdj_soundcard->dto );
    // state->direction = zdj_soundcard_get_direction_for_clock( &zdj_soundcard->dto );
    // state->ppqn = zdj_soundcard_get_ppqn_for_clock( &zdj_soundcard->dto );

    switch( zdj_soundcard_dto_get_sigtype_for_node_name( 
                &zdj_soundcard->dto, ZDJ_SOUNDCARD_NODE_NAME_XPORT_0 
            ) 
    ) {
        case ZDJ_SOUNDCARD_SIGNAL_XPORT_ANALOG_PPQN_1: deck_state->ppqn = 1; break;
        case ZDJ_SOUNDCARD_SIGNAL_XPORT_ANALOG_PPQN_2: deck_state->ppqn = 2; break;
        case ZDJ_SOUNDCARD_SIGNAL_XPORT_ANALOG_PPQN_4: deck_state->ppqn = 4; break;
        case ZDJ_SOUNDCARD_SIGNAL_XPORT_ANALOG_PPQN_24: deck_state->ppqn = 24; break;
        case ZDJ_SOUNDCARD_SIGNAL_XPORT_ANALOG_PPQN_96: deck_state->ppqn = 96; break;
        default: deck_state->ppqn = 4; break;
    }

    deck_state->set_bpm = zdj_soundcard_dto_get_val_for_node_name( 
        &zdj_soundcard->dto, ZDJ_SOUNDCARD_NODE_NAME_XPORT_0 
    );

    switch( zdj_soundcard_dto_get_sync_for_node_name( 
                &zdj_soundcard->dto, ZDJ_SOUNDCARD_NODE_NAME_XPORT_0 
            ) 
    ) {
        case ZDJ_SOUNDCARD_CLOCK_SYNC_NORMAL: 
            deck_state->sync_mode = ZDJ_XPORT_DECK_SYNC_MODE_NORMAL; break;
        case ZDJ_SOUNDCARD_CLOCK_SYNC_HALF: 
            deck_state->sync_mode = ZDJ_XPORT_DECK_SYNC_MODE_HALF; break;
        case ZDJ_SOUNDCARD_CLOCK_SYNC_DOUBLE: 
            deck_state->sync_mode = ZDJ_XPORT_DECK_SYNC_MODE_DOUBLE; break;
        case ZDJ_SOUNDCARD_CLOCK_SYNC_DECOUPLE: 
            deck_state->sync_mode = ZDJ_XPORT_DECK_SYNC_MODE_OFF; break;
    }

    switch( zdj_soundcard_dto_get_source_for_node_name( 
                &zdj_soundcard->dto, ZDJ_SOUNDCARD_NODE_NAME_XPORT_0 
            ) 
    ) {
        case ZDJ_SOUNDCARD_CLOCK_DIRECTION_INPUT: 
            deck_state->direction = ZDJ_XPORT_DECK_DIRECTION_INPUT; break;
        case ZDJ_SOUNDCARD_CLOCK_DIRECTION_OUTPUT: 
            deck_state->direction = ZDJ_XPORT_DECK_DIRECTION_OUTPUT; break;
    }
}
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <zerodj/controls/zdj_controls.h>
#include <zerodj/signal/deck/zdj_deck.h>
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

void zdj_soundcard_handle_deck_event(
    zdj_soundcard_t * soundcard, 
    zdj_control_event_t * event
) {
    // printf( "zdj_soundcard_handle_deck_event %d\n", event->id );
    if( event->id == ZDJ_DECK_CONTROL_TOGGLE_RECORD ) {
        // Get soundcard record node
        zdj_audio_record_node_state_t * recording_node_state = (zdj_audio_record_node_state_t*)zdj_soundcard->recording_node->state;
        
        if( recording_node_state->status == ZDJ_AUDIO_RECORD_ACTIVE ) {
            // Stop recording if currently running
            // zdj_disable_audio_record_capture( zdj_soundcard->recording_node );
            zdj_finish_audio_record_capture( zdj_soundcard->recording_node, true );
        } else {
            // Start a recording if not running
            zdj_new_audio_record_capture( zdj_soundcard->recording_node );
            // zdj_enable_audio_record_capture( zdj_soundcard->recording_node );
        }
    }
}
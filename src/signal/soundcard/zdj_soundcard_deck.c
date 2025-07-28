#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

zdj_error_type_t zdj_soundcard_link_deck( 
    zdj_soundcard_t * soundcard, 
    zdj_deck_t * deck 
) {
    if( !soundcard ) { return ZDJ_ERROR_SYS_ERROR; }
    
    // Link soundcard node's pipeline_nodes to deck's get_data fn. 
    // Used during fast-cycle mix flow to bring deck's audio data pipeline 
    // into the soundcard graph at the specified deck input node.
    zdj_soundcard_node_t * input_node;
    switch ( deck->num ) {
        case ZDJ_DECK_NUM_1:
            input_node = zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_INPUT );
            input_node->get_edge_data = deck->get_edge_data;
            break;
        case ZDJ_DECK_NUM_2:
            input_node = zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_INPUT );
            input_node->get_edge_data = deck->get_edge_data;
            break;
        case ZDJ_DECK_NUM_EXT:
            input_node = zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT );
            input_node->get_edge_data = deck->get_edge_data;
            break;
    }

    return ZDJ_ERROR_OKAY;
}
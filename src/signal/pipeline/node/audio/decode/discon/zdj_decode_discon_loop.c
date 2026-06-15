#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>

void zdj_decode_discon_install_loop( 
    zdj_pipeline_node_t * node, 
    zdj_decode_discon_request_t * req 
) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    // Use transport coords to grab the current layer under head to ensure uniqueness.
    zdj_decode_layer_t * layer_under_head = state->get_layer_containing_core_addr( 
        node, &state->head, ZDJ_ADDR_COORD_TRANSPORT
    );
    
    if( !layer_under_head ) { printf( "install_loop missing layer!\n" ); return; }

    state->remove_all_layers_except( node, layer_under_head );
    // Truncate current layer to start/end based on above.
    layer_under_head->truncate_to_loop( 
        layer_under_head, node, req->loop_start_origin_d, req->loop_end_origin_d 
    );
    // Ensure proper node refresh behavior
    state->refresh_mode = ZDJ_DECODE_REFRESH_DISCON_LOOP;
}

void zdj_decode_discon_edit_loop( 
    zdj_pipeline_node_t * node, 
    zdj_decode_discon_request_t * req 
) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    // Use transport coords to grab the current layer under head to ensure uniqueness.
    zdj_decode_layer_t * layer_under_head = state->get_layer_containing_core_addr( 
        node, &state->head, ZDJ_ADDR_COORD_TRANSPORT
    );

    if( !layer_under_head ) { printf( "edit_loop missing layer!\n" ); return; }

    state->remove_all_layers_except( node, layer_under_head );
    // Truncate current layer to start/end based on above.
    layer_under_head->retruncate_loop( 
        layer_under_head, node, req->loop_start_origin_d, req->loop_end_origin_d 
    );
}

void zdj_decode_discon_remove_loop( zdj_pipeline_node_t * node ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    zdj_decode_layer_t * layer_under_head = state->get_layer_containing_core_addr( 
        node, &state->head, ZDJ_ADDR_COORD_TRANSPORT 
    );

    if( !layer_under_head ) { printf( "remove_loop missing layer!\n" ); return; }

    state->remove_all_layers_except( node, layer_under_head );
    layer_under_head->untruncate( layer_under_head, node );

    state->refresh_mode = ZDJ_DECODE_REFRESH_CONTIGUOUS;
}
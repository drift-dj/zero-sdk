#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>

static void _request_discon( zdj_pipeline_node_t * node, zdj_decode_discon_request_t * req );
static void _remove_discon( zdj_pipeline_node_t * node );

void zdj_decode_init_node_discon_api( zdj_pipeline_node_t * node ) { 
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    state->request_discon = &_request_discon;
    state->remove_discon = &_remove_discon;
}

static void _request_discon( 
    zdj_pipeline_node_t * node,
    zdj_decode_discon_request_t * req
) {
    switch ( req->type ) {
        case ZDJ_DECODE_DISCON_REQUEST_NEW_LOOP:
            zdj_decode_discon_install_loop( node, req );
            break;
        case ZDJ_DECODE_DISCON_REQUEST_EDIT_LOOP:
            zdj_decode_discon_edit_loop( node, req );
            break;
        case ZDJ_DECODE_DISCON_REQUEST_SKIP_PLAY_TO_PLAY:
            zdj_decode_discon_install_skip( node, req );
            break;
        case ZDJ_DECODE_DISCON_REQUEST_SKIP_PLAY_TO_RESET:
            zdj_decode_discon_install_skip_to_reset( node, req );
            break;
        case ZDJ_DECODE_DISCON_REQUEST_SKIP_RESET_TO_PLAY:
            zdj_decode_discon_reset_and_play( node, req );
            break;
        case ZDJ_DECODE_DISCON_REQUEST_RESET:
            zdj_decode_discon_reset( node, req );
            break;
    }
}

static void _remove_discon( zdj_pipeline_node_t * node ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    // Use transport coords to grab the current layer under head to ensure uniqueness.
    zdj_decode_layer_t * layer_under_head = state->get_layer_containing_core_addr( 
        node, &state->head, ZDJ_ADDR_COORD_TRANSPORT
    );

    if( !layer_under_head ) { printf( "remove_discon missing layer!\n" ); return; }

    state->remove_all_layers_except( node, layer_under_head );
    layer_under_head->untruncate( layer_under_head, node );
    // Ensure proper node refresh behavior
    state->refresh_mode = ZDJ_DECODE_REFRESH_CONTIGUOUS;
}
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>

// Reminder, skip ONLY happens when platter is running
void zdj_decode_discon_install_skip( 
    zdj_pipeline_node_t * node, 
    zdj_decode_discon_request_t * req 
) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    // printf( "zdj_decode_discon_install_skip dep:%1.0f dest:%1.0f\n", req->skip_depart_origin_d, req->skip_dest_origin_d );

    // Use transport coords to grab the current layer under head to ensure uniqueness.
    zdj_decode_layer_t * layer_under_head;
    // Do some sanity-checking on LUH.
    if( state->head.origin_d > 0.1 ) {
        layer_under_head = state->get_layer_containing_core_addr( 
            node, &state->head, ZDJ_ADDR_COORD_TRANSPORT
        );
    } else if( state->head.origin_d > state->song_pcm_duration ) {
        layer_under_head = state->last_layer;
    } else if( state->head.origin_d < 1.0 ) {
        layer_under_head = state->first_layer;
    }

    if( !layer_under_head ) { layer_under_head = state->last_layer; }

    if( !layer_under_head ) { 
        zdj_decode_addr_t win_start; state->get_win_start_addr( node, &win_start );
        zdj_decode_addr_t win_end; state->get_win_end_addr( node, &win_end );
        printf( "install_skip missing layer!\n" ); 
        // bool node_empty = false;
        // if( !state->first_layer && !state->last_layer ) { node_empty = true; }
        // printf( "empt:%d fls:%1.0f | %1.0f[ t:%1.0f o:%1.0f ]%1.0f | lle:%1.0f dpd:%1.0f\n", 
        //     node_empty,
        //     state->first_layer->lead_in_start.transport_d,
        //     win_start.transport_d,
        //     state->head.transport_d,
        //     state->head.origin_d,
        //     win_end.transport_d,
        //     state->last_layer->lead_out_end.transport_d,
        //     req->skip_depart_origin_d
        // ); 
        return; 
    }

    state->remove_all_layers_except( node, layer_under_head );
    // Truncate current layer to start/end based on above.
    layer_under_head->truncate_to_skip( 
        layer_under_head, node, req->skip_depart_origin_d, req->skip_dest_origin_d 
    );

    // We may be skipping inside a loop or on a contiguous layer
    if( req->enable_loop ) {
        state->refresh_mode = ZDJ_DECODE_REFRESH_DISCON_LOOP;
    } else {
        state->refresh_mode = ZDJ_DECODE_REFRESH_DISCON_SKIP;
    }
}

void zdj_decode_discon_install_skip_to_reset( 
    zdj_pipeline_node_t * node, 
    zdj_decode_discon_request_t * req 
) {

}

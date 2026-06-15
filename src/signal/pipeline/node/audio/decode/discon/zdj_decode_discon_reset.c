#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>

///////////////////////////////////////////////////////////////////////////
// WARNING: If platter is moving, reset funcs will cause an audible pop. //
// These immediately overwrite the decode node's layers, there's no      //
// fade across the discontinuity like you get with the skip funcs.       //
///////////////////////////////////////////////////////////////////////////

void zdj_decode_discon_reset_and_play( 
    zdj_pipeline_node_t * node, 
    zdj_decode_discon_request_t * req 
) {

}

void zdj_decode_discon_reset( 
    zdj_pipeline_node_t * node, zdj_decode_discon_request_t * req 
) {

}

// void zdj_decode_discon_install_reset( 
//     zdj_pipeline_node_t * node, 
//     zdj_decode_discon_request_t * req 
// ) {
//     zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

//     // Clear all layers + hard-set the decode head
//     node->reset_window( node, req->dest_bg );

//     // Set up discon types depending on subtype
//     if( req->subtype == ZDJ_DECODE_DISCON_SUBTYPE_TO_CONTIGUOUS ) {
//         state->refresh_mode = ZDJ_DECODE_REFRESH_CONTIGUOUS;
//     } else if( req->subtype == ZDJ_DECODE_DISCON_SUBTYPE_TO_LOOP ) {
//         state->refresh_mode = ZDJ_DECODE_REFRESH_DISCON;
//         state->loop_start_origin_d = req->start_origin_d;
//         state->loop_pcm_len = req->end_origin_d - req->start_origin_d;
//         state->loop_fade_len = 300;
//     }


//     // Hard-set the decode head.
//     // if( decode_state->first_layer ) {
//     //     // If there's a layer present, use the current layers' transport coords to move the head
//     //     if( req_offset > 0 ) {
//     //         decode_state->addr_for_origin_d_coord_in_layer( decode_node, decode_state->first_layer, &decode_state->head, loop_start_origin_d + 1 );
//     //     } else {
//     //         decode_state->addr_for_origin_d_coord_in_layer( decode_node, decode_state->last_layer, &decode_state->head, loop_end_origin_d - 1000 );
//     //     }
//     // } else {
//     //     // If there's no layer present, create a new one based on loop state
//     //     zdj_decode_addr_t * loop_start_addr = calloc( 1, sizeof( zdj_decode_addr_t ) );
//     //     zdj_decode_init_addr( loop_start_addr );

//     //     // TODO: Do we actually need to use an entire ADDR here?


//     //     // if( req_offset > 0 ) {
//     //     //     decode_state->head.origin_d = loop_start_origin_d + 1;
//     //     //     decode_state->head.origin_i = (int64_t)decode_state->head.origin_d;
//     //     // } else {
//     //     //     decode_state->head.origin_d = loop_end_origin_d - 1000;
//     //     //     decode_state->head.origin_i = (int64_t)decode_state->head.origin_d;
//     //     // }
//     //     // decode_state->head.transport_d = 0.0;
//     //     // decode_state->head.transport_i = 0;
//     //     // decode_state->head.has_valid_origin = true;

//     //     // loop_start_addr->origin_d = loop_start_origin_d;
//     //     // loop_start_addr->origin_i = (int64_t)loop_start_addr->origin_d;
//     //     // loop_start_addr->transport_d = decode_state->head.transport_d - loop_start_origin_d;
//     //     // loop_start_addr->transport_i = (int64_t)loop_start_addr->transport_d;


        

//     //     // zdj_decode_layer_t * new_layer = zdj_new_decode_loop_layer( decode_node, loop_start_addr, &deck->controls.loop_state );
//     //     // decode_state->append_layer( decode_node, new_layer );

//     //     free( loop_start_addr );
//     // }
// }
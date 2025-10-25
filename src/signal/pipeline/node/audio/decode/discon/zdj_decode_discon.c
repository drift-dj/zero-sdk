#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>

static bool _discon_is_active( zdj_pipeline_node_t * node );
static void _add_loop_discon( zdj_pipeline_node_t * node, void * _controls );
static void _release_loop_discon( zdj_pipeline_node_t * node, void * _controls );
static void _move_loop_discon( zdj_pipeline_node_t * node, void * _controls );
static void _resize_loop_discon( zdj_pipeline_node_t * node, void * _controls );
static void _add_skip_discon( zdj_pipeline_node_t * node, void * _controls );

void zdj_decode_init_node_discon_api( zdj_pipeline_node_t * node ) { 
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    state->discon_is_active = &_discon_is_active;
    state->add_loop_discon = &_add_loop_discon;
    state->release_loop_discon = &_release_loop_discon;
    state->move_loop_discon = &_move_loop_discon;
    state->resize_loop_discon = &_resize_loop_discon;
    state->add_skip_discon = &_add_skip_discon;
}

static bool _discon_is_active( zdj_pipeline_node_t * node ) {
    return false;
}

static void _add_loop_discon( zdj_pipeline_node_t * node, void * _controls ) {
    printf( "_add_loop_discon\n" );
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)node->state;
    zdj_deck_control_state_t * controls = (zdj_deck_control_state_t*)_controls;

    // // Find Layer/Packet
    // zdj_decode_layer_t * layer_under_head = zdj_decode_get_layer_under_head( node );
    // if( !layer_under_head ) { return; } // Bug out if layers aren't ready yet
    // zdj_decode_packet_t * packet_under_head = zdj_decode_get_packet_under_head( node, layer_under_head );
    // if( !packet_under_head ){ return; } // Bug out if layers aren't ready yet

    // // Calculate loop_state addresses
    // int64_t depart_decode_addr = (controls->loop_state.quantize) ?
    //     controls->platter.needle.head : // <- quantize to beat grid
    //     controls->platter.needle.head;
    // // loop_state->start_pcm_addr = zdj_decode_get_pcm_addr_for_decode_addr( decode_node, depart_decode_addr );
    // controls->loop_state.start_pcm_addr = decode_state->pcm_addr_for_decode_addr( decode_state, depart_decode_addr );
    // controls->loop_state.end_pcm_addr = controls->loop_state.start_pcm_addr + controls->loop_state.pcm_len;    
    // controls->loop_state.fade_len = 300;

    // // printf( "loop_state - dep_dcd: %ld, sp:%ld -> ep:%ld (n%1.3f, dhd%ld)\n", 
    // //     depart_decode_addr,
    // //     loop_state->start_pcm_addr, loop_state->end_pcm_addr,
    // //     platter->needle.head, decode_state->head_decode_addr
    // // );

    // // Truncate layer under head to start of new discon addr
    // zdj_decode_truncate_layer( layer_under_head, depart_decode_addr, ZDJ_DECODE_DISCON_LOOP );

    // // Calculate loop layer init addresses
    // int64_t layer_start_pcm_addr = packet_under_head->packet_pcm_addr;
    // int64_t layer_start_decode_addr = packet_under_head->packet_decode_addr;
    // int64_t loop_start_pcm_addr = controls->loop_state.start_pcm_addr;
    // int64_t loop_start_decode_addr = depart_decode_addr;
    // int64_t loop_len = controls->loop_state.pcm_len;

    // // printf( "loop enable 0\n" );
    // // Create first new loop layer
    // zdj_decode_layer_t * loop_layer = zdj_decode_add_loop_layer( 
    //     node, 
    //     layer_start_decode_addr, 
    //     layer_start_pcm_addr,
    //     loop_start_decode_addr,
    //     loop_start_pcm_addr,
    //     loop_len
    // );
    // loop_layer->first_packet->is_back_extent = true;

    // // Remove current non-loop layer
    // zdj_decode_layer_t * top_layer = decode_state->first_layer;
    // zdj_decode_deinit_layer( top_layer );

    // decode_state->first_layer = loop_layer;
    // decode_state->last_layer = loop_layer;

    // // Zero move to fill the window forward
    // node->move_window( node, 0 );

    printf( "_add_loop_discon done\n" );
}

static void _release_loop_discon( zdj_pipeline_node_t * node, void * _controls ) {
    printf( "_release_loop_discon\n" );
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)node->state;
    zdj_deck_control_state_t * controls = (zdj_deck_control_state_t*)_controls;

    // // Reset layer under head's discons/linkage
    // zdj_decode_layer_t * layer_under_head = zdj_decode_get_layer_under_head( node );
    // layer_under_head->fwd_discon.type = ZDJ_DECODE_DISCON_INERT;
    // layer_under_head->back_discon.type = ZDJ_DECODE_DISCON_INERT;
    // layer_under_head->next = NULL;
    // layer_under_head->prev = NULL;
    // // Remove all other layers.
    // zdj_decode_layer_t * layer = decode_state->first_layer;
    // while( layer ) {
    //     zdj_decode_layer_t * next_layer = layer->next;
    //     if( layer != layer_under_head ){ zdj_decode_deinit_layer( layer ); }
    //     layer = next_layer;
    // }
    // decode_state->first_layer = layer_under_head;
    // decode_state->last_layer = layer_under_head;

    // // Reset packet under head's core start/end + linkage
    // zdj_decode_packet_t * packet_under_head = zdj_decode_get_packet_under_head( node, layer_under_head );
    // packet_under_head->core_start_addr = packet_under_head->packet_decode_addr;
    // packet_under_head->lead_in_start_addr = packet_under_head->core_start_addr;
    // packet_under_head->core_end_addr = packet_under_head->packet_decode_addr + packet_under_head->av_frame_sample_count;
    // packet_under_head->lead_out_end_addr = packet_under_head->core_end_addr;
    // packet_under_head->core_sample_count = packet_under_head->av_frame_sample_count;
    // packet_under_head->is_fwd_extent = false;
    // packet_under_head->is_back_extent = false;
    // packet_under_head->next = NULL;
    // packet_under_head->prev = NULL;

    // // Remove all other packets
    // zdj_decode_packet_t * packet = layer_under_head->first_packet;
    // while( packet ) {
    //     zdj_decode_packet_t * next_packet = packet->next;
    //     if( packet != packet_under_head ){ zdj_decode_deinit_packet( layer_under_head, packet ); }
    //     packet = next_packet;
    // }
    // layer_under_head->first_packet = packet_under_head;
    // layer_under_head->last_packet = packet_under_head;
    // layer_under_head->earliest_core_sample = packet_under_head->core_start_addr;
    // layer_under_head->latest_core_sample = packet_under_head->core_end_addr;

    // decode_state->earliest_core_sample = layer_under_head->earliest_core_sample;
    // decode_state->latest_core_sample = layer_under_head->latest_core_sample;

    // // Fill in the layer
    // zdj_decode_fill_layer( node, layer_under_head );
}

static void _move_loop_discon( zdj_pipeline_node_t * node, void * _controls ) {

}

static void _resize_loop_discon( zdj_pipeline_node_t * node, void * _controls ) {

}

static void _add_skip_discon( zdj_pipeline_node_t * node, void * _controls ) {
    
}
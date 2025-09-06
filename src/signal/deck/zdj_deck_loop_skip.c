#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <math.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/lib/zdj_deck_lib.h>
// #include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
// #include <zerodj/signal/soundcard/zdj_soundcard.h>

void zdj_deck_new_loop( zdj_deck_t * deck, int64_t len, bool quant ) {
    printf( "zdj_deck_new_loop\n" );
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;
    zdj_deck_platter_t * platter = &deck->controls.platter;
    zdj_pipeline_node_t * decode_node;
    loop_state->is_enabled = true;

    if( deck->type == ZDJ_DECK_TYPE_LIB ) {
        zdj_lib_deck_state_t * deck_state = (zdj_lib_deck_state_t*)deck->state;
        decode_node = deck_state->decode_node;
    // } else if( deck->type == ZDJ_DECK_TYPE_DJ ) {
    //     zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    //     decode_node = deck_state->decode_node;
    } else {
        return; // Should never get here.
    }

    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)decode_node->state;

    // Find Layer/Packet
    zdj_decode_layer_t * layer_under_head = zdj_decode_get_layer_under_head( decode_node );
    if( !layer_under_head ) { return; } // Bug out if layers aren't ready yet
    zdj_decode_packet_t * packet_under_head = zdj_decode_get_packet_under_head( decode_node, layer_under_head );
    if( !packet_under_head ){ return; } // Bug out if layers aren't ready yet

    // Calculate loop_state addresses
    int64_t depart_decode_addr = (quant) ?
        platter->needle.head : // <- quantize to beat grid
        platter->needle.head;
    loop_state->start_pcm_addr = zdj_decode_get_pcm_addr_for_decode_addr( decode_node, depart_decode_addr );
    loop_state->end_pcm_addr = loop_state->start_pcm_addr + len;    
    loop_state->fade_len = 300;

    printf( "loop_state - dep_dcd: %ld, sp:%ld -> ep:%ld (n%1.3f, dhd%ld)\n", 
        depart_decode_addr,
        loop_state->start_pcm_addr, loop_state->end_pcm_addr,
        platter->needle.head, decode_state->head_decode_addr
    );

    // Truncate layer under head to start of new discon addr
    zdj_decode_truncate_layer( layer_under_head, depart_decode_addr, ZDJ_DECODE_DISCON_LOOP );

    // Calculate loop layer init addresses
    int64_t layer_start_pcm_addr = packet_under_head->packet_pcm_addr;
    int64_t layer_start_decode_addr = packet_under_head->packet_decode_addr;
    int64_t loop_start_pcm_addr = loop_state->start_pcm_addr;
    int64_t loop_start_decode_addr = depart_decode_addr;
    int64_t loop_len = len;

    // Create first new loop layer
    zdj_decode_layer_t * loop_layer = zdj_decode_add_loop_layer( 
        decode_node, 
        layer_start_decode_addr, 
        layer_start_pcm_addr,
        loop_start_decode_addr,
        loop_start_pcm_addr,
        loop_len
    );

    // Zero move to fill the window forward
    decode_node->move_window( decode_node, 0 );
}

void zdj_deck_enable_loop( zdj_deck_t * deck, zdj_library_cuepoint_t * cuepoint ) {
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;
    // Add a skip to jump to start of loop
    loop_state->start_pcm_addr = cuepoint->sample;
    loop_state->end_pcm_addr = loop_state->start_pcm_addr + cuepoint->loop_len;
    loop_state->fade_len = 300;

    loop_state->is_enabled = true;

    // If loop is inside window, clear layer stack and fill with new stuff.

    // Else, edit layer under head to point to new discon state
}

void zdj_deck_disable_loop( zdj_deck_t * deck ) {
    printf( "disable loop\n" );
    zdj_lib_deck_state_t * deck_state = (zdj_lib_deck_state_t*)deck->state;
    zdj_pipeline_node_t * decode_node;

    deck->controls.loop_state.is_enabled = false;

    if( deck->type == ZDJ_DECK_TYPE_LIB ) {
        zdj_lib_deck_state_t * deck_state = (zdj_lib_deck_state_t*)deck->state;
        decode_node = deck_state->decode_node;
    // } else if( deck->type == ZDJ_DECK_TYPE_DJ ) {
    //     zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    //     decode_node = deck_state->decode_node;
    } else {
        return; // Should never get here.
    }
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)decode_node->state;

    // Reset layer under head's discons/linkage
    zdj_decode_layer_t * layer_under_head = zdj_decode_get_layer_under_head( decode_node );
    layer_under_head->fwd_discon.type = ZDJ_DECODE_DISCON_INERT;
    layer_under_head->back_discon.type = ZDJ_DECODE_DISCON_INERT;
    layer_under_head->next = NULL;
    layer_under_head->prev = NULL;
    // Remove all other layers.
    zdj_decode_layer_t * layer = decode_state->first_layer;
    while( layer ) {
        zdj_decode_layer_t * next_layer = layer->next;
        if( layer != layer_under_head ){ zdj_decode_deinit_layer( layer ); }
        layer = next_layer;
    }
    decode_state->first_layer = layer_under_head;
    decode_state->last_layer = layer_under_head;

    // Reset packet under head's core start/end + linkage
    zdj_decode_packet_t * packet_under_head = zdj_decode_get_packet_under_head( decode_node, layer_under_head );
    packet_under_head->core_start_addr = packet_under_head->packet_decode_addr;
    packet_under_head->lead_in_start_addr = packet_under_head->core_start_addr;
    packet_under_head->core_end_addr = packet_under_head->packet_decode_addr + packet_under_head->av_frame_sample_count;
    packet_under_head->lead_out_end_addr = packet_under_head->core_end_addr;
    packet_under_head->core_sample_count = packet_under_head->av_frame_sample_count;
    packet_under_head->is_fwd_extent = false;
    packet_under_head->is_back_extent = false;
    packet_under_head->next = NULL;
    packet_under_head->prev = NULL;

    // Remove all other packets
    zdj_decode_packet_t * packet = layer_under_head->first_packet;
    while( packet ) {
        zdj_decode_packet_t * next_packet = packet->next;
        if( packet != packet_under_head ){ zdj_decode_deinit_packet( packet ); }
        packet = next_packet;
    }
    layer_under_head->first_packet = packet_under_head;
    layer_under_head->last_packet = packet_under_head;
    layer_under_head->earliest_core_sample = packet_under_head->core_start_addr;
    layer_under_head->latest_core_sample = packet_under_head->core_end_addr;
    
    decode_state->earliest_core_sample = layer_under_head->earliest_core_sample;
    decode_state->latest_core_sample = layer_under_head->latest_core_sample;

    // Fill in the layer
    zdj_decode_fill_layer( decode_node, layer_under_head );
}

void zdj_deck_move_loop( zdj_deck_t * deck, int64_t distance, bool quant ) {
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;
    zdj_lib_deck_state_t * deck_state = (zdj_lib_deck_state_t*)deck->state;

    zdj_pipeline_node_t * decode_node;
    zdj_decode_node_state_t * decode_state;
    if( deck->type == ZDJ_DECK_TYPE_LIB ) {
        decode_node = deck_state->decode_node;
        decode_state = (zdj_decode_node_state_t*)decode_node->state;
    // } else if( deck->type == ZDJ_DECK_TYPE_DJ ) {
    //     zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    //     decode_node = deck_state->decode_node;
    } else {
        return; // Should never get here.
    }
    
    // ???
}

void zdj_deck_resize_loop( zdj_deck_t * deck, int64_t offset, bool quant ) {
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;
    
    if( quant ) {
        // Quantize move to nearest beat grid
        loop_state->end_pcm_addr += offset;
    } else {
        loop_state->end_pcm_addr += offset;
    }

    // Clear all layers not under head

    // Reset layer under head's discon state to new loop dimensions

    // Build new layers around layer under head
}

void zdj_deck_new_skip( zdj_deck_t * deck, int64_t distance, bool quant ) {
    zdj_deck_control_skip_state_t * skip_state = &deck->controls.skip_state;
    zdj_deck_platter_t * platter = &deck->controls.platter;

    zdj_pipeline_node_t * decode_node;
    if( deck->type == ZDJ_DECK_TYPE_LIB ) {
        zdj_lib_deck_state_t * deck_state = (zdj_lib_deck_state_t*)deck->state;
        decode_node = deck_state->decode_node;
    // } else if( deck->type == ZDJ_DECK_TYPE_DJ ) {
    //     zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    //     decode_node = deck_state->decode_node;
    } else {
        return; // Should never get here.
    }

    // Figure out when we'll enter the loop
    int64_t depart_decode_addr = (quant) ?
        platter->needle.head : // <- quantize to beat grid
        platter->needle.head;

    skip_state->depart_pcm_addr = zdj_decode_get_pcm_addr_for_decode_addr( decode_node, depart_decode_addr );
    skip_state->arrive_pcm_addr = skip_state->depart_pcm_addr + distance;    
    skip_state->fade_len = 300;

    zdj_decode_layer_t * layer = zdj_decode_add_skip_layer( 
        decode_node, 
        depart_decode_addr, 
        skip_state->depart_pcm_addr, 
        skip_state->arrive_pcm_addr 
    );
}
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>

#include <libavformat/avformat.h>
#include <libavcodec/packet.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavcodec/codec_id.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>

static bool _layer_is_empty( zdj_decode_layer_t * layer );
static bool _layer_can_add_packet_before( 
    zdj_pipeline_node_t * node, zdj_decode_layer_t * layer, int64_t address 
);
static bool _layer_can_add_packet_after( 
    zdj_pipeline_node_t * node, zdj_decode_layer_t * layer, int64_t address 
);


zdj_decode_layer_t * zdj_new_decode_layer( int64_t pcm_address, int64_t decode_address ) {
    zdj_decode_layer_t * layer = calloc( 1, sizeof( zdj_decode_layer_t ) );
    layer->prev = NULL;
    layer->next = NULL;
    layer->init_map_pcm = pcm_address;
    layer->init_map_decode = decode_address;
    layer->fwd_discon.type = ZDJ_DECODE_DISCON_NONE;
    layer->back_discon.type = ZDJ_DECODE_DISCON_NONE;
    return layer;
}

zdj_decode_layer_t * zdj_decode_add_loop_layer( 
    zdj_pipeline_node_t * node, 
    int64_t layer_start_decode_addr, 
    int64_t layer_start_pcm_addr, 
    int64_t loop_start_decode_addr, 
    int64_t loop_start_pcm_addr, 
    int64_t loop_len
) {
    printf( "zdj_decode_add_loop_layer %ld/%ld %ld/%ld %ld\n", 
        layer_start_decode_addr,
        layer_start_pcm_addr,
        loop_start_decode_addr,
        loop_start_pcm_addr,
        loop_len
    );
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    zdj_decode_layer_t * layer = zdj_new_decode_layer( layer_start_pcm_addr, layer_start_decode_addr );
    
    // Set up discon props
    layer->fwd_discon.type = ZDJ_DECODE_DISCON_LOOP;
    layer->fwd_discon.depart_decode_addr = loop_start_decode_addr+loop_len;
    layer->fwd_discon.depart_pcm_addr = loop_start_pcm_addr + loop_len;
    layer->fwd_discon.dest_pcm_addr = loop_start_pcm_addr;

    layer->back_discon.type = ZDJ_DECODE_DISCON_LOOP;
    layer->back_discon.depart_decode_addr = loop_start_decode_addr;
    layer->back_discon.depart_pcm_addr = loop_start_pcm_addr;
    layer->back_discon.dest_pcm_addr = loop_start_pcm_addr + loop_len;

    // Link layer into stack
    if( state->last_layer && 
        ( layer_start_decode_addr >= state->last_layer->init_map_decode ) 
    ) {
        printf( "adding loop layer to end\n" );
        layer->prev = state->last_layer;
        state->last_layer->next = layer;
        state->last_layer = layer;
        if( !state->first_layer ) { state->first_layer = layer; }
    } else {
        printf( "adding loop layer to beginning\n" );
        layer->next = state->first_layer;
        state->first_layer->prev = layer;
        state->first_layer = layer;
        if( !state->last_layer ) { state->last_layer = layer; }
    }

    zdj_decode_fill_layer( node, layer );

    return layer;
}

zdj_decode_layer_t * zdj_decode_add_skip_layer( 
    zdj_pipeline_node_t * node, 
    int64_t start_decode_addr, 
    int64_t depart_pcm_addr, 
    int64_t dest_pcm_addr 
) {
    zdj_decode_layer_t * layer = zdj_new_decode_layer( depart_pcm_addr, start_decode_addr );
    // Set up discon props
    // Link layer into stack
    zdj_decode_fill_layer( node, layer );

    return layer;
}

int64_t zdj_decode_discon_loop_length( zdj_decode_layer_discon_t * discon ) {
    return labs( discon->dest_pcm_addr - discon->depart_pcm_addr );
}

int64_t zdj_decode_discon_skip_length( zdj_decode_layer_discon_t * discon ) {
    return 0;
}

// Reset a layer's discontinuity state such that it cover an entire song.
void zdj_decode_layer_reset_discon( zdj_decode_layer_t * layer, int64_t end_pcm_addr ) {
    layer->back_discon.type = ZDJ_DECODE_DISCON_INERT;
    layer->back_discon.depart_pcm_addr = 0;

    layer->fwd_discon.type = ZDJ_DECODE_DISCON_INERT;
    layer->fwd_discon.depart_pcm_addr = end_pcm_addr;
}

zdj_decode_layer_t * zdj_decode_get_layer_under_head( zdj_pipeline_node_t * node ) {
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;
   
    // printf( "---layer check---\n" );
    // Loop thru layers, looking for one with core samples covering the decode head
    zdj_decode_layer_t * layer = node_state->first_layer;
    zdj_decode_layer_t * found_layer = NULL;
    while( layer ) {
        // printf( "[%ld - %ld - %ld]\n", 
        //     layer->earliest_core_sample,
        //     node_state->head_decode_addr,
        //     layer->latest_core_sample
        // );
        if( layer->earliest_core_sample < node_state->head_decode_addr &&
            layer->latest_core_sample >= node_state->head_decode_addr 
        ) {
            found_layer = layer;
        }
        layer = layer->next;
    }
    return found_layer;
}

void zdj_decode_fill_layer( 
    zdj_pipeline_node_t * node,
    zdj_decode_layer_t * layer
) {
    // printf( "_fill_layer\n" );
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;

    // Catch an empty layer here and add the first packet
    if( _layer_is_empty( layer ) ) { node_state->add_packet_to_empty_layer( node, layer ); }


    // Add packets to beginning of layer to fill window (may need more than one packet)
    while( _layer_can_add_packet_before( node, layer, node_state->head_win_start ) ) {
        // printf( "adding packet before layer\n" );
        node_state->add_packet_before_layer( node, layer ); 
    }

    // Add packets at end of layer to fill window (may need more than one packet)
    while( _layer_can_add_packet_after( node, layer, node_state->head_win_end ) ) { 
        // printf( "adding packet after layer\n" );
        node_state->add_packet_after_layer( node, layer ); 
    }

    // Remove packets before window start
    zdj_decode_packet_t * packet = layer->first_packet;
    while( packet ) {
        zdj_decode_packet_t * next_packet = packet->next;
        if( packet->lead_out_end_addr < node_state->head_win_start ) {
            // printf( "removing front packet\n" );
            zdj_decode_deinit_packet( packet );
            // Update layer address info
            if( next_packet ) {
                layer->earliest_core_sample = next_packet->core_start_addr;
                layer->earliest_lead_in_sample = next_packet->lead_in_start_addr;
                layer->first_packet = next_packet;
            } else {
                layer->first_packet = NULL;
                layer->last_packet = NULL;
            }
            packet = next_packet;
        } else { 
            packet = NULL;
            continue; // Exit once we're inside the window
        }
    }

    // Remove packets after window end
    packet = layer->last_packet;
    while( packet ) {
        zdj_decode_packet_t * prev_packet = packet->prev;
        if( packet->lead_in_start_addr > node_state->head_win_end ) {
            // printf( "removing end packet\n" );
            zdj_decode_deinit_packet( packet );
            // Update layer address info
            if( prev_packet ) {
                layer->latest_core_sample = prev_packet->core_end_addr;
                layer->latest_lead_out_sample = prev_packet->lead_out_end_addr;
                layer->last_packet = prev_packet;
            } else {
                layer->last_packet = NULL;
                layer->first_packet = NULL;
            }
            packet = prev_packet;
        } else { 
            packet = NULL;
            continue; // Exit once we're inside the window
        }
    }

    node_state->earliest_core_sample = fmin( node_state->earliest_core_sample, layer->earliest_core_sample );
    node_state->latest_core_sample = fmax( node_state->latest_core_sample, layer->latest_core_sample );

    // printf( "_fill_layer done\n" );
}

// Clip an existing, filled layer to the given clip_decode_addr
void zdj_decode_truncate_layer( 
    zdj_decode_layer_t * layer, 
    int64_t clip_decode_addr,
    zdj_decode_discon_type_t discon_type 
) {
    // Re-groom packets in layer:
    // Find packet containing clip_decode_addr
    //  Set core_end in packet to clip_decode_addr
    //  Set end_extent in packet
    // Remove all packets after
    zdj_decode_packet_t * packet = layer->first_packet;
    while( packet ) {
        zdj_decode_packet_t * next_packet = packet->next;
        if( packet->core_start_addr < clip_decode_addr &&
            packet->core_end_addr > clip_decode_addr 
        ) {
            packet->core_end_addr = clip_decode_addr;
            packet->lead_out_start_addr = clip_decode_addr;
            packet->lead_out_end_addr = clip_decode_addr;
            packet->is_fwd_extent = true;
            layer->latest_core_sample = packet->core_end_addr;
            layer->latest_lead_out_sample = packet->core_end_addr;
            layer->last_packet = packet;
        } else if ( packet->core_start_addr > clip_decode_addr ) {
            if( packet->prev ) { packet->prev->next = NULL; }
            zdj_decode_deinit_packet( packet );
        }
        packet = next_packet;
    }
}

void zdj_decode_deinit_layer( zdj_decode_layer_t * layer ) {
    // printf( "zdj_decode_deinit_layer: %p\n", layer );
    zdj_decode_packet_t * packet = layer->first_packet;
    while( packet ) { 
        zdj_decode_packet_t * next_packet = packet->next;
        zdj_decode_deinit_packet( packet );
        packet = next_packet;
    }
    free( layer );
    // printf( "zdj_decode_deinit_layer done\n" );
}

static bool _layer_is_empty( zdj_decode_layer_t * layer ) {
    if( layer->first_packet == NULL && layer->last_packet == NULL ) { return true; }
    else { return false; }
}

static bool _layer_can_add_packet_before( 
    zdj_pipeline_node_t * node, 
    zdj_decode_layer_t * layer, 
    int64_t address 
) {
    if( layer->first_packet->core_start_addr < address || // first_packet extends earlier than the given address
        layer->first_packet->is_back_extent // first_packet has been marked 'discon' during decode
    ) {
        return false;
    } else {
        return true;
    }
}

static bool _layer_can_add_packet_after( 
    zdj_pipeline_node_t * node, 
    zdj_decode_layer_t * layer, 
    int64_t address 
) {
    // printf( "_layer_can_add_packet_after: %ld - %ld\n", address, layer->last_packet->core_end_addr );
    if( layer->last_packet->core_end_addr > address || // first_packet extends earlier than the given address
        layer->last_packet->is_fwd_extent // first_packet has been marked 'discon' during decode
    ) {
        return false;
    } else {
        return true;
    }
}
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

static bool _is_empty( zdj_decode_layer_t * layer );
static void _prepend_packet( zdj_decode_layer_t * layer, zdj_decode_packet_t * packet );
static void _append_packet( zdj_decode_layer_t * layer, zdj_decode_packet_t * packet );
static void _insert_packet_after( 
    zdj_decode_layer_t * layer, zdj_decode_packet_t * new_packet, zdj_decode_packet_t * target_packet 
);
static void _insert_packet_before( 
    zdj_decode_layer_t * layer, zdj_decode_packet_t * new_packet, zdj_decode_packet_t * target_packet
);
static bool _can_remove_packet( 
    zdj_decode_layer_t * layer, zdj_pipeline_node_t * node, zdj_decode_packet_t * packet
);
static void _remove_packet( zdj_decode_layer_t * layer, zdj_decode_packet_t * packet );

static void _get_first_packet_addr( zdj_decode_layer_t * layer, zdj_decode_addr_t * addr );
static void _get_last_packet_addr( zdj_decode_layer_t * layer, zdj_decode_addr_t * addr );

static zdj_decode_packet_t * _get_packet_containing_addr( zdj_decode_layer_t * layer, zdj_decode_addr_t * addr, zdj_decode_addr_coord_t coord );

void zdj_decode_layer_init_packet_api( zdj_decode_layer_t * layer ) {
    layer->is_empty = &_is_empty;
    layer->prepend_packet = &_prepend_packet;
    layer->append_packet = &_append_packet;
    layer->insert_packet_before = &_insert_packet_before;
    layer->insert_packet_after = &_insert_packet_after;
    layer->remove_packet = &_remove_packet;
    layer->can_remove_packet = &_can_remove_packet;
    layer->get_first_packet_addr = &_get_first_packet_addr;
    layer->get_last_packet_addr = &_get_last_packet_addr;
    layer->get_packet_containing_addr = &_get_packet_containing_addr;
}

static bool _is_empty( zdj_decode_layer_t * layer ) {
    return layer->first_packet == NULL && layer->last_packet == NULL;
}

static void _prepend_packet( zdj_decode_layer_t * layer, zdj_decode_packet_t * packet ) {
    // printf( "_prepend_packet: %1.0f->%1.0f\n", packet->start_addr.origin_d,packet->end_addr.origin_d );
    layer->debug_packet_counter++;
    if( layer->first_packet ) {
        packet->next = layer->first_packet;
        layer->first_packet->prev = packet;
        layer->first_packet = packet;
    } else {
        layer->first_packet = packet;
        layer->last_packet = packet;
    }
}

static void _append_packet( zdj_decode_layer_t * layer, zdj_decode_packet_t * packet ) {
    // printf( "_append_packet: %1.0f->%1.0f\n", packet->start_addr.origin_d,packet->end_addr.origin_d );
    if( !layer || !packet ) { return; }
    layer->debug_packet_counter++;
    if( layer->last_packet ) {
        packet->prev = layer->last_packet;
        layer->last_packet->next = packet;
        layer->last_packet = packet;
    } else {
        layer->last_packet = packet;
        layer->first_packet = packet;
    }
}

static void _insert_packet_after( 
    zdj_decode_layer_t * layer, zdj_decode_packet_t * new_packet, zdj_decode_packet_t * target_packet 
) {
    // printf( "_insert_packet_after\n" );
    if( target_packet == layer->last_packet ) {
        layer->append_packet( layer, new_packet );
    } else {
        layer->debug_packet_counter++;
        new_packet->next = target_packet->next;
        new_packet->next->prev = new_packet;
        new_packet->prev = target_packet;
        new_packet->prev->next = new_packet;
    }
}

static void _insert_packet_before( 
    zdj_decode_layer_t * layer, zdj_decode_packet_t * new_packet, zdj_decode_packet_t * target_packet
) {
    // printf( "_insert_packet_before\n" );
    if( target_packet == layer->first_packet ) {
        layer->prepend_packet( layer, new_packet );
    } else {
        layer->debug_packet_counter++;
        new_packet->next = target_packet;
        new_packet->next->prev = new_packet;
        new_packet->prev = target_packet->prev;
        new_packet->prev->next = new_packet;
    }
}

static void _remove_packet( zdj_decode_layer_t * layer, zdj_decode_packet_t * packet ) {
    if( !packet ) { return; }
    layer->debug_packet_counter--;
    // printf( "remove_packet: %1.0f->%1.0f\n", packet->start_addr.origin_d,packet->end_addr.origin_d );
    if( packet == layer->first_packet && packet == layer->last_packet ) {
        layer->first_packet = NULL;
        layer->last_packet = NULL;
    } else if( packet == layer->first_packet ) {
        layer->first_packet = packet->next;
        if( layer->first_packet ) { layer->first_packet->prev = NULL; }
    } else if ( packet == layer->last_packet ) {
        layer->last_packet = packet->prev;
        if( layer->last_packet ) { layer->last_packet->next = NULL; }
    } else {
        packet->prev->next = packet->next;
        packet->next->prev = packet->prev;
    }
    packet->deinit( packet );
    // printf( "remove_packet done\n" );
}

static bool _can_remove_packet( 
    zdj_decode_layer_t * layer, zdj_pipeline_node_t * node, zdj_decode_packet_t * packet
) {
    if( !packet ){ return false; }
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;
    zdj_decode_addr_t win_start; node_state->get_win_start_addr( node, &win_start );
    zdj_decode_addr_t win_end; node_state->get_win_end_addr( node, &win_end );
    return packet->start_addr.greater_than( &packet->start_addr, &win_end, ZDJ_ADDR_COORD_TRANSPORT ) ||
           packet->start_addr.greater_than( &packet->start_addr, &layer->lead_out_end, ZDJ_ADDR_COORD_TRANSPORT ) ||
           packet->end_addr.less_than( &packet->end_addr, &win_start, ZDJ_ADDR_COORD_TRANSPORT ) ||
           packet->end_addr.less_than( &packet->end_addr, &layer->lead_in_start, ZDJ_ADDR_COORD_TRANSPORT );
}

static void _get_first_packet_addr( zdj_decode_layer_t * layer, zdj_decode_addr_t * addr ) {
    if( layer->first_packet ) {
        layer->first_packet->start_addr.copy( &layer->first_packet->start_addr, addr );
    }
}

static void _get_last_packet_addr( zdj_decode_layer_t * layer, zdj_decode_addr_t * addr ) {
    if( layer->last_packet ) {
        layer->last_packet->start_addr.copy( &layer->last_packet->start_addr, addr );
    }
}

static zdj_decode_packet_t * _get_packet_containing_addr( zdj_decode_layer_t * layer, zdj_decode_addr_t * addr, zdj_decode_addr_coord_t coord ) {
    
}
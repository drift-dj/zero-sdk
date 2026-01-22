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
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>

static void _accum( zdj_decode_layer_t * layer, zdj_pipeline_node_t * node );
static void _truncate( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer, void * _controls, double origin_d, zdj_decode_discon_type_t type, void * _loop_state );
void _untruncate( zdj_pipeline_node_t * node, struct zdj_decode_layer_t * layer );
static void _deinit( zdj_decode_layer_t * layer );

zdj_decode_layer_t * zdj_new_decode_continuous_layer( 
    zdj_pipeline_node_t * node, 
    zdj_decode_addr_t * init_addr 
) {
    // printf( "zdj_new_decode_continuous_layer\n" );
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    zdj_decode_layer_t * layer = calloc( 1, sizeof( zdj_decode_layer_t ) );

    layer->accum = &_accum;
    layer->fwd_discon_type = ZDJ_DECODE_DISCON_NONE;
    layer->back_discon_type = ZDJ_DECODE_DISCON_NONE;

    zdj_decode_layer_init_addr_api( layer );
    zdj_decode_layer_init_fill_api( layer );
    zdj_decode_layer_init_packet_api( layer );

    // Set up initial addresses
    zdj_decode_init_addr( &layer->init_addr );
    init_addr->copy( init_addr, &layer->init_addr );

    // Set layer start bounds to sample 0
    // Note that in continuous layer, there are no lead_in/out samples to crossfade.
    zdj_decode_init_addr( &layer->core_start );
    zdj_decode_init_addr( &layer->lead_in_start );
    zdj_decode_init_addr( &layer->lead_in_end );
    state->addr_for_origin_d_coord_in_layer( node, layer, &layer->core_start, 0.0 );
    state->addr_for_origin_d_coord_in_layer( node, layer, &layer->lead_in_start, 0.0 );
    state->addr_for_origin_d_coord_in_layer( node, layer, &layer->lead_in_end, 0.0  );

    // Set layer end bounds to duration of original song
    double duration = (double)state->song_pcm_duration;
    zdj_decode_init_addr( &layer->core_end );
    zdj_decode_init_addr( &layer->lead_out_start );
    zdj_decode_init_addr( &layer->lead_out_end );
    state->addr_for_origin_d_coord_in_layer( node, layer, &layer->core_end, duration );
    state->addr_for_origin_d_coord_in_layer( node, layer, &layer->lead_out_start, duration );
    state->addr_for_origin_d_coord_in_layer( node, layer, &layer->lead_out_end, duration );
    
    layer->truncate = &_truncate;
    layer->untruncate = &_untruncate;
    layer->deinit = &_deinit;
    layer->prev = NULL;
    layer->next = NULL;

    return layer;
}

zdj_decode_layer_t * zdj_new_decode_loop_layer( 
    zdj_pipeline_node_t * node, 
    zdj_decode_addr_t * loop_start_addr, 
    void * _loop_state
) {
    // printf( "zdj_new_decode_loop_layer\n" );
    // Set up layer core_start/end + lead_in/out
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    zdj_decode_layer_t * layer = calloc( 1, sizeof( zdj_decode_layer_t ) );
    zdj_deck_control_loop_state_t * loop_state = (zdj_deck_control_loop_state_t*)_loop_state;

    // printf( "zdj_new_decode_loop_layer: %1.0f/%1.0f %1.0f\n", loop_start_addr->transport_d, loop_start_addr->origin_d, loop_state->pcm_len );

    layer->accum = &_accum;
    layer->fwd_discon_type = ZDJ_DECODE_DISCON_LOOP;
    layer->back_discon_type = ZDJ_DECODE_DISCON_LOOP;
    layer->_loop_state = _loop_state;

    zdj_decode_layer_init_addr_api( layer );
    zdj_decode_layer_init_fill_api( layer );
    zdj_decode_layer_init_packet_api( layer );

    // Set up initial addresses
    zdj_decode_init_addr( &layer->init_addr );
    loop_start_addr->copy( loop_start_addr, &layer->init_addr );

    zdj_decode_init_addr( &layer->core_start );
    zdj_decode_init_addr( &layer->lead_in_start );
    zdj_decode_init_addr( &layer->lead_in_end );
    // // Core start
    // state->addr_for_origin_d_coord_in_layer( 
    //     node, layer, &layer->core_start, loop_start_addr->origin_d 
    // );
    loop_start_addr->copy( loop_start_addr, &layer->core_start );
    // // Lead in
    // state->addr_for_origin_d_coord_in_layer( 
    //     node, layer, &layer->lead_in_start, loop_start_addr->origin_d - state->layer_fade_len 
    // );
    loop_start_addr->copy( loop_start_addr, &layer->lead_in_start );
    state->offset_addr_by_transport_d_coord( node, &layer->lead_in_start, loop_state->fade_len * -1.0 );
    // state->addr_for_origin_d_coord_in_layer( 
    //     node, layer, &layer->lead_in_end, loop_start_addr->origin_d + state->layer_fade_len  
    // );
    loop_start_addr->copy( loop_start_addr, &layer->lead_in_end );
    state->offset_addr_by_transport_d_coord( node, &layer->lead_in_end, loop_state->fade_len );

    zdj_decode_init_addr( &layer->core_end );
    zdj_decode_init_addr( &layer->lead_out_start );
    zdj_decode_init_addr( &layer->lead_out_end );
    // // Core end
    // state->addr_for_origin_d_coord_in_layer( 
    //     node, layer, &layer->core_end, loop_start_addr->origin_d + loop_state->pcm_len 
    // );
    // // Lead out
    // state->addr_for_origin_d_coord_in_layer( 
    //     node, layer, &layer->lead_out_start, loop_start_addr->origin_d + loop_state->pcm_len - state->layer_fade_len 
    // );
    // state->addr_for_origin_d_coord_in_layer( 
    //     node, layer, &layer->lead_out_end, loop_start_addr->origin_d + loop_state->pcm_len + state->layer_fade_len 
    // );

    // Core End
    loop_start_addr->copy( loop_start_addr, &layer->core_end );
    state->offset_addr_by_transport_d_coord( node, &layer->core_end, loop_state->pcm_len );
    // Hard-set origin, since layer may not have valid origin for above offset call
    layer->core_end.origin_d = layer->core_start.origin_d + (double)loop_state->pcm_len;
    layer->core_end.origin_i = layer->core_start.origin_i + loop_state->pcm_len;
    // layer->core_end.origin_bg = layer->core_start.origin_bg + 
    
    // Lead Out
    layer->core_end.copy( &layer->core_end, &layer->lead_out_start );
    state->offset_addr_by_transport_d_coord( node, &layer->lead_out_start, loop_state->fade_len * -1.0 );
    layer->lead_out_start.origin_d = layer->core_end.origin_d - (double)loop_state->fade_len;
    layer->lead_out_start.origin_i = layer->core_end.origin_i - loop_state->pcm_len;
    // layer->core_end.origin_bg = layer->core_start.origin_bg + 

    layer->core_end.copy( &layer->core_end, &layer->lead_out_end );
    state->offset_addr_by_transport_d_coord( node, &layer->lead_out_end, loop_state->fade_len );
    layer->lead_out_end.origin_d = layer->core_end.origin_d + loop_state->fade_len;
    layer->lead_out_end.origin_i = layer->core_end.origin_i + loop_state->fade_len;
    // layer->core_end.origin_bg = layer->core_start.origin_bg + 
    
    layer->truncate = &_truncate;
    layer->untruncate = &_untruncate;
    layer->deinit = &_deinit;
    layer->prev = NULL;
    layer->next = NULL;

    return layer;
}

zdj_decode_layer_t * zdj_new_decode_skip_layer( 
    zdj_pipeline_node_t * node,  
    zdj_decode_addr_t * depart_addr
) {
    // Set up layer core_start/end + lead-in/out
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    zdj_decode_layer_t * layer = calloc( 1, sizeof( zdj_decode_layer_t ) );

    layer->accum = &_accum;
    layer->fwd_discon_type = ZDJ_DECODE_DISCON_NONE;
    layer->back_discon_type = ZDJ_DECODE_DISCON_SKIP;
    layer->_loop_state = NULL;

    zdj_decode_layer_init_addr_api( layer );
    zdj_decode_layer_init_fill_api( layer );
    zdj_decode_layer_init_packet_api( layer );

    // Set up initial addresses
    zdj_decode_init_addr( &layer->init_addr );
    depart_addr->copy( depart_addr, &layer->init_addr );

    zdj_decode_init_addr( &layer->core_start );
    zdj_decode_init_addr( &layer->lead_in_start );
    zdj_decode_init_addr( &layer->lead_in_end );
    state->addr_for_origin_d_coord_in_layer( node, layer, &layer->core_start, depart_addr->origin_d );
    state->addr_for_origin_d_coord_in_layer( 
        node, layer, &layer->lead_in_start, depart_addr->origin_d - state->layer_fade_len 
    );
    state->addr_for_origin_d_coord_in_layer( 
        node, layer, &layer->lead_in_end, depart_addr->origin_d + state->layer_fade_len  
    );

    zdj_decode_init_addr( &layer->core_end );
    zdj_decode_init_addr( &layer->lead_out_start );
    zdj_decode_init_addr( &layer->lead_out_end );
    state->addr_for_origin_d_coord_in_layer( node, layer, &layer->core_end, state->song_pcm_duration );
    state->addr_for_origin_d_coord_in_layer( 
        node, layer, &layer->lead_out_start, state->song_pcm_duration 
    );
    state->addr_for_origin_d_coord_in_layer( 
        node, layer, &layer->lead_out_end, state->song_pcm_duration
    );
    
    layer->truncate = &_truncate;
    layer->untruncate = &_untruncate;
    layer->deinit = &_deinit;
    layer->prev = NULL;
    layer->next = NULL;

    return layer;
}

zdj_decode_layer_t * zdj_new_decode_hyperscrub_layer( 
    zdj_pipeline_node_t * node,  
    zdj_decode_addr_t * depart_addr,
    zdj_decode_dir_t dir
) {
}

static void _accum( zdj_decode_layer_t * layer, zdj_pipeline_node_t * node ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    // Run through all packets in layer.
    // For packets intersecting with node's out_buffer, accumulate samples
    // into the out_buf with lead_in/out fades.
    zdj_decode_packet_t * packet = layer->first_packet;
    while( packet ) {
        if( packet->intersects_out_buf( packet, node ) ) {
            packet->render_to_out_buf( packet, layer, node );
        }
        packet = packet->next;
    }
}

static void _truncate( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer, void * _controls, double origin_d, zdj_decode_discon_type_t type, void * _loop_state ) {
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)node->state;
    zdj_deck_control_state_t * controls = (zdj_deck_control_state_t*)_controls;
    
    // Update layer's core_end + lead_out addrs for given coord
    if( type == ZDJ_DECODE_DISCON_SKIP ) {
        decode_state->addr_for_origin_d_coord_in_layer( 
            node, layer, &layer->core_end, origin_d 
        );
        decode_state->addr_for_origin_d_coord_in_layer( 
            node, layer, &layer->lead_out_start, origin_d - decode_state->layer_fade_len
        );
        decode_state->addr_for_origin_d_coord_in_layer( 
            node, layer, &layer->lead_out_end, origin_d + decode_state->layer_fade_len
        );

        layer->fwd_discon_type = type;


    } else if( type == ZDJ_DECODE_DISCON_LOOP && _loop_state ) {
        layer->fwd_discon_type = ZDJ_DECODE_DISCON_LOOP;
        layer->back_discon_type = ZDJ_DECODE_DISCON_LOOP;
        zdj_deck_control_loop_state_t * loop_state = (zdj_deck_control_loop_state_t*)_loop_state;
        decode_state->addr_for_origin_d_coord_in_layer( 
            node, layer, &layer->core_start, loop_state->start_origin_d 
        );
        decode_state->addr_for_origin_d_coord_in_layer( 
            node, layer, &layer->lead_in_start, loop_state->start_origin_d - decode_state->layer_fade_len
        );
        decode_state->addr_for_origin_d_coord_in_layer( 
            node, layer, &layer->lead_in_end, loop_state->start_origin_d + decode_state->layer_fade_len
        );

        decode_state->addr_for_origin_d_coord_in_layer( 
            node, layer, &layer->core_end, loop_state->end_origin_d 
        );
        decode_state->addr_for_origin_d_coord_in_layer( 
            node, layer, &layer->lead_out_start, loop_state->end_origin_d - decode_state->layer_fade_len
        );
        decode_state->addr_for_origin_d_coord_in_layer( 
            node, layer, &layer->lead_out_end, loop_state->end_origin_d + decode_state->layer_fade_len
        );
    }

    layer->_loop_state = _loop_state; // Will be null during skip

    // printf( "truncate layer: %p %p\n", layer, layer->_loop_state );
}

void _untruncate( zdj_pipeline_node_t * node, struct zdj_decode_layer_t * layer ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    
    // printf( "layer _untruncate\n" );
    layer->_loop_state = NULL;

    // Expand layer's start/end addrs back to song start/end
    state->addr_for_origin_d_coord_in_layer( node, layer, &layer->core_start, 0.0 );
    state->addr_for_origin_d_coord_in_layer( node, layer, &layer->lead_in_start, 0.0 );
    state->addr_for_origin_d_coord_in_layer( node, layer, &layer->lead_in_end, 0.0  );
    // Set layer end bounds to duration of original song
    double duration = (double)state->song_pcm_duration;
    state->addr_for_origin_d_coord_in_layer( node, layer, &layer->core_end, duration );
    state->addr_for_origin_d_coord_in_layer( node, layer, &layer->lead_out_start, duration );
    state->addr_for_origin_d_coord_in_layer( node, layer, &layer->lead_out_end, duration );

    // Reset layer's discon types
    layer->fwd_discon_type = ZDJ_DECODE_DISCON_NONE;
    layer->back_discon_type = ZDJ_DECODE_DISCON_NONE;

    // Remove all previous layers
    zdj_decode_layer_t * p_layer = layer->prev;
    while( p_layer ) {
        zdj_decode_layer_t * prev_layer = p_layer->prev;
        p_layer->deinit( p_layer );
        p_layer = prev_layer;
    }
    layer->prev = NULL;

    // Remove all next layers
    zdj_decode_layer_t * n_layer = layer->next;
    while( n_layer ) {
        zdj_decode_layer_t * next_layer = n_layer->next;
        n_layer->deinit( n_layer );
        n_layer = next_layer;
    }
    layer->next = NULL;

    state->first_layer = layer;
    state->last_layer = layer;
}

static void _deinit( zdj_decode_layer_t * layer ) {
    // printf( "zdj_decode_deinit_layer: %p\n", layer );
    zdj_decode_packet_t * packet = layer->first_packet;
    while( packet ) { 
        zdj_decode_packet_t * next_packet = packet->next;
        packet->deinit( packet );
        packet = next_packet;
    }
    free( layer );
    // printf( "zdj_decode_deinit_layer done\n" );
}
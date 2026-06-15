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

// Decode Node Address API //
//-------------------------//
static void _get_win_start_addr( zdj_pipeline_node_t * node, zdj_decode_addr_t * addr );
static void _get_win_end_addr( zdj_pipeline_node_t * node, zdj_decode_addr_t * addr );
static bool _win_contains_addr(
    zdj_pipeline_node_t * node, 
    zdj_decode_addr_t * addr, 
    zdj_decode_addr_coord_t ref_coord
);

static float _get_head_percent( zdj_pipeline_node_t * node );
static double _get_head_sec( zdj_pipeline_node_t * node );

static double _get_quantized_head_origin_bg( zdj_pipeline_node_t * node, double quant_val );

static void _set_addr_transport_i_coord( 
    zdj_pipeline_node_t * node, zdj_decode_addr_t * addr, int64_t val
);
static void _set_addr_transport_d_coord( 
    zdj_pipeline_node_t * node, zdj_decode_addr_t * addr, double val
);
static void _set_addr_transport_bg_coord( 
    zdj_pipeline_node_t * node, zdj_decode_addr_t * addr, double val
);
static void _offset_addr_by_transport_i_coord( 
    zdj_pipeline_node_t * node, zdj_decode_addr_t * addr, int64_t offset
);
static void _offset_addr_by_transport_d_coord( 
    zdj_pipeline_node_t * node, zdj_decode_addr_t * addr, double offset, bool ignore_loop
);
static void _offset_addr_by_transport_bg_coord( 
    zdj_pipeline_node_t * node, zdj_decode_addr_t * addr, double offset
);
static void _addr_for_transport_i_coord( 
    zdj_pipeline_node_t * node, zdj_decode_addr_t * addr, int64_t coord
);
static void _addr_for_transport_d_coord( 
    zdj_pipeline_node_t * node, zdj_decode_addr_t * addr, double coord
);
static void _addr_for_transport_bg_coord( 
    zdj_pipeline_node_t * node, zdj_decode_addr_t * addr, double coord
);
static void _addr_for_origin_i_coord_in_layer( 
    zdj_pipeline_node_t * node, zdj_decode_layer_t * layer, zdj_decode_addr_t * addr, int64_t coord
);
static void _addr_for_origin_d_coord_in_layer( 
    zdj_pipeline_node_t * node, zdj_decode_layer_t * layer, zdj_decode_addr_t * addr, double coord
);
static void _addr_for_origin_bg_coord_in_layer( 
    zdj_pipeline_node_t * node, zdj_decode_layer_t * layer, zdj_decode_addr_t * addr, double coord
);


static zdj_decode_layer_t * _get_layer_containing_addr( 
    zdj_pipeline_node_t * node, zdj_decode_addr_t * addr, zdj_decode_addr_coord_t ref_coord
);
static zdj_decode_layer_t * _get_layer_containing_core_addr( 
    zdj_pipeline_node_t * node, zdj_decode_addr_t * addr, zdj_decode_addr_coord_t ref_coord
);
static double _get_beatgrid_coord_for_d_coord( zdj_pipeline_node_t * node, double coord );
static double _get_d_offset_for_beatgrid_dist( zdj_pipeline_node_t * node, double dist );

static void _get_earliest_core_addr( zdj_pipeline_node_t * node, zdj_decode_addr_t * addr );
static void _get_latest_core_addr( zdj_pipeline_node_t * node, zdj_decode_addr_t * addr );
static void _get_earliest_lead_in_addr( zdj_pipeline_node_t * node, zdj_decode_addr_t * addr );
static void _get_latest_lead_out_addr( zdj_pipeline_node_t * node, zdj_decode_addr_t * addr );


// zdj_decode_addr_t API //
//-----------------------//
static void _copy( zdj_decode_addr_t * src_addr, zdj_decode_addr_t * dst_addr );
static bool _equal( 
    zdj_decode_addr_t * addr1, zdj_decode_addr_t * addr2, zdj_decode_addr_coord_t ref_coord 
);
static bool _less_than( 
    zdj_decode_addr_t * addr1, zdj_decode_addr_t * addr2, zdj_decode_addr_coord_t ref_coord 
);
static bool _greater_than( 
    zdj_decode_addr_t * addr1, zdj_decode_addr_t * addr2, zdj_decode_addr_coord_t ref_coord 
);
static void _update_buf_i_for_node_head( zdj_decode_addr_t * addr, zdj_pipeline_node_t * node );



void zdj_decode_init_node_addr_api( zdj_pipeline_node_t * node ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    state->get_win_start_addr = &_get_win_start_addr;
    state->get_win_end_addr = &_get_win_end_addr;
    state->win_contains_addr = &_win_contains_addr;
    
    state->get_head_percent = &_get_head_percent;
    state->get_head_sec = &_get_head_sec;
    state->get_quantized_head_origin_bg = &_get_quantized_head_origin_bg;

    state->set_addr_transport_i_coord = &_set_addr_transport_i_coord;
    state->set_addr_transport_d_coord = &_set_addr_transport_d_coord;
    state->set_addr_transport_bg_coord = &_set_addr_transport_bg_coord;

    state->offset_addr_by_transport_i_coord = &_offset_addr_by_transport_i_coord;
    state->offset_addr_by_transport_d_coord = &_offset_addr_by_transport_d_coord;
    state->offset_addr_by_transport_bg_coord = &_offset_addr_by_transport_bg_coord;

    state->addr_for_transport_i_coord = &_addr_for_transport_i_coord;
    state->addr_for_transport_d_coord = &_addr_for_transport_d_coord;
    state->addr_for_transport_bg_coord = &_addr_for_transport_bg_coord;

    state->addr_for_origin_i_coord_in_layer = &_addr_for_origin_i_coord_in_layer;
    state->addr_for_origin_d_coord_in_layer = &_addr_for_origin_d_coord_in_layer;
    state->addr_for_origin_bg_coord_in_layer = &_addr_for_origin_bg_coord_in_layer;

    state->get_layer_containing_addr = &_get_layer_containing_addr;
    state->get_layer_containing_core_addr = &_get_layer_containing_core_addr;
    state->get_beatgrid_coord_for_d_coord = &_get_beatgrid_coord_for_d_coord;
    state->get_d_offset_for_beatgrid_dist = &_get_d_offset_for_beatgrid_dist;

    state->get_earliest_core_addr = &_get_earliest_core_addr;
    state->get_latest_core_addr = &_get_latest_core_addr;
    state->get_earliest_lead_in_addr = &_get_earliest_lead_in_addr;
    state->get_latest_lead_out_addr = &_get_latest_lead_out_addr;
}

/////////////////////////////
// Decode Node Address API //
/////////////////////////////

static void _get_win_start_addr( zdj_pipeline_node_t * node, zdj_decode_addr_t * addr ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    // Copy the head addr
    state->head.copy( &state->head, addr );
    // Offset the addr by back sample count
    state->offset_addr_by_transport_i_coord( 
        node, 
        addr, 
        state->win_back_sample_count * -1
    );
}

static void _get_win_end_addr( zdj_pipeline_node_t * node, zdj_decode_addr_t * addr ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    // Copy the head addr
    state->head.copy( &state->head, addr );
    // Offset the addr by back sample count
    state->offset_addr_by_transport_i_coord( 
        node, 
        addr, 
        state->win_fwd_sample_count
    );
}

static bool _win_contains_addr(
    zdj_pipeline_node_t * node, 
    zdj_decode_addr_t * addr, 
    zdj_decode_addr_coord_t ref_coord
) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    zdj_decode_addr_t win_start;
    state->get_win_start_addr( node, &win_start );
    zdj_decode_addr_t win_end;
    state->get_win_end_addr( node, &win_end );

    return addr->greater_than( addr, &win_start, ZDJ_ADDR_COORD_TRANSPORT ) &&
           addr->less_than( addr, &win_end, ZDJ_ADDR_COORD_TRANSPORT );
}

static float _get_head_percent( zdj_pipeline_node_t * node ) {
    return 0.0f;
}

static double _get_head_sec( zdj_pipeline_node_t * node ) {
    return 0.0f;
}

static double _get_quantized_head_origin_bg( zdj_pipeline_node_t * node, double quant_val ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    if( state->song->performance &&
        state->song->performance->bpm 
    ) {
        return (double)round( state->head.origin_bg / quant_val ) * quant_val;
    } else {
        printf( "get_quantized_head_origin_bg CALLED W/O BPM\n" );
        return state->head.origin_d;
    }
}

static void _set_addr_transport_i_coord( 
    zdj_pipeline_node_t * node, zdj_decode_addr_t * addr, int64_t val
) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    addr->transport_i = val;
    addr->transport_d = (double)val;
    addr->transport_bg = state->get_beatgrid_coord_for_d_coord( node, addr->transport_d );
    
    addr->buf_i = state->head.buf_i - val;
    addr->buf_d = (double)addr->buf_i;
    addr->has_valid_buf = true;

    zdj_decode_layer_t * layer = state->get_layer_containing_addr( node, addr, ZDJ_ADDR_COORD_TRANSPORT );
    if( layer ) {
        addr->origin_d = layer->origin_d_coord_for_transport_d_coord( layer, addr->transport_d );
        addr->origin_i = round( addr->origin_d );
        addr->origin_bg = state->get_beatgrid_coord_for_d_coord( node, addr->origin_d );
        addr->has_valid_origin = true;
    } else {
        addr->has_valid_origin = false;
    }
}

static void _set_addr_transport_d_coord( 
    zdj_pipeline_node_t * node, zdj_decode_addr_t * addr, double val
) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    
    addr->transport_d = val;
    addr->transport_i = round( addr->transport_d );
    addr->transport_bg = state->get_beatgrid_coord_for_d_coord( node, addr->transport_d );
    
    addr->buf_d = state->head.buf_d - addr->transport_d;
    addr->buf_i = round( addr->buf_d );
    addr->has_valid_buf = true;

    zdj_decode_layer_t * layer = state->get_layer_containing_addr( node, addr, ZDJ_ADDR_COORD_TRANSPORT );
    if( layer ) {
        addr->origin_d = layer->origin_d_coord_for_transport_d_coord( layer, addr->transport_d );
        addr->origin_i = round( addr->origin_d );
        addr->origin_bg = state->get_beatgrid_coord_for_d_coord( node, addr->origin_d );
        addr->has_valid_origin = true;
    } else {
        addr->has_valid_origin = false;
    }
}

static void _set_addr_transport_bg_coord( 
    zdj_pipeline_node_t * node, zdj_decode_addr_t * addr, double val
) {
    
}


static void _offset_addr_by_transport_i_coord( 
    zdj_pipeline_node_t * node, zdj_decode_addr_t * addr, int64_t offset
) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    addr->transport_i += offset;
    addr->transport_d = (double)addr->transport_i;
    addr->transport_bg = state->get_beatgrid_coord_for_d_coord( node, addr->transport_d );
    
    addr->buf_i = state->head.buf_i - addr->transport_i;
    addr->buf_d = (double)addr->buf_i;
    addr->has_valid_buf = true;
   
    // Correct origin for any existing dicontinuity state
    zdj_decode_layer_t * layer = state->get_layer_containing_addr( node, addr, ZDJ_ADDR_COORD_TRANSPORT );
    if( layer ) {
        // If there's a layer containing the new offset address, use the layer's origin coords
        addr->origin_i = round( layer->origin_d_coord_for_transport_d_coord( layer, addr->transport_d ) );
        addr->has_valid_origin = true;
    } else {
        // If there's no layer containing the new offset address, just offset by given value
        addr->origin_i += offset;
        addr->has_valid_origin = false;
    }
    addr->origin_d = (double)addr->origin_i;
    addr->origin_bg = state->get_beatgrid_coord_for_d_coord( node, addr->origin_d );
}

static void _offset_addr_by_transport_d_coord( 
    zdj_pipeline_node_t * node, zdj_decode_addr_t * addr, double offset, bool ignore_loop
) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    addr->transport_d = addr->transport_d + offset;
    addr->transport_i = round( addr->transport_d );
    addr->transport_bg = state->get_beatgrid_coord_for_d_coord( node, addr->transport_d );
    
    addr->buf_d = state->head.buf_d - addr->transport_d;
    addr->buf_i = round( addr->buf_d );
    addr->has_valid_buf = true;

    // Correct origin for any existing dicontinuity state
    zdj_decode_layer_t * layer = state->get_layer_containing_addr( node, addr, ZDJ_ADDR_COORD_TRANSPORT );
    if( layer && !ignore_loop ) {
        // If there's a layer containing the new offset address, use the layer's origin coords
        addr->origin_d = layer->origin_d_coord_for_transport_d_coord( layer, addr->transport_d );
        addr->has_valid_origin = true;
    } else {
        // If there's no layer containing the new offset address, just offset by given value
        addr->origin_d += offset;
        addr->has_valid_origin = false;
    }
    addr->origin_i = round( addr->origin_d );
    addr->origin_bg = state->get_beatgrid_coord_for_d_coord( node, addr->origin_d );
}

static void _offset_addr_by_transport_bg_coord( 
    zdj_pipeline_node_t * node, zdj_decode_addr_t * addr, double offset
) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    if( !state->song->performance || !state->song->audio ) { 
        printf( "_offset_addr_by_transport_bg_coord w/o song data\n" );
        return; 
    }
    double bpm = state->song->performance->bpm;
    double sample_rate = state->song->audio->av_sample_rate;
    double bars_per_minute = bpm / 4.0;
	double samples_per_minute = sample_rate * 60.0;
	double samples_per_bar = samples_per_minute / bars_per_minute;

    double d_coord_offset = offset * samples_per_bar;

    state->offset_addr_by_transport_d_coord( node, addr, d_coord_offset, false );
}


static void _addr_for_transport_i_coord( 
    zdj_pipeline_node_t * node, zdj_decode_addr_t * addr, int64_t coord
) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    addr->transport_i = coord;
    addr->transport_d = (double)addr->transport_i;
    addr->transport_bg = state->get_beatgrid_coord_for_d_coord( node, addr->transport_d );
    
    addr->buf_i = state->head.buf_i - addr->transport_i;
    addr->buf_d = (double)addr->buf_i;
    addr->has_valid_buf = true;

    zdj_decode_layer_t * layer = state->get_layer_containing_addr( node, addr, ZDJ_ADDR_COORD_TRANSPORT );
    if( layer ) {
        addr->origin_i = round( layer->origin_d_coord_for_transport_d_coord( layer, addr->transport_d ) );
        addr->origin_d = (double)addr->origin_i;
        addr->origin_bg = state->get_beatgrid_coord_for_d_coord( node, addr->origin_d );
        addr->has_valid_origin = true;
    } else {
        addr->has_valid_origin = false;
    }
}

static void _addr_for_transport_d_coord( 
    zdj_pipeline_node_t * node, zdj_decode_addr_t * addr, double coord
) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    addr->transport_d = coord;
    addr->transport_i = round( addr->transport_d );
    addr->transport_bg = state->get_beatgrid_coord_for_d_coord( node, addr->transport_d );
    
    addr->buf_d = state->head.buf_d - addr->transport_d;
    addr->buf_i = round( addr->buf_d );
    addr->has_valid_buf = true;

    zdj_decode_layer_t * layer = state->get_layer_containing_addr( node, addr, ZDJ_ADDR_COORD_TRANSPORT );
    if( layer ) {
        addr->origin_d = layer->origin_d_coord_for_transport_d_coord( layer, addr->transport_d );
        addr->origin_i = round( addr->origin_d );
        addr->origin_bg = state->get_beatgrid_coord_for_d_coord( node, addr->origin_d );
        addr->has_valid_origin = true;
    } else {
        addr->has_valid_origin = false;
    }
}

static void _addr_for_transport_bg_coord( 
    zdj_pipeline_node_t * node, zdj_decode_addr_t * addr, double coord
) {
    printf( "_addr_for_transport_bg_coord called with no implementation!!!\n" );
}

static void _addr_for_origin_i_coord_in_layer( 
    zdj_pipeline_node_t * node, zdj_decode_layer_t * layer, zdj_decode_addr_t * addr, int64_t coord
) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    addr->origin_i = coord;
    addr->origin_d = (double)addr->origin_i;
    addr->origin_bg = state->get_beatgrid_coord_for_d_coord( node, addr->origin_d );
    addr->has_valid_origin = true;

    // Map against offset from layer's init addr
    double offset = addr->origin_d - layer->init_addr.origin_d;

    addr->transport_d = layer->init_addr.transport_d + offset;
    addr->transport_i = round( addr->transport_d );
    addr->transport_bg = state->get_beatgrid_coord_for_d_coord( node, addr->transport_d );
    
    addr->buf_i = state->head.buf_i - addr->transport_i;
    addr->buf_d = (double)addr->buf_i;
    addr->has_valid_buf = true;
}

static void _addr_for_origin_d_coord_in_layer( 
    zdj_pipeline_node_t * node, zdj_decode_layer_t * layer, zdj_decode_addr_t * addr, double coord
) {
    // printf( "_addr_for_origin_d_coord_in_layer: %p, %p, %1.0f\n", layer, addr, coord );
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    addr->origin_d = coord;
    addr->origin_i = round( addr->origin_d );
    addr->origin_bg = state->get_beatgrid_coord_for_d_coord( node, addr->origin_d );
    addr->has_valid_origin = true;

    // Map against offset from layer's init addr
    double offset = addr->origin_d - layer->init_addr.origin_d;

    addr->transport_d = layer->init_addr.transport_d + offset;
    addr->transport_i = round( addr->transport_d );
    addr->transport_bg = state->get_beatgrid_coord_for_d_coord( node, addr->transport_d );

    addr->buf_i = state->head.buf_i - addr->transport_i;
    addr->buf_d = (double)addr->buf_i;
    addr->has_valid_buf = true;
}

static void _addr_for_origin_bg_coord_in_layer( 
    zdj_pipeline_node_t * node, zdj_decode_layer_t * layer, zdj_decode_addr_t * addr, double coord
) {
    printf( "_addr_for_origin_bg_coord_in_layer called with no implementation!!!\n" );
}

static zdj_decode_layer_t * _get_layer_containing_addr( 
    zdj_pipeline_node_t * node, zdj_decode_addr_t * addr, zdj_decode_addr_coord_t ref_coord
) {
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;
    zdj_decode_layer_t * layer = node_state->first_layer;
    int iter_lim = 1000;
    int iter = 0;
    while( layer ) {
        if( layer->contains_core_addr( layer, addr, ref_coord ) ) {
            return layer;
        }
        layer = layer->next;
        if( iter++ > iter_lim ) {
            printf( "HIT ITER LIMIT (_get_layer_containing_core_addr)\n" );
            break;
        }
    }
    return NULL;
}

static zdj_decode_layer_t * _get_layer_containing_core_addr( 
    zdj_pipeline_node_t * node, zdj_decode_addr_t * addr, zdj_decode_addr_coord_t ref_coord
) {
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;
    zdj_decode_layer_t * layer = node_state->first_layer;
    int iter_lim = 1000;
    int iter = 0;
    while( layer ) {
        if( layer->contains_core_addr( layer, addr, ref_coord ) ) {
            return layer;
        }
        layer = layer->next;
        if( iter++ > iter_lim ) {
            printf( "HIT ITER LIMIT (_get_layer_containing_core_addr)\n" );
            break;
        }
    }
    return NULL;
}

static double _get_beatgrid_coord_for_d_coord( zdj_pipeline_node_t * node, double coord ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    // printf( "_get_beatgrid_coord_for_d_coord song:%p %p %p\n", state->song, state->song->performance, state->song->audio );
    if( !state->song->performance || !state->song->audio ) { return 0.0; }
    double bpm = state->song->performance->bpm;
    double sample_rate = state->song->audio->av_sample_rate;
    double bars_per_minute = bpm / 4.0;
	double samples_per_minute = sample_rate * 60.0;
	double samples_per_bar = samples_per_minute / bars_per_minute;

    double pcm_offset = coord - state->song->performance->beat_grid_start_sample;
	return pcm_offset / samples_per_bar;
}

static double _get_d_offset_for_beatgrid_dist( zdj_pipeline_node_t * node, double dist ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    if( !state->song->performance || !state->song->audio ) { return 0.0; }
    double bpm = state->song->performance->bpm;
    double sample_rate = state->song->audio->av_sample_rate;
    double bars_per_minute = bpm / 4.0;
	double samples_per_minute = sample_rate * 60.0;
	double samples_per_bar = samples_per_minute / bars_per_minute;

	return samples_per_bar * dist;
}

static void _get_earliest_core_addr( zdj_pipeline_node_t * node, zdj_decode_addr_t * addr ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    if( !state->first_layer ) { printf( "_get_latest_core_addr missing first_layer\n" ); return; }
    state->first_layer->core_start.copy( &state->first_layer->core_start, addr );
}

static void _get_latest_core_addr( zdj_pipeline_node_t * node, zdj_decode_addr_t * addr ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    if( !state->last_layer ) { printf( "_get_latest_core_addr missing last_layer\n" ); return; }
    state->last_layer->core_end.copy( &state->last_layer->core_end, addr );
}

static void _get_earliest_lead_in_addr( zdj_pipeline_node_t * node, zdj_decode_addr_t * addr ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    if( !state->first_layer ) { return; }
    state->first_layer->lead_in_end.copy( &state->first_layer->lead_in_end, addr );
}

static void _get_latest_lead_out_addr( zdj_pipeline_node_t * node, zdj_decode_addr_t * addr ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    if( !state->last_layer ) { return; }
    state->last_layer->lead_out_start.copy( &state->last_layer->lead_out_start, addr );
}



///////////////////////////
// zdj_decode_addr_t API //
///////////////////////////

void zdj_decode_init_addr( zdj_decode_addr_t * addr ) {
    memset( addr, 0, sizeof( zdj_decode_addr_t ) );
    addr->copy = &_copy;
    addr->equal = &_equal;
    addr->less_than = &_less_than;
    addr->greater_than = &_greater_than;
    addr->update_buf_i_for_node_head = &_update_buf_i_for_node_head;
}

static void _copy( zdj_decode_addr_t * src_addr, zdj_decode_addr_t * dst_addr ) {
    memcpy( dst_addr, src_addr, sizeof( zdj_decode_addr_t ) );
}

static bool _equal( 
    zdj_decode_addr_t * addr1, zdj_decode_addr_t * addr2, zdj_decode_addr_coord_t ref_coord 
) {
    // printf( "equal: %p = %p\n", addr1, addr2 );
    switch ( ref_coord ) {
        case ZDJ_ADDR_COORD_ORIGIN: 
            // Origin coords may need a looser definition of equality since
            // they are often in an unknown state of conversion between double/int types
            if( fabs(addr1->origin_d - addr2->origin_d) < zdj_eps ) {
                return true;
            } else if( round( addr1->origin_d ) - round(addr2->origin_d) == 0 ) {
                return true;
            } else {
                return false;
            }
        case ZDJ_ADDR_COORD_TRANSPORT: return ( fabs(addr1->transport_d - addr2->transport_d) < zdj_eps );
        case ZDJ_ADDR_COORD_OUT_BUF: return ( fabs(addr1->buf_d - addr2->buf_d) < zdj_eps );
    }
}

static bool _less_than( 
    zdj_decode_addr_t * addr1, zdj_decode_addr_t * addr2, zdj_decode_addr_coord_t ref_coord 
) {
    // printf( "less_than: %p < %p\n", addr1, addr2 );
    switch ( ref_coord ) {
        case ZDJ_ADDR_COORD_ORIGIN: return addr1->origin_d < addr2->origin_d;
        case ZDJ_ADDR_COORD_TRANSPORT: return addr1->transport_d < addr2->transport_d;
        case ZDJ_ADDR_COORD_OUT_BUF: return addr1->buf_d < addr2->buf_d;
    }
}

static bool _greater_than( 
    zdj_decode_addr_t * addr1, zdj_decode_addr_t * addr2, zdj_decode_addr_coord_t ref_coord 
) {
    // printf( "greater_than: %p > %p\n", addr1, addr2 );
    switch ( ref_coord ) {
        case ZDJ_ADDR_COORD_ORIGIN: return addr1->origin_d > addr2->origin_d;
        case ZDJ_ADDR_COORD_TRANSPORT: return addr1->transport_d > addr2->transport_d;
        case ZDJ_ADDR_COORD_OUT_BUF: return addr1->buf_d > addr2->buf_d;
    }
}

static void _update_buf_i_for_node_head( zdj_decode_addr_t * addr, zdj_pipeline_node_t * node ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    zdj_decode_addr_t win_start;
    state->get_win_start_addr( node, &win_start );
    addr->buf_i = addr->transport_i - win_start.transport_i;
    addr->buf_d = (double)addr->buf_i;    
}
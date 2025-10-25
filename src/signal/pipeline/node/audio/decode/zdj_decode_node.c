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

static void _update_wait( zdj_pipeline_node_t * node );
static void _deinit_state( zdj_pipeline_node_t * node );
static void _clear_out_buffer( zdj_pipeline_node_t * node ); 

static zdj_error_type_t _move_window( zdj_pipeline_node_t * node, double offset );
static zdj_error_type_t _reset_window( zdj_pipeline_node_t * node, double address );

static void _prepend_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer );
static void _append_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer );
static bool _can_remove_layer( zdj_decode_layer_t * layer );
static void _remove_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer );

zdj_pipeline_node_t * zdj_new_decode_node( 
    zdj_library_song_t * song,
    int64_t address,
    size_t back_sample_count,
    size_t fwd_sample_count
) {
    // printf( "zdj_new_decode_node\n" );
    // Make node
    zdj_pipeline_node_t * node = zdj_new_pipeline_node( );
    node->deinit_state = &_deinit_state;
    node->update_wait = &_update_wait;
    node->move_window = &_move_window;
    node->reset_window = &_reset_window;

    // Add state
    zdj_decode_node_state_t * state = calloc( 1, sizeof( zdj_decode_node_state_t ) );
    node->state = state;
    state->status = ZDJ_DECODE_NODE_INIT;
    state->song = song;
    state->song_pcm_duration = song->audio->duration * song->audio->av_sample_rate;
    state->win_back_sample_count = back_sample_count;
    state->win_fwd_sample_count = fwd_sample_count;
    state->win_sample_count = fwd_sample_count + back_sample_count;
    state->clear_out_buffer = &_clear_out_buffer;
    state->prepend_layer = &_prepend_layer;
    state->append_layer = &_append_layer;
    state->can_remove_layer = &_can_remove_layer;
    state->remove_layer = &_remove_layer;
    
    // printf( "zdj_new_decode_node 0\n" );

    zdj_decode_init_addr( &state->head );
    // Add address/discontinuity handlers 
    zdj_decode_init_node_addr_api( node );
    zdj_decode_init_node_discon_api( node );  
    // Alloc output buffer - sample_count * 4 is a bit of a mystery - malloc error if it's less. 
    state->out_buffer = calloc( state->win_sample_count * 4, sizeof( float ) );

    av_log_set_level( AV_LOG_QUIET );
    // av_log_set_level( AV_LOG_TRACE );

    // printf( "zdj_new_decode_node 1\n" );

    // Use the open command to build the Format Context
    state->fmt_ctx = avformat_alloc_context( );
    char filepath[ 512 ];
    strcpy( filepath, song->audio->filepath );
    int res = avformat_open_input( &state->fmt_ctx, filepath, NULL, NULL );
    if( res != 0 ) {
        printf( "avformat_open_input failed\n" );
    }
    res = avformat_find_stream_info( state->fmt_ctx, NULL );
    if( res != 0 ) {
        printf( "avformat_find_stream_info failed\n" );
    }
    // Build a Codec Context for decoding.
    AVCodec * codec = avcodec_find_decoder( state->song->audio->av_codec_id );
    state->codec_ctx = avcodec_alloc_context3( codec );
    state->codec_ctx->time_base.num = 1;
    state->codec_ctx->time_base.den = 44100;
    state->codec_ctx->channels = state->song->audio->av_channel_count;
    state->codec_ctx->request_sample_fmt = AV_SAMPLE_FMT_FLT; // Keep... in case.
    state->codec_ctx->thread_count = 0;
    res = avcodec_open2( state->codec_ctx, codec, NULL );

    switch( state->song->audio->av_codec_id ) {
        case AV_CODEC_ID_MP3:
            state->estimated_packet_sample_count = 1152;
            state->av_timebase_factor = 320;
            break;
        case AV_CODEC_ID_AAC:
            state->estimated_packet_sample_count = 511;
            state->av_timebase_factor = 1;
            // break;
        case AV_CODEC_ID_FLAC:
            state->estimated_packet_sample_count = 2048;
            state->av_timebase_factor = 1;
            // break;
        case AV_CODEC_ID_PCM_S16LE:
        case AV_CODEC_ID_PCM_S16BE:
        case AV_CODEC_ID_PCM_U16LE:
        case AV_CODEC_ID_PCM_U16BE:
        case AV_CODEC_ID_PCM_S24LE:
        case AV_CODEC_ID_PCM_S24BE:
        case AV_CODEC_ID_PCM_U24LE:
        case AV_CODEC_ID_PCM_U24BE:
        case AV_CODEC_ID_PCM_S32LE:
        case AV_CODEC_ID_PCM_S32BE:
        case AV_CODEC_ID_PCM_U32LE:
        case AV_CODEC_ID_PCM_U32BE:
        case AV_CODEC_ID_PCM_F32LE:
        case AV_CODEC_ID_PCM_F32BE:
            state->estimated_packet_sample_count = 2048;
            state->av_timebase_factor = 1;
            break;
    }

    // // Do inert window move to trigger pre-fill
    // node->reset_window( node, 0 );

    // printf( "zdj_new_decode_node 2\n" );

    // Add and fill first layer
    // ------------------------
    zdj_decode_layer_t * layer = zdj_new_decode_continuous_layer( node, &state->head );
    state->append_layer( node, layer );
    layer->fill( layer, node );

    // // Debug dump format
    // av_dump_format( state->fmt_ctx, 0, song->audio->filepath, 0 );
    // printf( "zdj_new_decode_node done\n" );

    return node;
}

// Execute a copy/accumulate of all of node's layers to the out_buffer.
// TODO: Find efficiency by changing this to a memmove model instead of
// clearing and refilling the entire window each time.
static void _update_wait( zdj_pipeline_node_t * node ) {
    // printf( "decode _update_wait\n" );
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_DECODE_UPDATE;
    
    // printf( "_update_wait 0\n" );
    state->clear_out_buffer( node );
    zdj_decode_layer_t * layer = state->first_layer;
    // printf( "_update_wait 1\n" );
    while( layer ) {
        // printf( "_update_wait 2\n" );
        layer->accum( layer, node );
        // printf( "_update_wait 3\n" );
        layer = layer->next;
        // printf( "_update_wait 4\n" );
    }
    
    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_UNCLAIMED;
    // printf( "decode _update_wait done\n" );
}

static void _deinit_state( zdj_pipeline_node_t * node ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    if( state->fmt_ctx ) { avformat_close_input( &state->fmt_ctx ); }
    if( state->codec_ctx ) { avcodec_free_context( &state->codec_ctx ); }
    if( state->fmt_ctx ) { avformat_free_context( state->fmt_ctx ); }
    if( state->out_buffer ) { free( state->out_buffer ); }
    // Release packet_layers
    if( state ) { node->state = NULL; free( state );  }
}

static zdj_error_type_t _move_window( zdj_pipeline_node_t * node, double offset ) {
    // printf( "decode _move_window\n" );
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_DECODE_WINDOW;

    // Move the head address by offset.
    state->offset_addr_by_transport_d_coord( node, &state->head, (double)offset );

    // Groom Existing Layers
    // --------------------
    zdj_decode_layer_t * layer = state->first_layer;
    while( layer ) {
        // Update each layer's buffer index coords
        layer->update_buf_coords_for_head( layer, node );
        // Re-fill layer packets after move
        layer->fill( layer, node );
        layer = layer->next;
    }

    // DEPRECATING
    // Add first layer if layers are empty
    // -----------------------------------
    if( !state->first_layer ) {
        printf( "===> Found Missing first_layer!\n" );
    }
    // DEPRECATING

    // Add Discon Layers
    // -----------------
    if( state->discon_is_active( node ) ) {

        zdj_decode_addr_t earliest_addr;
        state->get_earliest_core_addr( node, &earliest_addr );
        zdj_decode_addr_t latest_addr;
        state->get_latest_core_addr( node, &latest_addr );

        // Groom backwards from first layer, prepending layers until they extend beyond win start.
        while( state->win_contains_addr( node, &earliest_addr, ZDJ_ADDR_COORD_TRANSPORT ) ) {
            if( state->first_layer->back_discon.type != ZDJ_DECODE_DISCON_LOOP ) { continue; }
            // Make and offset an address for the new loop start
            zdj_decode_addr_t loop_start;
            state->first_layer->core_start.copy( &state->first_layer->core_start, &loop_start );
            int64_t loop_len = state->first_layer->back_discon.length;
            // Move the loop back in decode space by 1 loop length
            state->offset_addr_by_transport_d_coord( 
                node, 
                &loop_start, 
                (double)loop_len * -1
            );
            state->prepend_layer( node, zdj_new_decode_loop_layer( node, &loop_start, loop_len ) );
            // Update the earliest addr for next loop
            state->get_earliest_core_addr( node, &earliest_addr );
        }

        // Groom forward from new first layer, appending layers to fill window coords.
        while( state->win_contains_addr( node, &latest_addr, ZDJ_ADDR_COORD_TRANSPORT ) ) {
            // Add a new discon layer based on discon type of last layer
            if( state->last_layer->fwd_discon.type == ZDJ_DECODE_DISCON_LOOP ) {
                
                int64_t loop_len = state->last_layer->fwd_discon.length;
                zdj_decode_addr_t loop_start;
                state->last_layer->core_end.copy( &state->last_layer->core_end, &loop_start );
                state->append_layer( node, zdj_new_decode_loop_layer( node, &loop_start, loop_len ) );
            
            } if( state->last_layer->fwd_discon.type == ZDJ_DECODE_DISCON_SKIP ) {
                
                zdj_decode_addr_t skip_depart;
                // Note: This assumes the truncate has already happened.
                state->last_layer->core_end.copy( &state->last_layer->core_end, &skip_depart );
                zdj_decode_addr_t skip_dest;
                skip_depart.copy( &skip_depart, &skip_dest );
                // Offset only origin coord of dest's pcm addr.
                // TODO: Link deck controls skip state here.
                double skip_bg_len = 1.25f;
                state->offset_addr_origin_by_skip_bg( node, &skip_dest, skip_bg_len );
                state->append_layer( node, zdj_new_decode_skip_layer( node, &skip_depart, &skip_dest ) );
            }
            // Update latest addr for next loop
            state->get_latest_core_addr( node, &latest_addr );
        }
    }

    // Delete empty layers
    // -------------------
    layer = state->first_layer;
    while( layer ) {
        zdj_decode_layer_t * next_layer = layer->next;
        if ( state->can_remove_layer( layer ) ) { 
            state->remove_layer( node, layer );
        }
        layer = next_layer;
    }

    // Refresh the head's addr (including origin) since there may be a new layer
    // under the head after all the grooming above.
    state->set_addr_transport_d_coord( node, &state->head, state->head.transport_d );

    // printf( " decode _move_window done\n" );
    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_UNCLAIMED;
}

// DEPRECATING
static zdj_error_type_t _reset_window( zdj_pipeline_node_t * node, double address ) {
    printf( "======> WARNING!!! RESET WINDOW CALLED\n" );
    // If we're going to use this, we need to clear out the layer stack.
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    zdj_decode_init_addr( &state->head );
    state->set_addr_transport_d_coord( node, &state->head, address );
    // _move_window( node, address );
}
// DEPRECATING

static void _clear_out_buffer( zdj_pipeline_node_t * node ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    memset( state->out_buffer, 0, state->win_sample_count * 4 * sizeof( float ) );
}

static void _prepend_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    if( state->first_layer ) {
        layer->next = state->first_layer;
        state->first_layer->prev = layer;
        state->first_layer = layer;
    } else {
        state->first_layer = layer;
        state->last_layer = layer;
    }
}

static void _append_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    if( state->last_layer ) {
        layer->prev = state->last_layer;
        state->last_layer->next = layer;
        state->last_layer = layer;
    } else {
        state->last_layer = layer;
        state->first_layer = layer;
    }
}

static bool _can_remove_layer( zdj_decode_layer_t * layer ) {
    return layer->fwd_discon.type != ZDJ_DECODE_DISCON_NONE && layer->is_empty( layer );
}

static void _remove_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    // Stitch together surrounding layers + update node's first/last layer links
    if( layer->prev ) { layer->prev->next = layer->next; }
    if( layer->next ) { layer->next->prev = layer->prev; }
    if( layer == state->first_layer ) { state->first_layer = layer->next; }
    if( layer == state->last_layer ) { state->last_layer = layer->prev; }
    layer->deinit( layer );
}
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

static void _update_wait( zdj_pipeline_node_t * node );
static void _deinit_state( zdj_pipeline_node_t * node );
static void _clear_out_buffer( zdj_pipeline_node_t * node ); 

static void _install_ui_buffer( zdj_pipeline_node_t * node );

static zdj_error_type_t _move_window( zdj_pipeline_node_t * node, double offset );
static zdj_error_type_t _reset_window( zdj_pipeline_node_t * node, double address );

static void _refresh_layers( zdj_pipeline_node_t * node );
static void _prepend_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer );
static void _append_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer );
static bool _can_remove_layer( zdj_decode_layer_t * layer );
static void _remove_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer );
static void _remove_all_layers( zdj_pipeline_node_t * node );
static void _remove_all_layers_except( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer );

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
    state->song_pcm_duration = song->audio->duration_pcm;
    state->channel_count = song->audio->av_channel_count;
    state->win_back_sample_count = back_sample_count;
    state->win_fwd_sample_count = fwd_sample_count;
    state->win_sample_count = fwd_sample_count + back_sample_count;
    state->clear_out_buffer = &_clear_out_buffer;
    state->refresh_layers = &_refresh_layers;
    state->prepend_layer = &_prepend_layer;
    state->append_layer = &_append_layer;
    state->can_remove_layer = &_can_remove_layer;
    state->remove_layer = &_remove_layer;
    state->remove_all_layers_except = &_remove_all_layers_except;
    state->install_ui_buffer = &_install_ui_buffer;
    
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

    // printf( "code id: 0x%x\n", state->song->audio->av_codec_id );
    switch( state->song->audio->av_codec_id ) {
        case AV_CODEC_ID_MP3:
            state->estimated_packet_sample_count = 1152;
            state->av_timebase_factor = 320;
            state->requires_garbage = true;
            break;
        case AV_CODEC_ID_AAC:
            state->estimated_packet_sample_count = 511;
            state->av_timebase_factor = 1;
            state->requires_garbage = true;
            break;
        case AV_CODEC_ID_FLAC:
            state->estimated_packet_sample_count = 1152;
            state->av_timebase_factor = 1;
            state->requires_garbage = true;
            break;
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
            state->estimated_packet_sample_count = 1024;
            state->av_timebase_factor = 1;
            state->requires_garbage = false;
            break;
    }

    // Reset to initialize addresses
    node->reset_window( node, 0 );
    // Set to fill on empty
    state->refresh_mode = ZDJ_DECODE_REFRESH_CONTIGUOUS;

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
    
    state->clear_out_buffer( node );
    zdj_decode_layer_t * layer = state->first_layer;

    while( layer ) {
        zdj_perf_tag_t * move_tag;
        if ( zdj_perf_enabled( ) ) {
            move_tag = zdj_new_perf_tag_for_thread( ZDJ_SYSTEM_THREAD_DECK_AUDIO_CYCLE );
            move_tag->name = ZDJ_PERF_TAG_DECK_MOVE;
            move_tag->start = zdj_perf_time( );
        }
        layer->accum( layer, node ); 
        layer = layer->next;
        if ( zdj_perf_enabled( ) ) { move_tag->end = zdj_perf_time( ); }
    }

    // Fill ui buffer if requested
    if( state->ui_buffer_req ) {
        state->ui_buffer_req = false;
        memcpy( state->ui_buffer, state->out_buffer, state->win_sample_count * 4 * sizeof( float ) );
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
    if( state->ui_buffer ) { free( state->ui_buffer ); }
    // Release packet_layers
    if( state ) { node->state = NULL; free( state );  }
}

static zdj_error_type_t _move_window( zdj_pipeline_node_t * node, double offset ) {
    // printf( "decode _move_window: %1.3f\n", offset );
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_DECODE_WINDOW;

    // Move the head address by offset.
    state->offset_addr_by_transport_d_coord( node, &state->head, (double)offset, false );
    // Set head's buf coord.
    state->head.buf_d = floor( state->win_sample_count / 2 );
    state->head.buf_i = (int)state->head.buf_d;

}

static void _refresh_layers( zdj_pipeline_node_t * node ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    zdj_deck_control_loop_state_t * loop_state = (zdj_deck_control_loop_state_t*)state->loop_state;
    zdj_deck_control_skip_state_t * skip_state = (zdj_deck_control_skip_state_t*)state->skip_state;


    bool debug = false;

    // printf( "\n    vvv--- REFRESH LAYERS ---VVV\n\n" );
    int iter = 0;
    zdj_decode_layer_t * layer;

    if( !state->first_layer ) {
        // Respond to no-op on empty
        // ------------------------
        if( state->refresh_mode == ZDJ_DECODE_REFRESH_NOOP_ON_EMPTY ||
            state->refresh_mode == ZDJ_DECODE_REFRESH_NOOP_HYPERSCRUB 
        ) {
            // printf( "===> Empty decode + no-op on empty\n" );
            return;
        }

        // Respond to create contiguous on empty
        // ---------------------------------
        else if( state->refresh_mode == ZDJ_DECODE_REFRESH_CONTIGUOUS ) {
            // Create a new layer w/current head coords
            layer = zdj_new_decode_continuous_layer( node, &state->head );
            state->append_layer( node, layer );
        }

        // Respond to discon empty
        // ---------------------------------
        else if( state->refresh_mode == ZDJ_DECODE_REFRESH_DISCON_LOOP ) {
            printf( "empty node @discon: loop\n" );
            zdj_decode_addr_t loop_start;
            zdj_decode_init_addr( &loop_start );
            state->addr_for_transport_d_coord( node, &loop_start, state->head.transport_d );
            // Set ONLY the origin coords to the loop's start
            loop_start.origin_d = loop_state->start_origin_d;
            loop_start.origin_i = (int64_t)loop_start.origin_d;
            loop_start.origin_bg = zdj_signal_beatgrid_count_for_pcm_count( 
                loop_start.origin_d,
                state->song->audio->av_sample_rate,
                state->song->performance->bpm
            );
            state->prepend_layer( node, zdj_new_decode_loop_layer( node, &loop_start ) );
            state->first_layer->update_buf_coords_for_head( state->first_layer, node );
            state->first_layer->fill( state->first_layer, node );
        }
        else if( state->refresh_mode == ZDJ_DECODE_REFRESH_DISCON_SKIP ) {
            printf( "empty node @discon: skip\n" );
            zdj_decode_addr_t skip_start;
            zdj_decode_init_addr( &skip_start );
            state->addr_for_transport_d_coord( node, &skip_start, state->head.transport_d );
            // Set ONLY the origin coords to the loop's start
            skip_start.origin_d = skip_state->dest_origin_d;
            skip_start.origin_i = (int64_t)skip_start.origin_d;
            skip_start.origin_bg = zdj_signal_beatgrid_count_for_pcm_count( 
                skip_start.origin_d,
                state->song->audio->av_sample_rate,
                state->song->performance->bpm
            );
            state->prepend_layer( node, zdj_new_decode_continuous_layer( node, &skip_start ) );
            state->first_layer->update_buf_coords_for_head( state->first_layer, node );
            state->first_layer->fill( state->first_layer, node );
        }
    }

    // Untruncate/Retruncate Skip Layer under the head
    // -----------------------------------------------
    layer = state->get_layer_containing_core_addr( node, &state->head, ZDJ_ADDR_COORD_TRANSPORT );
    if ( layer && layer->back_discon_type == ZDJ_DECODE_LAYER_DISCON_TYPE_SKIP ) {
        
        if( debug ) { printf( ">>> Untruncate Skip\n" ); }

        state->remove_all_layers_except( node, layer );
        if( state->refresh_mode == ZDJ_DECODE_REFRESH_DISCON_LOOP ) {
            skip_state->locked = false;
            layer->retruncate_loop( 
                layer, node, loop_state->start_origin_d, loop_state->end_origin_d 
            );
            layer->fill( layer, node );
        } else if( state->refresh_mode == ZDJ_DECODE_REFRESH_DISCON_SKIP ) {
            skip_state->locked = false;
            layer->untruncate( layer, node );
            layer->fill( layer, node );
            state->refresh_mode = ZDJ_DECODE_REFRESH_CONTIGUOUS;
        }

        if( debug ) { printf( "<<< Untruncate Skip\n" ); }
    }

    // Groom Existing Layers
    // ---------------------

    if( debug ) { printf( ">>> Existing\n" ); }
    
    layer = state->first_layer;
    iter = 0;
    while( layer ) {
        // Update each layer's buffer index coords
        layer->update_buf_coords_for_head( layer, node );
        // Re-fill layer packets after move
        layer->fill( layer, node );
        layer = layer->next;

        if( iter++ > ZDJ_DECODE_MAX_ITER ) {
            printf( "HIT ITER LIMIT (Groom Existing Layers)\n" );
            break;
        }
    }

    if( debug ) { printf( "<<< Existing\n" ); }

    // If window doesn't contain any song origin coords, we're done.
    // -------------------------------------------------------------
    zdj_decode_addr_t win_start; state->get_win_start_addr( node, &win_start );
    zdj_decode_addr_t win_end; state->get_win_end_addr( node, &win_end );
    if( win_start.origin_d > state->song_pcm_duration ||
        win_end.origin_d < 0
    ) {
        printf( "Window outside song: [%1.0f - %1.0f]\n", win_start.origin_d, win_end.origin_d );
        return;
    }

    // Add Discon Layers
    // -----------------
    if( state->refresh_mode == ZDJ_DECODE_REFRESH_DISCON_LOOP ||
        state->refresh_mode == ZDJ_DECODE_REFRESH_DISCON_SKIP 
    ) {

        zdj_decode_addr_t earliest_addr;
        state->get_earliest_core_addr( node, &earliest_addr );
        zdj_decode_addr_t latest_addr;
        state->get_latest_core_addr( node, &latest_addr );

        // BACK FILL 
        // ---------
        // Groom backwards from first layer, prepending layers until they extend beyond win start.
        iter = 0;
        while( state->win_contains_addr( node, &earliest_addr, ZDJ_ADDR_COORD_TRANSPORT ) ) {
            
            if( debug ) { printf( ">>> Add Discon Back" ); }

            // Steady-state Loop mode (back fill)
            // ----------------------------------
            if( state->first_layer->back_discon_type == ZDJ_DECODE_LAYER_DISCON_TYPE_LOOP ) {
                
                if( debug ) { printf( " (Loop)\n" ); }

                // Make and offset an address for the new loop start
                zdj_decode_addr_t loop_start;
                state->first_layer->core_start.copy( &state->first_layer->core_start, &loop_start );
                // Move the entire loop start addr back in decode space by 1 loop length
                zdj_deck_control_loop_state_t * loop_state = (zdj_deck_control_loop_state_t*)state->loop_state;
                state->offset_addr_by_transport_d_coord( 
                    node, 
                    &loop_start, 
                    (double)loop_state->pcm_len * -1,
                    false
                );
                // Set ONLY the origin coords to the loop's start
                loop_start.origin_d = loop_state->start_origin_d;
                loop_start.origin_i = (int64_t)loop_start.origin_d;
                loop_start.origin_bg = zdj_signal_beatgrid_count_for_pcm_count( 
                    loop_start.origin_d,
                    state->song->audio->av_sample_rate,
                    state->song->performance->bpm
                );
                state->prepend_layer( node, zdj_new_decode_loop_layer( node, &loop_start ) );
                state->first_layer->update_buf_coords_for_head( state->first_layer, node );
                state->first_layer->fill( state->first_layer, node );
            } 
            
            // Update earliest addr for next loop
            state->get_earliest_lead_in_addr( node, &earliest_addr );

            // Only allow a handfull of iterations so we don't spend too much time here
            if( iter++ > 10 ) { printf( "HIT ITER LIMIT (Add Back Discon Layers)\n" ); break; }
            
            if( debug ) { printf( "<<< Add Discon Back\n" ); }
        }     

        // FWD FILL
        // --------
        // Groom forward from last layer, appending layers to fill window coords.
        iter = 0;
        while( state->win_contains_addr( node, &latest_addr, ZDJ_ADDR_COORD_TRANSPORT ) ) {
            // Add a new discon layer based on discon type of last layer
            zdj_decode_addr_t layer_start;
            zdj_decode_init_addr( &layer_start );

            if( debug ) { printf( ">>> Add Discon Fwd" ); }

            // Steady-state Loop mode (fwd fill)
            // ---------------------------------
            if( state->last_layer->fwd_discon_type == ZDJ_DECODE_LAYER_DISCON_TYPE_LOOP ) {

                if( debug ) { printf( " (Loop)\n" ); }

                // Copy the last layer's end addr into the new layer's start addr
                state->last_layer->core_end.copy( &state->last_layer->core_end, &layer_start );
                // Update origin coords per-discon request
                layer_start.origin_d = loop_state->start_origin_d;
                layer_start.origin_i = (int64_t)layer_start.origin_d;
                layer_start.origin_bg = zdj_signal_beatgrid_count_for_pcm_count( 
                    layer_start.origin_d,
                    state->song->audio->av_sample_rate,
                    state->song->performance->bpm
                );
                
                state->append_layer( node, zdj_new_decode_loop_layer( node, &layer_start ) );
                state->last_layer->update_buf_coords_for_head( state->last_layer, node );
                state->last_layer->fill( state->last_layer, node );


            // One-shot Skip modes (fwd fill)
            // ------------------------------
            } else if( state->last_layer->fwd_discon_type == ZDJ_DECODE_LAYER_DISCON_TYPE_SKIP ) {
                // Skip-in-Loop / Skip-to-loop
                // ---------------------------
                if( state->refresh_mode == ZDJ_DECODE_REFRESH_DISCON_LOOP ) {

                    if( debug ) { printf( " (Skip in Loop)\n" ); }

                    // Copy the last layer's end addr into the new layer's start addr
                    state->last_layer->core_end.copy( &state->last_layer->core_end, &layer_start );
                    // Update origin coords per-discon request
                    layer_start.origin_d = skip_state->dest_origin_d;
                    layer_start.origin_i = (int64_t)layer_start.origin_d;
                    layer_start.origin_bg = zdj_signal_beatgrid_count_for_pcm_count( 
                        layer_start.origin_d,
                        state->song->audio->av_sample_rate,
                        state->song->performance->bpm
                    );
                    
                    state->append_layer( node, zdj_new_decode_loop_layer( node, &layer_start ) );
                    state->last_layer->back_discon_type = ZDJ_DECODE_LAYER_DISCON_TYPE_SKIP;
                    state->last_layer->update_buf_coords_for_head( state->last_layer, node );
                    state->last_layer->fill( state->last_layer, node );

                // Skip in contig. layer
                // ---------------------
                } else if( state->refresh_mode == ZDJ_DECODE_REFRESH_DISCON_SKIP ) {

                    if( debug ) { printf( " (Skip)\n" ); }

                    // Copy the last layer's end addr into the new layer's start addr
                    state->last_layer->core_end.copy( &state->last_layer->core_end, &layer_start );
                    // Update origin coords per-discon request
                    layer_start.origin_d = skip_state->dest_origin_d;
                    layer_start.origin_i = (int64_t)layer_start.origin_d;
                    layer_start.origin_bg = zdj_signal_beatgrid_count_for_pcm_count( 
                        layer_start.origin_d,
                        state->song->audio->av_sample_rate,
                        state->song->performance->bpm
                    );
                    
                    state->append_layer( node, zdj_new_decode_skip_layer( node, &layer_start ) );
                    state->last_layer->back_discon_type = ZDJ_DECODE_LAYER_DISCON_TYPE_SKIP;
                    state->last_layer->update_buf_coords_for_head( state->last_layer, node );
                    state->last_layer->fill( state->last_layer, node );
                } 
            }

            // Only allow a handfull of iterations so we don't spend too much time here
            if( iter++ > 10 ) { printf( "HIT ITER LIMIT (Add Fwd Discon Layers)\n" ); break; }

            // Update latest addr for next loop
            state->get_latest_lead_out_addr( node, &latest_addr );

            if( debug ) { printf( "<<< Add Discon Fwd\n" ); }
        }
    }


    // Delete empty layers
    // -------------------
    layer = state->first_layer;
    iter = 0;
    while( layer ) {
        zdj_decode_layer_t * next_layer = layer->next;
        if ( state->can_remove_layer( layer ) ) { 
            if( debug ) { printf( ">>> Delete Empty: %p\n", layer ); }
            state->remove_layer( node, layer );
        }
        layer = next_layer;

        if( iter++ > ZDJ_DECODE_MAX_ITER ) {
            printf( "HIT ITER LIMIT (Delete Empty Layers)\n" );
            break;
        }
    }

    // Refresh the head's addr (including origin) since there may be a new layer
    // under the head after all the grooming above.
    state->set_addr_transport_d_coord( node, &state->head, state->head.transport_d );
    // Set head's buf coord.
    state->head.buf_d = floor( state->win_sample_count / 2 );
    state->head.buf_i = (int)state->head.buf_d;

    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_UNCLAIMED;

    // printf( "_refresh_layers done\n" );
}

static zdj_error_type_t _reset_window( zdj_pipeline_node_t * node, double origin_coord ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    zdj_decode_init_addr( &state->head );
    state->head.origin_d = origin_coord;
    state->head.origin_i = (int64_t)origin_coord;

    zdj_decode_layer_t * layer = state->first_layer;
    int iter = 0;
    while( layer ) {
        zdj_decode_layer_t * next_layer = layer->next;
        state->remove_layer( node, layer );
        layer = next_layer;

        if( iter++ > ZDJ_DECODE_MAX_ITER ) {
            printf( "HIT ITER LIMIT (reset_window)\n" );
            break;
        }
    }
    state->first_layer = NULL;
    state->last_layer = NULL;

    // printf( "decode _reset_window addr:%1.0f head t:%1.0fo:%1.0f\n", origin_coord, state->head.transport_d, state->head.origin_d );

    // layer = zdj_new_decode_continuous_layer( node, &state->head );
    // state->append_layer( node, layer );
    // // printf( "reset_window filling layer: tp:%1.0f->%1.0f o:%1.0f->%1.0f\n", 
    // //     layer->core_start.transport_d, layer->core_end.transport_d,
    // //     layer->core_start.origin_d, layer->core_end.origin_d 
    // // );
    // layer->fill( layer, node );
}

static void _clear_out_buffer( zdj_pipeline_node_t * node ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    memset( state->out_buffer, 0, state->win_sample_count * 4 * sizeof( float ) );
}

static void _install_ui_buffer( zdj_pipeline_node_t * node ) {
    printf( "_install_ui_buffer: %p\n", node );
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    state->ui_buffer = calloc( state->win_sample_count * 4, sizeof( float ) );
    state->ui_buffer_req = true;
}

static void _prepend_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    if( state->first_layer ) {
        layer->next = state->first_layer;
        state->first_layer->prev = layer;
        state->first_layer = layer;
    } else if( !state->first_layer && !state->last_layer ) {
        state->last_layer = layer;
        state->first_layer = layer;
        layer->next = NULL;
        layer->prev = NULL;
    } else if( !state->first_layer && state->last_layer ) {
        printf( "empty first layer!!!\n" );
    }
}

static void _append_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    if( state->last_layer ) {
        layer->prev = state->last_layer;
        state->last_layer->next = layer;
        state->last_layer = layer;
    } else if( !state->first_layer && !state->last_layer ) {
        state->last_layer = layer;
        state->first_layer = layer;
        layer->next = NULL;
        layer->prev = NULL;
    } else if( state->first_layer && !state->last_layer ) {
        printf( "empty last layer!!!\n" );
    }
}

static bool _can_remove_layer( zdj_decode_layer_t * layer ) {
    return layer->is_empty( layer );
}

static void _remove_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    // Stitch together surrounding layers + update node's first/last layer links
    if( layer->prev ) { layer->prev->next = layer->next; }
    if( layer->next ) { layer->next->prev = layer->prev; }
    if( layer == state->first_layer ) { state->first_layer = layer->next; }
    if( layer == state->last_layer ) { state->last_layer = layer->prev; }
    if( state->first_layer && !state->last_layer ) { state->last_layer = state->first_layer; }
    if( !state->first_layer && state->last_layer ) { state->first_layer = state->last_layer; }
    layer->deinit( layer );
}

static void _remove_all_layers_except( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer ) {
    // printf( "XXX RMV ALL XCPT: %p\n", layer );
    if( !layer ){ return; }
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)node->state;

    zdj_decode_layer_t * prev_layer = layer->prev;
    int iter = 0;
    while( prev_layer ) {
        zdj_decode_layer_t * new_prev_layer = prev_layer->prev;
        if ( decode_state->can_remove_layer( prev_layer ) ) { 
            decode_state->remove_layer( node, prev_layer );
        }
        prev_layer = new_prev_layer;

        if( iter++ > ZDJ_DECODE_MAX_ITER ) {
            printf( "HIT ITER LIMIT (remove_all_layers_except)\n" );
            break;
        }
    }
    iter = 0;
    zdj_decode_layer_t * next_layer = layer->next;
    while( next_layer ) {
        zdj_decode_layer_t * new_next_layer = next_layer->next;
        if ( decode_state->can_remove_layer( next_layer ) ) { 
            decode_state->remove_layer( node, next_layer );
        }
        next_layer = new_next_layer;

        if( iter++ > ZDJ_DECODE_MAX_ITER ) {
            printf( "HIT ITER LIMIT (remove_all_layers_except)\n" );
            break;
        }
    }
    decode_state->first_layer = layer;
    decode_state->last_layer = layer;
}

// double zdj_decode_quantize_addr_to_beatgrid( 
double zdj_decode_guantize_origin_d_for_beatgrid(
    double origin_d, 
    double quant_val,
    zdj_library_song_t * song,
    zdj_decode_quantize_type_t type
) {
    if( song->performance && 
        song->performance->has_beat_grid && 
        song->performance->bpm > 0.0 
    ) {
        double bpm = song->performance->bpm;
        int rate = song->audio->av_sample_rate;
        double raw_bg = zdj_signal_beatgrid_count_for_pcm_count( origin_d, rate, bpm );
        
        double quant_bg;
        switch ( type ) {
            case ZDJ_DECODE_QUANTIZE_ROUND: quant_bg = round( raw_bg / quant_val ) * quant_val; break;
            case ZDJ_DECODE_QUANTIZE_CEIL: quant_bg = ceil( raw_bg / quant_val ) * quant_val; break;
            case ZDJ_DECODE_QUANTIZE_FLOOR: quant_bg = floor( raw_bg / quant_val ) * quant_val; break;
        }
        return zdj_signal_pcm_count_for_beatgrid_count( quant_bg, bpm, rate );

    } else {
        return origin_d;
    }
}
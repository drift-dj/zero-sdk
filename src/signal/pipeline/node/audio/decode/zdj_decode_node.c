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

static zdj_error_type_t _move_window( zdj_pipeline_node_t * node, int offset );
static zdj_error_type_t _reset_window( zdj_pipeline_node_t * node, uint32_t address );
static zdj_error_type_t _resize_window( 
    zdj_pipeline_node_t * node, 
    uint32_t back_infill_targ, 
    uint32_t fwd_infill_targ 
);

// static void _deinit_packet( zdj_decode_packet_t * packet );


static int64_t _calculate_preceeding_init_map_pcm( zdj_decode_layer_t * layer );
static int64_t _calculate_following_init_map_pcm( zdj_decode_layer_t * layer );
static int64_t _calculate_preceeding_init_map_decode( zdj_decode_layer_t * layer );
static int64_t _calculate_following_init_map_decode( zdj_decode_layer_t * layer );

zdj_pipeline_node_t * zdj_new_decode_node( 
    zdj_library_song_t * song,
    uint64_t address,
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
    state->win_back_sample_count = back_sample_count;
    state->win_fwd_sample_count = fwd_sample_count;
    state->win_sample_count = back_sample_count + fwd_sample_count;
    state->channel_count = state->song->audio->av_channel_count;
    state->song_pcm_duration = state->song->audio->duration;
    state->win_head_sample = back_sample_count;

    state->head_decode_addr = 0;
    state->head_pcm_addr = 0;

    // Alloc output buffer - sample_count * 4 is a bit of a mystery - malloc error if it's less. 
    state->out_buffer = calloc( state->win_sample_count * 4, sizeof( float ) );

    char filepath[ 512 ];
    strcpy( filepath, song->audio->filepath );

    av_log_set_level( AV_LOG_QUIET );

    // Use the open command to build the Format Context
    state->fmt_ctx = avformat_alloc_context( );
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
    res = avcodec_open2( state->codec_ctx, codec, NULL );

    switch( state->song->audio->av_codec_id ) {
        case AV_CODEC_ID_MP3:
            state->add_packet_after_layer = &zdj_decode_add_packet_after_mp3_layer;
            state->add_packet_before_layer = &zdj_decode_add_packet_before_mp3_layer;
            state->add_packet_to_empty_layer = &zdj_decode_add_packet_to_empty_mp3_layer;
            break;
        case AV_CODEC_ID_AAC:
            // state->add_packet_after_layer = &zdj_decode_add_packet_after_aac_layer;
            // state->add_packet_before_layer = &zdj_decode_add_packet_before_aac_layer;
            // state->add_packet_to_empty_layer = &zdj_decode_add_packet_to_empty_aac_layer;
            // break;
        case AV_CODEC_ID_FLAC:
            // state->add_packet_after_layer = &zdj_decode_add_packet_after_flac_layer;
            // state->add_packet_before_layer = &zdj_decode_add_packet_before_flac_layer;
            // state->add_packet_to_empty_layer = &zdj_decode_add_packet_to_empty_flac_layer;
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
            state->add_packet_after_layer = &zdj_decode_add_packet_after_pcm_layer;
            state->add_packet_before_layer = &zdj_decode_add_packet_before_pcm_layer;
            state->add_packet_to_empty_layer = &zdj_decode_add_packet_to_empty_pcm_layer;
            break;
    }

    // Do inert window move to trigger pre-fill
    node->reset_window( node, 0 );

    // // Debug dump format
    // av_dump_format( state->fmt_ctx, 0, song->audio->filepath, 0 );
    // printf( "zdj_new_decode_node done\n" );

    return node;
}

float _dn_ts1_p = 0;
float _dn_ts1_f = 945;
float _dn_ts2_p = 0;
float _dn_ts2_f = 300;

// Execute a copy/accumulate of all of node's discon_seqs to the out_buffer.
static void _update_wait( zdj_pipeline_node_t * node ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    zdj_decode_layer_t * layer = state->first_layer;

    // printf( " decode _update_wait\n" );

    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_DECK_UPDATE;
    // Clear out_buffer - count is empirical to avoid crash.
    // Figure out why * 4 is needed.  Don't alter without testing.
    memset( state->out_buffer, 0, state->win_sample_count * 4 * sizeof( float ) );
    
    int layer_count = 0;
    int packet_count = 0;
    int accum_tally = 0;
    // Run thru packet_layers, mapping packet samples thru accumulator to out_buffer.
    while( layer ) {
        // printf( "layer\n" );
        layer_count++;
        zdj_decode_packet_t * packet = layer->first_packet;
        while( packet ) {
            // printf( "packet\n" );
            packet_count++;
            if( packet->type == ZDJ_DECODE_PACKET_TYPE_NORMAL && state->accum ) { 
                accum_tally += state->accum( node, packet ); 
            }
            packet = packet->next;
        }
        layer = layer->next;
    }

    state->debug_layer_count = layer_count;
    state->debug_packet_count = packet_count;
    state->debug_accum_tally = accum_tally;

    // Write test sine to out_buffer
    // _dn_ts1_p = zdj_signal_gen_sine( 
    //     _dn_ts1_f, 
    //     _dn_ts1_p, 
    //     state->win_sample_count,
    //     state->out_buffer,
    //     state->channel_count,
    //     0,
    //     3000.0
    // );
    // _dn_ts2_p = zdj_signal_gen_sine( 
    //     _dn_ts2_f, 
    //     _dn_ts2_p, 
    //     state->win_sample_count,
    //     state->out_buffer,
    //     state->channel_count,
    //     1,
    //     3000.0
    // );
    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_UNCLAIMED;
    // printf( " decode _update_wait done\n" );
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

static zdj_error_type_t _move_window( zdj_pipeline_node_t * node, int offset ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_DECODE_WINDOW;
    // Update the decode-space address/window
    state->head_decode_addr += offset;
    state->head_win_end = state->head_decode_addr + state->win_fwd_sample_count;
    state->head_win_start = state->head_decode_addr - state->win_back_sample_count;
    
    // printf( "_move_window %d to: [%ld,%ld]\n", offset, state->head_win_end, state->head_win_start );

    // Build sample bounds
    // --------------------
    if( state->first_layer ) { state->earliest_core_sample = state->first_layer->earliest_core_sample; }
    if( state->last_layer ) { state->latest_core_sample = state->last_layer->latest_core_sample; }

    // Fill existing layers
    // --------------------
    zdj_decode_layer_t * layer = state->first_layer;
    while( layer ) {
        // printf( "filling existing layer: %p\n", layer );
        // Groom layer to window
        zdj_decode_fill_layer( node, layer );
        layer = layer->next;
    }

    // Add first layer if layers are empty
    // -----------------------------------
    if( !state->first_layer ) {
        printf( "adding initial layer\n" );
        // printf( "state->earliest_core_sample: %ld\n", state->earliest_core_sample );
        zdj_decode_layer_t * layer = zdj_new_decode_layer( 0, 0 );
        state->first_layer = layer;
        state->last_layer = layer;
        zdj_decode_layer_reset_discon( layer, state->song_pcm_duration );
        zdj_decode_fill_layer( node, layer );
    }

    // Add new layers if discon state requires
    // ---------------------------------------
    // Groom backwards from first layer, prepending layers to fill window coords.
    while( state->earliest_core_sample > state->head_win_start ) {
        // Add a new discon layer based on discon type of last layer
        if( state->first_layer->back_discon.type == ZDJ_DECODE_DISCON_LOOP ) {
            int64_t loop_len = zdj_decode_discon_loop_length( &state->first_layer->back_discon );
            printf( "adding loop layer: %ld, %ld, %ld\n", 
                state->first_layer->back_discon.depart_decode_addr,
                loop_len,
                state->first_layer->back_discon.depart_decode_addr - loop_len
            );
            zdj_decode_add_loop_layer( 
                node,
                state->first_layer->init_map_decode - loop_len, 
                state->first_layer->init_map_pcm, 
                state->first_layer->back_discon.depart_decode_addr - loop_len,
                state->first_layer->back_discon.depart_pcm_addr,
                loop_len
            );
        }
    }

    // Groom forward from new first layer, appending layers to fill window coords.
    while( state->latest_core_sample < state->head_win_end ) {
        // Add a new discon layer based on discon type of last layer
        if( state->last_layer->fwd_discon.type == ZDJ_DECODE_DISCON_LOOP ) {
            int64_t loop_len = zdj_decode_discon_loop_length( &state->last_layer->fwd_discon );
            zdj_decode_add_loop_layer( 
                node,
                state->last_layer->init_map_decode + loop_len, 
                state->last_layer->init_map_pcm, 
                state->last_layer->fwd_discon.depart_decode_addr,
                state->last_layer->fwd_discon.depart_pcm_addr,
                loop_len
            );
        } if( state->last_layer->fwd_discon.type == ZDJ_DECODE_DISCON_SKIP ) {
            zdj_decode_add_skip_layer( 
                node,
                state->last_layer->fwd_discon.depart_decode_addr,
                state->last_layer->fwd_discon.depart_pcm_addr,
                zdj_decode_discon_skip_length( &state->last_layer->fwd_discon )
            );
        }
    }

    // Delete empty layers
    // -------------------
    layer = state->first_layer;
    while( layer ) {
        if( layer->first_packet ) { layer = layer->next; continue; }
        zdj_decode_layer_t * next_layer = layer->next;
        // Stitch together surrounding layers
        if( layer->prev ) { layer->prev->next = layer->next; }
        if( layer->next ) { layer->next->prev = layer->prev; }
        if( layer == state->first_layer ) { state->first_layer = layer->next; }
        if( layer == state->last_layer ) { state->last_layer = layer->prev; }
        if( layer->first_packet == NULL ) { zdj_decode_deinit_layer( layer ); }
        layer = next_layer;
    }

    // Re-build sample bounds
    // ----------------------
    if( state->first_layer ) { state->earliest_core_sample = state->first_layer->earliest_core_sample; }
    if( state->last_layer ) { state->latest_core_sample = state->last_layer->latest_core_sample; }

    // Update the head pcm address based on packet_under_head
    // ------------------------------------------------------
    if( state->first_layer ) {
        zdj_decode_layer_t * layer = zdj_decode_get_layer_under_head( node );
        zdj_decode_packet_t * packet = zdj_decode_get_packet_under_head( node, layer );
        if( layer && packet ) {
            // Take offset from decode head to packet decode start addr.
            int64_t head_offset = state->head_decode_addr - packet->packet_decode_addr;
            // Offset the packet's pcm start addr to get head pcm addr.
            state->head_pcm_addr = packet->packet_pcm_addr + head_offset;
            // printf( "%ld - %ld = %ld, %ld: %ld\n", 
            //     state->head_decode_addr, 
            //     packet->packet_decode_addr,
            //     head_offset,
            //     packet->packet_pcm_addr,
            //     state->head_pcm_addr
            // );
        }
    }

    // printf( "earliest pkt: %ld latest pkt: %ld\n", state->earliest_core_sample, state->latest_core_sample );

    // printf( "_move_window done\n" );
    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_UNCLAIMED;
}

static zdj_error_type_t _reset_window( zdj_pipeline_node_t * node, uint32_t address ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    // Update the decode-space address
    state->head_decode_addr = 0;

    // Update the song pcm-space address
    state->head_pcm_addr = address;

    // Trigger a refill of the packet layer stack
    _move_window( node, address );
}

static zdj_error_type_t _resize_window( 
    zdj_pipeline_node_t * node, 
    uint32_t back_infill_targ, 
    uint32_t fwd_infill_targ 
) {

}

int64_t zdj_decode_get_pcm_addr_for_decode_addr( zdj_pipeline_node_t * node, int64_t decode_address ) {
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;
    // Get offset from decode_node pcm addr to decode_node decode addr
    int64_t pcm_offset = node_state->head_pcm_addr - node_state->head_decode_addr;
    // Get offset from requested address to decode_node decode addr
    // int64_t address_offset = pcm_offset + decode_address - node_state->head_decode_addr;
    int64_t address_offset = pcm_offset + node_state->head_decode_addr;

    printf( "zdj_decode_get_pcm_addr_for_decode_addr( %ld ): %ld - %ld = %ld ... %ld\n", 
        decode_address, 
        node_state->head_pcm_addr,
        node_state->head_decode_addr,
        pcm_offset, 
        address_offset );
    // Total everything 
    // return node_state->head_pcm_addr - address_offset;
    // return pcm_offset;
    return address_offset;
}
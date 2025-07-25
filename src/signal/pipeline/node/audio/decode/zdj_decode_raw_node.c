#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavcodec/codec_id.h>

#include <zerodj/error/zdj_error.h>
#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_raw_node.h>
#include <zerodj/signal/pipeline/node/file/zdj_file_read_node.h>

#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

static void _zdj_decode_raw_node_update_wait( zdj_pipeline_node_t * node );
static void _zdj_decode_raw_node_deinit_state( zdj_pipeline_node_t * node );
static zdj_error_type_t _zdj_decode_raw_node_init_parsed_decoder( zdj_pipeline_node_t * node );
static zdj_error_type_t _zdj_decode_raw_node_init_pcm_decoder( zdj_pipeline_node_t * node );
static zdj_error_type_t _zdj_decode_raw_node_decode_parsed( zdj_decode_raw_data_frame_t * frame );
static zdj_error_type_t _zdj_decode_raw_node_decode_pcm( zdj_decode_raw_data_frame_t * frame );

zdj_pipeline_node_t * zdj_new_decode_raw_node( 
    zdj_library_song_t * song, 
    uint32_t address, 
    size_t frame_count 
) {
    zdj_pipeline_node_t * node = zdj_new_pipeline_node( );
    node->deinit_state = &_zdj_decode_raw_node_deinit_state;
    node->update_wait = &_zdj_decode_raw_node_update_wait;

    // Set up state
    zdj_decode_raw_node_state_t * state = calloc( 1, sizeof( zdj_decode_raw_node_state_t ) );
    node->state = state;
    state->song = song;

    // Set up the window based on frame_count
    // zdj_pipeline_window_state_resize( node->window_state, 0, frame_count );

    // Init decode system based on codec type
    if( zdj_library_audio_is_raw_pcm( song->audio ) ) {
        state->decode = &_zdj_decode_raw_node_decode_pcm;
        if( _zdj_decode_raw_node_init_pcm_decoder( node ) != ZDJ_ERROR_OKAY ) {
            // decode_node should enter fail state here.
            // Song DTO needs more detail than has_error to make this happen.
            song->has_error = true;
        }
    } else {
        state->decode = &_zdj_decode_raw_node_decode_parsed;
        if( _zdj_decode_raw_node_init_parsed_decoder( node ) != ZDJ_ERROR_OKAY ) {
            // decode_node should enter fail state here.
            // Song DTO needs more detail than has_error to make this happen.
            song->has_error = true;
        }
    }

    // Create file_file_node for path
    state->file_node = zdj_new_file_read_node( song->audio->filepath, 2048, sizeof( char ), 0 );
    state->file_node->open( state->file_node );

    return node;
}

void _zdj_decode_raw_node_update_wait( zdj_pipeline_node_t * node ) {
    zdj_decode_raw_node_state_t * state = (zdj_decode_raw_node_state_t*)node->state;

    // If there's an active request...
    if( state->req_active ) {
        // Figure out if new frames are needed...
        if( state->req_song_samp_addr < state->first_frame->song_samp_addr ) {
            // Back infill.
            // Calculate seek target to give us a few pre-decoded frames
            // which will be discarded. Number based on encoding type.

        } else if ( 
            state->req_song_samp_addr > (state->last_frame->song_samp_addr+state->last_frame->buf_end_index) ||
            (state->req_song_samp_addr+state->req_samp_count) > (state->last_frame->song_samp_addr+state->last_frame->buf_end_index)
        ) {
            // Forward fill.
            // Loop, creating frames to be decoded.

        }

        // Run through all un-decoded frames...
        zdj_decode_raw_data_frame_t * frame = state->first_frame;
        while( frame ) {
            if( !frame->is_decoded ) {
                // Check if file_read_node's window includes the requested song_samp_addrs.
                // bool file_node_has_data = zdj_pipeline_window_contains_address_range( 
                //     state->file_node->window_state, 
                //     frame->req_song_samp_addr, 
                //     state->req_song_samp_addr+state->req_samp_count 
                // );
                // Move file read window and update if needed.
                // if( !file_node_has_data ) {
                    // If frame's file_read_node window doesn't include all samples,
                    // move file_read_node's sample window.
                    // int offset = state->req_song_samp_addr - state->file_node->window_state->ext_ref_addr - state->file_node->window_state->back_len;
                    // state->file_node->move_window( state->file_node, offset );
                    // state->file_node->update_wait( state->file_node );
                // }
                // Run node's decode flow.
                state->decode( frame );
                frame->is_decoded = true;
            }
            frame = frame->next;
        }

        // memcpy data from frames to destination address.
        
        
        state->req_active = false;
    }
}

zdj_error_type_t zdj_decode_raw_node_request_samples( 
    zdj_pipeline_node_t * node,
    uint32_t req_song_samp_addr,
    size_t req_samp_count,
    size_t req_samp_size,
    int req_ch_count,
    void * req_dest_addr
) {
    zdj_decode_raw_node_state_t * state = (zdj_decode_raw_node_state_t*)node->state;

    state->req_active = true;
    state->req_song_samp_addr = req_song_samp_addr;
    state->req_samp_count = req_samp_count;
    state->req_samp_size = req_samp_size;
    state->req_ch_count = req_ch_count;
    state->req_dest_addr = req_dest_addr;

    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t _zdj_decode_raw_node_init_parsed_decoder( zdj_pipeline_node_t * node ) {
    zdj_decode_raw_node_state_t * state = (zdj_decode_raw_node_state_t*)node->state;

    // Find codec
    state->codec = avcodec_find_decoder( state->song->audio->av_codec_id );
    if ( !state->codec ) { 
        return ZDJ_ERROR_BAD_AUDIO_ENCODING;
    }

    // Set up parser
    state->parser = av_parser_init( state->codec->id );
    if ( !state->parser ) {
        return ZDJ_ERROR_BAD_AUDIO_ENCODING;
    }

    // Set up codec context
    state->codec_context = avcodec_alloc_context3( state->codec );
    if ( !state->codec_context ) {
        av_parser_close( state->parser );
        return ZDJ_ERROR_BAD_AUDIO_ENCODING;
    }

    // Open the codec
    if ( avcodec_open2( state->codec_context, state->codec, NULL ) < 0 ) {
        avcodec_free_context( &state->codec_context );
        av_parser_close( state->parser );
        return ZDJ_ERROR_BAD_AUDIO_ENCODING;
    }

    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t _zdj_decode_raw_node_init_pcm_decoder( zdj_pipeline_node_t * node ) {

}

zdj_error_type_t _zdj_decode_raw_node_decode_parsed( zdj_decode_raw_data_frame_t * frame ) {

}

zdj_error_type_t _zdj_decode_raw_node_decode_pcm( zdj_decode_raw_data_frame_t * frame ) {

}

void _zdj_decode_raw_node_deinit_state( zdj_pipeline_node_t * node ) {
    // Teardown codec resources
    // Teardown file read node
}
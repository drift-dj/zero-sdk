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
#include <zerodj/signal/pipeline/node/audio/library_decode/zdj_library_decode_node.h>

static void _deinit_state( zdj_pipeline_node_t * node );
static void _update_wait( zdj_pipeline_node_t * node );
static int _decode( zdj_pipeline_node_t * node, AVPacket * av_packet, AVFrame * av_frame );


zdj_pipeline_node_t * zdj_new_library_decode_node( zdj_library_song_t * song ) {
    // printf( "zdj_new_library_decode_node\n" );
    // Make node
    zdj_pipeline_node_t * node = zdj_new_pipeline_node( );
    node->deinit_state = &_deinit_state;
    node->update_wait = &_update_wait;

    // Add state
    zdj_library_decode_node_state_t * state = calloc( 1, sizeof( zdj_library_decode_node_state_t ) );
    node->state = state;
    state->song = song;
    state->progress = 0.0f;
    state->done = false;
    state->available_samples = 0;
    state->channel_count = song->audio->av_channel_count;

    state->out_buffer = calloc( 10000, sizeof( float ) );

    // av_log_set_level( AV_LOG_QUIET );

    // Use the open command to build the Format Context
    state->fmt_ctx = avformat_alloc_context( );
    int res = avformat_open_input( &state->fmt_ctx, song->audio->filepath, NULL, NULL );
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

    return node;
}

static void _update_wait( zdj_pipeline_node_t * node ) {
    // printf( "_update_wait\n" );
    zdj_library_decode_node_state_t * state = (zdj_library_decode_node_state_t*)node->state;

    // Decode a bunch of packets into the buffer and count 'em.
    int decode_count = 0;
    int channel_count = state->channel_count;

    // Fill the new packet's av_frame with the next chunk of samples
    AVPacket * av_packet = av_packet_alloc( );
    AVFrame * av_frame = av_frame_alloc( );

    decode_count = _decode( node, av_packet, av_frame );

    state->available_samples = decode_count;

    if( decode_count == 0 ) { 
        state->song->analysis_state = ZDJ_LIBRARY_ANALYSIS_STATE_DONE; 
    } else if( decode_count < 0 ) {
        state->song->audio->has_libav_error = true;
        state->song->has_error = true;
        state->song->analysis_state = ZDJ_LIBRARY_ANALYSIS_STATE_DONE; 
    }

    state->pcm_addr += decode_count;
    state->song->analysis_progress = ((float)state->pcm_addr / (float)state->song->audio->av_sample_rate) / (float)state->song->audio->duration;

    int16_t * frame_buf_s16;
    int32_t * frame_buf_s32;
    float * frame_buf_flt;
    switch ( av_frame->format ) {
        case AV_SAMPLE_FMT_S16: 
            frame_buf_s16 = (int16_t*)av_frame->data[ 0 ];
            for( int i=0; i<decode_count; i++ ){
                state->out_buffer[ i*channel_count ] = frame_buf_s16[ i*channel_count ];
                if( channel_count == 2 ) { 
                    state->out_buffer[ (i*channel_count)+1 ] = frame_buf_s16[ (i*channel_count)+1 ]; 
                }
            }
            break;
        case AV_SAMPLE_FMT_S32: 
            frame_buf_s32 = (int32_t*)av_frame->data[ 0 ];
            for( int i=0; i<decode_count; i++ ){
                state->out_buffer[ i*channel_count ] = frame_buf_s32[ i*channel_count ];
                if( channel_count == 2 ) { 
                    state->out_buffer[ (i*channel_count)+1 ] = frame_buf_s32[ (i*channel_count)+1 ]; 
                }
            }
            break;
        case AV_SAMPLE_FMT_FLT:
            frame_buf_flt = (float*)av_frame->data[ 0 ];
            for( int i=0; i<decode_count; i++ ){
                state->out_buffer[ i*channel_count ] = frame_buf_flt[ i*channel_count ];
                if( channel_count == 2 ) { 
                    state->out_buffer[ (i*channel_count)+1 ] = frame_buf_flt[ (i*channel_count)+1 ]; 
                }
            }
            break;
        default: break;
    }

    av_packet_unref( av_packet );
    av_packet_free( &av_packet );
    av_frame_unref( av_frame );
    av_frame_free( &av_frame );
}

static void _deinit_state( zdj_pipeline_node_t * node ) {
    zdj_library_decode_node_state_t * state = (zdj_library_decode_node_state_t*)node->state;
    if( state->fmt_ctx ) { avformat_close_input( &state->fmt_ctx ); }
    if( state->codec_ctx ) { avcodec_free_context( &state->codec_ctx ); }
    if( state->fmt_ctx ) { avformat_free_context( state->fmt_ctx ); }
    if( state->out_buffer ) { free( state->out_buffer ); }
    // Release packet_layers
    if( state ) { node->state = NULL; free( state );  }
}

// Retrieve and decode one av_packet of samples to PCM data.
// Return the number of sample frames decoded.
static int _decode( 
    zdj_pipeline_node_t * node,
    AVPacket * av_packet, 
    AVFrame * av_frame
) {
    zdj_library_decode_node_state_t * state = (zdj_library_decode_node_state_t*)node->state;
    int decoded_frame_count = 0;
    int res;

    // Fill the Packet with compressed data
    res = av_read_frame( state->fmt_ctx, av_packet );
    if( res != 0 ) {
        if( state->fmt_ctx->pb->eof_reached ) {
            // packet->has_eof = true;
            return decoded_frame_count;
        } else {
            // packet->has_decode_error = true;
            // printf( "av_read_frame failed\n" );
            return -1;
        }
    }

    // MP3s seem to have trouble sending the first packet or two.
    // Retry a few times before just giving up.
    bool exit = false;
    int attempt = 0;
    while ( !exit ) {
        // Push the compressed packet data into the decoder
        res = avcodec_send_packet( state->codec_ctx, av_packet );
        if ( res >= 0 ) { 
            exit = true;
            continue;
        }
        if( attempt > 2 ) {
            state->song->audio->has_libav_error = true;
            state->song->audio->libav_error = res;
            state->song->has_error = true;
            return 0; 
        }
        // Attempt to read another frame to see if things improve
        attempt++;
        av_read_frame( state->fmt_ctx, av_packet );
    }

    // Pull the decompressed samples from the decoder into the packet's data buffer
    res = 0;
    res = avcodec_receive_frame( state->codec_ctx, av_frame );
    if ( res == AVERROR( EAGAIN ) || res == AVERROR_EOF ) {
        // Handle decoding error
        // break;
        // packet->has_eof = true;
        decoded_frame_count = -1;
        // printf( "%p eof2!: %d\n", packet, packet->has_eof );
    } else if ( res < 0 ) {
        // continue;
    } else {
        decoded_frame_count = av_frame->nb_samples;
    }

    return decoded_frame_count;
}
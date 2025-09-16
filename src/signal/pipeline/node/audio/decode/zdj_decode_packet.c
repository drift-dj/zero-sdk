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

// Retrieve and decode one av_packet of samples to PCM data.
// Return the number of sample frames decoded.
int zdj_decode_packet( 
    zdj_pipeline_node_t * node, 
    zdj_decode_packet_t * packet,
    AVPacket * av_packet, 
    AVFrame * av_frame
) {
    // printf( "zdj_decode_packet: %p %p %p\n", packet, av_packet, av_frame );

    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    int res;

    // Fill the Packet with compressed data
    res = av_read_frame( state->fmt_ctx, av_packet );
    if( res != 0 ) {
        if( state->fmt_ctx->pb->eof_reached ) {
            packet->has_eof = true;
            // printf( "%p eof: %d\n", packet, packet->has_eof );
        } else {
            packet->has_decode_error = true;
            printf( "av_read_frame failed\n" );
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
    int decoded_frame_count = 0;
    res = 0;
    res = avcodec_receive_frame( state->codec_ctx, av_frame );
    if ( res == AVERROR( EAGAIN ) || res == AVERROR_EOF ) {
        // Handle decoding error
        // break;
        packet->has_eof = true;
        // printf( "%p eof2!: %d\n", packet, packet->has_eof );
    } else if ( res < 0 ) {
        // continue;
    } else {
        decoded_frame_count = av_frame->nb_samples;
    }

    // Store the new packet/frame
    // packet->packet_pcm_addr = av_packet->pts / packet->av_timebase_factor;
    packet->av_frame_sample_count = decoded_frame_count;
    packet->av_packet = av_packet;
    packet->av_frame = av_frame;

    // printf( "zdj_decode_packet done: %p %p %p\n", packet, av_packet, av_frame );

    return decoded_frame_count;
}

zdj_decode_packet_t * zdj_decode_get_packet_under_head( 
    zdj_pipeline_node_t * node, 
    zdj_decode_layer_t * layer 
) {
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;
    // Loop thru layers, looking for one with core samples covering the decode head
    zdj_decode_packet_t * packet = layer->first_packet;
    while( packet ) {
        if( packet->core_start_addr <= node_state->head_decode_addr &&
            packet->core_end_addr > node_state->head_decode_addr 
        ) {
            return packet;
        }
        packet = packet->next;
    }

    return NULL;
}

void zdj_decode_deinit_packet( zdj_decode_packet_t * packet ) {
    // printf( "zdj_packet_deinit\n" );
    // Stitch surrounding packets together if we're attached to other packets
    if( packet->prev ) {
        packet->prev->next = packet->next;
    }
    if( packet->next ) {
        packet->next->prev = packet->prev;
    }
    packet->next = NULL;
    packet->prev = NULL;
    if( packet->av_packet ){ av_packet_unref( packet->av_packet ); av_packet_free( &packet->av_packet ); }
    if( packet->av_frame ){ av_frame_unref( packet->av_frame ); av_frame_free( &packet->av_frame ); }
    free( packet );
    // printf( "zdj_packet_deinit done\n" );
}

void zdj_decode_make_admin_packet( 
    zdj_decode_layer_t * layer,
    zdj_decode_packet_t * packet, 
    zdj_decode_packet_t * linked_packet, 
    zdj_decode_packet_type_t type,
    zdj_decode_node_packet_direction_t direction,
    int len 
) {
    if( direction == ZDJ_PACKET_DIRECTION_BACK ) { 
        packet->type = type;
        packet->av_frame_sample_count = len;
        packet->packet_pcm_addr = linked_packet->packet_pcm_addr - len;
        packet->packet_decode_addr = linked_packet->packet_decode_addr - len;
        packet->core_start_addr = linked_packet->core_start_addr - len;
        packet->lead_in_start_addr = packet->core_start_addr;
        packet->core_end_addr = packet->core_start_addr + len;
        packet->lead_out_end_addr = packet->core_end_addr;

        layer->earliest_core_sample = packet->core_start_addr;
        layer->earliest_lead_in_sample = packet->core_start_addr;

        packet->next = linked_packet;
        linked_packet->prev = packet;
        packet->prev = NULL;
        layer->first_packet = packet;
    } else if( direction == ZDJ_PACKET_DIRECTION_FWD ) { 
        packet->type = type;
        packet->av_frame_sample_count = len;
        packet->packet_pcm_addr = linked_packet->packet_pcm_addr + linked_packet->av_frame_sample_count;
        packet->packet_decode_addr = linked_packet->packet_decode_addr + linked_packet->av_frame_sample_count;
        packet->core_start_addr = linked_packet->core_start_addr + linked_packet->av_frame_sample_count;
        packet->lead_in_start_addr = packet->core_start_addr;
        packet->core_end_addr = packet->core_start_addr + len;
        packet->lead_out_end_addr = packet->core_end_addr;

        layer->latest_core_sample = packet->core_end_addr;
        layer->latest_lead_out_sample = packet->core_end_addr;

        packet->next = NULL;
        linked_packet->next = packet;
        packet->prev = linked_packet;
        layer->last_packet = packet;
    }
}

bool zdj_decode_packet_contains_decode_addr( zdj_decode_packet_t * packet, int64_t decode_addr ) {
    // printf( "packet: %ld -> %ld contains: %ld\n", 
    //     packet->packet_decode_addr,
    //     packet->packet_decode_addr + packet->av_frame_sample_count,
    //     decode_addr
    // );
    return ( packet->packet_decode_addr < decode_addr && 
             packet->packet_decode_addr + packet->av_frame_sample_count > decode_addr );
}

void zdj_decode_flush_packets( zdj_pipeline_node_t * node ) {
    // printf( "zdj_decode_flush_packets\n" );
    // zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    // int res;

    // // Submit an empty packet to put decoder into flush mode
    // res = avcodec_send_packet( state->codec_ctx, NULL );
    // if ( res >= 0 ) { 
    //     // printf( "error sending flush packet\n" );
    //     return;
    // }
    
    // bool exit = false;
    // int attempt = 0;
    // AVFrame * av_frame = av_frame_alloc( );
    // while ( attempt < 10 ) {
    //     // Read until nothing comes out
    //     attempt++;
    //     res = avcodec_receive_frame( state->codec_ctx, av_frame );
    //     // if ( res == AVERROR( EAGAIN ) || res == AVERROR_EOF ) {
    //     if ( res == AVERROR_EOF ) {
    //         // printf( "found eof after %d receives\n", attempt );
    //         av_frame_unref( av_frame );
    //         av_frame_free( &av_frame );
    //         // printf( "zdj_decode_flush_packets done\n" );
    //         return;
    //     }
    // }
   
}
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

static bool _intersects_out_buf( zdj_decode_packet_t * packet, zdj_pipeline_node_t * node );

static void _render_flt_data_to_out_buf( zdj_decode_packet_t * packet, void * _layer, zdj_pipeline_node_t * node );
static void _render_null_data_to_out_buf( zdj_decode_packet_t * packet, void * _layer, zdj_pipeline_node_t * node );
static double _xfade_coeff_for_transport_d_coord( zdj_decode_packet_t * packet, void * _layer, double coord );

static void _deinit( zdj_decode_packet_t * packet );

zdj_decode_packet_t * zdj_decode_packet( 
    zdj_pipeline_node_t * node,
    zdj_decode_layer_t * layer,
    AVFormatContext * fmt_ctx,
    AVCodecContext * codec_ctx,
    int av_timebase_factor
) {
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;

    AVFrame * av_frame = av_frame_alloc( );
    AVPacket * av_packet = av_packet_alloc( );
    // TODO: Use av_packet data to get a sensible size for this.
    av_new_packet( av_packet, 4000 );

    bool packet_has_eof = false;
    bool packet_has_decode_error = false;

    // Fill the Packet with compressed data
    int res = av_read_frame( fmt_ctx, av_packet );
    if( res != 0 ) {
        if( fmt_ctx->pb->eof_reached ) {
            packet_has_eof = true;
            // printf( "%p eof: %d\n", packet, packet->has_eof );
        } else {
            packet_has_decode_error = true;
            printf( "av_read_frame failed\n" );
        }
    }

    // MP3s seem to have trouble sending the first packet or two.
    // Retry a few times before just giving up.
    bool exit = false;
    int attempt = 0;
    while ( !exit ) {
        // Push the compressed packet data into the decoder
        res = avcodec_send_packet( codec_ctx, av_packet );
        if ( res >= 0 ) { 
            // printf( "good or no decode - exit\n" );
            exit = true;
            continue;
        }
        if( attempt > 20 ) {
            printf( "failed decode: %s\n", av_err2str( res ) );
            
            node_state->song->audio->has_libav_error = true;
            node_state->song->audio->libav_error = res;
            node_state->song->has_error = true;
            return NULL; 
        }
        // Attempt to read another frame to see if things improve
        attempt++;
        av_read_frame( fmt_ctx, av_packet );
    }

    // Pull the decompressed samples from the decoder into the packet's data buffer
    int decoded_frame_count = 0;
    res = 0;
    res = avcodec_receive_frame( codec_ctx, av_frame );

    if ( res == AVERROR( EAGAIN ) || res == AVERROR_EOF ) {
        // Handle decoding error
        // break;
        packet_has_eof = true;
        // printf( "%p eof2!: %d\n", packet, packet->has_eof );
    } else if ( res < 0 ) {
        // continue;
    } else {
        decoded_frame_count = av_frame->nb_samples;
    }

    // Make packet and return
    zdj_decode_packet_t * packet = calloc( 1, sizeof( zdj_decode_packet_t ) );
    packet->av_packet = av_packet;
    packet->av_frame = av_frame;
    packet->sample_count = av_frame->nb_samples;
    packet->deinit = &_deinit;
    packet->intersects_out_buf = &_intersects_out_buf;
    packet->xfade_coeff_for_transport_d_coord = &_xfade_coeff_for_transport_d_coord;

    // Add the render func based on data type
    switch ( av_frame->format ) {
        // case AV_SAMPLE_FMT_U8: state->accum = &_accum_u8; break;
        // case AV_SAMPLE_FMT_S16: state->accum = &_accum_s16; break;
        // case AV_SAMPLE_FMT_S32: state->accum = &_accum_s32; break;
        // case AV_SAMPLE_FMT_S64: state->accum = &_accum_s64; break;
        case AV_SAMPLE_FMT_FLT: 
            packet->render_to_out_buf = &_render_flt_data_to_out_buf; 
            printf( "===>setting flt render format\n" );
            break;
        // case AV_SAMPLE_FMT_DBL: state->accum = &_accum_dbl; break;
        // case AV_SAMPLE_FMT_U8P: state->accum = &_accum_u8p; break;
        // case AV_SAMPLE_FMT_S16P: state->accum = &_accum_s16p; break;
        // case AV_SAMPLE_FMT_S32P: state->accum = &_accum_s32p; break;
        // case AV_SAMPLE_FMT_S64P: state->accum = &_accum_s64p; break;
        // case AV_SAMPLE_FMT_FLTP: state->accum = &_accum_fltp; break;
        // case AV_SAMPLE_FMT_DBLP: state->accum = &_accum_dblp; break;
        default: packet->render_to_out_buf = &_render_null_data_to_out_buf; break;
    }
    
    // Build addrs
    zdj_decode_init_addr( &packet->start_addr );
    int64_t start_origin_i_coord = av_packet->pts / av_timebase_factor;
    node_state->addr_for_origin_i_coord_in_layer( node, layer, &packet->start_addr, start_origin_i_coord );
    zdj_decode_init_addr( &packet->end_addr );
    int64_t end_origin_i_coord = start_origin_i_coord + av_frame->nb_samples;
    node_state->addr_for_origin_i_coord_in_layer( node, layer, &packet->end_addr, end_origin_i_coord );

    printf( "decoded packet @ %ld/%1.0f->%ld/%1.0f\n", 
        start_origin_i_coord, packet->start_addr.origin_d,
        end_origin_i_coord, packet->end_addr.origin_d
    );
    return packet;
}

void zdj_decode_garbage_packet( 
    zdj_pipeline_node_t * node,
    zdj_decode_layer_t * layer,
    AVFormatContext * fmt_ctx,
    AVCodecContext * codec_ctx
) {
    
}

static void _deinit( zdj_decode_packet_t * packet ) {
    if( packet->av_packet ){ av_packet_unref( packet->av_packet ); av_packet_free( &packet->av_packet ); }
    if( packet->av_frame ){ av_frame_unref( packet->av_frame ); av_frame_free( &packet->av_frame ); }
    free( packet );
}

static bool _intersects_out_buf( zdj_decode_packet_t * packet, zdj_pipeline_node_t * node ) {
    printf( "packet->_intersects_out_buf\n" );
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;
    if( packet->start_addr.has_valid_buf && packet->end_addr.has_valid_buf ) {
        printf( "0: %d < %ld && %d >= 0\n",
            packet->start_addr.buf_i,
            node_state->win_sample_count,
            packet->end_addr.buf_i
        );
        return ( packet->start_addr.buf_i < node_state->win_sample_count && packet->end_addr.buf_i >= 0 );
    } else {
        printf( "1\n" );
        return false;
    }
}

static double _xfade_coeff_for_transport_d_coord( zdj_decode_packet_t * packet, void * _layer, double coord ) {
    zdj_decode_layer_t * layer = (zdj_decode_layer_t*)_layer;
    // Map layer lead_in/out to packet addrs to build xfade val
    if( layer->lead_in_start.transport_d < coord && layer->lead_in_end.transport_d > coord ) {
        double num = coord - layer->lead_in_start.transport_d;
        double den = layer->lead_in_end.transport_d - layer->lead_in_start.transport_d;
        return num / den;
    } else if( layer->lead_out_start.transport_d < coord && layer->lead_out_end.transport_d > coord ) {
        double num = coord - layer->lead_out_start.transport_d;
        double den = layer->lead_out_end.transport_d - layer->lead_out_start.transport_d;
        return (den-num) / den;
    } else if( layer->core_start.transport_d < coord && layer->core_end.transport_d > coord ) {
        return 1.0f;
    } else { 
        return 0.0f;
    }
}

static void _render_flt_data_to_out_buf( zdj_decode_packet_t * packet, void * _layer, zdj_pipeline_node_t * node ) {
    printf( "_render_flt_data_to_out_buf\n" );
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;
    zdj_decode_layer_t * layer = (zdj_decode_layer_t*)_layer;
    // Cast the av_frame buffer to type
    float * packet_buf = (float*)packet->av_frame->data[ 0 ];
    float packet_buf_val;
    double xfade_coeff;

    // Loop thru all samples in packet
    int out_buf_sample, out_buf_index, packet_buf_sample, packet_buf_index;
    for( packet_buf_sample=0; packet_buf_sample<packet->sample_count; packet_buf_sample++ ) {
        out_buf_sample = packet_buf_sample + packet->start_addr.buf_i;
        // Clip buf samples to out_buf bounds
        if( out_buf_sample >= 0 && out_buf_sample < node_state->win_sample_count ) {
            packet_buf_index = packet_buf_sample * node_state->channel_count;
            out_buf_index = out_buf_sample * node_state->channel_count;
            // Apply lead_in/out fades to packet sample
            xfade_coeff = packet->xfade_coeff_for_transport_d_coord( 
                packet, layer, packet->start_addr.transport_d+packet_buf_sample 
            );
            packet_buf_val = packet_buf[ packet_buf_index ] * xfade_coeff;
            node_state->out_buffer[ out_buf_index ] = zdj_signal_accum_floats( 
                node_state->out_buffer[ out_buf_index ], packet_buf_val 
            );
            
            // Handle right channel if stereo
            if( node_state->channel_count == 2 ) {
                packet_buf_index = packet_buf_sample * node_state->channel_count + 1;
                out_buf_index = out_buf_sample * node_state->channel_count + 1;
                // Apply lead_in/out fades to packet sample
                xfade_coeff = packet->xfade_coeff_for_transport_d_coord( 
                    packet, layer, packet->start_addr.transport_d+packet_buf_sample 
                );
                packet_buf_val = packet_buf[ packet_buf_index ] * xfade_coeff;
                node_state->out_buffer[ out_buf_index ] = zdj_signal_accum_floats( 
                    node_state->out_buffer[ out_buf_index ], packet_buf_val 
                );
            }
        }
    }
}

static void _render_null_data_to_out_buf( zdj_decode_packet_t * packet, void * _layer, zdj_pipeline_node_t * node ) {
    return; // No-op
}

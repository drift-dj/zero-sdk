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

static void _render_s16_data_to_out_buf( zdj_decode_packet_t * packet, void * _layer, zdj_pipeline_node_t * node );
static void _render_s32_data_to_out_buf( zdj_decode_packet_t * packet, void * _layer, zdj_pipeline_node_t * node );
static void _render_flt_data_to_out_buf( zdj_decode_packet_t * packet, void * _layer, zdj_pipeline_node_t * node );
static void _render_null_data_to_out_buf( zdj_decode_packet_t * packet, void * _layer, zdj_pipeline_node_t * node );
static double _xfade_coeff_for_transport_d_coord( zdj_decode_packet_t * packet, void * _layer, double coord );
// static void _right_justify_av_frame_data( AVFrame * av_frame, int move_sample_distance, int move_sample_count, int channel_count );

static void _deinit( zdj_decode_packet_t * packet );

zdj_decode_packet_t * zdj_decode_packet( 
    zdj_pipeline_node_t * node,
    zdj_decode_layer_t * layer,
    AVFormatContext * fmt_ctx,
    AVCodecContext * codec_ctx,
    int av_timebase_factor,
    zdj_decode_packet_justify_t justify
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
            printf( "%p eof @pts: %lu\n", av_packet, av_packet->pts );
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
        if( attempt > 3 ) {
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

    if ( res == AVERROR( EAGAIN ) ) {
        printf( "eagain\n" );
    } else if ( res == AVERROR_EOF ) {
        // Handle decoding error
        // break;
        packet_has_eof = true;
        printf( "%p eof2! samps: %d\n", av_packet, av_frame->nb_samples );
    } else if ( res < 0 ) {
        // continue;
        printf( "decode err\n" );
    } else {
        decoded_frame_count = av_frame->nb_samples;
    }

    // Make packet and return
    zdj_decode_packet_t * packet = calloc( 1, sizeof( zdj_decode_packet_t ) );
    packet->av_packet = av_packet;
    packet->av_frame = av_frame;
    packet->sample_count = av_frame->nb_samples;
    // packet->sample_count = node_state->estimated_packet_sample_count;
    packet->justify = justify;
    packet->deinit = &_deinit;
    packet->intersects_out_buf = &_intersects_out_buf;
    packet->xfade_coeff_for_transport_d_coord = &_xfade_coeff_for_transport_d_coord;

    

    // Add the render func based on data type
    switch ( av_frame->format ) {
        case AV_SAMPLE_FMT_S16: // wav, aif, flac
            packet->render_to_out_buf = &_render_s16_data_to_out_buf; 
            break;
        case AV_SAMPLE_FMT_S32: // wav, aif, flac
            packet->render_to_out_buf = &_render_s32_data_to_out_buf; 
            break;
        case AV_SAMPLE_FMT_FLT: 
            packet->render_to_out_buf = &_render_flt_data_to_out_buf; 
            // printf( "===>setting flt render format\n" );
            break;
        default: packet->render_to_out_buf = &_render_null_data_to_out_buf; break;
    }
    
    zdj_decode_init_addr( &packet->start_addr );
    zdj_decode_init_addr( &packet->end_addr );
    int64_t start_origin_i_coord = av_packet->pts / av_timebase_factor;
    int64_t end_origin_i_coord = start_origin_i_coord + av_frame->nb_samples;

    node_state->addr_for_origin_i_coord_in_layer( 
        node, layer, &packet->start_addr, start_origin_i_coord 
    );
    node_state->addr_for_origin_i_coord_in_layer( 
        node, layer, &packet->end_addr, end_origin_i_coord 
    );

    printf( "[%p].init_addr t:%1.0f/o:%1.0f packet:[%ld - %ld]\n",
        layer, layer->init_addr.transport_d, layer->init_addr.origin_d,
        start_origin_i_coord, end_origin_i_coord
    );
    
    // TODO: find a cleaner way to signal the last packet in a song
    if( packet_has_eof ) { packet->is_eof = true; return packet; }
    
    // printf( "===> decoded packet %s @ %ld/%1.0f->%ld/%1.0f: %ld\n", 
    //     (justify==ZDJ_DECODE_JUSTIFY_LEFT) ? "LEFT_JUST" : "RIGHT_JUST",
    //     start_origin_i_coord, packet->start_addr.origin_d,
    //     end_origin_i_coord, packet->end_addr.origin_d,
    //     end_origin_i_coord - start_origin_i_coord
    // );
    return packet;
}

void zdj_decode_garbage_packet( 
    zdj_pipeline_node_t * node,
    zdj_decode_layer_t * layer,
    AVFormatContext * fmt_ctx,
    AVCodecContext * codec_ctx
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
            printf( "%p eof @pts: %lu\n", av_packet, av_packet->pts );
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
        if( attempt > 3 ) {
            printf( "failed decode: %s\n", av_err2str( res ) );
            
            node_state->song->audio->has_libav_error = true;
            node_state->song->audio->libav_error = res;
            node_state->song->has_error = true;
            return; 
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
        printf( "%p eof2! samps: %d\n", av_packet, av_frame->nb_samples );
    } else if ( res < 0 ) {
        // continue;
        printf( "decode err\n" );
    } else {
        decoded_frame_count = av_frame->nb_samples;
    }

    // printf( "decoded: %d samps\n", decoded_frame_count );

    if( av_packet ){ av_packet_unref( av_packet ); av_packet_free( &av_packet ); }
    if( av_frame ){ av_frame_unref( av_frame ); av_frame_free( &av_frame ); }
}

static void _deinit( zdj_decode_packet_t * packet ) {
    if( packet->av_packet ){ av_packet_unref( packet->av_packet ); av_packet_free( &packet->av_packet ); }
    if( packet->av_frame ){ av_frame_unref( packet->av_frame ); av_frame_free( &packet->av_frame ); }
    free( packet );
}

static bool _intersects_out_buf( zdj_decode_packet_t * packet, zdj_pipeline_node_t * node ) {
    // printf( "packet->_intersects_out_buf\n" );
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;
    if( packet->start_addr.has_valid_buf && packet->end_addr.has_valid_buf ) {
        // printf( "0: %d < %ld && %d >= 0\n",
        //     packet->start_addr.buf_i,
        //     node_state->win_sample_count,
        //     packet->end_addr.buf_i
        // );
        return ( packet->start_addr.buf_i < node_state->win_sample_count && packet->end_addr.buf_i >= 0 );
    } else {
        // printf( "1\n" );
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

static void _render_s16_data_to_out_buf( zdj_decode_packet_t * packet, void * _layer, zdj_pipeline_node_t * node ) {
    // printf( "_render_s16_data_to_out_buf\n" );
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;
    zdj_decode_layer_t * layer = (zdj_decode_layer_t*)_layer;
    // Cast the av_frame buffer to type
    int16_t * packet_buf = (int16_t*)packet->av_frame->data[ 0 ];
    float packet_buf_val;
    double xfade_coeff;

    // If this is a contiguous layer (no xfades) just memcpy from data to out_buf

    // Loop thru all samples in packet
    int out_buf_sample, out_buf_index, packet_buf_sample, packet_buf_index;
    for( packet_buf_sample=0; packet_buf_sample<packet->sample_count; packet_buf_sample++ ) {
        
        // TODO: Set out_buf_sample based on packet->justify
        if( packet->justify == ZDJ_DECODE_JUSTIFY_LEFT ) {
            out_buf_sample = packet_buf_sample + packet->start_addr.buf_i;
        } else if( packet->justify == ZDJ_DECODE_JUSTIFY_RIGHT ) {
            out_buf_sample = packet->start_addr.buf_i + node_state->estimated_packet_sample_count - packet->sample_count + packet_buf_sample;
        }

        // Clip buf samples to out_buf bounds
        if( out_buf_sample >= 0 && out_buf_sample < node_state->win_sample_count ) {
            packet_buf_index = packet_buf_sample * node_state->channel_count;
            out_buf_index = out_buf_sample * node_state->channel_count;

            // Apply lead_in/out fades to packet sample
            xfade_coeff = packet->xfade_coeff_for_transport_d_coord( 
                packet, layer, packet->start_addr.transport_d+packet_buf_sample 
            );
            packet_buf_val = ((float)packet_buf[ packet_buf_index ] / (float)INT16_MAX) * xfade_coeff;
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
                packet_buf_val = ((float)packet_buf[ packet_buf_index ] / (float)INT16_MAX) * xfade_coeff;
                node_state->out_buffer[ out_buf_index ] = zdj_signal_accum_floats( 
                    node_state->out_buffer[ out_buf_index ], packet_buf_val
                );
            }
        }
    }
    // printf( "_render_flt_data_to_out_buf done\n" );
}

static void _render_s32_data_to_out_buf( zdj_decode_packet_t * packet, void * _layer, zdj_pipeline_node_t * node ) {
    // printf( "_render_s16_data_to_out_buf\n" );
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;
    zdj_decode_layer_t * layer = (zdj_decode_layer_t*)_layer;
    // Cast the av_frame buffer to type
    int32_t * packet_buf = (int32_t*)packet->av_frame->data[ 0 ];
    double packet_buf_val;
    double xfade_coeff;

    // If this is a contiguous layer (no xfades) just memcpy from data to out_buf

    // Loop thru all samples in packet
    int out_buf_sample, out_buf_index, packet_buf_sample, packet_buf_index;
    for( packet_buf_sample=0; packet_buf_sample<packet->sample_count; packet_buf_sample++ ) {
        
        // TODO: Set out_buf_sample based on packet->justify
        if( packet->justify == ZDJ_DECODE_JUSTIFY_LEFT ) {
            out_buf_sample = packet_buf_sample + packet->start_addr.buf_i;
        } else if( packet->justify == ZDJ_DECODE_JUSTIFY_RIGHT ) {
            out_buf_sample = packet->start_addr.buf_i + node_state->estimated_packet_sample_count - packet->sample_count + packet_buf_sample;
        }

        // Clip buf samples to out_buf bounds
        if( out_buf_sample >= 0 && out_buf_sample < node_state->win_sample_count ) {
            packet_buf_index = packet_buf_sample * node_state->channel_count;
            out_buf_index = out_buf_sample * node_state->channel_count;

            // Apply lead_in/out fades to packet sample
            xfade_coeff = packet->xfade_coeff_for_transport_d_coord( 
                packet, layer, packet->start_addr.transport_d+packet_buf_sample 
            );
            packet_buf_val = ((float)packet_buf[ packet_buf_index ] / (float)INT32_MAX) * xfade_coeff;
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
                packet_buf_val = ((double)packet_buf[ packet_buf_index ] / (double)INT32_MAX) * xfade_coeff;
                node_state->out_buffer[ out_buf_index ] = zdj_signal_accum_floats( 
                    node_state->out_buffer[ out_buf_index ], (float)packet_buf_val
                );
            }
        }
    }
    // printf( "_render_flt_data_to_out_buf done\n" );
}

static void _render_flt_data_to_out_buf( zdj_decode_packet_t * packet, void * _layer, zdj_pipeline_node_t * node ) {
    // printf( "_render_flt_data_to_out_buf\n" );
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;
    zdj_decode_layer_t * layer = (zdj_decode_layer_t*)_layer;
    // Cast the av_frame buffer to type
    float * packet_buf = (float*)packet->av_frame->data[ 0 ];
    float packet_buf_val;
    double xfade_coeff;


    // IMPORTANT: We're not 100% sure that out buf data stride correctly matches channel count

    // If this is a contiguous layer (no xfades) just memcpy from data to out_buf
    // How to detect no xfade case
    // if( !node->loop_enabled || !packet->contains_xfade( packet, layer ) ) {

    // TODO: this needs to be packet specific, not the whole layer
    if( layer->back_discon_type == ZDJ_DECODE_DISCON_NONE ) {
        // printf( "memcpy packet\n" );
        int packet_start_sample, packet_end_sample;
        int packet_start_index, packet_copy_bytes;
        int out_buf_start_sample, out_buf_end_sample;
        int out_buf_dest_index;

        // Packet_start_sample must account for a packet which starts BEFORE out buf
        if( packet->start_addr.buf_i >= 0 ) {
            packet_start_sample = 0;
            out_buf_start_sample = packet->start_addr.buf_i;
        } else {
            // Crop to start of window
            packet_start_sample = packet->start_addr.buf_i * -1;
            out_buf_start_sample = 0;
        }

        // Packet_end_sample must account for a packet which ends AFTER out buf
        if( packet->end_addr.buf_i < node_state->win_sample_count ) {
            packet_end_sample = packet->sample_count;
            out_buf_end_sample = packet->end_addr.buf_i;
        } else {
            // Crop to end of window
            packet_end_sample = packet->sample_count - (packet->end_addr.buf_i - node_state->win_sample_count);
            out_buf_end_sample = node_state->win_sample_count;
        }

        out_buf_dest_index = out_buf_start_sample * node_state->channel_count;
        packet_start_index = packet_start_sample * node_state->channel_count;
        packet_copy_bytes = (packet_end_sample - packet_start_sample) * node_state->channel_count * sizeof( float );

        // printf( "copy %d samp/ %d byt\n", packet_end_sample - packet_start_sample, packet_copy_bytes );

        memcpy( &node_state->out_buffer[out_buf_dest_index], &packet_buf[ packet_start_index ], packet_copy_bytes );

        // TODO: bring in packet justify
        // if( packet->justify == ZDJ_DECODE_JUSTIFY_LEFT ) {
        //     out_buf_sample = packet_buf_sample + packet->start_addr.buf_i;
        // } else if( packet->justify == ZDJ_DECODE_JUSTIFY_RIGHT ) {
        //     out_buf_sample = packet->start_addr.buf_i + node_state->estimated_packet_sample_count - packet->sample_count + packet_buf_sample;
        // }
       
        // Do memcpy
    } else {

        // printf( "accumulate packet\n" );

        // Loop thru all samples in packet
        int out_buf_sample, out_buf_index, packet_buf_sample, packet_buf_index;
        for( packet_buf_sample=0; packet_buf_sample<packet->sample_count; packet_buf_sample++ ) {
            
            // TODO: Set out_buf_sample based on packet->justify
            if( packet->justify == ZDJ_DECODE_JUSTIFY_LEFT ) {
                out_buf_sample = packet_buf_sample + packet->start_addr.buf_i;
            } else if( packet->justify == ZDJ_DECODE_JUSTIFY_RIGHT ) {
                out_buf_sample = packet->start_addr.buf_i + node_state->estimated_packet_sample_count - packet->sample_count + packet_buf_sample;
            }

            // printf( "o: %d\n", out_buf_sample );

            // Clip buf samples to out_buf bounds
            if( out_buf_sample >= 0 && out_buf_sample < node_state->win_sample_count ) {
                packet_buf_index = packet_buf_sample * node_state->channel_count;
                out_buf_index = out_buf_sample * node_state->channel_count;

                // Apply lead_in/out fades to packet sample
                xfade_coeff = packet->xfade_coeff_for_transport_d_coord( 
                    packet, layer, packet->start_addr.transport_d+packet_buf_sample 
                );
                // Temporarily disable crossfade
                packet_buf_val = packet_buf[ packet_buf_index ] * xfade_coeff;
                node_state->out_buffer[ out_buf_index ] = zdj_signal_accum_floats( 
                    node_state->out_buffer[ out_buf_index ], packet_buf_val
                );
                // packet_buf_val = packet_buf[ packet_buf_index ];
                // node_state->out_buffer[ out_buf_index ] = packet_buf_val;
                // Temporarily disable crossfade

                // printf( "p:%d o:%d v:%1.4f\n", packet_buf_sample, out_buf_sample, packet_buf_val );
                
                // Handle right channel if stereo
                if( node_state->channel_count == 2 ) {
                    packet_buf_index = packet_buf_sample * node_state->channel_count + 1;
                    out_buf_index = out_buf_sample * node_state->channel_count + 1;
                    // Apply lead_in/out fades to packet sample
                    xfade_coeff = packet->xfade_coeff_for_transport_d_coord( 
                        packet, layer, packet->start_addr.transport_d+packet_buf_sample 
                    );
                    // Temporarily disable crossfade
                    packet_buf_val = packet_buf[ packet_buf_index ] * xfade_coeff;
                    node_state->out_buffer[ out_buf_index ] = zdj_signal_accum_floats( 
                        node_state->out_buffer[ out_buf_index ], packet_buf_val
                    );
                    // packet_buf_val = packet_buf[ packet_buf_index ];
                    // node_state->out_buffer[ out_buf_index ] = packet_buf_val;
                    // Temporarily disable crossfade
                }
            }
        }
    }
    // printf( "_render_flt_data_to_out_buf done\n" );
}

static void _render_null_data_to_out_buf( zdj_decode_packet_t * packet, void * _layer, zdj_pipeline_node_t * node ) {
    return; // No-op
}

// static void _right_justify_av_frame_data( AVFrame * av_frame, int move_sample_distance, int move_sample_count, int channel_count ) {
//     // Move 
//     // Add the render func based on data type
//     printf( "_right_justify_av_frame_data dist: %d cnt: %d chn:  %d\n", move_sample_distance, move_sample_count, channel_count );

//     int move_byte_distance;
//     int move_byte_count;
//     uint8_t * buf = av_frame->data[ 0 ];
    
//     switch ( av_frame->format ) {
//         case AV_SAMPLE_FMT_FLT: 
//             // move_byte_distance = move_sample_distance * channel_count * sizeof( float );
//             move_byte_count = move_sample_count * channel_count * sizeof( float );
//             float * packet_buf = (float*)av_frame->data[ 0 ];
//             memcpy( &packet_buf[ move_sample_distance*channel_count ], &packet_buf[ 0 ], move_byte_count );
//             // memcpy( &packet_buf[ 800 ], &packet_buf[ 0 ], move_byte_count );
//             // memset( av_frame->data[ 0 ], 0, 1152 * channel_count * sizeof( float ) );
//             // for( int i=0; i<move_sample_count; i++ ) {
//             //     // printf( "s:%d d:%d\n", src, dst );
//             //     packet_buf[ (move_sample_distance+i)*channel_count ] = packet_buf[ i*channel_count ];
//             //     packet_buf[ i*channel_count ] = 0;
//             //     if( channel_count == 2 ) {
//             //         packet_buf[ ((move_sample_distance+i)*channel_count)+1 ] = packet_buf[ (i*channel_count)+1 ];
//             //         packet_buf[ (i*channel_count)+1 ] = 0;
//             //     }
//             // }
//             break;
//         default: return;
//     }
//     // uint8_t * src = &buf[ 0 ];
//     // uint8_t * dst = &buf[ move_byte_distance ];
//     // uint8_t * dst = &buf[ 1000 ];
//     // printf( "_right_justify_av_frame_data: %p %p %p %d %d %d\n", buf, src, dst, move_sample_distance, move_byte_distance, move_byte_count );
//     // memmove( dst, src, move_byte_count );
//     // memmove( dst, src, 800 );
//     // memcpy( dst, src, move_byte_count );
// }
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>

#include <libavformat/avformat.h>
#include <libavcodec/packet.h>
#include <libavutil/error.h>
#include <libavcodec/codec_id.h>

#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>

static float _accum_floats( float val_1, float val_2 ) {
    float val = val_1+val_2;
    if( val > 1.0 ) { return 1.0; } 
    else if( val < -1.0 ) { return -1.0; } 
    else { return val; }
}

static void _make_buffer_indexes( 
    zdj_pipeline_node_t * node, 
    zdj_decode_packet_t * packet,
    int * packet_buffer_start,
    int * out_buffer_start,
    int * sample_count 
) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    // Decode-space start/end of node's window
    int node_win_start = state->head_decode_addr - state->win_back_sample_count;
    int node_win_end = state->head_decode_addr + state->win_fwd_sample_count;

    // Step 1 - Clip packet samples to node's window.
    // Do this in decode space, since both are indexed against the same coordinate system.
    // Packet sample start/end (in decode-space)
    // Start = decode-space addr of first sample in packet, clipped to node's window start addr
    int packet_offset_in_win = packet->packet_decode_addr - node_win_start;
    int packet_clipped_start, packet_clipped_end;
    packet_clipped_start = fmax(
        packet->packet_decode_addr,
        node_win_start
    );
    // End = decode-space addr of last sample in packet, clipped to node's window end addr
    packet_clipped_end = fmin( 
        node_win_end, // decode-space addr of node's window end
        packet->packet_decode_addr + packet->av_frame_sample_count // decode-space address of last sample in packet
    );
    // Sample copy count
    *sample_count = packet_clipped_end - packet_clipped_start;

    // Step 2 - Transform clipped coordinates to individual buffer spaces.
    // Packet samples exist within the av_frame's buffer index space.
    // Out_buf samples exist with the out_buffer's index space.
    int packet_sample_start = packet_clipped_start - packet->packet_decode_addr;
    *packet_buffer_start = packet_sample_start * state->channel_count;
    int out_sample_start = packet_clipped_start - node_win_start;
    *out_buffer_start = out_sample_start * state->channel_count;


    // Find packet under head and report copy stuff
    // if( packet->packet_decode_addr < state->head_decode_addr &&
    //     packet->packet_decode_addr+packet->av_frame_sample_count > state->head_decode_addr 
    // ) {
    //     printf( "packet under head: %ld->%ld: %d\n", 
    //         packet->packet_decode_addr,
    //         packet->packet_decode_addr+packet->av_frame_sample_count,
    //         *sample_count 
    //     );
    // }
}

int zdj_decode_node_accum_u8( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) { }

// Interleaved Signed 16-bit PCM samples
int zdj_decode_node_accum_s16( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {
    // printf( "zdj_decode_node_accum_s16\n" );
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    int packet_buffer_start, out_buffer_start, sample_count;
    _make_buffer_indexes( node, packet, &packet_buffer_start, &out_buffer_start, &sample_count );

    // Cast the av_frame buffer to type
    int16_t * packet_buf = (int16_t*)packet->av_frame->data[ 0 ];
    float packet_buf_val;

    for( int i=0; i<sample_count; i++ ) {
        double xfade_coeff = 1.0;
        for( int c=0; c<state->channel_count; c++ ) {
            int out_buf_index = out_buffer_start + (i*state->channel_count) + c;
            int packet_buf_index = packet_buffer_start + (i*state->channel_count) + c;
            packet_buf_val = ((double)packet_buf[ packet_buf_index ] / (double)INT16_MAX) * xfade_coeff;
            state->out_buffer[ out_buf_index ] += packet_buf_val;
        }
        // Capture pcm addr if we encounter the out_buffer head during write
        if( out_buffer_start+i == state->win_head_sample ) {
            state->head_pcm_addr = packet->packet_pcm_addr + i;
            state->head_percent = ((double)state->head_pcm_addr/44100.0) / state->song_pcm_duration;
        }
    }

    // Return the total number of buffer samples
    return sample_count;
}

int zdj_decode_node_accum_s32( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {
    printf( "zdj_decode_node_accum_s32\n" );
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    int packet_buffer_start, out_buffer_start, sample_count;
    _make_buffer_indexes( node, packet, &packet_buffer_start, &out_buffer_start, &sample_count );

    // Cast the av_frame buffer to type
    int32_t * packet_buf = (int32_t*)packet->av_frame->data[ 0 ];
    float packet_buf_val;

    for( int i=0; i<sample_count; i++ ) {
        double xfade_coeff = 1.0;
        for( int c=0; c<state->channel_count; c++ ) {
            int out_buf_index = out_buffer_start + (i*state->channel_count) + c;
            int packet_buf_index = packet_buffer_start + (i*state->channel_count) + c;
            packet_buf_val = ((double)packet_buf[ packet_buf_index ] / (double)INT32_MAX) * xfade_coeff;
            state->out_buffer[ out_buf_index ] += packet_buf_val;
        }
        // Capture pcm addr if we encounter the out_buffer head during write
        if( out_buffer_start+i == state->win_head_sample ) {
            state->head_pcm_addr = packet->packet_pcm_addr + i;
            state->head_percent = ((double)state->head_pcm_addr/44100.0) / state->song_pcm_duration;
        }
    }

    // Return the total number of buffer samples
    return sample_count;
}

int zdj_decode_node_accum_s64( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {
    // printf( "zdj_decode_node_accum_s64\n" );
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    int packet_buffer_start, out_buffer_start, sample_count;
    _make_buffer_indexes( node, packet, &packet_buffer_start, &out_buffer_start, &sample_count );

    // Cast the av_frame buffer to type
    int64_t * packet_buf = (int64_t*)packet->av_frame->data[ 0 ];
    float packet_buf_val;

    for( int i=0; i<sample_count; i++ ) {
        double xfade_coeff = 1.0;
        for( int c=0; c<state->channel_count; c++ ) {
            int out_buf_index = out_buffer_start + (i*state->channel_count) + c;
            int packet_buf_index = packet_buffer_start + (i*state->channel_count) + c;
            packet_buf_val = ((double)packet_buf[ packet_buf_index ] / (double)INT64_MAX) * xfade_coeff;
            state->out_buffer[ out_buf_index ] += packet_buf_val;
        }
        // Capture pcm addr if we encounter the out_buffer head during write
        if( out_buffer_start+i == state->win_head_sample ) {
            state->head_pcm_addr = packet->packet_pcm_addr + i;
            state->head_percent = ((double)state->head_pcm_addr/44100.0) / state->song_pcm_duration;
        }
    }

    // Return the total number of buffer samples
    return sample_count;
}

int zdj_decode_node_accum_flt( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {
    // printf( "zdj_decode_node_accum_flt: %p %p %p\n", packet, packet->av_packet, packet->av_frame );
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    int packet_buffer_start, out_buffer_start, sample_count;
    _make_buffer_indexes( node, packet, &packet_buffer_start, &out_buffer_start, &sample_count );

    // Cast the av_frame buffer to type
    float * packet_buf = (float*)packet->av_frame->data[ 0 ];
    float packet_buf_val;
    bool xfade = false;
    bool xfade_coeff = 1.0;

    for( int i=0; i<sample_count; i++ ) {
        // Build crossfade coefficient from lead-in/out state
        if( xfade ) { float xfade_coeff = 1.0; }
        for( int c=0; c<state->channel_count; c++ ) {
            int out_buf_index = out_buffer_start + (i*state->channel_count) + c;
            int packet_buf_index = packet_buffer_start + (i*state->channel_count) + c;
            packet_buf_val = packet_buf[ packet_buf_index ] * xfade_coeff;
            state->out_buffer[ out_buf_index ] = _accum_floats(
                state->out_buffer[ out_buf_index ],
                packet_buf_val
            );
        }
        // Capture pcm addr if we encounter the out_buffer head during write
        if( out_buffer_start+i == state->win_head_sample ) {
            state->head_pcm_addr = packet->packet_pcm_addr + i;
            state->head_percent = ((double)state->head_pcm_addr/44100.0) / state->song_pcm_duration;
        }
    }

    // Return the total number of buffer samples
    return sample_count;
}

int zdj_decode_node_accum_dbl( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {
    // printf( "zdj_decode_node_accum_dbl\n" );
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    int packet_buffer_start, out_buffer_start, sample_count;
    _make_buffer_indexes( node, packet, &packet_buffer_start, &out_buffer_start, &sample_count );

    // Cast the av_frame buffer to type
    double * packet_buf = (double*)packet->av_frame->data[ 0 ];
    float packet_buf_val;

    for( int i=0; i<sample_count; i++ ) {
        double xfade_coeff = 1.0;
        for( int c=0; c<state->channel_count; c++ ) {
            int out_buf_index = out_buffer_start + (i*state->channel_count) + c;
            int packet_buf_index = packet_buffer_start + (i*state->channel_count) + c;
            packet_buf_val = packet_buf[ packet_buf_index ] * xfade_coeff;
            state->out_buffer[ out_buf_index ] += packet_buf_val;
        }
        // Capture pcm addr if we encounter the out_buffer head during write
        if( out_buffer_start+i == state->win_head_sample ) {
            state->head_pcm_addr = packet->packet_pcm_addr + i;
            state->head_percent = ((double)state->head_pcm_addr/44100.0) / state->song_pcm_duration;
        }
    }

    // Return the total number of buffer samples
    return sample_count;
}

int zdj_decode_node_accum_u8p( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) { }

// Planar Signed 16-bit
int zdj_decode_node_accum_s16p( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {
    printf( "zdj_decode_node_accum_s16p\n" );
    // zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    // zdj_decode_node_accum_state_t accum;
    // _make_accum( node, packet, &accum );

    // int chan_count = packet->av_frame->channels;

    // int write_tally = 0;

    // for( int i=0; i<=accum.out_buf_copy_len; i++ ) {
    //     for( int chan=1; chan<=chan_count; chan++ ) {
    //         // Map sample addresses - planar mapping for av_frame's data buf
    //         accum.packet_buf_sample = accum.packet_buf_copy_start+i;
    //         accum.packet_buf_index = accum.packet_buf_sample;

    //         accum.out_buf_sample = accum.out_buf_copy_start+i;
    //         accum.out_buf_index = (accum.out_buf_sample * chan_count) + chan;

    //         // Cast the av_frame's plane buffer to type
    //         int16_t * typed_ref = (int16_t*)packet->av_frame->data[ chan ];

    //         // Convert the av_frame sample to float and accumulate
    //         state->out_buffer[ accum.out_buf_index ] = _accum_floats(
    //             state->out_buffer[ accum.out_buf_index ],
    //             (float)typed_ref[ accum.packet_buf_index ] / (float)INT16_MAX
    //         );

    //         // Tally each buffer write
    //         write_tally++;

    //         // Store decode head addr
    //         if( accum.out_buf_sample == state->win_head_sample ) {
    //             state->head_pcm_addr.i_val = packet->packet_pcm_addr.i_val + i;
    //             state->head_percent = (double)state->head_pcm_addr.i_val / state->song_pcm_duration;
    //         }
    //     }
    // }

    // // Return the total number of buffer writes
    // return write_tally;
}

int zdj_decode_node_accum_s32p( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {
    printf( "zdj_decode_node_accum_s32p\n" );
    // zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    // zdj_decode_node_accum_state_t accum;
    // _make_accum( node, packet, &accum );

    // int chan_count = packet->av_frame->channels;

    // int write_tally = 0;

    // for( int i=0; i<=accum.out_buf_copy_len; i++ ) {
    //     for( int chan=1; chan<=chan_count; chan++ ) {
    //         // Map sample addresses - planar mapping for av_frame's data buf
    //         accum.packet_buf_sample = accum.packet_buf_copy_start+i;
    //         accum.packet_buf_index = accum.packet_buf_sample;

    //         accum.out_buf_sample = accum.out_buf_copy_start+i;
    //         accum.out_buf_index = (accum.out_buf_sample * chan_count) + chan;

    //         // Cast the av_frame's plane buffer to type
    //         int32_t * typed_ref = (int32_t*)packet->av_frame->data[ chan ];

    //         // Convert the av_frame sample to float and accumulate
    //         state->out_buffer[ accum.out_buf_index ] = _accum_floats(
    //             state->out_buffer[ accum.out_buf_index ],
    //             (float)typed_ref[ accum.packet_buf_index ] / (float)INT32_MAX
    //         );

    //         // Tally each buffer write
    //         write_tally++;

    //         // Store decode head addr
    //         if( accum.out_buf_sample == state->win_head_sample ) {
    //             state->head_pcm_addr.i_val = packet->packet_pcm_addr.i_val + i;
    //             state->head_percent = (double)state->head_pcm_addr.i_val / state->song_pcm_duration;
    //         }
    //     }
    // }

    // // Return the total number of buffer writes
    // return write_tally;
}

int zdj_decode_node_accum_s64p( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {

}

int zdj_decode_node_accum_fltp( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {
    printf( "zdj_decode_node_accum_fltp\n" );
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    int packet_buffer_start, out_buffer_start, sample_count;
    _make_buffer_indexes( node, packet, &packet_buffer_start, &out_buffer_start, &sample_count );

    // Cast the av_frame buffer to type
    float * packet_buf = (float*)packet->av_frame->data[ 0 ];
    float packet_buf_val;
    bool xfade = false;
    bool xfade_coeff = 1.0;
    int out_buf_index, packet_buf_index;


    for( int i=0; i<sample_count; i++ ) {
        // Build crossfade coefficient from lead-in/out state
        if( xfade ) { float xfade_coeff = 1.0; }
        
        for( int c=0; c<state->channel_count; c++ ) {
            out_buf_index = out_buffer_start + (i*state->channel_count) + c;
            packet_buf_index = packet_buffer_start + (i*state->channel_count) + c;
            packet_buf_val = packet_buf[ packet_buf_index ] * xfade_coeff;
            printf( "out_buf sam: %d/%d pb: %d/%lu %d->%d ob: %d\n",
                i, sample_count, 
                packet->av_frame->nb_samples,
                packet->av_frame->channel_layout,
                packet_buffer_start, packet_buf_index,
                out_buf_index
            );
            state->out_buffer[ out_buf_index ] = _accum_floats(
                state->out_buffer[ out_buf_index ],
                packet_buf_val
            );
        }
    }

    // Capture pcm addr if we encounter the out_buffer head during write.
    // This is not currently very accurate.  It's actually the address at the beginning of the
    // buffer.  Need to move it to the head addr, but bound it to song pcm space (no -1)
    int out_sample_start = out_buffer_start/state->channel_count;
    if( out_sample_start < state->win_head_sample &&
        out_sample_start + sample_count > state->win_head_sample 
    ) {
        state->head_pcm_addr = packet->packet_pcm_addr + (packet_buffer_start/state->channel_count);
    }

    // Return the total number of buffer samples
    return sample_count;

    // // printf( "zdj_decode_node_accum_fltp: %s / %d / %d / ls0: %d / ls1: %d / ns: %d / sz: %d\n", state->song->audio->filepath,
    // //     av_sample_fmt_is_planar( packet->av_frame->format ),
    // //     state->codec_ctx->channels,
    // //     packet->av_frame->linesize[ 0 ],
    // //     packet->av_frame->linesize[ 1 ],
    // //     packet->av_frame->nb_samples,
    // //     av_get_bytes_per_sample( packet->av_frame->format )
    // // );

    // zdj_decode_node_accum_state_t accum;
    // _make_accum( node, packet, &accum );

    // int chan_count = packet->av_frame->channels;

    // // Sticking this here for now.  Can be more accurate if we capture the window head sample.
    // // state->head_pcm_addr.i_val = packet->packet_pcm_addr.i_val;
    // // state->head_percent = (double)state->head_pcm_addr.i_val / state->song_pcm_duration;
    // // printf( "head: %1.3f: %ld/%1.3f\n", state->head_percent, state->head_pcm_addr.i_val, state->song_pcm_duration );

    // int write_tally = 0;

    // for( int i=0; i<=accum.out_buf_copy_len; i++ ) {
    //     for( int chan=1; chan<=chan_count; chan++ ) {
    //         // Map sample addresses - planar mapping for av_frame's data buf
    //         accum.packet_buf_sample = accum.packet_buf_copy_start+i;
    //         accum.packet_buf_index = accum.packet_buf_sample;

    //         accum.out_buf_sample = accum.out_buf_copy_start+i;
    //         accum.out_buf_index = (accum.out_buf_sample * chan_count) + chan;

    //         // Cast the av_frame's plane buffer to type
    //         float * typed_ref = packet->av_frame->data[ chan-1 ];

    //         // Convert the av_frame sample to float and accumulate            
    //         state->out_buffer[ accum.out_buf_index ] = _accum_floats(
    //             state->out_buffer[ accum.out_buf_index ],
    //             typed_ref[ accum.packet_buf_index ]
    //         );

    //         // Tally each buffer write
    //         write_tally++;

    //         // Store decode head addr
    //         if( accum.out_buf_sample == state->win_head_sample ) {
    //             state->head_pcm_addr.i_val = packet->packet_pcm_addr.i_val + i;
    //             state->head_percent = ((double)state->head_pcm_addr.i_val/44100.0) / state->song_pcm_duration;
    //             // printf( "head: %1.3f: %ld/%1.3f\n", state->head_percent, state->head_pcm_addr.i_val, state->song_pcm_duration );
    //         }
    //     }
    // }

    // printf( "zdj_decode_node_accum_fltp done\n" );
    // // Return the total number of buffer writes
    // return write_tally;
}

int zdj_decode_node_accum_dblp( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {

}

float _ts1_p = 0;
float _ts1_f = 945;
float _ts2_p = 0;
float _ts2_f = 300;

int zdj_decode_node_accum_test_sine( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    // // printf( "zdj_decode_node_accum_flt: %s / %d\n", state->song->audio->filepath, packet->av_frame->linesize[ 0 ] );

    // zdj_decode_node_accum_state_t accum;
    // _make_accum( node, packet, &accum );

    // int chan_count = packet->av_frame->channels;

    // int write_tally = 0;

    // _ts1_p = zdj_signal_gen_sine( 
    //     _ts1_f, 
    //     _ts1_p, 
    //     accum.out_buf_copy_len,
    //     &state->out_buffer[ accum.out_buf_copy_start ],
    //     chan_count,
    //     0,
    //     (double)INT32_MAX
    // );
    
    // // Return the total number of buffer writes
    // return write_tally;
}
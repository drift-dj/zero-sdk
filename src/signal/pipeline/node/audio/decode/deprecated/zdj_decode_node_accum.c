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

typedef struct {
    int packet_buf_copy_start;
    int packet_buf_copy_end;
    int packet_buf_copy_len;
    int packet_buf_sample;
    int packet_buf_index;

    int out_buf_copy_start;
    int out_buf_copy_end;
    int out_buf_copy_len;
    int out_buf_sample;
    int out_buf_index;
} zdj_decode_node_accum_state_t;

static float _accum_floats( float val_1, float val_2 ) {
    val_1 += val_2;
    float val = val_1+val_2;
    if( val > 1.0 ) { return 1.0; } 
    else if( val < -1.0 ) { return -1.0; } 
    else { return val; }
}

static void _make_accum( 
    zdj_pipeline_node_t * node,
    zdj_decode_packet_t * packet, 
    zdj_decode_node_accum_state_t * accum 
) {
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;

    // Locate a sample index in the packet's samples where the copy src should start.
    accum->packet_buf_copy_start = ( packet->xfade_in_buf_addr.i_val < 0 ) ? 
        packet->xfade_in_buf_addr.i_val * -1 : 0;
    // Debug packet source copy start
    node_state->debug_accum_src_st_samp = accum->packet_buf_copy_start;

    // Locate a sample index in the out_buffer's samples where the copy dst should start.
    accum->out_buf_copy_start = fmax( 0, packet->xfade_in_buf_addr.i_val );
    // Debug packet dest copy start
    node_state->debug_accum_dest_st_samp = accum->out_buf_copy_start;

    // Locate a sample index in the out_buffer's samples where the copy dst should end.
    accum->out_buf_copy_end = fmin( 
        packet->xfade_in_buf_addr.i_val + packet->av_frame_sample_count, 
        node_state->win_sample_count
    ); 
    // Debug packet dest copy end
    node_state->debug_accum_dest_en_samp = accum->out_buf_copy_end;

    accum->out_buf_copy_len = accum->out_buf_copy_end - accum->out_buf_copy_start;
}

int zdj_decode_node_accum_u8( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) { }

// Interleaved Signed 16-bit PCM samples
int zdj_decode_node_accum_s16( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {
    // printf( "zdj_decode_node_accum_s16\n" );
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    zdj_decode_node_accum_state_t accum;
    _make_accum( node, packet, &accum );

    int chan_count = packet->av_frame->channels;

    int write_tally = 0;

    for( int i=0; i<accum.out_buf_copy_len; i++ ) {
        for( int chan=1; chan<=chan_count; chan++ ) {
            // Map sample addresses
            accum.packet_buf_sample = accum.packet_buf_copy_start+i;
            accum.packet_buf_index = (accum.packet_buf_sample * chan_count) + chan;

            accum.out_buf_sample = accum.out_buf_copy_start+i;
            accum.out_buf_index = (accum.out_buf_sample * chan_count) + chan;

            // Cast the av_frame buffer to type
            int16_t * typed_ref = (int16_t*)packet->av_frame->data[ 0 ];

            // Convert the av_frame sample to float and accumulate
            state->out_buffer[ accum.out_buf_index ] = _accum_floats(
                state->out_buffer[ accum.out_buf_index ],
                (float)typed_ref[ accum.packet_buf_index ] / (float)INT16_MAX
            );

            // Tally each buffer write
            write_tally++;

            // Store decode head addr
            if( accum.out_buf_sample == state->win_head_sample ) {
                state->head_pcm_addr.i_val = packet->packet_pcm_addr.i_val + i;
                state->head_percent = ((double)state->head_pcm_addr.i_val/44100.0) / state->song_pcm_duration;
            }
        }
    }

    // Return the total number of buffer writes
    return write_tally;
}

int zdj_decode_node_accum_s32( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {
    // printf( "zdj_decode_node_accum_s32\n" );
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    zdj_decode_node_accum_state_t accum;
    _make_accum( node, packet, &accum );

    int chan_count = packet->av_frame->channels;

    int write_tally = 0;

    for( int i=0; i<=accum.out_buf_copy_len; i++ ) {
        for( int chan=1; chan<=chan_count; chan++ ) {
            // Map sample addresses
            accum.packet_buf_sample = accum.packet_buf_copy_start+i;
            accum.packet_buf_index = (accum.packet_buf_sample * chan_count) + chan;

            accum.out_buf_sample = accum.out_buf_copy_start+i;
            accum.out_buf_index = (accum.out_buf_sample * chan_count) + chan;

            // Cast the av_frame buffer to type
            int32_t * typed_ref = (int32_t*)packet->av_frame->data[ 0 ];

            // Convert the av_frame sample to float and accumulate
            state->out_buffer[ accum.out_buf_index ] = _accum_floats(
                state->out_buffer[ accum.out_buf_index ],
                (float)typed_ref[ accum.packet_buf_index ] / (float)INT32_MAX
            );

            // Tally each buffer write
            write_tally++;

            // Store decode head addr
            if( accum.out_buf_sample == state->win_head_sample ) {
                state->head_pcm_addr.i_val = packet->packet_pcm_addr.i_val + i;
                state->head_percent = ((double)state->head_pcm_addr.i_val/44100.0) / state->song_pcm_duration;
            }
        }
    }

    // Return the total number of buffer writes
    return write_tally;
}

int zdj_decode_node_accum_s64( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {
    // printf( "zdj_decode_node_accum_s64\n" );
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    zdj_decode_node_accum_state_t accum;
    _make_accum( node, packet, &accum );

    int chan_count = packet->av_frame->channels;

    int write_tally = 0;

    for( int i=0; i<=accum.out_buf_copy_len; i++ ) {
        for( int chan=1; chan<=chan_count; chan++ ) {
            // Map sample addresses
            accum.packet_buf_sample = accum.packet_buf_copy_start+i;
            accum.packet_buf_index = (accum.packet_buf_sample * chan_count) + chan;

            accum.out_buf_sample = accum.out_buf_copy_start+i;
            accum.out_buf_index = (accum.out_buf_sample * chan_count) + chan;

            // Cast the av_frame buffer to type
            int64_t * typed_ref = (int64_t*)packet->av_frame->data[ 0 ];

            // Convert the av_frame sample to float and accumulate
            state->out_buffer[ accum.out_buf_index ] = _accum_floats(
                state->out_buffer[ accum.out_buf_index ],
                (float)typed_ref[ accum.packet_buf_index ] / (float)INT64_MAX
            );

            // Tally each buffer write
            write_tally++;

            // Store decode head addr
            if( accum.out_buf_sample == state->win_head_sample ) {
                state->head_pcm_addr.i_val = packet->packet_pcm_addr.i_val + i;
                state->head_percent = ((double)state->head_pcm_addr.i_val/44100.0) / state->song_pcm_duration;
            }
        }
    }

    // Return the total number of buffer writes
    return write_tally;
}

int zdj_decode_node_accum_flt( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    // printf( "zdj_decode_node_accum_flt: %s / %d\n", state->song->audio->filepath, packet->av_frame->linesize[ 0 ] );

    zdj_decode_node_accum_state_t accum;
    _make_accum( node, packet, &accum );

    int chan_count = packet->av_frame->channels;

    int write_tally = 0;

    for( int i=0; i<=accum.out_buf_copy_len; i++ ) {
        for( int chan=1; chan<=chan_count; chan++ ) {
            // Map sample addresses
            accum.packet_buf_sample = accum.packet_buf_copy_start+i;
            accum.packet_buf_index = (accum.packet_buf_sample * chan_count) + chan;

            accum.out_buf_sample = accum.out_buf_copy_start+i;
            accum.out_buf_index = (accum.out_buf_sample * chan_count) + chan;

            // Cast the av_frame buffer to type
            float * typed_ref = packet->av_frame->data[ 0 ];

            // Convert the av_frame sample to float and accumulate
            state->out_buffer[ accum.out_buf_index ] = _accum_floats(
                state->out_buffer[ accum.out_buf_index ],
                typed_ref[ accum.packet_buf_index ]
            );

            // Tally each buffer write
            write_tally++;
            
            // Store decode head addr
            if( accum.out_buf_sample == state->win_head_sample ) {
                state->head_pcm_addr.i_val = packet->packet_pcm_addr.i_val + i;
                state->head_percent = ((double)state->head_pcm_addr.i_val/44100.0) / state->song_pcm_duration;
            }
        }
    }

    // Return the total number of buffer writes
    return write_tally;
}

int zdj_decode_node_accum_dbl( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {
    // printf( "zdj_decode_node_accum_dbl\n" );
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    zdj_decode_node_accum_state_t accum;
    _make_accum( node, packet, &accum );

    int chan_count = packet->av_frame->channels;

    int write_tally = 0;

    for( int i=0; i<=accum.out_buf_copy_len; i++ ) {
        for( int chan=1; chan<=chan_count; chan++ ) {
            // Map sample addresses
            accum.packet_buf_sample = accum.packet_buf_copy_start+i;
            accum.packet_buf_index = (accum.packet_buf_sample * chan_count) + chan;

            accum.out_buf_sample = accum.out_buf_copy_start+i;
            accum.out_buf_index = (accum.out_buf_sample * chan_count) + chan;

            // Cast the av_frame buffer to type
            double * typed_ref = (double*)packet->av_frame->data[ 0 ];

            // Convert the av_frame sample to float and accumulate
            state->out_buffer[ accum.out_buf_index ] = _accum_floats(
                state->out_buffer[ accum.out_buf_index ],
                (float)typed_ref[ accum.packet_buf_index ]
            );

            // Tally each buffer write
            write_tally++;

            // Store decode head addr
            if( accum.out_buf_sample == state->win_head_sample ) {
                state->head_pcm_addr.i_val = packet->packet_pcm_addr.i_val + i;
                state->head_percent = ((double)state->head_pcm_addr.i_val/44100.0) / state->song_pcm_duration;
            }
        }
    }

    // Return the total number of buffer writes
    return write_tally;
}

int zdj_decode_node_accum_u8p( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) { }

// Planar Signed 16-bit
int zdj_decode_node_accum_s16p( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {
    printf( "zdj_decode_node_accum_s16p\n" );
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    zdj_decode_node_accum_state_t accum;
    _make_accum( node, packet, &accum );

    int chan_count = packet->av_frame->channels;

    int write_tally = 0;

    for( int i=0; i<=accum.out_buf_copy_len; i++ ) {
        for( int chan=1; chan<=chan_count; chan++ ) {
            // Map sample addresses - planar mapping for av_frame's data buf
            accum.packet_buf_sample = accum.packet_buf_copy_start+i;
            accum.packet_buf_index = accum.packet_buf_sample;

            accum.out_buf_sample = accum.out_buf_copy_start+i;
            accum.out_buf_index = (accum.out_buf_sample * chan_count) + chan;

            // Cast the av_frame's plane buffer to type
            int16_t * typed_ref = (int16_t*)packet->av_frame->data[ chan ];

            // Convert the av_frame sample to float and accumulate
            state->out_buffer[ accum.out_buf_index ] = _accum_floats(
                state->out_buffer[ accum.out_buf_index ],
                (float)typed_ref[ accum.packet_buf_index ] / (float)INT16_MAX
            );

            // Tally each buffer write
            write_tally++;

            // Store decode head addr
            if( accum.out_buf_sample == state->win_head_sample ) {
                state->head_pcm_addr.i_val = packet->packet_pcm_addr.i_val + i;
                state->head_percent = (double)state->head_pcm_addr.i_val / state->song_pcm_duration;
            }
        }
    }

    // Return the total number of buffer writes
    return write_tally;
}

int zdj_decode_node_accum_s32p( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {
    printf( "zdj_decode_node_accum_s32p\n" );
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    zdj_decode_node_accum_state_t accum;
    _make_accum( node, packet, &accum );

    int chan_count = packet->av_frame->channels;

    int write_tally = 0;

    for( int i=0; i<=accum.out_buf_copy_len; i++ ) {
        for( int chan=1; chan<=chan_count; chan++ ) {
            // Map sample addresses - planar mapping for av_frame's data buf
            accum.packet_buf_sample = accum.packet_buf_copy_start+i;
            accum.packet_buf_index = accum.packet_buf_sample;

            accum.out_buf_sample = accum.out_buf_copy_start+i;
            accum.out_buf_index = (accum.out_buf_sample * chan_count) + chan;

            // Cast the av_frame's plane buffer to type
            int32_t * typed_ref = (int32_t*)packet->av_frame->data[ chan ];

            // Convert the av_frame sample to float and accumulate
            state->out_buffer[ accum.out_buf_index ] = _accum_floats(
                state->out_buffer[ accum.out_buf_index ],
                (float)typed_ref[ accum.packet_buf_index ] / (float)INT32_MAX
            );

            // Tally each buffer write
            write_tally++;

            // Store decode head addr
            if( accum.out_buf_sample == state->win_head_sample ) {
                state->head_pcm_addr.i_val = packet->packet_pcm_addr.i_val + i;
                state->head_percent = (double)state->head_pcm_addr.i_val / state->song_pcm_duration;
            }
        }
    }

    // Return the total number of buffer writes
    return write_tally;
}

int zdj_decode_node_accum_s64p( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {

}

int zdj_decode_node_accum_fltp( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    // printf( "zdj_decode_node_accum_fltp: %s / %d / %d / ls0: %d / ls1: %d / ns: %d / sz: %d\n", state->song->audio->filepath,
    //     av_sample_fmt_is_planar( packet->av_frame->format ),
    //     state->codec_ctx->channels,
    //     packet->av_frame->linesize[ 0 ],
    //     packet->av_frame->linesize[ 1 ],
    //     packet->av_frame->nb_samples,
    //     av_get_bytes_per_sample( packet->av_frame->format )
    // );

    zdj_decode_node_accum_state_t accum;
    _make_accum( node, packet, &accum );

    int chan_count = packet->av_frame->channels;

    // Sticking this here for now.  Can be more accurate if we capture the window head sample.
    // state->head_pcm_addr.i_val = packet->packet_pcm_addr.i_val;
    // state->head_percent = (double)state->head_pcm_addr.i_val / state->song_pcm_duration;
    // printf( "head: %1.3f: %ld/%1.3f\n", state->head_percent, state->head_pcm_addr.i_val, state->song_pcm_duration );

    int write_tally = 0;

    for( int i=0; i<=accum.out_buf_copy_len; i++ ) {
        for( int chan=1; chan<=chan_count; chan++ ) {
            // Map sample addresses - planar mapping for av_frame's data buf
            accum.packet_buf_sample = accum.packet_buf_copy_start+i;
            accum.packet_buf_index = accum.packet_buf_sample;

            accum.out_buf_sample = accum.out_buf_copy_start+i;
            accum.out_buf_index = (accum.out_buf_sample * chan_count) + chan;

            // Cast the av_frame's plane buffer to type
            float * typed_ref = packet->av_frame->data[ chan-1 ];

            // Convert the av_frame sample to float and accumulate            
            state->out_buffer[ accum.out_buf_index ] = _accum_floats(
                state->out_buffer[ accum.out_buf_index ],
                typed_ref[ accum.packet_buf_index ]
            );

            // Tally each buffer write
            write_tally++;

            // Store decode head addr
            if( accum.out_buf_sample == state->win_head_sample ) {
                state->head_pcm_addr.i_val = packet->packet_pcm_addr.i_val + i;
                state->head_percent = ((double)state->head_pcm_addr.i_val/44100.0) / state->song_pcm_duration;
                // printf( "head: %1.3f: %ld/%1.3f\n", state->head_percent, state->head_pcm_addr.i_val, state->song_pcm_duration );
            }
        }
    }

    // printf( "zdj_decode_node_accum_fltp done\n" );
    // Return the total number of buffer writes
    return write_tally;
}

int zdj_decode_node_accum_dblp( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {

}

float _ts1_p = 0;
float _ts1_f = 945;
float _ts2_p = 0;
float _ts2_f = 300;

int zdj_decode_node_accum_test_sine( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;

    // printf( "zdj_decode_node_accum_flt: %s / %d\n", state->song->audio->filepath, packet->av_frame->linesize[ 0 ] );

    zdj_decode_node_accum_state_t accum;
    _make_accum( node, packet, &accum );

    int chan_count = packet->av_frame->channels;

    int write_tally = 0;

    _ts1_p = zdj_signal_gen_sine( 
        _ts1_f, 
        _ts1_p, 
        accum.out_buf_copy_len,
        &state->out_buffer[ accum.out_buf_copy_start ],
        chan_count,
        0,
        (double)INT32_MAX
    );
    
    // Return the total number of buffer writes
    return write_tally;
}
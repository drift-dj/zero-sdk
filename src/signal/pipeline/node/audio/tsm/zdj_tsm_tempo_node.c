#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

#include <rubberband-c.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/deck/dj/zdj_deck_dj.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/signal/pipeline/node/audio/tsm/zdj_tsm_tempo_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

static void _update_wait( zdj_pipeline_node_t * node );
static void _deinit_state( zdj_pipeline_node_t * node );

zdj_pipeline_node_t * zdj_new_tsm_tempo_node( 
    bool stereo, 
    int sample_count,
    zdj_pipeline_node_t * decode_node 
) {
    // Make node
    zdj_pipeline_node_t * node = zdj_new_pipeline_node( );
    node->deinit_state = &_deinit_state;
    node->update_wait = &_update_wait;

    // Add state
    zdj_tsm_tempo_node_state_t * state = calloc( 1, sizeof( zdj_tsm_tempo_node_state_t ) );
    node->state = state;
    state->stereo = stereo;
    state->channel_count = stereo + 1;
    state->sample_count = sample_count;
    state->decode_node = decode_node;
    state->has_rate_update = false;

    // Capture starting state of decode out_buf indexes
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)decode_node->state;

    // Alloc output buffer
    state->out_buffer = calloc( state->sample_count * state->channel_count, sizeof( float ) );

    return node;
}

// Pull and stretch a cycle of samples from the decode node
static void _update_wait( zdj_pipeline_node_t * node ) {
    zdj_tsm_tempo_node_state_t * state = (zdj_tsm_tempo_node_state_t*)node->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)state->decode_node->state;

    if( state->rate > 1000.0 || state->rate < 0.0001 ) { return; }

    double rb_rate = 1.0 / state->rate;
    // printf( "tempo node rate: %1.4f / %1.4f\n", state->rate, rb_rate );
    // if( rb_rate > 0.001f ) { rubberband_set_time_ratio( state->rb, rb_rate ); } else { return; }
    rubberband_set_time_ratio( state->rb, rb_rate );
    
    // double a_start, b_start;
    // double a_end, b_end;
    // a_start = zdj_perf_time( );


    zdj_decode_addr_t win_start;
    zdj_decode_addr_t win_end;
    decode_state->get_win_start_addr( state->decode_node, &win_start );
    decode_state->get_win_end_addr( state->decode_node, &win_end );

    int done = 0;
    int source_index = 0;
    while ( done < ZDJ_SOUNDCARD_BUF_LEN ) {
        int available = rubberband_available( state->rb );
        if ( available < 0 ) break;
        int reqd = rubberband_get_samples_required( state->rb );
        
        // Decode node's window has likely moved since our last call.
        // Figure out where the last sample we copied falls in decode's current window.
        int sample_offset = state->decode_coord - win_start.transport_i;
        if( sample_offset < 0 ) { 
            printf( "TSM Missed %d decode samples\n", sample_offset * -1 ); 
            printf( "dc:%1.0f, tp_i:%lu\n", state->decode_coord, win_start.transport_i );
            sample_offset = 0; 
        }

        // Make some input buffers to feed to RB
        float * l_rb_in = calloc( ZDJ_SOUNDCARD_BUF_LEN, sizeof( float ) );
        float * r_rb_in = calloc( ZDJ_SOUNDCARD_BUF_LEN, sizeof( float ) );
        // De-interleave samples from the dcod buf
        
        if (available < (ZDJ_SOUNDCARD_BUF_LEN - done) || reqd > 0) {
            for (int i = 0; i < ZDJ_SOUNDCARD_BUF_LEN; ++i) {
                // RB does the interpolation, so we get frames at 1.0 offset
                if( state->decode_coord > win_end.transport_i ) { continue; }
                l_rb_in[ i ] = decode_state->out_buffer[ sample_offset * state->channel_count ];
                if( state->channel_count == 2 ) {
                    r_rb_in[ i ] = decode_state->out_buffer[ (sample_offset * state->channel_count) + 1 ];
                }
                sample_offset++;
                state->decode_coord++;
            }

            // Process the input samples
            const float * rb_in_channels[ 2 ] = { l_rb_in, r_rb_in }; 
            rubberband_process( state->rb, rb_in_channels, ZDJ_SOUNDCARD_BUF_LEN, false );
        }

        free( l_rb_in );
        free( r_rb_in );

        int count = rubberband_available( state->rb );

        // printf( "RB has %d samples\n", count );

        if ( count == 0 ) continue;
        if ( count > ( ZDJ_SOUNDCARD_BUF_LEN - done ) ) {
            count = ZDJ_SOUNDCARD_BUF_LEN - done;
        }

        float * l_rb_out = calloc( count, sizeof( float ) );
        float * r_rb_out = calloc( count, sizeof( float ) );
        float * rb_out_channels[ 2 ] = { l_rb_out, r_rb_out }; 

        // Grab the stretched samples
        size_t len = rubberband_retrieve( state->rb, rb_out_channels, count );
        // Note that q here is internal to the buffer of samples retrieved from the stretecher
        // There's no notion of where this buffer goes in the larger stream of output samples
        if( state->fade_out ) {
            // Handle a commanded fade-out across the buffer span
            float fade_val;
            for ( int q=0; q<len; q++ ) {
                int out_buf_index = (done*state->channel_count) + (q*state->channel_count);
                fade_val = (float)out_buf_index / ((float)ZDJ_SOUNDCARD_BUF_LEN * state->channel_count);
                state->out_buffer[ out_buf_index ] = rb_out_channels[0][q] * (1.0 - fade_val);
                if( state->channel_count == 2 ) { 
                    state->out_buffer[ out_buf_index+1 ] = rb_out_channels[1][q] * (1.0 - fade_val);
                }
            }
        } else {
            for ( int q=0; q<len; q++ ) {
                int out_buf_index = (done*state->channel_count) + (q*state->channel_count);
                state->out_buffer[ out_buf_index ] = rb_out_channels[0][q];
                if( state->channel_count == 2 ) { 
                    state->out_buffer[ out_buf_index+1 ] = rb_out_channels[1][q]; 
                }
            }
        }
        done += len;

        free( l_rb_out );
        free( r_rb_out );

    }
}

void zdj_reset_tsm_tempo_node( zdj_pipeline_node_t * node ) { 
    zdj_tsm_tempo_node_state_t * tsm_tempo_state = (zdj_tsm_tempo_node_state_t*)node->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)tsm_tempo_state->decode_node->state;
    
    // Set the read head to something reasonable
    tsm_tempo_state->decode_coord = decode_state->head.transport_d;

    // Clear RB's state
    rubberband_reset( tsm_tempo_state->rb );

    if( tsm_tempo_state->rate > 1000.0 || tsm_tempo_state->rate < 0.0001 ) { return; }

    double rb_rate = 1.0 / tsm_tempo_state->rate;
    rubberband_set_time_ratio( tsm_tempo_state->rb, rb_rate );

    // Gather data about the required pre-charge/discard settings for the given rate
    tsm_tempo_state->rb_preferred_start_pad = rubberband_get_preferred_start_pad( tsm_tempo_state->rb );
    tsm_tempo_state->rb_start_delay = rubberband_get_start_delay( tsm_tempo_state->rb );

    // Pre-charge the rubberband state with the required number of samples.
    if( tsm_tempo_state->channel_count == 1 ) {
        float l_rb_in[ tsm_tempo_state->rb_preferred_start_pad ];
        memset( l_rb_in, 0, tsm_tempo_state->rb_preferred_start_pad * sizeof( float ) );
        const float * rb_in_channels[ 1 ] = { l_rb_in }; 
        rubberband_process( tsm_tempo_state->rb, rb_in_channels, tsm_tempo_state->rb_preferred_start_pad, false );

        float l_rb_out[ tsm_tempo_state->rb_start_delay ];
        float * rb_out_channels[ 1 ] = { l_rb_out }; 
        size_t len = rubberband_retrieve( tsm_tempo_state->rb, rb_out_channels, tsm_tempo_state->rb_start_delay );

    } else if ( tsm_tempo_state->channel_count == 2 ) {
        float l_rb_in[ tsm_tempo_state->rb_preferred_start_pad ];
        float r_rb_in[ tsm_tempo_state->rb_preferred_start_pad ];
        memset( l_rb_in, 0, tsm_tempo_state->rb_preferred_start_pad * sizeof( float ) );
        memset( r_rb_in, 0, tsm_tempo_state->rb_preferred_start_pad * sizeof( float ) );
        const float * rb_in_channels[ 2 ] = { l_rb_in, r_rb_in }; 
        rubberband_process( tsm_tempo_state->rb, rb_in_channels, tsm_tempo_state->rb_preferred_start_pad, false );

        float l_rb_out[ tsm_tempo_state->rb_start_delay ];
        float r_rb_out[ tsm_tempo_state->rb_start_delay ];
        float * rb_out_channels[ 2 ] = { l_rb_out, r_rb_out }; 

        int count = rubberband_available( tsm_tempo_state->rb );
        size_t len = rubberband_retrieve( tsm_tempo_state->rb, rb_out_channels, count );
    }

    // printf( "zdj_reset_tsm_tempo_node done\n" );
}

static void _deinit_state( zdj_pipeline_node_t * node ) {

}

void zdj_tsm_tempo_node_clear_out_buf( zdj_pipeline_node_t * node ) {
    zdj_tsm_tempo_node_state_t * state = (zdj_tsm_tempo_node_state_t*)node->state;
    memset( (void*)state->out_buffer, 0, state->sample_count * state->channel_count * sizeof( float ) );
}

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>

#include <rubberband-c.h>

#include <zerodj/library/zdj_library.h>
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

    double rb_rate = 1.0 / state->rate;

    // if( state->rate > 0.02f ) { rubberband_set_time_ratio( state->rb, state->rate ); } else { return; }
    if( rb_rate > 0.001f ) { rubberband_set_time_ratio( state->rb, rb_rate ); } else { return; }
    
    // printf( "tempo node rate: %1.1f / %1.1f\n", state->rate, rb_rate );

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
        // printf( "tempo_decode_coord: %1.1f decode_win_start: %ld\n", state->decode_coord, decode_state->head_win_start );
        // printf( "sample offset: %d - %1.1f\n", sample_offset, state->decode_coord );
        if( sample_offset < 0 ) { printf( "TSM Missed %d decode samples\n", sample_offset * -1 ); sample_offset = 0; }

        // Make some input buffers to feed to RB
        float * l_rb_in = calloc( ZDJ_SOUNDCARD_BUF_LEN, sizeof( float ) );
        float * r_rb_in = calloc( ZDJ_SOUNDCARD_BUF_LEN, sizeof( float ) );
        // De-interleave samples from the dcod buf
        
        if (available < (ZDJ_SOUNDCARD_BUF_LEN - done) || reqd > 0) {
            
            // // Make some input buffers to feed to RB
            // float l_rb_in[ ZDJ_SOUNDCARD_BUF_LEN ] = { 0 };
            // float r_rb_in[ ZDJ_SOUNDCARD_BUF_LEN ] = { 0 };
            // // De-interleave samples from the dcod buf

            // double a_start = zdj_perf_time( );
            for (int i = 0; i < ZDJ_SOUNDCARD_BUF_LEN; ++i) {
                // RB does the interpolation, so we get frames at 1.0 offset
                if( state->decode_coord > win_end.transport_i ) { /*printf( "TSM Tried to read beyond end of decode window\n" ); */continue; }
                l_rb_in[ i ] = decode_state->out_buffer[ sample_offset * state->channel_count ];
                if( state->channel_count == 2 ) {
                    r_rb_in[ i ] = decode_state->out_buffer[ (sample_offset * state->channel_count) + 1 ];
                }
                sample_offset++;
                state->decode_coord++;
            }
            // double a_end = zdj_perf_time( );
            // printf( "t:%1.3f\n", (a_end - a_start) / 1000000.0 );
            
            // Process the input samples
            // const float * rb_in_channels[ 2 ] = { l_rb_in, r_rb_in }; 
            const float * rb_in_channels[ 2 ] = { l_rb_in, r_rb_in }; 

            // a_start = zdj_perf_time( );
            
            rubberband_process( state->rb, rb_in_channels, ZDJ_SOUNDCARD_BUF_LEN, false );

            // a_end = zdj_perf_time( );
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

        // b_start = zdj_perf_time( );

        // Grab the stretched samples
        size_t len = rubberband_retrieve( state->rb, rb_out_channels, count );
        // Note that q here is internal to the buffer of samples retrieved from the stretecher
        // There's no notion of where this buffer goes in the larger stream of output samples
        for ( int q=0; q<len; q++ ) {
            int out_buf_index = (done*state->channel_count) + (q*state->channel_count);
            // printf( "%d -> %d\n", q, out_buf_index );
            state->out_buffer[ out_buf_index ] = rb_out_channels[0][q];
            if( state->channel_count == 2 ) { state->out_buffer[ out_buf_index+1 ] = rb_out_channels[1][q]; }
        }
        done += len;

        free( l_rb_out );
        free( r_rb_out );

        // b_end = zdj_perf_time( );

        // double a_tot = (a_end - a_start) / 1000000.0;
        // double b_tot = (b_end - b_start) / 1000000.0;
        // if( a_tot+b_tot > 4.0 ) {
            // printf( "a:%1.1f b:%1.1f\n", a_tot, b_tot );
        // }
    }

    // p_end = zdj_perf_time( );
    // double p_tot = (p_end - p_start) / 1000000.0;
    // if( p_tot > 4.0 ) {
    //     printf( " tsm:%1.5f ", p_tot );
    // }
}

void zdj_reset_tsm_tempo_node( zdj_pipeline_node_t * node ) { 
    // printf( "zdj_reset_tsm_tempo_node\n" );
    zdj_tsm_tempo_node_state_t * state = (zdj_tsm_tempo_node_state_t*)node->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)state->decode_node->state;

    // Clear RB's state
    rubberband_reset( state->rb );

    // Update the time ratio
    if( state->rate > 0.02f ) { rubberband_set_time_ratio( state->rb, state->rate ); } else {
        return;
    }

    // Gather data about the required pre-charge/discard settings for the given rate
    state->rb_preferred_start_pad = rubberband_get_preferred_start_pad( state->rb );
    state->rb_start_delay = rubberband_get_start_delay( state->rb );

    // printf( "pad %ld delay %ld\n", state->rb_preferred_start_pad, state->rb_start_delay );   

    // Pre-charge the rubberband state with the required number of samples.
    if( state->channel_count == 1 ) {
        float l_rb_in[ state->rb_preferred_start_pad ];
        memset( l_rb_in, 0, state->rb_preferred_start_pad * sizeof( float ) );
        const float * rb_in_channels[ 1 ] = { l_rb_in }; 
        rubberband_process( state->rb, rb_in_channels, state->rb_preferred_start_pad, false );

        float l_rb_out[ state->rb_start_delay ];
        float * rb_out_channels[ 1 ] = { l_rb_out }; 
        size_t len = rubberband_retrieve( state->rb, rb_out_channels, state->rb_start_delay );

    } else if ( state->channel_count == 2 ) {
        float l_rb_in[ state->rb_preferred_start_pad ];
        float r_rb_in[ state->rb_preferred_start_pad ];
        memset( l_rb_in, 0, state->rb_preferred_start_pad * sizeof( float ) );
        memset( r_rb_in, 0, state->rb_preferred_start_pad * sizeof( float ) );
        const float * rb_in_channels[ 2 ] = { l_rb_in, r_rb_in }; 
        // printf( "stereo process\n" );
        rubberband_process( state->rb, rb_in_channels, state->rb_preferred_start_pad, false );

        float l_rb_out[ state->rb_start_delay ];
        float r_rb_out[ state->rb_start_delay ];
        float * rb_out_channels[ 2 ] = { l_rb_out, r_rb_out }; 

        int count = rubberband_available( state->rb );
        // printf( "stereo retrieve: %p %p count: %d\n", state->rb, rb_out_channels, count );
        size_t len = rubberband_retrieve( state->rb, rb_out_channels, count );
        // printf( "stereo retrieve done\n" );
    }
    
    
    // // Pull the start_delay back out of the output and discard
    

    // printf( "zdj_reset_tsm_tempo_node done\n" );
}

static void _deinit_state( zdj_pipeline_node_t * node ) {

}
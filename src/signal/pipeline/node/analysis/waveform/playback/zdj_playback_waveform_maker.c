#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/analysis/waveform/zdj_waveform.h>
#include <zerodj/signal/pipeline/node/audio/library_decode/zdj_library_decode_node.h>
#include <zerodj/ui/zdj_ui.h>

static void _update_wait( zdj_pipeline_node_t * node );
static void _deinit_state( zdj_pipeline_node_t * node );

static void _waveform_maker_prep_window( zdj_pipeline_node_t * node );
static void _waveform_maker_wait_window( zdj_pipeline_node_t * node );
static void _waveform_maker_capture_window( zdj_pipeline_node_t * node );

zdj_pipeline_node_t * zdj_new_playback_waveform_maker( 
    zdj_pipeline_node_t * decode_node,
    char * filepath,
    int samples_per_point,
    int hi_pass_freq
) {
    zdj_library_decode_node_state_t * decode_state = (zdj_library_decode_node_state_t*)decode_node->state;

    // printf( "%s zdj_new_playback_waveform_maker\n", decode_state->song->audio->filepath );

    zdj_pipeline_node_t * maker = zdj_new_pipeline_node( );
    maker->update_wait = &_update_wait;
    maker->deinit_state = &_deinit_state;

    // Set up state
    zdj_playback_waveform_maker_state_t * state = calloc( 1, sizeof( zdj_playback_waveform_maker_state_t ) );
    state->phase = ZDJ_WAVEFORM_MAKER_PHASE_PREP_WINDOW;
    maker->state = state;

    state->decode_node = decode_node;
    strcpy( state->song_entity_id, decode_state->song->entity_id );

    // Set up point buffer
    state->window_width = 44100 / hi_pass_freq;
    state->kernel = zdj_new_gaussian( state->window_width, 1.0 );
    state->input_sample_counter = 0;
    state->total_points = 0;
    state->point_tally = 1;
    state->point_stride = samples_per_point;
    state->window_buf = calloc( state->window_width, sizeof( float ) );
    state->window_start_pcm_addr = 0;

    // Set up waveform output file
    state->waveform_header = calloc( 1, sizeof( zdj_playback_waveform_header_t ) );
    strcpy( state->waveform_header->song_entity_id, decode_state->song->entity_id );
    state->waveform_fd = fopen( filepath, "w" );
    if( !state->waveform_fd ) { return NULL; }
    fwrite( state->waveform_header, sizeof( zdj_playback_waveform_header_t ), 1, state->waveform_fd );

    // printf( "%s zdj_new_playback_waveform_maker done: %p\n", decode_state->song->audio->filepath, maker );

    return maker;
}

void zdj_close_playback_waveform_maker( zdj_pipeline_node_t * node ) {
    // Update waveform header with point count and re-write
    zdj_playback_waveform_maker_state_t * node_state = (zdj_playback_waveform_maker_state_t*)node->state;
    node_state->waveform_header->frame_count = node_state->point_tally;
    fseek( node_state->waveform_fd, 0, SEEK_SET );
    fwrite( node_state->waveform_header, sizeof( zdj_playback_waveform_header_t ), 1, node_state->waveform_fd );
    fclose( node_state->waveform_fd );
}

static void _waveform_maker_prep_window( zdj_pipeline_node_t * node ) {
    // printf( "_waveform_maker_prep_window: %p\n", node );
    zdj_playback_waveform_maker_state_t * waveform_state = (zdj_playback_waveform_maker_state_t*)node->state;
    zdj_library_decode_node_state_t * decode_state = (zdj_library_decode_node_state_t*)waveform_state->decode_node->state;

    // printf( "%s _waveform_maker_prep_window: %ld\n", decode_state->song->audio->filepath, decode_state->pcm_addr.i_val );

    // Clear out the window_buf
    memset( waveform_state->window_buf, 0.0, waveform_state->window_width );

    // Setup the window start PCM address to capture the next point
    waveform_state->window_start_pcm_addr = waveform_state->point_tally * waveform_state->point_stride;
    waveform_state->point_tally++;
    waveform_state->window_cur_sample = 0;

    // printf( "window_start_pcm_addr: %ld\n", waveform_state->window_start_pcm_addr  );

    // Advance to wait phase
    waveform_state->phase = ZDJ_WAVEFORM_MAKER_PHASE_WAIT_WINDOW;
    _waveform_maker_wait_window( node );
}

static void _waveform_maker_wait_window( zdj_pipeline_node_t * node ) { 
    zdj_playback_waveform_maker_state_t * node_state = (zdj_playback_waveform_maker_state_t*)node->state;
    zdj_library_decode_node_state_t * decode_state = (zdj_library_decode_node_state_t*)node_state->decode_node->state;


    // printf( "_waveform_maker_wait_window win:%ld  dcd[%ld,%ld]\n", 
    //     node_state->window_start_pcm_addr,
    //     decode_start_addr,
    //     decode_end_addr
    // );

    if( decode_state->pcm_addr > node_state->window_start_pcm_addr ) {
        // If decode window's pcm addr has passed the addr we're waiting for, mark and start capturing
        node_state->phase = ZDJ_WAVEFORM_MAKER_PHASE_CAPTURE_WINDOW;
        _waveform_maker_capture_window( node );
    }
}


// Note that while we're capturing, window is unlikely to fill up from a single
// decode_node->out_buffer.  Do this statefully so we can capture as many samples
// as required across multiple decode_node->out_buffers.
static void _waveform_maker_capture_window( zdj_pipeline_node_t * node ) {
    // printf( "_waveform_maker_capture_window\n" );
    zdj_playback_waveform_maker_state_t * waveform_state = (zdj_playback_waveform_maker_state_t*)node->state;
    zdj_library_decode_node_state_t * decode_state = (zdj_library_decode_node_state_t*)waveform_state->decode_node->state;

    // printf( "decode_state->available_samples: %ld\n", decode_state->available_samples );
    // printf( "decode_state->pcm_addr: %ld\n", decode_state->pcm_addr );
    // printf( "waveform_state->window_start_pcm_addr: %ld\n", waveform_state->window_start_pcm_addr );
    // printf( "waveform_state->window_width: %d\n", waveform_state->window_width );
    // printf( "waveform_state->window_cur_sample: %d\n", waveform_state->window_cur_sample );
    // Fill window from decode_node out_buffer - we've already confirmed our window address
    // is within the bounds of the decode_node buffer.
    // int64_t decode_end_addr = decode_state->pcm_addr;
    // int64_t decode_start_addr = decode_end_addr - decode_state->available_samples;
    // Move back from end of decode's available samples
    int decode_buf_copy_sample = decode_state->available_samples - ( decode_state->pcm_addr - waveform_state->window_start_pcm_addr );
    int decode_buf_copy_len = fmin( 
        decode_state->available_samples - decode_buf_copy_sample, // From copy index to the decode's window end
        waveform_state->window_width - waveform_state->window_cur_sample // From copy index to waveform's window end
    );

    // printf( "capturing %d samps @%d %s\n", decode_buf_copy_len, decode_buf_copy_sample, decode_state->song->audio->filepath );
    double val = 0.0;
    for( int i=decode_buf_copy_sample; i<decode_buf_copy_sample+decode_buf_copy_len; i++ ) {
        int index = (i*decode_state->channel_count);
        waveform_state->window_buf[ waveform_state->window_cur_sample ] = fabs( decode_state->out_buffer[ index ] );
        waveform_state->window_cur_sample++;
    }

    // printf( "waveform_state->window_cur_sample: %d\n", waveform_state->window_cur_sample );
    
    // If the window is full, convolve using the kernel and write a new point to the file.
    if( waveform_state->window_cur_sample == waveform_state->window_width ) {
        double tally = 0.0;
        // Run window thru kernel
        for( int i=0; i<waveform_state->window_width; i++ ) {
            tally += waveform_state->window_buf[ i ] * waveform_state->kernel->lut[ i ];
        }
        tally /= (double)waveform_state->window_width;
        uint8_t point = (uint8_t)(tally * 255);
        waveform_state->point_tally++;

        // Write point to file
        // printf( "point: %f\n", tally );
        fwrite( &point, sizeof( uint8_t ), 1, waveform_state->waveform_fd );
    }

    waveform_state->phase = ZDJ_WAVEFORM_MAKER_PHASE_PREP_WINDOW;
}

// Note waveform maker behavior is undefined if discontinuities are present in decode_node.
static void _update_wait( zdj_pipeline_node_t * node ) {
    // printf( "waveform_maker _update_wait\n" );
    zdj_playback_waveform_maker_state_t * node_state = (zdj_playback_waveform_maker_state_t*)node->state;
    zdj_library_decode_node_state_t * decode_state = (zdj_library_decode_node_state_t*)node_state->decode_node->state;
    node_state->input_sample_counter += decode_state->available_samples;

    switch( node_state->phase ) {
        case ZDJ_WAVEFORM_MAKER_PHASE_PREP_WINDOW:
            _waveform_maker_prep_window( node );
            break;
        case ZDJ_WAVEFORM_MAKER_PHASE_WAIT_WINDOW:
            _waveform_maker_wait_window( node );
            break;
        case ZDJ_WAVEFORM_MAKER_PHASE_CAPTURE_WINDOW:
            _waveform_maker_capture_window( node );
            break;
    }
    // printf( "waveform_maker _update_wait done\n" );
}

static void _deinit_state( zdj_pipeline_node_t * node ) {
    zdj_playback_waveform_maker_state_t * state = (zdj_playback_waveform_maker_state_t*)node->state;
    if( state->waveform_header ) { free( state->waveform_header ); }
    if( state->kernel ) { zdj_gaussian_free( state->kernel ); }
}
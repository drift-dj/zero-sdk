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
static void _waveform_maker_build_point( zdj_pipeline_node_t * node );

zdj_pipeline_node_t * zdj_new_waveform_maker( 
    zdj_pipeline_node_t * decode_node,
    char * filepath,
    int samples_per_point,
    int hi_pass_freq
) {
    zdj_library_decode_node_state_t * decode_state = (zdj_library_decode_node_state_t*)decode_node->state;

    // printf( "%s zdj_new_waveform_maker\n", decode_state->song->audio->filepath );

    zdj_pipeline_node_t * maker = zdj_new_pipeline_node( );
    maker->update_wait = &_update_wait;
    maker->deinit_state = &_deinit_state;

    // Set up state
    zdj_waveform_maker_state_t * state = calloc( 1, sizeof( zdj_waveform_maker_state_t ) );
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

    // printf( "%s zdj_new_waveform_maker done: %p\n", decode_state->song->audio->filepath, maker );

    return maker;
}

void zdj_close_waveform_maker( zdj_pipeline_node_t * node ) {
    // printf( "zdj_close_waveform_maker\n" );
    // Update waveform header with point count and re-write
    zdj_waveform_maker_state_t * node_state = (zdj_waveform_maker_state_t*)node->state;
    node_state->waveform_header->frame_count = node_state->point_tally;
    fseek( node_state->waveform_fd, 0, SEEK_SET );
    fwrite( node_state->waveform_header, sizeof( zdj_playback_waveform_header_t ), 1, node_state->waveform_fd );
    fclose( node_state->waveform_fd );
}

static void _waveform_maker_prep_window( zdj_pipeline_node_t * node ) {
    // printf( "_waveform_maker_prep_window: %p\n", node );
    zdj_waveform_maker_state_t * waveform_state = (zdj_waveform_maker_state_t*)node->state;
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
    zdj_waveform_maker_state_t * node_state = (zdj_waveform_maker_state_t*)node->state;
    zdj_library_decode_node_state_t * decode_state = (zdj_library_decode_node_state_t*)node_state->decode_node->state;


    // printf( "_waveform_maker_wait_window win:%ld  dcd[%ld,%ld]\n", 
    //     node_state->window_start_pcm_addr,
    //     decode_start_addr,
    //     decode_end_addr
    // );

    if( decode_state->pcm_addr >= node_state->window_start_pcm_addr ) {
        // If decode window's pcm addr has passed the addr we're waiting for, mark and start capturing
        node_state->previous_decode_pcm_addr = node_state->window_start_pcm_addr;
        node_state->phase = ZDJ_WAVEFORM_MAKER_PHASE_CAPTURE_WINDOW;
        _waveform_maker_capture_window( node );
    }
}


int test_tally = 0;
// Note that while we're capturing, window is unlikely to fill up from a single
// decode_node->out_buffer.  Do this statefully so we can capture as many samples
// as required across multiple decode_node->out_buffers.
static void _waveform_maker_capture_window( zdj_pipeline_node_t * node ) {
    // printf( "_waveform_maker_capture_window\n" );
    zdj_waveform_maker_state_t * waveform_state = (zdj_waveform_maker_state_t*)node->state;
    zdj_library_decode_node_state_t * decode_state = (zdj_library_decode_node_state_t*)waveform_state->decode_node->state;
    
    
    // Find decode node start sample
    // int decode_start_sample = decode_state->pcm_addr - waveform_state->window_start_pcm_addr;
    int decode_start_sample = decode_state->pcm_addr - waveform_state->previous_decode_pcm_addr;

    // Find decode out_buf start index
    int decode_start_index = decode_start_sample * decode_state->channel_count;

    // Find waveform window start index
    int waveform_start_index = waveform_state->window_cur_sample;

    // Find sample count - the shorter of:
    //  - remaining samples required to fill waveform point window
    int remaining_waveform_samples = waveform_state->window_width - waveform_state->window_cur_sample;
    //  - samples available in decode_node, noting possible non-zero start sample
    int available_decode_samples = decode_state->available_samples - decode_start_sample;

    int sample_count = fmin( remaining_waveform_samples, available_decode_samples );

    // if( test_tally++ < 5 ) {
    //     printf( "decode_state->pcm_addr: %ld\n", decode_state->pcm_addr );
    //     printf( "available_decode_samples: %d\n", available_decode_samples );
    //     printf( "waveform_state->window_start_pcm_addr: %ld\n", waveform_state->window_start_pcm_addr );
    //     printf( "decode_start_sample: %d\n", decode_start_sample );
    //     printf( "decode_start_index: %d\n", decode_start_index );
    //     printf( "waveform_start_index: %d\n", waveform_start_index );
    //     printf( "remaining_waveform_samples: %d\n", remaining_waveform_samples );
    //     printf( "\n" );
    // }

    waveform_state->previous_decode_pcm_addr = decode_state->pcm_addr + decode_state->available_samples;

    // printf( "samps: %d %d %d %ld\n", sample_count, available_decode_samples, decode_start_sample, waveform_state->window_start_pcm_addr );
    float decode_samp;
    for( int i=0; i<sample_count; i++ ) {
        decode_samp = decode_state->out_buffer[ (i + decode_start_index) * decode_state->channel_count ];
        waveform_state->window_buf[ i + waveform_start_index ] = fabs(decode_samp);
        // printf( "decode[ %d+%d=%d ]  window[ %d+%d=%d ] = %1.3f\n", 
        //     i, decode_start_index, i+decode_start_index,
        //     i, waveform_start_index, i+waveform_start_index, 
        //     waveform_state->window_buf[ i + waveform_start_index ] 
        // );
        waveform_state->window_cur_sample++;
    }

    // Exit capture once we've filled the window
    if( waveform_state->window_cur_sample >= waveform_state->window_width ) {
        // printf( "building point from %d samples\n", waveform_state->window_cur_sample );
        _waveform_maker_build_point( node );
    }
}

static void _waveform_maker_build_point( zdj_pipeline_node_t * node ) {
    zdj_waveform_maker_state_t * waveform_state = (zdj_waveform_maker_state_t*)node->state;
    
    double tally = 0.0;
    // Run window thru kernel
    for( int i=0; i<waveform_state->window_width; i++ ) {
        // tally += waveform_state->window_buf[ i ] * waveform_state->kernel->lut[ i ];
        tally += waveform_state->window_buf[ i ];
    }
    tally /= (double)waveform_state->window_width;
    uint8_t point = (uint8_t)(tally * 255);

    // Write point to file
    // printf( "point[ %1.0f ]: %d\n", waveform_state->point_tally, point );
    fwrite( &point, sizeof( uint8_t ), 1, waveform_state->waveform_fd );

    waveform_state->phase = ZDJ_WAVEFORM_MAKER_PHASE_PREP_WINDOW;
}

// Note waveform maker behavior is undefined if discontinuities are present in decode_node.
static void _update_wait( zdj_pipeline_node_t * node ) {
    // printf( "waveform_maker _update_wait\n" );
    zdj_waveform_maker_state_t * node_state = (zdj_waveform_maker_state_t*)node->state;
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
        case ZDJ_WAVEFORM_MAKER_PHASE_BUILD_POINT:
            _waveform_maker_build_point( node );
            break;
    }
    // printf( "waveform_maker _update_wait done\n" );
}

static void _deinit_state( zdj_pipeline_node_t * node ) {
    zdj_waveform_maker_state_t * state = (zdj_waveform_maker_state_t*)node->state;
    if( state->waveform_header ) { free( state->waveform_header ); }
    if( state->kernel ) { zdj_gaussian_free( state->kernel ); }
}
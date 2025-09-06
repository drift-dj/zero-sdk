#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/analysis/waveform/zdj_waveform.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/ui/zdj_ui.h>

static void _zdj_live_waveform_render( zdj_pipeline_node_t * waveform );
static void _handle_soundcard_node_push( void * _waveform, zdj_pipeline_node_t * mix_buffer, bool stereo );

zdj_pipeline_node_t * zdj_new_live_waveform( void ) {
    zdj_pipeline_node_t * waveform = zdj_new_pipeline_node( );

    zdj_live_waveform_state_t * state = calloc( 1, sizeof( zdj_live_waveform_state_t ) );
    waveform->state = state;
    state->render = &_zdj_live_waveform_render;
    state->handle_soundcard_node_push = &_handle_soundcard_node_push;
    state->source_buf = calloc( ZDJ_LIVE_WAVEFORM_SAMPLE_COUNT, sizeof( float ) );
    // Render buf len is variable, but won't be larger than screen w.
    state->render_buf = calloc( ZDJ_SCREEN_W, sizeof( float ) );

    state->samples_per_point = 60;
    state->point_count = 64;
    // state->g = zdj_new_gaussian( state->samples_per_point, 1.0 );
    state->g = zdj_new_gaussian( state->point_count, 1.0 );

    return waveform;
}

// Input will come from UI layer as user adjust scale control.
zdj_error_type_t zdj_live_waveform_set_scale( zdj_pipeline_node_t * waveform, float scale ) {
    zdj_live_waveform_state_t * waveform_state = (zdj_live_waveform_state_t*)waveform->state;
    // Re-init gaussian with kernel size based on scale val
    if( waveform_state->g ) { zdj_gaussian_free( waveform_state->g ); }
    // waveform_state->g = zdj_new_gaussian( scale, 1.0 );
    waveform_state->g = zdj_new_gaussian( scale, scale );
    // Set samples_per_point based on scale val
    waveform_state->samples_per_point = scale;
}

// Input will come at init time, as UI layer declares rendering params.
zdj_error_type_t zdj_live_waveform_set_point_count( zdj_pipeline_node_t * waveform, int point_count ) {
    zdj_live_waveform_state_t * waveform_state = (zdj_live_waveform_state_t*)waveform->state;
    waveform_state->point_count = point_count;
}

// Head and tail are complicated by the anti-aliasing used to scale samples down
// for rendering on the screen.  Each point on screen uses a gaussian kernel to 
// convolve multiple samples on either side of the point's sample centroid.  
// So extra samples are required at the edges to give the first/last points on screen
// enough samples to convolve thru the kernel.
// Thus, source_push_head includes the extra samples needed for that kernel.
// Whereas source_render_head is the centroid in buffer space for the right-most on-screen point.

// Push head is advanced during soundcard_node_push.
// Remaining head/tail are re-calculated from push_head during render.

// Simplified ring buffer layout
// Note the extra space for anti-aliasing samples between push/render head/tail.

//       v-extent of anti-alias kernel behind tail    source_push_head-v
// [ x,x,`,x,x,|,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,x,|,x,x,|,x,x,x,x,x ]
//             ^-source_render_tail           source_render_head-^


// Bring a batch of samples over from a soundcard node's mix buffer.
// This copies the new samples starting at current head moving forward in local ring buf.
void _handle_soundcard_node_push( void * _waveform, zdj_pipeline_node_t * mix_buffer, bool stereo ) {
    zdj_pipeline_node_t * waveform = (zdj_pipeline_node_t*)_waveform;
    zdj_live_waveform_state_t * waveform_state = (zdj_live_waveform_state_t*)waveform->state;

    int stride = stereo ? 2 : 1;
    float * dest_buf = mix_buffer->get_data( mix_buffer );
    int dest_index = ceil( waveform_state->source_push_head );
    for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
        // If stereo, copy left channel only
        waveform_state->source_buf[ dest_index ] = dest_buf[ i*stride ];
        // Advance index, bounding it within ring buf.
        dest_index++;
        dest_index %= ZDJ_LIVE_WAVEFORM_SAMPLE_COUNT;
    }

    // Since head is a float, can't use modulo to bound it within ring buf
    waveform_state->source_push_head += (float)ZDJ_SOUNDCARD_BUF_LEN;
    if( waveform_state->source_push_head > (float)ZDJ_LIVE_WAVEFORM_SAMPLE_COUNT ) {
        waveform_state->source_push_head -= (float)ZDJ_LIVE_WAVEFORM_SAMPLE_COUNT;
    }

}

void _zdj_live_waveform_render( zdj_pipeline_node_t * waveform ) {
    zdj_live_waveform_state_t * waveform_state = (zdj_live_waveform_state_t*)waveform->state;
    // Calculate full width of source sample count.
    // Assume samples_per_point has been limited to prevent asking for a buffer
    // larger than the compile-time storage defined at ZDJ_LIVE_WAVEFORM_SAMPLE_COUNT.
    float new_source_win_w = waveform_state->samples_per_point * waveform_state->point_count + waveform_state->g->width;

    // If a new window width is wider than current width,
    // we have to step up cycle-by-cycle to new window width.
    // We want to avoid a rapidly increased window size showing
    // garbage samples at the beginning of the window because
    // sample buffers haven't filled in the space yet.
    waveform_state->source_win_w = fmin( new_source_win_w, waveform_state->source_win_w + (float)ZDJ_SOUNDCARD_BUF_LEN );

    waveform_state->source_render_head = waveform_state->source_push_head - (waveform_state->g->width/2);
    if( waveform_state->source_render_head < 0 ) {
        waveform_state->source_render_head += (float)ZDJ_LIVE_WAVEFORM_SAMPLE_COUNT;
    }

    // Offset tail from head and wrap around ring buffer.
    waveform_state->source_render_tail = waveform_state->source_render_head - waveform_state->source_win_w;
    if( waveform_state->source_render_tail < 0 ) {
        waveform_state->source_render_tail += (float)ZDJ_LIVE_WAVEFORM_SAMPLE_COUNT;
    }

    // Starting at source_render_tail, convolve multiple source samples into render point 
    // until we reach source_render_head.
    // Stride render points over source buf by samples_per_point.
    // Calculate buffer space addresses in terms of centroids between samples, 
    // then linear interpolate samples to get actual value.
    int source_ind = 0;
    int temp_source_ind = 0;
    int s;
    int half_stride = floor( waveform_state->samples_per_point / 1.8 );
    float val;
    for( int i=0; i<waveform_state->point_count; i++ ) {
        source_ind = (i*waveform_state->samples_per_point) + waveform_state->source_render_tail;
        source_ind %= ZDJ_LIVE_WAVEFORM_SAMPLE_COUNT;
        val = waveform_state->source_buf[ source_ind ];
        // val = waveform_state->source_buf[ source_ind ] * waveform_state->g->lut[ 0 ];
        // run out to samples_per_point/2 in each direction to get average over samples_per_point
        for( s=1; s<half_stride; s++ ) {
            temp_source_ind = source_ind + s;
            temp_source_ind %= ZDJ_LIVE_WAVEFORM_SAMPLE_COUNT;
            val += waveform_state->source_buf[ temp_source_ind ];
            temp_source_ind = source_ind - s;
            if( temp_source_ind < 0 ) { temp_source_ind += ZDJ_LIVE_WAVEFORM_SAMPLE_COUNT; }
            val += waveform_state->source_buf[ temp_source_ind ];
            
            // temp_source_ind = source_ind + s;
            // temp_source_ind %= ZDJ_LIVE_WAVEFORM_SAMPLE_COUNT;
            // val += waveform_state->source_buf[ temp_source_ind ] * waveform_state->g->lut[ s ];
            // temp_source_ind = source_ind - s;
            // if( temp_source_ind < 0 ) { temp_source_ind += ZDJ_LIVE_WAVEFORM_SAMPLE_COUNT; }
            // val += waveform_state->source_buf[ temp_source_ind ] * waveform_state->g->lut[ s ];
        }
        val /= waveform_state->samples_per_point;
        waveform_state->render_buf[ i ] = val;
    }

    // Rendered point array will be used by view to draw waveform lines.
}
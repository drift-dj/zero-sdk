#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/analysis/waveform/zdj_waveform.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/ui/zdj_ui.h>

static void _render( zdj_pipeline_node_t * node, zdj_rect_t * frame );
static void _deinit_state( zdj_pipeline_node_t * node );

zdj_pipeline_node_t * zdj_new_thumbnail_waveform( char * filepath, int pixel_width ) {
    // printf( "loading thumb: %s\n", filepath );
    if( access( filepath, F_OK ) != 0 ) { return NULL; }

    zdj_pipeline_node_t * waveform = zdj_new_pipeline_node( );

    zdj_waveform_state_t * state = calloc( 1, sizeof( zdj_waveform_state_t ) );
    waveform->state = state;
    state->render = &_render;

    // Load header
    state->waveform_header = calloc( 1, sizeof( zdj_waveform_header_t ) );
    state->waveform_fd = fopen( filepath, "r" );
    fread( 
        state->waveform_header, 
        sizeof( zdj_waveform_header_t ), 
        1, 
        state->waveform_fd 
    );
    // Load entire set of points from data file
    state->point_buf = calloc( state->waveform_header->frame_count, sizeof( uint8_t ) );
    fread( state->point_buf, sizeof( uint8_t ), state->waveform_header->frame_count, state->waveform_fd );
    return waveform;
}

static void _render( zdj_pipeline_node_t * node, zdj_rect_t * frame ) {
    // printf( "thumb waveform _render\n" );
    zdj_waveform_state_t * state = (zdj_waveform_state_t*)node->state;
    // Build a point stride and set of screen dimensions based on view frame
    float point_count = (float)state->waveform_header->frame_count;
    float point_stride = (frame->w / point_count);    

    // Draw each point in the waveform into the texture
    float x = 0.0f;
    float h;
    for( int i=0; i<point_count; i++ ) {
        h = ( (float)state->point_buf[ i ] / (float)state->waveform_header->norm_val ) * frame->h;
        lineColor( zdj_renderer( ), round(x), frame->h, round(x), frame->h-h, ZDJ_WHITE );
        x += point_stride;
        // printf( "x: %1.3f\n", x );
    }
}

static void _deinit_state( zdj_pipeline_node_t * node ) {
    
}
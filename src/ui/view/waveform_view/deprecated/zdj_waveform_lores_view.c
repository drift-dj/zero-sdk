#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/library/waveform/build/zdj_waveform_builder.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/waveform_view/zdj_waveform_view.h>

static void _zdj_waveform_comp_view_draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_waveform_comp_view_deinit_state( zdj_view_t * view );

static int _playback_screen_x_for_sample_time( int sample_time, int win_start_sample, int win_end_sample, int screen_w );
static int _playback_sample_time_for_waveform_frame( int waveform_frame );

zdj_view_t * zdj_new_waveform_lores_view( zdj_rect_t * frame, zdj_library_song_t * song ) {
    // Open waveform file + read header
    // char filepath[ 2048 ];
    // snprintf( filepath, sizeof( filepath ), "%s/%s", 
    //     ZDJ_LIBRARY_WAVEFORM_DIR,
    //     song->entity_id
    // );
    // FILE * fd = fopen( filepath, "r" );
    // if( !fd ) { return NULL; }
    // zdj_waveform_header_t header;
    // fread( &header, sizeof( zdj_waveform_header_t ), 1, fd );
    // if( header.frame_count < 1 ){ return NULL; }

    // // Alloc waveform point storage
    // zdj_waveform_comp_view_state_t * state = calloc( 1, sizeof( zdj_waveform_comp_view_state_t ) );
    // state->total_points = header.frame_count;
    // state->points = calloc( header.frame_count, sizeof( uint8_t ) );
    // state->x_zoom = 0.5;

    // // Read waveform points
    // fread( state->points, sizeof( uint8_t ), header.frame_count, fd );
    // fclose( fd );

    // // Set up views
    zdj_view_t * view = zdj_new_view( frame );
    // view->state = state;
    // view->deinit_state = &_zdj_waveform_comp_view_deinit_state;
    // view->draw = &_zdj_waveform_comp_view_draw;

    // // Add leader view
    // state->leader = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ], NULL );
    // zdj_add_subview( view, state->leader );

    return view;
}

void _zdj_waveform_lores_view_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {

    // zdj_waveform_comp_view_state_t * state = (zdj_waveform_comp_view_state_t*)view->state;

    // uint8_t * fft_data = state->points;
    // int playhead_frame = state->current_sample;
    // double delta_t_scale = 1.0;
    // double data_zoom_x = state->x_zoom;
    // int screen_x = clip->dst.x;
    // int screen_y = clip->dst.y;
    // int screen_w = clip->dst.w;
    // int screen_h = 20;
    // // bool bg_mode

    // // Find sample-time window start and end based on playhead location and x-zoom factor
    // double win_sample_time_width = (double)ZDJ_WAVEFORM_WINDOW_SAMPLE_STRIDE * ( (double)screen_w / (data_zoom_x*delta_t_scale) );
    
    // double win_sample_start = playhead_frame/* - 3800*/ - (win_sample_time_width / 2.0);
    // double win_sample_end = playhead_frame/* - 3800*/ + (win_sample_time_width / 2.0);

    // double waveform_frame_start = floor( win_sample_start / ZDJ_WAVEFORM_WINDOW_SAMPLE_STRIDE );
    // double waveform_frame_end = ceil( win_sample_end / ZDJ_WAVEFORM_WINDOW_SAMPLE_STRIDE );
    // int waveform_point_count = waveform_frame_end - waveform_frame_start;

    // // int waveform_baseline = screen_y+screen_h+12;
    // int waveform_baseline = screen_y+1;
    // double waveform_h_coeff = screen_h * 0.009;

    // Sint16 x_vals[ waveform_point_count + 2 ];
    // Sint16 top_y_vals[ waveform_point_count + 2 ];
    // Sint16 bottom_y_vals[ waveform_point_count + 2 ];

    // // Loop thru waveform frames to be rendered, finding x and y vals
    // // Start at 1, end one val before final point
    // // first and last points are meant to complete the polygon to y=0
    // int lead_in_x = -1;    
    // for( int i=1; i<waveform_point_count+1; i++ ) {
    //     x_vals[ i ] = _playback_screen_x_for_sample_time( 
    //         _playback_sample_time_for_waveform_frame( waveform_frame_start+i ),
    //         win_sample_start,
    //         win_sample_end,
    //         screen_w 
    //     ) + screen_x;
    //     if( waveform_frame_start+i > 0 && waveform_frame_start+i < state->total_points ) {
    //         top_y_vals[ i ] = waveform_baseline - (waveform_h_coeff * fft_data[ (int)waveform_frame_start+i ]);
    //         bottom_y_vals[ i ] = waveform_baseline + (waveform_h_coeff * fft_data[ (int)waveform_frame_start+i ]);
    //     } else {
    //         top_y_vals[ i ] = waveform_baseline;
    //         bottom_y_vals[ i ] = waveform_baseline;
    //         lead_in_x = x_vals[ i ];
    //     }
    // }
    // x_vals[ 0 ] = screen_x;
    // top_y_vals[ 0 ] = waveform_baseline;
    // bottom_y_vals[ 0 ] = waveform_baseline;
    // x_vals[ waveform_point_count+1 ] = screen_w+screen_x;
    // top_y_vals[ waveform_point_count+1 ] = waveform_baseline;
    // bottom_y_vals[ waveform_point_count+1 ] = waveform_baseline;

    // if( lead_in_x > -1 ) { 
    //     state->leader->frame->x = lead_in_x - 128; 
    // } else {
    //     state->leader->frame->x = -128;
    // }

    // int p;
    // filledPolygonRGBA( zdj_renderer( ), &x_vals, &bottom_y_vals, waveform_point_count+2, 255, 255, 255, 255 );
}

void _zdj_waveform_lores_view_deinit_state( zdj_view_t * view ) {

}

// Make a ratio of sample_time to win_start sample vs. total window sample space
// Value between 0 - 1 will be visible
// Convert that value using screen_w to an pixel coordinate
int _playback_screen_x_for_sample_time( int sample_time, int win_start_sample, int win_end_sample, int screen_w ) {
    double screen_space_coord = (double)( sample_time - win_start_sample ) / (double)( win_end_sample - win_start_sample );
    int screen_x = (int)round( screen_space_coord * (double)screen_w );
    return screen_x;
}

int _playback_sample_time_for_waveform_frame( int waveform_frame ) {
    return waveform_frame * ZDJ_WAVEFORM_WINDOW_SAMPLE_STRIDE;
}
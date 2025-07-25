#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/waveform_view/zdj_waveform_view.h>

static void _zdj_waveform_view_draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_waveform_view_deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_waveform_view( zdj_rect_t * frame, zdj_library_song_t * song ) {
    zdj_waveform_view_state_t * state = calloc( 1, sizeof( zdj_waveform_view_state_t ) );

    // Stand up pipeline
    // state->file_node = zdj_new_file_node( song );
    // state->decode_node = zdj_new_decode_node( state->file_node );
    // state->pcm_node = zdj_new_pcm_node( state->decode_node );
    // state->render_node = zdj_new_waveform_render_node( state->pcm_node ); 

    // state->decode_node = zdj_new_playback_decode_node( song );


    // state->frame_float = new_decod_sample_float( );
    // state->g_sigma = 100;
    // state->gaussian = zdj_new_gaussian( state->g_sigma );

    zdj_view_t * view = zdj_new_view( frame );
    view->state = state;
    view->deinit_state = &_zdj_waveform_view_deinit_state;
    view->draw = &_zdj_waveform_view_draw;

    return view;
}

void _zdj_waveform_view_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {

    zdj_waveform_view_state_t * state = (zdj_waveform_view_state_t*)view->state;

    int screen_x = clip->dst.x;
    int screen_y = clip->dst.y;
    int screen_w = clip->dst.w;
    int screen_h = clip->dst.h;

    int waveform_baseline = screen_y + (screen_h / 2);

        
    // Update the MP3 decode state - hacky for testing
    // zdj_playback_decode_node_state_t * node_state = (zdj_playback_decode_node_state_t*)state->decode_node->state;
    // // mp3_reset_decod_to_sample( node_state, state->current_sample );
    // if( state->sample_offset_request != 0 ) { 
    //     offset_mp3_decod_address( node_state, state->sample_offset_request );
    //     state->sample_offset_request = 0;
    // }
    // printf( "sampl: %u\n", state->current_sample ); 
    
    
    // float accum;
    float vals[ 64 ];
    // double hires_waveform_h_coeff = 0.002;

    int point_count = screen_w / 2;
    // // Sample
    // for ( int l=0; l<point_count; l++ ) {
    //     // Stride thru sample buffer by gauss sigma
    //     int sample = round( l * state->gaussian->sigma );
        
    //     get_decod_sample_at_offset( node_state, state->frame_float, sample, false );
    //     accum = state->frame_float->pcm_values[ 0 ] * state->gaussian->steps[ 0 ] * hires_waveform_h_coeff;

    //     for( int g=1; g<state->gaussian->width; g++ ) {
    //         // Reach back gauss->width
    //         get_decod_sample_at_offset( node_state, state->frame_float, sample - g, false );
    //         accum += state->frame_float->pcm_values[ 0 ] * state->gaussian->steps[ g ] * hires_waveform_h_coeff;

    //         // Reach ahead gauss->width
    //         get_decod_sample_at_offset( node_state, state->frame_float, sample + g, false );
    //         accum += state->frame_float->pcm_values[ 0 ] * state->gaussian->steps[ g ] * hires_waveform_h_coeff;
    //     }
    //     vals[ l ] = accum;           
    // }

    for ( int l=0; l<point_count-1; l++ ) {
        // pixelRGBA( zdj_renderer( ), l*2, 15+vals[ l ], 255, 255, 255, 255 );
        if( vals[ l ] < -screen_h / 2 ){ vals[ l ] = -screen_h / 2; }
        if( vals[ l+1 ] < -screen_h / 2 ){ vals[ l+1 ] = -screen_h / 2; }
        if( vals[ l ] > screen_h / 2 ){ vals[ l ] = screen_h / 2; }
        if( vals[ l+1 ] > screen_h / 2 ){ vals[ l+1 ] = screen_h / 2; }
        lineRGBA( zdj_renderer( ), screen_x+l*2+4, waveform_baseline+vals[ l ], screen_x+l*2+2+4, waveform_baseline+vals[ l+1 ], 255, 255, 255, 255 );
    }
}

void _zdj_waveform_view_deinit_state( zdj_view_t * view ) {

}
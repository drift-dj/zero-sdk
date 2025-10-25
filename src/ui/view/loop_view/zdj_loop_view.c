#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/deck/dj/zdj_deck_dj.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/analysis/waveform/zdj_waveform.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/loop_view/zdj_loop_view.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_loop_view( 
    zdj_rect_t * frame, 
    zdj_deck_t * deck,
    double zoom_val
) {
    // printf( "zdj_new_loop_view\n" );

    // Build view
    zdj_view_t * view = zdj_new_view( frame );
    view->type = ZDJ_VIEW_BEATGRID;
    view->draw = &_draw;
    view->deinit_state = &_deinit_state;

    zdj_loop_view_state_t * state = calloc( 1, sizeof( zdj_loop_view_state_t ) );
    view->state = state;
    state->deck = deck;
    state->zoom_val = zoom_val;

    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    state->decode_node = deck_state->decode_node;

    // printf( "zdj_new_loop_view done\n" );
    return view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // printf( "loop_view draw\n" );
    zdj_loop_view_state_t * view_state = (zdj_loop_view_state_t*)view->state;
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)view_state->deck->state;

    // Early exit if loop isn't enabled
    if( !view_state->deck->controls.loop_state.is_enabled ) { return; }

    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)view_state->decode_node->state;

    // Remove grid markers
    zdj_remove_all_subviews_of( view );

    double samples_per_pixel = view_state->zoom_val * ZDJ_PLAYBACK_WAVEFORM_SAMPLE_STRIDE;
    double half_window = view->frame.w/2;

    double start_pcm_offset = decode_state->head.origin_d - view_state->deck->controls.loop_state.start_pcm_addr;
    double start_screen_x = half_window - ( start_pcm_offset / samples_per_pixel );
   
    double end_pcm_offset = view_state->deck->controls.loop_state.end_pcm_addr - decode_state->head.origin_d;
    double end_screen_x = half_window + ( end_pcm_offset / samples_per_pixel );

    double draw_x = start_screen_x;
    while( draw_x < end_screen_x ) {
        // boxColor( zdj_renderer( ), draw_x, view->frame.y, draw_x+2, view->frame.y+2, ZDJ_BLACK );
        pixelColor( zdj_renderer( ), draw_x+1, view->frame.y+1, ZDJ_WHITE );
        draw_x += 5;
    }

    // printf( "loop ph:%1.1f so:%1.1f st:%1.1f eo:%1.1f en:%1.1f\n", 
    //     pcm_head, start_pcm_offset, start_screen_x, end_pcm_offset, end_screen_x 
    // );
    // printf( "loop_view draw done\n" );
}

static void _deinit_state( zdj_view_t * view ) {
    // zdj_waveform_view_state_t * view_state = (zdj_waveform_view_state_t*)view->state;
}
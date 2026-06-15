#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/deck/dj/zdj_deck_dj.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/beatgrid_view/zdj_beatgrid_view.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_playback_beatgrid_view( 
    zdj_rect_t * frame, 
    zdj_beatgrid_style_t style,
    zdj_deck_t * deck,
    zdj_library_song_t * song,
    double zoom_val
) {
    // printf( "zdj_new_playback_beatgrid_view\n" );

    // Build view
    zdj_view_t * view = zdj_new_view( frame );
    view->type = ZDJ_VIEW_BEATGRID;
    view->draw = &_draw;
    view->deinit_state = &_deinit_state;

    zdj_beatgrid_view_state_t * state = calloc( 1, sizeof( zdj_beatgrid_view_state_t ) );
    view->state = state;
    state->style = style;
    state->deck = deck;
    state->song = song;
    state->zoom_val = zoom_val;

    // printf( "zdj_new_playback_beatgrid_view done\n" );
    return view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // printf( "playback_beatgrid_view draw\n" );
    zdj_beatgrid_view_state_t * view_state = (zdj_beatgrid_view_state_t*)view->state;
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)view_state->deck->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;

    // Remove grid markers
    zdj_remove_all_subviews_of( view );

    if( view_state->zoom_val > 20.0 ) { return; }

    // printf( "needle pcm addr: %f %p\n", pcm_head, view_state->song->performance );
    // Convert song PCM space to beatgrid count
    double pcm_offset = decode_state->head.origin_d - view_state->song->performance->beat_grid_start_sample;
    // Convert PCM addr offset to beatgrid space
    double beatgrid_head = zdj_signal_beatgrid_count_for_pcm_count( 
        pcm_offset,
        view_state->song->audio->av_sample_rate,
        view_state->song->performance->bpm
    );

    // Get zoom factor in beatgrid count space
    double samples_per_pixel = view_state->zoom_val * ZDJ_PLAYBACK_WAVEFORM_SAMPLE_STRIDE;
    // printf( "samples per pixel: %f", samples_per_pixel );
    double beats_per_pixel = zdj_signal_beatgrid_count_for_pcm_count( 
        samples_per_pixel,
        view_state->song->audio->av_sample_rate,
        view_state->song->performance->bpm
    );

    double half_window = view->frame.w/2;
    // Find earliest beatgrid count in window
    double earliest_beatgrid_count = beatgrid_head - (half_window*beats_per_pixel);
    // Find latest beatgrid count in window
    double latest_beatgrid_count = beatgrid_head + (half_window*beats_per_pixel);

    // Quantize earliest count based on zoom level
    double quant_div = 1.0f / 16.0f;
    double quant_earliest_count = ceil( earliest_beatgrid_count / quant_div ) * quant_div;
    if( quant_earliest_count < 0.0001f ) { quant_earliest_count = 0.0f; }


    double count = quant_earliest_count;
    while( count < latest_beatgrid_count ) {
        // Get pixel x for quantized beatgrid div
        double div_x = half_window - ( (beatgrid_head - count) / beats_per_pixel );
        
        // Get beat for div
        switch( zdj_beatgrid_mark_for_count( count ) ) {
            case ZDJ_BEATGRID_MARK_32ND:
            case ZDJ_BEATGRID_MARK_16TH:
            case ZDJ_BEATGRID_MARK_8TH:
                // pixelColor( zdj_renderer( ), div_x, clip->screen.y - 1, ZDJ_BLACK );
                // pixelColor( zdj_renderer( ), div_x, clip->screen.y + 1, ZDJ_BLACK );
                // pixelColor( zdj_renderer( ), div_x, clip->screen.y, ZDJ_WHITE );
                // pixelColor( zdj_renderer( ), div_x-1, clip->screen.y, ZDJ_BLACK );
                // pixelColor( zdj_renderer( ), div_x+1, clip->screen.y, ZDJ_BLACK );
                break;
            case ZDJ_BEATGRID_MARK_QUARTER:
            case ZDJ_BEATGRID_MARK_ORIGIN:
            case ZDJ_BEATGRID_MARK_HALF:
            case ZDJ_BEATGRID_MARK_WHOLE:
            case ZDJ_BEATGRID_MARK_BAR_N:
                pixelColor( zdj_renderer( ), div_x, clip->screen.y - 2, ZDJ_BLACK );
                pixelColor( zdj_renderer( ), div_x, clip->screen.y + 2, ZDJ_BLACK );
                lineColor( zdj_renderer( ), div_x-1, clip->screen.y-1, div_x-1,  clip->screen.y+1, ZDJ_BLACK );
                lineColor( zdj_renderer( ), div_x, clip->screen.y-1, div_x, clip->screen.y+1, ZDJ_WHITE );
                lineColor( zdj_renderer( ), div_x+1, clip->screen.y-1, div_x+1,  clip->screen.y+1, ZDJ_BLACK );
                break;
            default: break;
        }

        count += quant_div;
    }

    // printf( "playback_beatgrid_view draw done\n" );
}

static void _deinit_state( zdj_view_t * view ) {
    // zdj_waveform_view_state_t * view_state = (zdj_waveform_view_state_t*)view->state;
}
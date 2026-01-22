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
#include <zerodj/ui/view/flag_view/zdj_flag_view.h>
// #include <zerodj/ui/view/label_view/zdj_label_view.h>


static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_edit_beatgrid_view( 
    zdj_rect_t * frame, 
    zdj_beatgrid_style_t style,
    zdj_deck_t * deck,
    zdj_library_song_t * song,
    double zoom_val
) {
    // printf( "zdj_new_edit_beatgrid_view\n" );

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

    // printf( "zdj_new_edit_beatgrid_view done\n" );
    return view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // printf( "edit_beatgrid_view draw\n" );
    zdj_beatgrid_view_state_t * view_state = (zdj_beatgrid_view_state_t*)view->state;
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)view_state->deck->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;

    // Remove grid markers
    zdj_remove_all_subviews_of( view );

    // Get decode head origin coord
    double pcm_head = decode_state->head.origin_d;
    // printf( "needle pcm addr: %ld\n", pcm_addr );
    // Convert song PCM space to beatgrid count
    double pcm_offset = pcm_head - view_state->song->performance->beat_grid_start_sample;
    // Convert PCM addr offset to beatgrid space
    double beatgrid_head = zdj_signal_beatgrid_count_for_pcm_count( 
        pcm_offset,
        view_state->song->audio->av_sample_rate,
        view_state->song->performance->bpm
    );


    // printf( "beatgrid_head: %1.3f\n", beatgrid_head );

    // Get zoom factor in beatgrid count space
    double samples_per_pixel = view_state->zoom_val * ZDJ_PLAYBACK_WAVEFORM_SAMPLE_STRIDE;
    // printf( "samples per pixel: %f", samples_per_pixel );
    double beats_per_pixel = zdj_signal_beatgrid_count_for_pcm_count( 
        samples_per_pixel,
        view_state->song->audio->av_sample_rate,
        view_state->song->performance->bpm
    );

    double half_window = view->frame.w/2;
    // printf( "half win: %1.1f bpp: %f\n", half_window, beats_per_pixel );
    // Find earliest beatgrid count in window
    double earliest_beatgrid_count = beatgrid_head - (half_window*beats_per_pixel);

    // Find latest beatgrid count in window
    double song_beatgrid_pcm_length = view_state->song->audio->duration_pcm - view_state->song->performance->beat_grid_start_sample;
    double latest_song_beatgrid_count = zdj_signal_beatgrid_count_for_pcm_count( 
        song_beatgrid_pcm_length,
        view_state->song->audio->av_sample_rate,
        view_state->song->performance->bpm
    );
    double latest_window_beatgrid_count = beatgrid_head + (half_window*beats_per_pixel);
    double latest_beatgrid_count = fmin( latest_song_beatgrid_count, latest_window_beatgrid_count );

    // Manually map quantiziation to zoom level.
    // Adjust values to suit.
    double zoom_coeff = view_state->zoom_val / zdj_playback_waveform_max_zoom_val;
    double quant_coeff = zoom_coeff * zoom_coeff * zoom_coeff;
    double quant_div;
    if( zoom_coeff < 0.001032f ) {
        // Lower bound at 16nd notes
        quant_div = 1.0 / 16.0;
    } else if( zoom_coeff < 0.002074f ) {
        // Lower bound at 8th notes
        quant_div = 1.0 / 8.0;
    } else if( zoom_coeff < 0.004900f ) {
        // Lower bound at quarter
        quant_div = 1.0 / 4.0;
    } else if( zoom_coeff < 0.023f ) {
        // Lower bound at bars
        quant_div = 1.0;
    } else if( zoom_coeff < 0.125f ) {
        // Lower bound at bars
        quant_div = 4.0;
    } else if( zoom_coeff < 0.303 ) {
        // Lower bound at bars
        quant_div = 16.0;
    } else if( zoom_coeff < 0.535 ) {
        // Lower bound at bars
        quant_div = 32.0;
    } else {
        // quant_div = ceil( quant_coeff / 2.0 ) * 2.0;
        // quant_div = 1.0;
        quant_div = 64.0;
    }
    // Quantize earliest count based on zoom level
    double quant_earliest_count = ceil( earliest_beatgrid_count / quant_div ) * quant_div;
    if( quant_earliest_count < 0.0001f ) { quant_earliest_count = 0.0f; }
    // Walk forward thru counts based on zoom level, adding divisions
    double count = quant_earliest_count;
    while( count < latest_beatgrid_count ) {
        // Get pixel x for quantized beatgrid div
        double div_x = view->frame.x + half_window - ( (beatgrid_head - count) / beats_per_pixel );
        
        char str[ 4 ];

        zdj_view_t * zero_div = NULL;
        zdj_view_t * zero_div_top = NULL;
        zdj_view_t * zero_div_bot = NULL;
        zdj_view_t * n_bar_label = NULL;
        zdj_view_t * n_bar_bg = NULL;
        
        float view_top = view->frame.y + 3;
        float view_bottom = view_top+view->frame.h;
        

        // Get beat for div
        switch( zdj_beatgrid_mark_for_count( count ) ) {
            case ZDJ_BEATGRID_MARK_ORIGIN:
                zero_div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BG_MARK_ORIGIN ], NULL );
                zero_div->frame.x = round(div_x-1);
                break;
            case ZDJ_BEATGRID_MARK_32ND:
                break;
            case ZDJ_BEATGRID_MARK_16TH:
                zero_div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BG_MARK_ARROW ], NULL );
                zero_div->frame.x = round(div_x-2);
                break;
            case ZDJ_BEATGRID_MARK_8TH:
                zero_div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BG_MARK_ARROW ], NULL );
                zero_div->frame.x = round(div_x-2);
                break;
            case ZDJ_BEATGRID_MARK_QUARTER:
                zero_div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BG_MARK_ARROW ], NULL );
                zero_div->frame.x = round(div_x-2);
                break;
            case ZDJ_BEATGRID_MARK_HALF:
                break;
            case ZDJ_BEATGRID_MARK_WHOLE:
                // printf( "whole\n" );
                zero_div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BG_MARK_ARROW ], NULL );
                zero_div->frame.x = round(div_x-2);
                break;
            case ZDJ_BEATGRID_MARK_BAR_N:
                // printf( "bar n\n" );
                zero_div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BG_MARK_BAR ], NULL );
                zero_div->frame.x = round(div_x-2);
                zero_div->frame.y = -1;

                snprintf( str, sizeof( str ), "%d", (int)count+1 );
                n_bar_label = zdj_new_flag_view( ZDJ_FLAG_TYPE_BAR, str );
                n_bar_label->frame.x = round(div_x-1);
                n_bar_label->frame.y = round(view->frame.h/2) - 6;
                n_bar_label->frame.w = 21;
                break;
            default: break;
        }
        
        if( zero_div ){ zdj_add_subview( view, zero_div ); }
        if( n_bar_label ){ zdj_add_subview( view, n_bar_label ); }
        count += quant_div;
    }

    // printf( "edit_beatgrid_view draw done\n" );
}

static void _deinit_state( zdj_view_t * view ) {
    // zdj_waveform_view_state_t * view_state = (zdj_waveform_view_state_t*)view->state;
}
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

    // Remove grid markers
    zdj_remove_all_subviews_of( view );

    // Convert deck needle head to song PCM space
    // double pcm_head = (double)zdj_deck_get_pcm_addr_for_needle_head( view_state->deck, deck_state->decode_node );
    double pcm_head = view_state->deck->controls.platter.needle.head;
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
    double latest_beatgrid_count = beatgrid_head + (half_window*beats_per_pixel);

    // Manually map quantiziation to zoom level.
    // Adjust values to suit.
    double zoom_coeff = view_state->zoom_val / zdj_playback_waveform_max_zoom_val;
    double quant_coeff = zoom_coeff * zoom_coeff * zoom_coeff * 64.0f;
    double quant_div;
    if( quant_coeff < 0.003f ) {
        // Lower bound at 16nd notes
        quant_div = 1.0 / 16.0;
    } else if( quant_coeff < 0.01f ) {
        // Lower bound at 8th notes
        quant_div = 1.0 / 8.0;
    } else if( quant_coeff < 0.02f ) {
        // Lower bound at quarter
        quant_div = 1.0 / 4.0;
    } else if( quant_coeff < 0.4f ) {
        // Lower bound at half notes
        quant_div = 1.0 / 2.0;
    } else if( quant_coeff < 0.2f ) {
        // Lower bound at bars
        quant_div = 1.0;
    } else {
        quant_div = ceil( quant_coeff / 8.0 ) * 8.0;
    }

    // printf( "quant c:%f div:%f\n", quant_coeff, quant_div );

    // Quantize earliest count based on zoom level
    double quant_earliest_count = ceil( earliest_beatgrid_count / quant_div ) * quant_div;
    if( quant_earliest_count < 0.0001f ) { quant_earliest_count = 0.0f; }
    // Walk forward thru counts based on zoom level, adding divisions

    // printf( "bg win: [ %f | %f/%1.1f | %f ]\n",
    //     earliest_beatgrid_count,
    //     beatgrid_head,
    //     pcm_head,
    //     latest_beatgrid_count
    // );
    // printf( "quant_div: %f early: %f quant early: %f\n", quant_div, earliest_beatgrid_count, quant_earliest_count );

    double count = quant_earliest_count;
    while( count < latest_beatgrid_count ) {
        // Get pixel x for quantized beatgrid div
        double div_x = half_window - ( (beatgrid_head - count) / beats_per_pixel );
        
        zdj_view_t * zero_div = NULL;
        
        

        // Get beat for div
        switch( zdj_beatgrid_mark_for_count( count ) ) {
            case ZDJ_BEATGRID_MARK_ORIGIN:
                // printf( "origin\n" );
                zero_div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_NAR_V_DIV ], NULL );
                zero_div->frame.x = div_x-1;
                zero_div->frame.y = 1;
                zero_div->frame.h = 29;
                break;
            case ZDJ_BEATGRID_MARK_32ND:
                break;
            case ZDJ_BEATGRID_MARK_16TH:
                zero_div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_DIV ], NULL );
                zero_div->frame.x = div_x;
                zero_div->frame.y = 12;
                zero_div->frame.h = 6;
                break;
            case ZDJ_BEATGRID_MARK_8TH:
                zero_div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_DIV ], NULL );
                zero_div->frame.x = div_x;
                zero_div->frame.y = 12;
                zero_div->frame.h = 6;
                break;
            case ZDJ_BEATGRID_MARK_QUARTER:
                zero_div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_DIV ], NULL );
                zero_div->frame.x = div_x;
                zero_div->frame.y = 10;
                zero_div->frame.h = 14;
                break;
            case ZDJ_BEATGRID_MARK_HALF:
            case ZDJ_BEATGRID_MARK_WHOLE:
                // printf( "whole\n" );
                zero_div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_NAR_V_DIV ], NULL );
                zero_div->frame.x = div_x-1;
                zero_div->frame.y = 10;
                zero_div->frame.h = 14;
                break;
            case ZDJ_BEATGRID_MARK_BAR_N:
                // printf( "bar n\n" );
                zero_div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_NAR_V_DIV ], NULL );
                zero_div->frame.x = div_x-1;
                zero_div->frame.y = 5;
                zero_div->frame.h = 19;
                break;
            default: break;
        }
        
        if( zero_div ){ zdj_add_subview( view, zero_div ); }
        count += quant_div;
    }

    // printf( "edit_beatgrid_view draw done\n" );
}

static void _deinit_state( zdj_view_t * view ) {
    // zdj_waveform_view_state_t * view_state = (zdj_waveform_view_state_t*)view->state;
}
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/signal/pipeline/node/audio/io/zdj_io_node.h>
#include <zerodj/system/perf/zdj_perf.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/thread_view/zdj_thread_view.h>

void _zdj_thread_view_draw( zdj_view_t * view, zdj_view_clip_t * clip );
void _zdj_thread_view_update_layout( zdj_view_t * view, zdj_view_clip_t * clip );
void _zdj_thread_view_deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_thread_view( zdj_rect_t * frame ) {
    zdj_view_t * view = zdj_new_view( frame );
    view->draw = _zdj_thread_view_draw;
    view->deinit_state = _zdj_thread_view_deinit_state;

    // Build the thread_view state instance
    zdj_thread_view_state_t * state = calloc( 1, sizeof( zdj_thread_view_state_t ) );
    view->state = state;

    return view;
}

void _zdj_thread_view_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_MID_GRAY );

    zdj_thread_view_state_t * state = (zdj_thread_view_state_t*)view->state;
    if( state->update_counter++ == 0 ) {
        if( state->start_time == 0 ) { state->start_time = zdj_perf_time( ); }
        _zdj_thread_view_update_layout( view, clip );
    }
    if( state->update_counter > 80 ) { state->update_counter = 0; }
}

void _zdj_thread_view_update_layout( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_thread_view_state_t * state = (zdj_thread_view_state_t*)view->state;
    zdj_io_analog_node_state_t * io_state = zdj_soundcard->analog_io_node->state;

    zdj_remove_all_subviews_of( view );

    char str[ 256 ];
    int y = 2;

    // Build cycle count data

    // Perf doesn't stat until panel deploys, so assume elapsed time apporximately
    // covers entire perf report.
    double elapsed_time = (double)( zdj_perf_time( ) - state->start_time ) / 1000000000;
    uint64_t sample_count = io_state->shared_audio_state->cycle_count * ZDJ_SOUNDCARD_BUF_LEN;
    double samps_per_time = (double)sample_count / elapsed_time;
    snprintf( str, sizeof( str ), "Miss: %u/%lu %1.0f", 
        io_state->shared_audio_state->miss_count,
        io_state->shared_audio_state->cycle_count,
        samps_per_time
    );
    zdj_view_t * miss_label = zdj_new_label_view( str, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    miss_label->frame->y = y;
    y+= 9;
    zdj_add_subview( view, miss_label );

    zdj_perf_report_t * report = zdj_perf_make_cycle_timing_report( );
    zdj_reset_perf( );

    zdj_perf_report_line_t * line;
    zdj_view_t * label;
    double cad_msec;
    double cad_hz;
    double dur_msec;
    
    line = zdj_perf_report_line_for_name( report, ZDJ_PERF_TAG_CONTROL_CYCLE );
    cad_msec = (double)line->avg_cadence / 1000000.0;
    cad_hz = 1000.0 / cad_msec;
    dur_msec = (double)line->avg_dur / 1000000.0;
    snprintf( str, sizeof( str ), "%1.1fmS/%1.0fHz  %1.1fmS (CTRL)", cad_msec, cad_hz, dur_msec );
    label = zdj_new_label_view( str, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    label->frame->y = y;
    y+= 9;
    zdj_add_subview( view, label );

    line = zdj_perf_report_line_for_name( report, ZDJ_PERF_TAG_AUDIO_BUF_CYCLE );
    cad_msec = (double)line->avg_cadence / 1000000.0;
    cad_hz = 1000.0 / cad_msec;
    dur_msec = (double)line->avg_dur / 1000000.0;
    snprintf( str, sizeof( str ), "%1.1fmS/%1.0fHz  %1.1fmS (AUDI)", cad_msec, cad_hz, dur_msec );
    label = zdj_new_label_view( str, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    label->frame->y = y;
    y+= 9;
    zdj_add_subview( view, label );

    line = zdj_perf_report_line_for_name( report, ZDJ_PERF_TAG_UI_CYCLE );
    cad_msec = (double)line->avg_cadence / 1000000.0;
    cad_hz = 1000.0 / cad_msec;
    dur_msec = (double)line->avg_dur / 1000000.0;
    snprintf( str, sizeof( str ), "%1.1fmS/%1.0fHz  %1.1fmS (UI)", cad_msec, cad_hz, dur_msec );
    label = zdj_new_label_view( str, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    label->frame->y = y;
    y+= 9;
    zdj_add_subview( view, label );
}

void _zdj_thread_view_deinit_state( zdj_view_t * view ) {
    // zdj_thread_view_state_t * state = (zdj_thread_view_state_t*)view->state;
    // SDL_DestroyTexture( state->tex );
    // free( state );
    // view->state = NULL;
}
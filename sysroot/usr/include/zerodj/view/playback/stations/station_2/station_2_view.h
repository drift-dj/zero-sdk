#ifndef STATION_2_VIEW_H
#define STATION_2_VIEW_H

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    zdj_view_t * eq_page;
    zdj_view_t * trim_page;
    zdj_view_t * loop_page;
    zdj_view_t * sync_page;
    zdj_view_t * filt_page;
    zdj_view_t * delay_page;
    zdj_view_t * waveform_view;
    zdj_view_t * beatgrid_view;
    zdj_view_t * cuepoints;
    zdj_view_t * loop_view;
    zdj_view_t * title_ticker;
    zdj_view_t * playhead_marker;
    zdj_view_t * playback_head;
    zdj_view_t * nudge_view;
    zdj_view_t * empty_view;
    zdj_view_t * empty_deck_icon;
    zdj_view_t * empty_open_icon;
    zdj_view_t * empty_bottom_line;
    zdj_view_t * empty_grille;
} station_2_view_state_t;

zdj_view_t * new_station_2_view( void );

#endif
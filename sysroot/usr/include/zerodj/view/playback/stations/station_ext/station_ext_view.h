#ifndef STATION_EXT_VIEW_H
#define STATION_EXT_VIEW_H

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    zdj_view_t * eq_page;
    zdj_view_t * trim_page;
    zdj_view_t * sync_page;
    zdj_view_t * filt_page;
    zdj_view_t * delay_page;
    zdj_view_t * waveform_view;
    zdj_view_t * beatgrid_view;
    zdj_view_t * loop_view;
    zdj_view_t * nudge_view;
} station_ext_view_state_t;

zdj_view_t * new_station_ext_view( void );

void load_station_ext_deck_ui( zdj_deck_t * deck );
void unload_station_ext_deck_ui( zdj_deck_t * deck );

#endif
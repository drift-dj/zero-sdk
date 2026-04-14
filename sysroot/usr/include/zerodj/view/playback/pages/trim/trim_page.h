#ifndef TRIM_PAGE_H
#define TRIM_PAGE_H

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    zdj_deck_station_t station;
    zdj_deck_t * deck;
    
    zdj_view_t * circle;
    zdj_view_t * star;
    zdj_view_t * tri;

    zdj_view_t * tr_gain;
    zdj_view_t * tr_db;

    zdj_view_t * cue_mute;
    zdj_view_t * cue_gain;
    zdj_view_t * cue_db;
} trim_page_state_t;

zdj_view_t * new_trim_page_view( zdj_deck_t * deck );

#endif
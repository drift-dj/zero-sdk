#ifndef SYNC_PAGE_H
#define SYNC_PAGE_H

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    zdj_deck_station_t station;
    zdj_deck_t * deck;
    
    zdj_view_t * circle;
    zdj_view_t * star;
    zdj_view_t * tri;

    zdj_view_t * sync_en;
    zdj_view_t * sync_factor;
} sync_page_state_t;

zdj_view_t * new_sync_page_view( zdj_deck_t * deck );

#endif
#ifndef LOOP_PAGE_H
#define LOOP_PAGE_H

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    zdj_deck_station_t station;
    zdj_deck_t * deck;
    
    zdj_view_t * loop_chip;
    zdj_view_t * loop_len;

    zdj_view_t * skip_chip;
    zdj_view_t * skip_len;
    zdj_view_t * skip_status;
} loop_page_state_t;

zdj_view_t * new_loop_page_view( zdj_deck_t * deck );

#endif
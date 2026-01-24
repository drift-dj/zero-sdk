#ifndef EQ_PAGE_H
#define EQ_PAGE_H

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    zdj_deck_station_t station;
    zdj_deck_t * deck;
    
    zdj_view_t * eq_chip;
    zdj_view_t * eq_state_lo;
    zdj_view_t * eq_state_mid;
    zdj_view_t * eq_state_hi;

    zdj_view_t * filter_chip;
    zdj_view_t * filter_state;
} eq_page_state_t;

zdj_view_t * new_eq_page_view( zdj_deck_t * deck );

#endif
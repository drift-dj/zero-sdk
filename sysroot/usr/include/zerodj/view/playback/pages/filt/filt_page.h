#ifndef FILT_PAGE_H
#define FILT_PAGE_H

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    zdj_deck_station_t station;
    zdj_deck_t * deck;
    
    zdj_view_t * cutoff;
    zdj_view_t * res;
    
} filt_page_state_t;

zdj_view_t * new_filt_page_view( zdj_deck_t * deck );

#endif
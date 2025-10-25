#ifndef TRIM_PAGE_H
#define TRIM_PAGE_H

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    zdj_view_t * tr_chip;
    zdj_view_t * tr_gain;

    zdj_view_t * cue_chip;
    zdj_view_t * cue_gain;
} trim_page_state_t;

zdj_view_t * new_trim_page_view( zdj_deck_t * deck );

#endif
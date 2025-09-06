#ifndef EQ_PAGE_H
#define EQ_PAGE_H

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    int id;
    zdj_view_t * label;
} eq_page_state_t;

zdj_view_t * new_eq_page_view( void );

#endif
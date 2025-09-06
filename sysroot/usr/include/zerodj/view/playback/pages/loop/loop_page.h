#ifndef LOOP_PAGE_H
#define LOOP_PAGE_H

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    int id;
} loop_page_state_t;

zdj_view_t * new_loop_page_view( void );

#endif
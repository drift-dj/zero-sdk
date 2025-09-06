#ifndef TRIM_PAGE_H
#define TRIM_PAGE_H

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    int id;
} trim_page_state_t;

zdj_view_t * new_trim_page_view( void );

#endif
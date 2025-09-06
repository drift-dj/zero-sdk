#ifndef SYNC_PAGE_H
#define SYNC_PAGE_H

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    int id;
} sync_page_state_t;

zdj_view_t * new_sync_page_view( void );

#endif
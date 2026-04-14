#ifndef ROOT_MENU_H
#define ROOT_MENU_H

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    bool needs_layout_update;
} root_menu_state_t;

zdj_view_t * new_root_menu( void );
zdj_view_t * refresh_root_menu( void );

#endif
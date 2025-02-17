#ifndef ZDJ_MENU_VIEW_H
#define ZDJ_MENU_VIEW_H

#include <SDL2/SDL.h>

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    char * title;
    zdj_view_t * header_view;
    zdj_view_t * scroll_view;
    zdj_ui_orient_t scroll_dir;
    int scroll_index;
    int section_count;
    int item_count;
} zdj_menu_view_state_t;

zdj_view_t * zdj_new_menu_view( char * title, zdj_ui_orient_t scroll_dir, zdj_rect_t * frame );
void zdj_menu_view_add_header( zdj_view_t * menu_view, zdj_view_t * header );
void zdj_menu_view_add_section( zdj_view_t * menu_view, zdj_view_t * section );
void zdj_menu_view_add_item( zdj_view_t * menu_view, zdj_view_t * item );


#endif
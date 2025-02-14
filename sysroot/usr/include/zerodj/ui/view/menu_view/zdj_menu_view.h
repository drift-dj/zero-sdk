#ifndef ZDJ_MENU_VIEW_H
#define ZDJ_MENU_VIEW_H

#include <SDL2/SDL.h>

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    zdj_view_t * scroll_view;
    zdj_ui_orient_t scroll_dir;
    int scroll_index;
    int section_count;
    int item_count;
} zdj_menu_view_state_t;

// menu_item_linkage may be passed in from front-end app during menu init to define 
// custom implementations for rendering, event handling, and display value.
// For menu items with progress bars, settings, text input, etc.
// NONE to ANY fields may be defined.
typedef struct {
    void ( *draw )( zdj_view_t *, zdj_view_clip_t * );
    void ( *handle_hmi_event )( zdj_view_t *, void * );
    void ( *update_data )( zdj_view_t *, void * );
} zdj_menu_linkage_t;

zdj_view_t * zdj_new_menu_view( zdj_menu_linkage_t * linkage, zdj_ui_orient_t scroll_dir, zdj_rect_t * frame );
void zdj_menu_view_add_section( zdj_view_t * menu_view, zdj_view_t * section );
void zdj_menu_view_add_item( zdj_view_t * menu_view, zdj_view_t * item );


#endif
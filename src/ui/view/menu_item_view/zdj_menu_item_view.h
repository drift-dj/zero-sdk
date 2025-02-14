#ifndef ZDJ_MENU_ITEM_VIEW_H
#define ZDJ_MENU_ITEM_VIEW_H

#include <zerodj/ui/view/menu_view/zdj_menu_view.h>

typedef enum {
    ZDJ_MENU_ITEM_LAYOUT_UNKNOWN,
    ZDJ_MENU_ITEM_LAYOUT_BASIC_R,
    ZDJ_MENU_ITEM_LAYOUT_BASIC_L,
    ZDJ_MENU_ITEM_LAYOUT_TOGGLE_R,
    ZDJ_MENU_ITEM_LAYOUT_TOGGLE_L
} zdj_menu_item_view_layout_t;

typedef enum {
    ZDJ_MENU_ITEM_ACTION_UNKNOWN,
    ZDJ_MENU_ITEM_ACTION_VIEW,
    ZDJ_MENU_ITEM_ACTION_MENU,
    ZDJ_MENU_ITEM_ACTION_TOGGLE,
    ZDJ_MENU_ITEM_ACTION_INPUT
} zdj_menu_item_view_action_t;

typedef struct {
    zdj_ui_data_t * data;
    int scroll_index;
    zdj_menu_item_view_layout_t static_layout;
    zdj_menu_item_view_action_t static_action;
    zdj_view_t * ticker;
    zdj_view_t * ticker_hilite;
    bool is_blinking;
    int blink_timer;
} zdj_menu_item_view_state_t;

zdj_view_t * zdj_new_menu_item( char * title, zdj_menu_linkage_t * linkage );

zdj_view_t * zdj_menu_item_for_scroll_index( zdj_view_t * view, int index );

void zdj_menu_item_static_draw_simple_l( zdj_view_t * view, zdj_view_clip_t * clip );
void zdj_menu_item_static_draw_simple_r( zdj_view_t * view, zdj_view_clip_t * clip );

#endif
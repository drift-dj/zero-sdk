#ifndef ZDJ_MENU_ITEM_VIEW_H
#define ZDJ_MENU_ITEM_VIEW_H

#define ZDJ_MENU_ITEM_MARGIN_L 10
#define ZDJ_MENU_ITEM_MARGIN_R 0

typedef enum {
    ZDJ_MENU_ITEM_LAYOUT_UNKNOWN,
    ZDJ_MENU_ITEM_LAYOUT_BASIC_L,
    ZDJ_MENU_ITEM_LAYOUT_BASIC_R,
    ZDJ_MENU_ITEM_LAYOUT_DATA_L,
    ZDJ_MENU_ITEM_LAYOUT_DATA_R,
    ZDJ_MENU_ITEM_LAYOUT_TOGGLE_L,
    ZDJ_MENU_ITEM_LAYOUT_TOGGLE_R,
    ZDJ_MENU_ITEM_LAYOUT_SLIDER_L,
    ZDJ_MENU_ITEM_LAYOUT_SLIDER_R
} zdj_menu_item_view_layout_t;

typedef enum {
    ZDJ_MENU_ITEM_ACTION_UNKNOWN,
    ZDJ_MENU_ITEM_ACTION_VIEW,
    ZDJ_MENU_ITEM_ACTION_MENU,
    ZDJ_MENU_ITEM_ACTION_ALERT,
    ZDJ_MENU_ITEM_ACTION_MODAL,
    ZDJ_MENU_ITEM_ACTION_DROPDOWN,
    ZDJ_MENU_ITEM_ACTION_TOGGLE,
    ZDJ_MENU_ITEM_ACTION_SLIDER,
    ZDJ_MENU_ITEM_ACTION_CYCLE,
    ZDJ_MENU_ITEM_ACTION_INPUT
} zdj_menu_item_view_action_t;

typedef struct {
    char * title;
    zdj_ui_data_t * data;
    void ( *update_data )( zdj_view_t * );
    int scroll_index;
    zdj_menu_item_view_layout_t layout;
    zdj_menu_item_view_action_t action;
    char * link;
    bool has_valid_display;
    bool is_hilite;
    bool is_blinking;
    int blink_timer;
    zdj_view_t * title_ticker;
    zdj_view_t * title_ticker_hi;
    zdj_view_t * data_ticker;
} zdj_menu_item_view_state_t;

zdj_view_t * zdj_new_menu_item( char * title );

zdj_view_t * zdj_menu_item_for_scroll_index( zdj_view_t * view, int index );

zdj_menu_item_view_layout_t zdj_menu_item_layout_for_string( char * string );
zdj_menu_item_view_action_t zdj_menu_item_action_for_string( char * string );

void zdj_menu_item_draw_basic_l( zdj_view_t * view, zdj_view_clip_t * clip );
void zdj_menu_item_draw_basic_r( zdj_view_t * view, zdj_view_clip_t * clip );
void zdj_menu_item_draw_data_l( zdj_view_t * view, zdj_view_clip_t * clip );
void zdj_menu_item_draw_data_r( zdj_view_t * view, zdj_view_clip_t * clip );
void zdj_menu_item_draw_toggle_l( zdj_view_t * view, zdj_view_clip_t * clip );
void zdj_menu_item_draw_toggle_r( zdj_view_t * view, zdj_view_clip_t * clip );
void zdj_menu_item_draw_slider_l( zdj_view_t * view, zdj_view_clip_t * clip );
void zdj_menu_item_draw_slider_r( zdj_view_t * view, zdj_view_clip_t * clip );

#endif
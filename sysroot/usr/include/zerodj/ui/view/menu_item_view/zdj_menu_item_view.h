#ifndef ZDJ_MENU_ITEM_VIEW_H
#define ZDJ_MENU_ITEM_VIEW_H

#define ZDJ_MENU_ITEM_MARGIN_L 1
#define ZDJ_MENU_ITEM_MARGIN_R 1

typedef enum {
    ZDJ_MENU_ITEM_LAYOUT_CUSTOM,
    ZDJ_MENU_ITEM_LAYOUT_BASIC_L,
    ZDJ_MENU_ITEM_LAYOUT_BASIC_R,
    ZDJ_MENU_ITEM_LAYOUT_DATA_L,
    ZDJ_MENU_ITEM_LAYOUT_DATA_R,
    ZDJ_MENU_ITEM_LAYOUT_TOGGLE_L,
    ZDJ_MENU_ITEM_LAYOUT_TOGGLE_R,
    ZDJ_MENU_ITEM_LAYOUT_SLIDER_L,
    ZDJ_MENU_ITEM_LAYOUT_SLIDER_R,
    ZDJ_MENU_ITEM_LAYOUT_DIR_L,
    ZDJ_MENU_ITEM_LAYOUT_DIR_R
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
    ZDJ_MENU_ITEM_ACTION_INPUT,
    ZDJ_MENU_ITEM_ACTION_DIR_BACK,
    ZDJ_MENU_ITEM_ACTION_DIR_SELECT,
    ZDJ_MENU_ITEM_ACTION_DIR_ENTER,
    ZDJ_MENU_ITEM_ACTION_FILE_SELECT
} zdj_menu_item_view_action_t;

typedef void ( *update_layout_t )( zdj_view_t* );

typedef struct {
    char * title;
    zdj_ui_data_t * data;
    void ( *update_data )( zdj_view_t * );
    int scroll_index;
    update_layout_t update_layout;
    zdj_menu_item_view_action_t action;
    char * link;
    bool has_valid_display;
    bool is_hilite;
    bool is_blinking;
    int blink_timer;
    zdj_view_t * normal_view;
    zdj_view_t * hilite_view;
} zdj_menu_item_view_state_t;

zdj_view_t * zdj_new_menu_item( char * title );

zdj_view_t * zdj_menu_item_for_scroll_index( zdj_view_t * view, int index );

update_layout_t zdj_menu_item_update_for_layout( zdj_menu_item_view_layout_t layout );

void zdj_menu_item_basic_l_update_layout( zdj_view_t * view );
void zdj_menu_item_basic_r_update_layout( zdj_view_t * view );
void zdj_menu_item_data_l_update_layout( zdj_view_t * view );
void zdj_menu_item_data_r_update_layout( zdj_view_t * view );
void zdj_menu_item_dir_l_update_layout( zdj_view_t * view );
void zdj_menu_item_dir_r_update_layout( zdj_view_t * view );
void zdj_menu_item_slider_l_update_layout( zdj_view_t * view );
void zdj_menu_item_slider_r_update_layout( zdj_view_t * view );
void zdj_menu_item_toggle_l_update_layout( zdj_view_t * view );
void zdj_menu_item_toggle_r_update_layout( zdj_view_t * view );

#endif
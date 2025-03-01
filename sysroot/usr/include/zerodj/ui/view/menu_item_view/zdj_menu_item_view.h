// Copyright (c) 2025 Drift DJ Industries

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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
typedef void ( *update_data_t )( zdj_ui_data_t*, char* );

typedef struct {
    char * title;
    zdj_ui_data_t * data;
    update_data_t update_data;
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

bool zdj_menu_item_layout_is_dynamic( zdj_menu_item_view_layout_t layout );

void zdj_menu_item_basic_l_update_layout( zdj_view_t * view );
void zdj_menu_item_basic_r_update_layout( zdj_view_t * view );
void zdj_menu_item_nohilite_l_update_layout( zdj_view_t * view );
void zdj_menu_item_nohilite_r_update_layout( zdj_view_t * view );
void zdj_menu_item_data_l_update_layout( zdj_view_t * view );
void zdj_menu_item_data_r_update_layout( zdj_view_t * view );
void zdj_menu_item_dir_l_update_layout( zdj_view_t * view );
void zdj_menu_item_dir_r_update_layout( zdj_view_t * view );
void zdj_menu_item_slider_l_update_layout( zdj_view_t * view );
void zdj_menu_item_slider_r_update_layout( zdj_view_t * view );
void zdj_menu_item_toggle_l_update_layout( zdj_view_t * view );
void zdj_menu_item_toggle_r_update_layout( zdj_view_t * view );

#endif
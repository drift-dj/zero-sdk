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

#include <zerodj/library/zdj_library.h>
#include <zerodj/system/usb/zdj_usb.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>

#ifndef ZDJ_MENU_ITEM_VIEW_H
#define ZDJ_MENU_ITEM_VIEW_H

#define ZDJ_MENU_ITEM_MARGIN_L 1
#define ZDJ_MENU_ITEM_MARGIN_R 1

typedef enum {
    ZDJ_MENU_ITEM_LAYOUT_BASIC_L,
    ZDJ_MENU_ITEM_LAYOUT_BASIC_R,
    ZDJ_MENU_ITEM_LAYOUT_BASIC_LG,
    ZDJ_MENU_ITEM_LAYOUT_BASIC_LAUNCH_R,
    ZDJ_MENU_ITEM_LAYOUT_BASIC_L_EDIT,
    ZDJ_MENU_ITEM_LAYOUT_BASIC_MED_L_EDIT,
    ZDJ_MENU_ITEM_LAYOUT_DATA_L,
    ZDJ_MENU_ITEM_LAYOUT_DATA_R,
    ZDJ_MENU_ITEM_LAYOUT_FILE,
    ZDJ_MENU_ITEM_LAYOUT_DIR,
    ZDJ_MENU_ITEM_LAYOUT_DIR_SELECT,
    ZDJ_MENU_ITEM_LAYOUT_DIR_UP,
    ZDJ_MENU_ITEM_LAYOUT_ICON,
    ZDJ_MENU_ITEM_LAYOUT_INERT,
    ZDJ_MENU_ITEM_LAYOUT_INERT_DATA,
    ZDJ_MENU_ITEM_LAYOUT_INERT_STATUS,
    ZDJ_MENU_ITEM_LAYOUT_LAUNCH_BIG,
    ZDJ_MENU_ITEM_LAYOUT_LAUNCH_SM,
    ZDJ_MENU_ITEM_LAYOUT_PLAYLIST,
    ZDJ_MENU_ITEM_LAYOUT_SLIDER,
    ZDJ_MENU_ITEM_LAYOUT_SONG,
    ZDJ_MENU_ITEM_LAYOUT_SONG_PLAYLIST,
    ZDJ_MENU_ITEM_LAYOUT_SONG_IMPORT,
    ZDJ_MENU_ITEM_LAYOUT_TOGGLE,
    ZDJ_MENU_ITEM_LAYOUT_ASSET,
    ZDJ_MENU_ITEM_LAYOUT_CUEPOINT,
    ZDJ_MENU_ITEM_LAYOUT_USB_DEVICE,
    ZDJ_MENU_ITEM_LAYOUT_BROWSER_DEVICE,
    ZDJ_MENU_ITEM_LAYOUT_KEYBOARD,
    ZDJ_MENU_ITEM_LAYOUT_CUSTOM
} zdj_menu_item_view_layout_t;

typedef enum {
    ZDJ_MENU_ITEM_BROWSER_DEVICE_TYPE_ZERO,
    ZDJ_MENU_ITEM_BROWSER_DEVICE_TYPE_LINUX,
    ZDJ_MENU_ITEM_BROWSER_DEVICE_TYPE_MSD
} zdj_menu_item_view_browser_device_type_t;

// DEPRECATED
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
    ZDJ_MENU_ITEM_ACTION_FILE_SELECT,
    ZDJ_MENU_ITEM_ACTION_FILE_DELETE,
    ZDJ_MENU_ITEM_ACTION_FILE_MOVE,
    ZDJ_MENU_ITEM_ACTION_FILE_DUPLICATE,
    ZDJ_MENU_ITEM_ACTION_FILE_OPEN
} zdj_menu_item_view_action_t;
// DEPRECATED

typedef enum {
    ZDJ_MENU_ITEM_DATA_TYPE_CHAR,
    ZDJ_MENU_ITEM_DATA_TYPE_CHAR_EDIT,
    ZDJ_MENU_ITEM_DATA_TYPE_BOOL,
    ZDJ_MENU_ITEM_DATA_TYPE_INT,
    ZDJ_MENU_ITEM_DATA_TYPE_DOUBLE_0,
    ZDJ_MENU_ITEM_DATA_TYPE_DOUBLE_1,
    ZDJ_MENU_ITEM_DATA_TYPE_DOUBLE_2
} zdj_menu_item_data_display_type_t;

typedef enum {
    ZDJ_MENU_ITEM_OPTIONS_NONE,
    ZDJ_MENU_ITEM_OPTIONS_DJ_SONG_MENU,
    ZDJ_MENU_ITEM_OPTIONS_LIB_PLAYLIST,
    ZDJ_MENU_ITEM_OPTIONS_DJ_PLAYLIST,
    ZDJ_MENU_ITEM_OPTIONS_FILE,
    ZDJ_MENU_ITEM_OPTIONS_DIR,
} zdj_menu_item_options_type_t;

typedef enum {
    ZDJ_MENU_ITEM_ACTION_SELECT,
    ZDJ_MENU_ITEM_ACTION_DELETE,
    ZDJ_MENU_ITEM_ACTION_OPEN,
    ZDJ_MENU_ITEM_ACTION_MOVE_FILE,
    ZDJ_MENU_ITEM_ACTION_DUPLICATE,
    ZDJ_MENU_ITEM_ACTION_ADD_TO_PLAYLIST,
    ZDJ_MENU_ITEM_ACTION_START_MOVE,
    ZDJ_MENU_ITEM_ACTION_END_MOVE,
    ZDJ_MENU_ITEM_ACTION_EDIT,
    ZDJ_MENU_ITEM_ACTION_LEAD_DECK_1,
    ZDJ_MENU_ITEM_ACTION_LEAD_DECK_2,
    ZDJ_MENU_ITEM_ACTION_DONE
} zdj_menu_item_action_t;

typedef void ( *init_layout_t )( zdj_view_t* );
typedef void ( *update_layout_t )( zdj_view_t* );
typedef void ( *enter_edit_mode_t )( zdj_view_t* );
typedef void ( *exit_edit_mode_t )( zdj_view_t* );

typedef struct {
    char title[ 256 ];
    char subtitle[ 256 ];
    zdj_ui_data_t data;
    zdj_menu_item_data_display_type_t data_type;
    char data_prefix[ 32 ];
    char data_suffix[ 32 ];
    int scroll_index;
    zdj_menu_item_view_layout_t layout;
    bool needs_layout_init;
    init_layout_t init_layout;
    bool needs_layout_update;
    update_layout_t update_layout;
    zdj_ui_asset_t icon;
    zdj_ui_asset_t icon_hi;
    zdj_menu_item_view_action_t action; // DEPERECATED
    char link[ 256 ];
    bool is_hilite;
    bool is_blinking;
    int blink_timer;
    int blink_length;
    int blink_period;
    int blink_duty;
    bool handles_hmi;
    bool captures_all_events;
    bool edit_enabled;
    bool edit_active;
    double edit_option_index;
    zdj_menu_item_options_type_t edit_options_type;
    zdj_menu_item_action_t edit_action;
    enter_edit_mode_t enter_edit_mode;
    exit_edit_mode_t exit_edit_mode;
    bool scroll_to_exit_edit_mode;
    zdj_view_t * normal_view;
    zdj_view_t * hilite_view;
    zdj_view_t * title_view;
    zdj_view_t * data_view;
    zdj_view_t * div_view;
} zdj_menu_item_view_state_t;

zdj_view_t * zdj_new_menu_item( char * title, zdj_menu_item_view_layout_t layout );
zdj_view_t * zdj_new_icon_menu_item( 
    char * title, 
    zdj_ui_asset_t icon,
    zdj_ui_asset_t icon_hi 
);
zdj_view_t * zdj_new_asset_menu_item( 
    zdj_ui_asset_t icon,
    zdj_ui_asset_t icon_hi,
    bool hide_normal
);
zdj_view_t * zdj_new_data_menu_item( 
    char * title, 
    zdj_menu_item_view_layout_t layout,
    zdj_menu_item_data_display_type_t data_type,
    char * prefix,
    char * suffix
);
zdj_view_t * zdj_new_cuepoint_menu_item( char * name, char * cuepoint_eid );
zdj_view_t * zdj_new_usb_device_menu_item( zdj_usb_device_t * device_dto );
zdj_view_t * zdj_new_browser_device_menu_item( 
    char * title,
    char * path, 
    zdj_menu_item_view_browser_device_type_t type 
);
zdj_view_t * zdj_new_song_menu_item( 
    zdj_library_song_t * song, 
    bool show_title_and_artist,
    bool show_key, 
    bool show_camelot,
    bool show_bpm,
    zdj_menu_item_view_layout_t layout
);

zdj_view_t * zdj_menu_item_for_scroll_index( zdj_view_t * view, int index );

void zdj_menu_item_scroll_options( zdj_view_t * item, int dir );
// void zdj_menu_item_enter_edit_mode( zdj_view_t * item );
// void zdj_menu_item_exit_edit_mode( zdj_view_t * item );
void zdj_menu_item_enter_move_mode( zdj_view_t * item );
void zdj_menu_item_exit_move_mode( zdj_view_t * item );

void zdj_menu_item_set_layout( zdj_view_t * menu_item, zdj_menu_item_view_layout_t layout );

void zdj_menu_item_basic_l_init_layout( zdj_view_t * view );
void zdj_menu_item_basic_r_init_layout( zdj_view_t * view );
void zdj_menu_item_basic_lg_init_layout( zdj_view_t * view );
void zdj_menu_item_basic_launch_r_init_layout( zdj_view_t * view );
void zdj_menu_item_basic_l_edit_init_layout( zdj_view_t * view );
void zdj_menu_item_basic_med_l_edit_init_layout( zdj_view_t * view );
void zdj_menu_item_browser_device_init_layout( zdj_view_t * view );
void zdj_menu_item_data_l_init_layout( zdj_view_t * view );
void zdj_menu_item_data_r_init_layout( zdj_view_t * view );
void zdj_menu_item_file_init_layout( zdj_view_t * view );
void zdj_menu_item_dir_init_layout( zdj_view_t * view );
void zdj_menu_item_dir_select_init_layout( zdj_view_t * view );
void zdj_menu_item_dir_up_init_layout( zdj_view_t * view );
void zdj_menu_item_icon_init_layout( zdj_view_t * view );
void zdj_menu_item_inert_init_layout( zdj_view_t * view );
void zdj_menu_item_inert_data_init_layout( zdj_view_t * view );
void zdj_menu_item_inert_status_init_layout( zdj_view_t * view );
void zdj_menu_item_launch_big_init_layout( zdj_view_t * view );
void zdj_menu_item_launch_sm_init_layout( zdj_view_t * view );
void zdj_menu_item_playlist_init_layout( zdj_view_t * view );
void zdj_menu_item_slider_init_layout( zdj_view_t * view );
void zdj_menu_item_song_init_layout( zdj_view_t * view );
void zdj_menu_item_song_update_layout( zdj_view_t * view );
void zdj_menu_item_song_import_init_layout( zdj_view_t * view );
void zdj_menu_item_toggle_init_layout( zdj_view_t * view );
void zdj_menu_item_asset_init_layout( zdj_view_t * view );
void zdj_menu_item_cuepoint_init_layout( zdj_view_t * view );
void zdj_menu_item_usb_device_init_layout( zdj_view_t * view );

void zdj_menu_item_data_l_update_layout( zdj_view_t * view );
void zdj_menu_item_data_r_update_layout( zdj_view_t * view );
void zdj_menu_item_file_update_layout( zdj_view_t * view );
void zdj_menu_item_inert_data_update_layout( zdj_view_t * view );
void zdj_menu_item_inert_status_update_layout( zdj_view_t * view );
void zdj_menu_item_playlist_update_layout( zdj_view_t * view );
void zdj_menu_item_slider_update_layout( zdj_view_t * view );
void zdj_menu_item_song_playlist_update_layout( zdj_view_t * view );
void zdj_menu_item_song_import_update_layout( zdj_view_t * view );
void zdj_menu_item_toggle_update_layout( zdj_view_t * view );

#endif
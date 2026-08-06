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

#ifndef ZDJ_MENU_VIEW_H
#define ZDJ_MENU_VIEW_H

#include <SDL2/SDL.h>

#include <zerodj/ui/zdj_ui.h>

#define ZDJ_BACK_INDEX -1

typedef enum {
    ZDJ_MENU_INPUT_MODE_NORMAL,
    ZDJ_MENU_INPUT_MODE_EDIT_ITEM_OPTIONS,
    ZDJ_MENU_INPUT_MODE_EDIT_ITEM_POSITION
} zdj_menu_view_input_mode_t;

typedef struct {
    float position;
    float velocity;
    float momentum;
    float drag;
    float jog_input;
    float jog_force;
    int out_index;
} zdj_menu_view_scroll_filter_t;

typedef struct {
    int id;
    zdj_view_t * header_view;
    // has_back can be true even if menu doesn't have a header_view.
    // It can be used by an enclosing view as a header-less menu to determine
    // the enclosing view's back button visibility.  See file browser, for ex.
    bool has_back;
    zdj_menu_view_input_mode_t input_mode;
    bool edit_enabled; // DEPRECATING
    bool short_press_edit_enabled;
    bool long_press_edit_enabled;
    bool long_press_to_edit; // DEPRECATING
    zdj_view_t * edit_item; // ref to menu item currently taking edit inputs.
    int edit_item_move_top_index;
    int edit_item_move_bottom_index;
    double move_item_index;
    zdj_view_t * scroll_view;
    zdj_rect_t scroll_view_frame;
    bool scroll_enabled;
    bool scroll_animated;
    zdj_ui_orient_t scroll_dir;
    int scroll_index;
    int section_count;
    int item_count;
    zdj_menu_view_scroll_filter_t * scroll_filter;
    char lib_db_table[ 256 ]; // <- for lib db menus
    bool needs_layout_update; // <- for front-end use
} zdj_menu_view_state_t;

zdj_view_t * zdj_new_menu_view( zdj_ui_orient_t scroll_dir, zdj_rect_t * frame );
zdj_view_t * zdj_new_lib_menu_view( 
    zdj_ui_orient_t scroll_dir, 
    char * table_name, 
    zdj_rect_t * frame 
);
void zdj_menu_view_set_scrollview_frame( zdj_view_t * menu_view, zdj_rect_t * frame );
void zdj_menu_view_add_header( zdj_view_t * menu_view, zdj_view_t * header );
void zdj_menu_view_add_section( zdj_view_t * menu_view, zdj_view_t * section );
void zdj_menu_view_add_item( zdj_view_t * menu_view, zdj_view_t * item );
void zdj_menu_view_add_chrome_item( zdj_view_t * menu_view, zdj_view_t * item );
void zdj_menu_view_insert_item( zdj_view_t * menu_view, zdj_view_t * item, int index );
void zdj_menu_view_move_item( zdj_view_t * menu_view, zdj_view_t * item, int dir );
void zdj_menu_view_remove_all_items( zdj_view_t * menu_view );
void zdj_menu_view_remove_item_at_scroll_index( zdj_view_t * menu_view, int index );
void zdj_menu_view_remove_all_subviews( zdj_view_t * menu_view );
void zdj_menu_view_set_scroll_index( zdj_view_t * menu_view, int index );
void zdj_menu_view_add_padding( zdj_view_t * menu_view, int size );

zdj_view_t * zdj_menu_view_item_at_current_scroll_index( zdj_view_t * menu_view );
zdj_view_t * zdj_menu_view_item_at_scroll_index( zdj_view_t * menu_view, int index );
zdj_view_t * zdj_menu_view_get_item_for_data_ptr( zdj_view_t * menu_view, void * ptr );
zdj_view_t * zdj_menu_view_get_item_for_data_c_val( zdj_view_t * menu_view, char * c_val );

void zdj_menu_view_add_scroll_filter_input( zdj_view_t * view, int input );
void zdj_menu_view_update_scroll_filter( zdj_view_t * view );

// void zdj_menu_view_exit_edit_mode( zdj_view_t * view );

void zdj_menu_draw( zdj_view_t * view, zdj_view_clip_t * clip );
void zdj_menu_handle_control( zdj_view_t * view, zdj_control_event_t * _event );

#endif
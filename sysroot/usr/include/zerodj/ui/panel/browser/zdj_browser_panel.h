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

#ifndef ZDJ_BROWSER_PANEL_H
#define ZDJ_BROWSER_PANEL_H

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    // PANEL_VIEW_BASE EXTENSION - do not edit
    zdj_view_t * menu;
    zdj_view_t * overlay;
    int overlay_counter;
    bool needs_layout_update;
    zdj_view_t * event_target;
    void (*exit_cb) ( void* );
    // PANEL_VIEW_BASE EXTENSION - do not edit

    char path[ 256 ];
    zdj_ui_data_t data;
    zdj_view_t * header_view;
    zdj_view_t * menu_container;
    zdj_view_t * devices_menu;
    bool ( *file_validator )( char * );
    bool read_only;
    bool allow_nav;
    char select_dir_title[ 64 ];
    int usb_host_counter;
    bool is_device_menu;
    bool show_hidden;

} zdj_browser_panel_state_t;


zdj_view_t * zdj_new_browser_panel( void );
zdj_view_t * zdj_new_browser_panel_device_menu( zdj_view_t * browser, zdj_rect_t * frame );
void zdj_browser_panel_refresh_devices_menu( zdj_view_t * browser, zdj_view_t * menu );

zdj_view_t * zdj_new_browser_panel_file_menu_for_path( 
    zdj_view_t * browser,
    zdj_rect_t * frame, 
    char * path, 
    char * select_dir_title
);

void zdj_browser_panel_item_hmi_delegate( zdj_view_t * view, zdj_control_event_t * _event );

#endif
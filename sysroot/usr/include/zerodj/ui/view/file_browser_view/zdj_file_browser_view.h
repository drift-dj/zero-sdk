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

#ifndef ZDJ_FILE_BROWSER_VIEW_H
#define ZDJ_FILE_BROWSER_VIEW_H

#include <zerodj/ui/zdj_ui.h>

typedef enum {
    ZDJ_FILE_BROWSER_TYPE_SELECT_DIR,
    ZDJ_FILE_BROWSER_TYPE_SELECT_FILE,
    ZDJ_FILE_BROWSER_TYPE_SELECT_ANY,
    ZDJ_FILE_BROWSER_TYPE_BROWSE
} zdj_file_browser_type_t;

typedef enum {
    ZDJ_FILE_BROWSER_EXIT_STATUS_CANCEL,
    ZDJ_FILE_BROWSER_EXIT_STATUS_SELECT,
    ZDJ_FILE_BROWSER_EXIT_STATUS_DONE
} zdj_file_browser_exit_status_t;

typedef struct {
    zdj_file_browser_exit_status_t status;
    char dir[ 256 ];
    char filename[ 256 ];
    char filepath[ 256 ];
} zdj_file_browser_exit_context_t;

typedef struct {
    char path[ 256 ];
    zdj_ui_data_t data;
    zdj_view_t * header_view;
    zdj_view_t * menu_container;
    zdj_view_t * devices_menu;
    void ( *handle_file_browser_exit )( zdj_view_t *, zdj_file_browser_exit_context_t * );
    bool ( *file_validator )( char * );
    bool read_only;
    bool allow_nav;
    zdj_file_browser_type_t type;
    char select_dir_title[ 64 ];
    int usb_host_counter;
    bool is_device_menu;
    bool show_hidden;
} zdj_file_browser_view_state_t;

zdj_view_t * zdj_new_file_browser_view( 
    zdj_rect_t * frame, 
    char * path,
    bool read_only,
    bool allow_nav,
    zdj_file_browser_type_t type,
    char * select_dir_title,
    bool show_hidden
);

void zdj_file_browser_item_hmi_delegate( zdj_view_t * view, zdj_control_event_t * _event );

zdj_view_t * zdj_new_file_browser_menu_for_path( 
    zdj_view_t * browser,
    zdj_rect_t * frame, 
    char * path, 
    char * select_dir_title
);
zdj_view_t * zdj_new_device_browser_menu( zdj_view_t * browser, zdj_rect_t * frame );
void zdj_refresh_device_browser_menu( zdj_view_t * browser, zdj_view_t * menu );

#endif
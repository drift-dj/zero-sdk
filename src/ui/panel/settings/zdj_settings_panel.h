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

#ifndef ZDJ_SETTINGS_PANEL_H
#define ZDJ_SETTINGS_PANEL_H

#include <zerodj/system/installer/zdj_installer.h>

typedef struct {
    // PANEL_VIEW_BASE EXTENSION - do not edit
    zdj_view_t * menu;
    zdj_view_t * overlay;
    int overlay_counter;
    bool needs_layout_update;
    zdj_view_t * event_target;
    void (*exit_cb) ( void* );
    // PANEL_VIEW_BASE EXTENSION - do not edit

    // zdj_view_t * menu;
    // bool active;
    // bool view_needs_refresh;
    // zdj_view_t * overlay;
    // int overlay_counter;
    void * data;
    char str[256];
    bool b;
} zdj_settings_panel_state_t;

zdj_view_t * zdj_new_settings_panel( void );

// zdj_view_t * zdj_new_settings_software_panel( zdj_settings_panel_state_t * parent_panel );
// zdj_view_t * zdj_new_settings_app_panel( zdj_install_t * install );
// zdj_view_t * zdj_new_settings_installer_panel( zdj_installer_t * installer );
// zdj_view_t * zdj_new_settings_os_panel( zdj_settings_panel_state_t * parent_panel );
// zdj_view_t * zdj_new_settings_os_install_view( char * mount_path, zdj_os_sysreg_t * sysreg );
// zdj_view_t * zdj_new_settings_developer_panel( zdj_settings_panel_state_t * parent_panel );

#endif
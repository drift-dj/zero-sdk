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

#ifndef ZDJ_UI_PANEL_H
#define ZDJ_UI_PANEL_H

#include <zerodj/system/error/zdj_error.h>

typedef enum {
    ZDJ_UI_PANEL_ASSIST,
    ZDJ_UI_PANEL_RECORDING,
    ZDJ_UI_PANEL_SOUNDCARD,
    ZDJ_UI_PANEL_WIDGET,
    // ZDJ_UI_PANEL_DEBUG,
    ZDJ_UI_PANEL_COUNT
} zdj_panel_name_t;

typedef struct { 
    bool deployed;
    zdj_panel_name_t current_panel_name;
    zdj_view_t * current_panel;
    zdj_view_t * assist_panel;
    zdj_view_t * debug_panel;
    zdj_view_t * recording_panel;
    zdj_view_t * soundcard_panel;
    zdj_view_t * widget_panel;
    double panel_scroll_index;
    zdj_control_map_id_t exit_map; // Map to which we return when retracting panel
} zdj_panel_state_t;

zdj_error_type_t zdj_ui_panel_init( void );

void zdj_ui_panel_toggle( void );
void zdj_ui_panel_toggle_assist( void );
void zdj_ui_panel_toggle_debug( void );
void zdj_ui_panel_toggle_recording( void );
void zdj_ui_panel_toggle_soundcard( void );
void zdj_ui_panel_toggle_widget( void );

void zdj_ui_panel_scroll_next( void );
void zdj_ui_panel_scroll_prev( void );

#endif
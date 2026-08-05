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

#ifndef ZDJ_UI_WIDGET_H
#define ZDJ_UI_WIDGET_H

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/ui/zdj_ui.h>

typedef struct { 
    zdj_view_t * crash_widget;
    zdj_view_t * debug_widget;
    zdj_view_t * perf_widget;
    zdj_view_t * recording_widget;
    zdj_view_t * screencap_widget;
    zdj_view_t * volume_widget;
    zdj_view_t * notify_widget;
} zdj_widget_state_t;

zdj_error_type_t zdj_ui_widget_init( void );
zdj_view_t * zdj_ui_get_notify_widget( void );
void zdj_ui_widget_update_soundcard( void );

void zdj_ui_widget_show_crash_log( void );

#endif
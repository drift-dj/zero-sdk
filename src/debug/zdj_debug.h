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

#ifndef ZDJ_DEBUG_H
#define ZDJ_DEBUG_H

#include <zerodj/ui/zdj_ui.h>

typedef enum {
    ZDJ_DEBUG_UI_MEMORY,
    ZDJ_DEBUG_UI_HMI,
    ZDJ_DEBUG_UI_VIEW_STACK
} zdj_debug_ui_type_t;

typedef struct {
    unsigned long pid;
    zdj_debug_ui_type_t ui_type;
    bool show_ui;
    int update_counter;
    int update_duration;
    zdj_view_clip_t * draw_clip;
    zdj_view_t * line_1;
    zdj_view_t * line_2;
} zdj_debug_state_t;

extern zdj_debug_state_t * zdj_debug_state;

void zdj_debug_init( void );
void zdj_debug_ui_draw( void );

void zdj_debug_ui_show( void );
void zdj_debug_ui_hide( void );
void zdj_debug_ui_toggle( void );

#endif
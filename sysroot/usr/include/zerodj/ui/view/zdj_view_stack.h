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

#ifndef ZDJ_VIEW_STACK_H
#define ZDJ_VIEW_STACK_H

#include <zerodj/ui/zdj_ui.h>

extern zdj_view_t * zdj_view_stack_root_view;
extern zdj_view_t * zdj_view_stack_widget_panel;
extern zdj_view_t * zdj_view_stack_accessibility_panel;
extern zdj_view_t * zdj_view_stack_debug_panel;

zdj_view_t * zdj_root_view( void );
zdj_view_t * zdj_panel_view( void );
zdj_view_t * zdj_widget_view( void );
// zdj_view_t * zdj_widget_panel( void );
// zdj_view_t * zdj_accessibility_panel( void );
// zdj_view_t * zdj_debug_panel( void );
// zdj_view_t * zdj_perf_panel( void );

void zdj_view_stack_init( void );
void zdj_view_stack_deinit( void );
void zdj_view_stack_update( void );

void zdj_view_stack_clear_screen( void );

void zdj_view_stack_handle_special_events( int start_ind, int end_ind );
void zdj_view_stack_handle_events( int start_ind, int end_ind, zdj_view_t * view );
void zdj_view_stack_draw( zdj_view_t * view, zdj_view_clip_t * clip );

void zdj_view_stack_update_subview_clip( zdj_view_t * subview, zdj_view_clip_t * superview_clip );
zdj_view_t * zdj_view_stack_top_subview_of( zdj_view_t * view );
zdj_view_t * zdj_view_stack_bottom_subview_of( zdj_view_t * view );

#endif
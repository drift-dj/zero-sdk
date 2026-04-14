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

#ifndef ZDJ_ANIM_H
#define ZDJ_ANIM_H

#include <stdbool.h>

#include <zerodj/ui/zdj_ui.h>

#define ZDJ_ANIM_LENGTH_MENU_IN 50
#define ZDJ_ANIM_LENGTH_MENU_OUT 30
#define ZDJ_ANIM_LENGTH_MENU_BLINK_LENGTH 7
#define ZDJ_ANIM_LENGTH_MENU_BLINK_DELAY 30

int zdj_anim_show_predelay( void );
int zdj_anim_hide_predelay( void );
float zdj_anim_show_hide_frames( void );

void zdj_set_anim( zdj_anim_t * anim, zdj_anim_type_t type );

void zdj_anim_init_view_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_view_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_view( zdj_anim_t * anim, zdj_view_t * view );

void zdj_anim_init_menu_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_menu_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_menu( zdj_anim_t * anim, zdj_view_t * view );

void zdj_anim_init_menu_stack_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_menu_stack_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_menu_stack( zdj_anim_t * anim, zdj_view_t * view );

void zdj_anim_init_modal_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_modal_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_modal( zdj_anim_t * anim, zdj_view_t * view );

void zdj_anim_init_header_activate( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_header_deactivate( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_header( zdj_anim_t * anim, zdj_view_t * view );

void zdj_anim_init_header_back_activate( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_header_back_deactivate( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_header_back( zdj_anim_t * anim, zdj_view_t * view );

void zdj_anim_init_header_close_activate( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_header_close_deactivate( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_header_close( zdj_anim_t * anim, zdj_view_t * view );

void zdj_anim_init_dialog_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_dialog_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_dialog( zdj_anim_t * anim, zdj_view_t * view );

void zdj_anim_init_debug_panel_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_debug_panel_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_debug_panel( zdj_anim_t * anim, zdj_view_t * view );

void zdj_anim_init_dj_deck_page_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_dj_deck_page_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_dj_deck_page( zdj_anim_t * anim, zdj_view_t * view );

void zdj_anim_init_volume_panel_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_volume_panel_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_volume_panel( zdj_anim_t * anim, zdj_view_t * view );

void zdj_anim_init_record_panel_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_record_panel_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_record_panel( zdj_anim_t * anim, zdj_view_t * view );

void zdj_anim_init_record_widget_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_record_widget_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_record_widget( zdj_anim_t * anim, zdj_view_t * view );

void zdj_anim_init_debug_widget_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_debug_widget_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_debug_widget( zdj_anim_t * anim, zdj_view_t * view );

void zdj_anim_init_perf_widget_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_perf_widget_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_perf_widget( zdj_anim_t * anim, zdj_view_t * view );

void zdj_anim_init_notify_widget_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_notify_widget_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_notify_widget( zdj_anim_t * anim, zdj_view_t * view );

void zdj_anim_init_panel_in_next( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_panel_out_next( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_panel_in_prev( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_panel_out_prev( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_panel_deploy( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_panel_retract( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_panel( zdj_anim_t * anim, zdj_view_t * view );

#endif
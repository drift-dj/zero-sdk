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

zdj_anim_t * zdj_new_anim( zdj_anim_type_t type );
void zdj_anim_deinit( zdj_anim_t * anim );


void zdj_anim_init_view_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_view_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_view( zdj_anim_t * anim, zdj_view_t * view );
void zdj_anim_deinit_view( zdj_anim_t * anim );

void zdj_anim_init_menu_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_menu_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_menu( zdj_anim_t * anim, zdj_view_t * view );
void zdj_anim_deinit_menu( zdj_anim_t * anim );

void zdj_anim_init_menu_stack_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_menu_stack_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_menu_stack( zdj_anim_t * anim, zdj_view_t * view );
void zdj_anim_deinit_menu_stack( zdj_anim_t * anim );

void zdj_anim_init_modal_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_modal_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_modal( zdj_anim_t * anim, zdj_view_t * view );
void zdj_anim_deinit_modal( zdj_anim_t * anim );

void zdj_anim_init_header_activate( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_header_deactivate( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_header( zdj_anim_t * anim, zdj_view_t * view );
void zdj_anim_deinit_header( zdj_anim_t * anim );

void zdj_anim_init_dialog_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_dialog_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_dialog( zdj_anim_t * anim, zdj_view_t * view );
void zdj_anim_deinit_dialog( zdj_anim_t * anim );

void zdj_anim_init_debug_panel_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_debug_panel_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_debug_panel( zdj_anim_t * anim, zdj_view_t * view );
void zdj_anim_deinit_debug_panel( zdj_anim_t * anim );

void zdj_anim_init_dj_deck_page_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_init_dj_deck_page_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
void zdj_anim_update_dj_deck_page( zdj_anim_t * anim, zdj_view_t * view );
void zdj_anim_deinit_dj_deck_page( zdj_anim_t * anim );

#endif
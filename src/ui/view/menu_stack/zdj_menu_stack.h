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

#ifndef ZDJ_MENU_STACK_H
#define ZDJ_MENU_STACK_H

#include <zerodj/ui/zdj_ui.h>


typedef void ( *zdj_menu_stack_retract_cb_t )( void* );

typedef struct {
    int id;
    zdj_view_t * menus;
    bool can_retract;
    bool is_enabled;
    zdj_menu_stack_retract_cb_t retract_cb;
    void * retract_data;
} zdj_menu_stack_state_t;

zdj_view_t * zdj_new_menu_stack( 
    zdj_rect_t * frame, 
    zdj_menu_stack_retract_cb_t retract_cb,
    void * retract_data 
);

// Show+Enable/Hide+Disable an existing menu_stack without altering its view stack linkage
void zdj_menu_stack_deploy( zdj_view_t * menu_stack );
void zdj_menu_stack_retract( zdj_view_t * menu_stack );
// Set a root menu reference - used to capture root menu's back command to auto-retract.
void zdj_menu_stack_set_root_menu( zdj_view_t * menu_stack, zdj_view_t * root_menu );



#endif
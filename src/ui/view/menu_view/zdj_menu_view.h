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

#ifndef ZDJ_MENU_VIEW_H
#define ZDJ_MENU_VIEW_H

#include <SDL2/SDL.h>

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    zdj_view_t * header_view;
    // has_back can be true even if menu doesn't have a header_view.
    // It can be used by an enclosing view as a header-less menu to determine
    // the enclosing view's back button visibility.  See file browser, for ex.
    bool has_back;
    zdj_view_t * scroll_view;
    bool scroll_enabled;
    zdj_ui_orient_t scroll_dir;
    int scroll_index;
    int section_count;
    int item_count;
} zdj_menu_view_state_t;

zdj_view_t * zdj_new_menu_view( zdj_ui_orient_t scroll_dir, zdj_rect_t * frame );
void zdj_menu_view_add_header( zdj_view_t * menu_view, zdj_view_t * header );
void zdj_menu_view_add_section( zdj_view_t * menu_view, zdj_view_t * section );
void zdj_menu_view_add_item( zdj_view_t * menu_view, zdj_view_t * item );
void zdj_menu_view_set_scroll_index( zdj_view_t * menu_view, int index );


#endif
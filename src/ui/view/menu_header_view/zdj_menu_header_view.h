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

#ifndef ZDJ_MENU_HEADER_VIEW_H
#define ZDJ_MENU_HEADER_VIEW_H

typedef enum {
    ZDJ_MENU_HEADER_STYLE_NONE,
    ZDJ_MENU_HEADER_STYLE_NORMAL
} zdj_menu_view_header_style_t;

typedef enum {
    ZDJ_MENU_HEADER_BACK_STYLE_NONE,
    ZDJ_MENU_HEADER_BACK_STYLE_BACK,
    ZDJ_MENU_HEADER_BACK_STYLE_CANCEL,
    ZDJ_MENU_HEADER_BACK_STYLE_X
} zdj_menu_view_header_back_style_t;

typedef struct {
    zdj_menu_view_header_style_t style;
    char * name;
    char * title;
    zdj_view_t * name_label;
    zdj_view_t * title_ticker;
    zdj_view_t * title_divider;
    bool has_valid_display;
    bool is_hilite;
    bool is_blinking;
    int blink_timer;
    zdj_menu_view_header_back_style_t back_style;
    bool has_back;
    bool show_back;
    bool hide_back;
    void ( *handle_back )( zdj_view_t * );
    zdj_view_t * back_view;
} zdj_menu_header_view_state_t;

zdj_view_t * zdj_new_menu_header( 
    char * name, 
    char * title, 
    zdj_menu_view_header_style_t style, 
    zdj_menu_view_header_back_style_t back_btn_style 
);

#endif
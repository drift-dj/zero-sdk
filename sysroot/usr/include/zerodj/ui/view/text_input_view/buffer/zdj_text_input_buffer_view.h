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

#ifndef ZDJ_TEXT_INPUT_BUFFER_VIEW_H
#define ZDJ_TEXT_INPUT_BUFFER_VIEW_H

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    char * str;
    int cursor_index;
    zdj_view_t * left_label;
    zdj_view_t * right_label;
    zdj_view_t * cursor_label;
    bool has_valid_layout;
    int cursor_counter;
    int cursor_char;
} zdj_text_input_buffer_view_state_t;

zdj_view_t * zdj_new_text_input_buffer_view( char * input_str );
void zdj_text_input_buffer_backspace( zdj_view_t * input_buffer );
void zdj_text_input_buffer_insert( zdj_view_t * input_buffer );

#endif
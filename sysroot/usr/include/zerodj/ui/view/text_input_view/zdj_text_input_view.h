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

#ifndef ZDJ_TEXT_INPUT_VIEW_H
#define ZDJ_TEXT_INPUT_VIEW_H

#include <SDL2/SDL.h>

#include <zerodj/ui/zdj_ui.h>

typedef enum {
    ZDJ_TEXT_INPUT_ACTION_OKAY,
    ZDJ_TEXT_INPUT_ACTION_CANCEL
} zdj_text_input_view_action_t;

typedef void ( *zdj_text_input_callback_t )( zdj_text_input_view_action_t, char * );

typedef struct {
    char * input_str;
    zdj_text_input_callback_t cb;
    zdj_view_t * input_buffer;
    zdj_view_t * keyboard_menu;
    bool shift_key_active;
    bool keyboard_chrome_selected;
} zdj_text_input_view_state_t;

zdj_view_t * zdj_new_text_input_view( zdj_text_input_callback_t cb, char * input );

#endif
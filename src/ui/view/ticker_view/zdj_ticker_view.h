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

#ifndef ZDJ_TICKER_VIEW_H
#define ZDJ_TICKER_VIEW_H

#include <SDL2/SDL.h>

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    char * str;
    zdj_font_t font;
    zdj_justify_t justify;
    zdj_view_t * texture_view;
    int text_h;
    int single_text_w;
    int double_text_w;
    int text_w; // For superviews which need to know final width of ticker
    bool is_scrolling;
    float scroll_rate;
    float scroll_offset;
} zdj_ticker_state_t;

zdj_view_t * zdj_new_ticker_view( 
    char * str,
    zdj_font_t font,
    zdj_justify_t justify,
    SDL_Color color
);

int zdj_ticker_view_get_text_w( zdj_view_t * view );

#endif
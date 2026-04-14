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

#ifndef ZDJ_LOOP_VIEW_H
#define ZDJ_LOOP_VIEW_H

#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/ui/zdj_ui.h>

typedef enum {
    ZDJ_LOOP_VIEW_TYPE_PLAYBACK,
    ZDJ_LOOP_VIEW_TYPE_EDIT,
    ZDJ_LOOP_VIEW_TYPE_HIDDEN,
} zdj_loop_view_type_t;

typedef struct {
    zdj_loop_view_type_t type;
    zdj_deck_t * deck;
    zdj_pipeline_node_t * decode_node;
    double zoom_val;
    float draw_y;
} zdj_loop_view_state_t;

zdj_view_t * zdj_new_loop_view( 
    zdj_rect_t * frame, 
    zdj_deck_t * deck,
    double zoom_val,
    zdj_loop_view_type_t type
);

#endif
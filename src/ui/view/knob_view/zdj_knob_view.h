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

#ifndef ZDJ_KNOB_TAPE_VIEW_H
#define ZDJ_KNOB_TAPE_VIEW_H

#include <SDL2/SDL.h>

#include <zerodj/ui/zdj_ui.h>

typedef enum {
    ZDJ_KNOB_TYPE_DET_UNITY,
    ZDJ_KNOB_TYPE_DET_MIDDLE,
    ZDJ_KNOB_TYPE_FILT_BI,
} zdj_knob_type_t;

typedef struct {
    zdj_knob_type_t type;
    zdj_rect_t frame;
    float val;
    zdj_view_t * bg;
    zdj_view_t * tape;
    zdj_view_t * edge;
    zdj_view_t * detent;
    zdj_view_t * label_1;
    zdj_view_t * label_2;
} zdj_knob_state_t;

zdj_view_t * zdj_new_knob_view( zdj_knob_type_t type, zdj_rect_t * frame );
void zdj_knob_view_set_val( zdj_view_t * knob_view, double val );

#endif
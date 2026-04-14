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

#ifndef ZDJ_FLAG_VIEW_H
#define ZDJ_FLAG_VIEW_H

#include <SDL2/SDL.h>

#include <zerodj/ui/zdj_ui.h>

typedef enum {
    ZDJ_FLAG_TYPE_CUE_MINI,
    ZDJ_FLAG_TYPE_CUE_NORM,
    ZDJ_FLAG_TYPE_CUE_LOOP,
    ZDJ_FLAG_TYPE_CUE_NORM_TOP,
    ZDJ_FLAG_TYPE_CUE_NORM_BOTTOM,
    ZDJ_FLAG_TYPE_CUE_LOOP_TOP,
    ZDJ_FLAG_TYPE_CUE_LOOP_BOTTOM,
    ZDJ_FLAG_TYPE_BAR,
    ZDJ_FLAG_TYPE_TEXT
} zdj_flag_type_t;


typedef struct {
    zdj_flag_type_t type;
    char str[ 64 ];
    SDL_Texture * tex;
    int tex_w;
    int tex_h;
} zdj_flag_state_t;

zdj_view_t * zdj_new_flag_view( 
    zdj_flag_type_t type,
    char * str
);

#endif
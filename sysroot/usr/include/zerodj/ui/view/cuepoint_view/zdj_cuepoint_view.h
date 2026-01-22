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

#ifndef ZDJ_CUE_POINT_VIEW_H
#define ZDJ_CUE_POINT_VIEW_H

#include <SDL2/SDL.h>

#include <zerodj/ui/zdj_ui.h>

typedef enum {
    ZDJ_CUEPOINT_STYLE_EDIT,
    ZDJ_CUEPOINT_STYLE_STATION_1_PLAYBACK,
    ZDJ_CUEPOINT_STYLE_STATION_2_PLAYBACK,
    ZDJ_CUEPOINT_STYLE_STATION_EXT_PLAYBACK
} zdj_cuepoint_style_t;

typedef struct {
    zdj_cuepoint_style_t style;
    zdj_deck_t * deck;
    zdj_library_song_t * song;
    double zoom_val;
    zdj_library_cuepoint_t * cuepoints;
} zdj_cuepoint_view_state_t;

zdj_view_t * zdj_new_cuepoint_view( 
    zdj_rect_t * frame, 
    zdj_cuepoint_style_t style,
    zdj_deck_t * deck,
    zdj_library_song_t * song,
    char * suppress_eid,
    double zoom_val 
);
#endif
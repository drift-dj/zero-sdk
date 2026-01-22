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

#ifndef ZDJ_WAVEFORM_VIEW_H
#define ZDJ_WAVEFORM_VIEW_H

#include <zerodj/signal/pipeline/node/analysis/waveform/zdj_waveform.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/ui/zdj_ui.h>

#define ZDJ_WAVEFORM_ZOOM_MAX 1
#define ZDJ_WAVEFORM_ZOOM_LORES_MAX 256
#define ZDJ_WAVEFORM_ZOOM_MIN 8192

typedef struct {
    zdj_pipeline_node_t * pipe;
    zdj_soundcard_node_t * node;
    zdj_pipeline_node_t * waveform_node;
    double prev_pcm_head;

    double raw_point_head;
    double draw_point_head;
} zdj_live_waveform_view_state_t;

typedef struct {
    zdj_waveform_style_t style;
    zdj_pipeline_node_t * waveform_node;
    SDL_Texture * waveform_tex;
    double zoom_val;
    double ( *get_center_ratio )( zdj_view_t * );
    double ( *get_zoom_ratio )( zdj_view_t * );
} zdj_waveform_view_state_t;

zdj_view_t * zdj_new_live_waveform_view( zdj_rect_t * frame, zdj_soundcard_node_t * node );
zdj_view_t * zdj_new_playback_waveform_view( 
    zdj_rect_t * frame, 
    zdj_waveform_style_t style,
    zdj_deck_t * deck,
    zdj_pipeline_node_t * decode_node,
    zdj_library_song_t * song, 
    double zoom_val,
    bool hires
);
zdj_view_t * zdj_new_thumb_waveform_view( zdj_rect_t * frame, zdj_library_song_t * song );

#endif
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

#ifndef ZDJ_WAVEFORM_H
#define ZDJ_WAVEFORM_H

#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/ui/zdj_ui.h>

// Live waveform buffer holds 1 sec audio plus extra for gaussian at edges
#define ZDJ_LIVE_WAVEFORM_SAMPLE_COUNT 44100+1024

typedef struct {
    uint32_t buffer_ind;
    uint64_t sample_addr;
} zdj_sample_buf_addr_t;

typedef struct {
    zdj_soundcard_node_t * soundcard_node;
    float * source_buf;
    float * render_buf;
    float point_count;
    float samples_per_point;
    float source_win_w;
    float source_push_head;
    float source_render_head;
    float source_push_tail;
    float source_render_tail;
    // float render_win_datum_offset;
    // zdj_sample_buf_addr_t datum_addr;
    zdj_gaussian_t * g;
    void ( *handle_soundcard_node_push )( void *, zdj_pipeline_node_t *, bool );
    void ( *render )( zdj_pipeline_node_t * );
} zdj_live_waveform_state_t;

zdj_pipeline_node_t * zdj_new_live_waveform( void );
zdj_error_type_t zdj_live_waveform_set_scale( zdj_pipeline_node_t * waveform, float scale );
zdj_error_type_t zdj_live_waveform_set_point_count( zdj_pipeline_node_t * waveform, int point_count );

zdj_pipeline_node_t * zdj_new_playback_waveform( char * filepath );
zdj_pipeline_node_t * zdj_new_thumb_waveform( char * filepath );

#endif
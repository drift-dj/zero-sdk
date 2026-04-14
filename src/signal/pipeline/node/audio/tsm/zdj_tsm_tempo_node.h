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

#ifndef ZDJ_TSM_TEMPO_NODE_H
#define ZDJ_TSM_TEMPO_NODE_H

#include <rubberband-c.h>

#include <zerodj/signal/pipeline/zdj_pipeline.h>

typedef struct {
    bool stereo;
    int channel_count;
    int sample_count;
    double rate;
    bool has_rate_update;
    volatile float * out_buffer;

    RubberBandState rb;
    // float **rb_in_ptrs; // Array of 2 [float] pointers containing input de-interleaved stereo PCM data
    // float **rb_out_ptrs; // Array of 2 [float] pointers containing processed de-interleaved stereo PCM
    const float *rb_in_ptrs[2]; // Array of 2 [float] pointers containing input de-interleaved stereo PCM data
    const float *rb_out_ptrs[2]; // Array of 2 [float] pointers containing processed de-interleaved stereo PCM
    size_t rb_start_delay;
    size_t rb_preferred_start_pad;

    zdj_pipeline_node_t * decode_node;
    double decode_coord;
    int64_t prev_decode_buf_start_addr;

} zdj_tsm_tempo_node_state_t;

zdj_pipeline_node_t * zdj_new_tsm_tempo_node( 
    bool stereo, 
    int sample_count,
    zdj_pipeline_node_t * decode_node 
);

void zdj_reset_tsm_tempo_node( zdj_pipeline_node_t * node );

#endif
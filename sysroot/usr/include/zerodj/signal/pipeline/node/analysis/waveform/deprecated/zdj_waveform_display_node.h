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

#ifndef ZDJ_WAVEFORM_DISPLAY_NODE_H
#define ZDJ_WAVEFORM_DISPLAY_NODE_H

#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>

typedef struct {
    double scale;
    int point_count;
    zdj_gaussian_t * gaussian;
    zdj_pipeline_node_t * pcm_node;
} zdj_waveform_display_node_state_t;

zdj_pipeline_node_t * zdj_new_waveform_display_node( 
    zdj_pipeline_node_t * pcm_node, 
    double scale, 
    int point_count 
);

zdj_error_type_t zdj_waveform_display_node_set_scale( 
    zdj_pipeline_node_t * node, 
    double scale 
);

#endif
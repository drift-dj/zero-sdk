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

#ifndef ZDJ_DECODE_NODE_H
#define ZDJ_DECODE_NODE_H

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>

typedef enum {
    ZDJ_PCM_INIT,
    ZDJ_PCM_READY,
    ZDJ_PCM_RUNNING,
    ZDJ_PCM_DONE,
    ZDJ_PCM_IDLE
} zdj_decode_node_status_t;

typedef struct {
    zdj_library_song_t * song;
    zdj_pipeline_node_t * raw_node;
    zdj_decode_node_status_t status;
    size_t win_back_infill;
    size_t win_fwd_infill;
} zdj_decode_node_state_t;

zdj_pipeline_node_t * zdj_new_decode_node( 
    zdj_library_song_t * song,
    size_t win_fwd,
    size_t win_back
);

#endif
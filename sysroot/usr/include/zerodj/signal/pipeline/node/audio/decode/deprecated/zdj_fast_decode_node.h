Copyright (c) 2025 Drift DJ Industries

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

#ifndef ZDJ_FAST_DECODE_NODE_H
#define ZDJ_FAST_DECODE_NODE_H

#include <mpg123.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>

typedef enum {
    ZDJ_FAST_DECODE_INIT,
    ZDJ_FAST_DECODE_READY,
    ZDJ_FAST_DECODE_RUNNING,
    ZDJ_FAST_DECODE_DONE,
    ZDJ_FAST_DECODE_IDLE
} zdj_fast_decode_node_status_t;

typedef struct {
    zdj_library_song_t * song_dto;
    zdj_fast_decode_node_status_t status;
    sem_t * wait_sem;
    sem_t * ready_sem;
    pthread_t thread;
} zdj_fast_decode_node_state_t;

zdj_pipeline_node_t * zdj_new_fast_decode_node( zdj_library_song_t * song );

#endif
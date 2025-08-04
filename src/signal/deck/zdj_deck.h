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

#ifndef ZDJ_DECK_H
#define ZDJ_DECK_H

#include <stdbool.h>
#include <pthread.h>

#include <zerodj/signal/pipeline/zdj_pipeline.h>

typedef enum {
    ZDJ_DECK_TYPE_PLAYBACK,
    ZDJ_DECK_TYPE_EXTERNAL,
    ZDJ_DECK_TYPE_TEST
} zdj_deck_type_t;

typedef enum {
    ZDJ_DECK_NUM_1,
    ZDJ_DECK_NUM_2,
    ZDJ_DECK_NUM_EXT
} zdj_deck_num_t;

typedef struct {
    pthread_t * control_sim_thread;
} zdj_deck_transport_t;

typedef struct {
    zdj_deck_type_t type;
    zdj_deck_num_t num;

    // Link into the soundcard graph's fast-cycle mix flow.
    void ( *get_edge_data )( void *, zdj_pipeline_node_t *, bool );

    // internal state
    void * state;
    zdj_deck_transport_t transport;
} zdj_deck_t;

typedef struct {
    float s1_p;
    float s1_f;
    float s2_p;
    float s2_f;
    float s3_p;
    float s3_f;
    float s4_p;
    float s4_f;
} zdj_test_deck_state_t;

typedef struct {
     // Internal audio pipeline
    zdj_pipeline_node_t * dsp_node;
    zdj_pipeline_node_t * tsm_node;
    zdj_pipeline_node_t * decode_node;
    zdj_pipeline_node_t * file_node;
} zdj_playback_deck_state_t;

typedef struct {

} zdj_external_deck_state_t;

zdj_deck_t * zdj_new_deck( zdj_deck_type_t type, zdj_deck_num_t num );
zdj_error_type_t zdj_deinit_deck( zdj_deck_t * deck );

// One entry point for handling control inputs
// One update cycle for running the control simulation
// One entry point for data requests from other pipelines


#endif
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

#ifndef ZDJ_TEST_DECK_H
#define ZDJ_TEST_DECK_H

#include <stdbool.h>
#include <pthread.h>

typedef struct {

} zdj_test_transport_t;

typedef struct {
    // Links into the soundcard graph
    zdj_pipeline_node_t * input_link;
    zdj_pipeline_node_t * prefade_link;
    zdj_pipeline_node_t * bus_link;

    // Internal audio pipeline
    zdj_pipeline_node_t * tone_node;

    zdj_test_transport_t transport;
} zdj_test_deck_t;

zdj_test_deck_t * zdj_new_test_deck( void );
zdj_error_type_t zdj_deinit_test_deck( zdj_test_deck_t * deck );

// One entry point for handling control inputs
// One update cycle for running the control simulation
// One entry point for data requests from other pipelines


#endif
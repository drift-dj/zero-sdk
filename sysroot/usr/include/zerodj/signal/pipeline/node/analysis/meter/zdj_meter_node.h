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

#ifndef ZDJ_METER_NODE_H
#define ZDJ_METER_NODE_H

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>

typedef enum {
    ZDJ_METER_NODE_TYPE_AUDIO,
    ZDJ_METER_NODE_TYPE_CLOCK,
    ZDJ_METER_NODE_TYPE_CV,
    ZDJ_METER_NODE_TYPE_MIDI
} zdj_meter_node_type_t;

typedef struct {
    zdj_meter_node_type_t type;
    void ( *add_frame )( struct zdj_pipeline_node_t *, float, float );
    int channel_count;
    // instant meter data
    float instant_val_0;
    float instant_val_1;
    // audio vu meter data
    float lowpass_val_0;
    float lowpass_peak_0;
    float lowpass_val_1;
    float lowpass_peak_1;
    // audio chan 0 ol lamps
    bool has_ol_0_0;
    int timer_ol_0_0;
    bool has_ol_1_0;
    int timer_ol_1_0;
    bool has_clip_0;
    int timer_clip_0;
    // audio chan 1 ol lamps
    bool has_ol_0_1;
    int timer_ol_0_1;
    bool has_ol_1_1;
    int timer_ol_1_1;
    bool has_clip_1;
    int timer_clip_1;
    // clock signal data
    float clock_bpm;
    bool has_clock_pulse;
    int timer_clock_pulse;
    int clock_beat;
} zdj_meter_node_state_t;

zdj_pipeline_node_t * zdj_new_meter_node( zdj_meter_node_type_t type, int channel_count  );
zdj_error_type_t zdj_meter_node_reset( zdj_pipeline_node_t * meter_node );

#endif
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

#ifndef ZDJ_HMI_SHARED_STATE_MODEL_H
#define ZDJ_HMI_SHARED_STATE_MODEL_H

#include <stdint.h>

typedef struct {
    int32_t out_state;
    int32_t out_pb_state;
    int32_t jog_state;
    int32_t jog_pb_state;
    int32_t tone_1_state;
    int32_t tone_1_pb_state;
    int32_t tone_2_state;
    int32_t tone_2_pb_state;
    int32_t tone_3_state;
    int32_t tone_3_pb_state;
    int32_t hotcue_pb_state;
    int32_t play_pb_state;
    int32_t nav_pb_state;
    int32_t fn_1_pb_state;
    int32_t fn_2_pb_state;
    int32_t fn_3_pb_state;
    int32_t fade_1_state;
    int32_t fade_2_state;
    int32_t xfade_state;
} zdj_hmi_m7_state_model_t;

#endif
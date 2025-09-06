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

#ifndef ZDJ_DECK_CONTROLS_H
#define ZDJ_DECK_CONTROLS_H

// #include <stdint.h>
// #include <stdbool.h>

// #include <zerodj/signal/pipeline/zdj_pipeline.h>
// #include <zerodj/system/error/zdj_error.h>

// // Loops crossfade smoothly from start addr to end addr.
// // This crossfade can span multiple buffers/update cycles.
// // While the crossfade is active, updates to start/end address
// // won't take effect until crossfade is over.
// typedef enum {
//     ZDJ_DECK_LOOP_PHASE_INACTIVE,
//     ZDJ_DECK_LOOP_PHASE_CAPTURE,
//     ZDJ_DECK_LOOP_PHASE_RUN,
//     ZDJ_DECK_LOOP_PHASE_CROSSFADING,
//     ZDJ_DECK_LOOP_PHASE_EXIT
// } zdj_deck_loop_phase_t;

// typedef struct {
//     zdj_deck_loop_phase_t phase;
//     zdj_pipeline_addr_t start_addr;
//     zdj_pipeline_addr_t end_addr;
//     zdj_pipeline_addr_t capture_addr;
//     int fade_len;
// } zdj_deck_loop_state_t;


// // Beat skip can't be updated while actively crossfading.
// typedef enum {
//     ZDJ_DECK_SKIP_PHASE_INACTIVE, // Idle - do nothing
//     ZDJ_DECK_SKIP_PHASE_STAGED, // Address has been determined, crossfade has not yet started
//     ZDJ_DECK_SKIP_PHASE_CROSSFADING, // Crossfade is running
// } zdj_deck_skip_phase_t;

// typedef struct {
//     zdj_deck_skip_phase_t phase;
//     zdj_pipeline_addr_t depart_addr;
//     zdj_pipeline_addr_t arrive_addr;
//     int fade_len;
// } zdj_deck_skip_state_t;


// typedef struct {
//     float instant_pitch; // Current cycle's pitch value
//     float set_pitch; // Ultimate pitch set point
//     float instant_scrub; // Current cycle's scrub value
//     int64_t set_scrub; // 1st order scrub address
//     float instant_nudge; // Current cycle's nudge value
//     float ui_nudge; // Nudge value modified for UI display
//     bool drive_enabled; // play/pause
// } zdj_deck_drive_state_t;

// typedef struct zdj_deck_control_state_t { 
//     zdj_deck_drive_state_t drive_state;
//     zdj_deck_skip_state_t skip_state;
//     zdj_deck_loop_state_t loop_state;
// } zdj_deck_control_state_t;


#endif
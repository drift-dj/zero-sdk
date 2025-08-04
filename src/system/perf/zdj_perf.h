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

#ifndef ZDJ_PERF_H
#define ZDJ_PERF_H

#include <stdint.h>
#include <stdbool.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/thread/zdj_thread.h>

typedef enum {
    ZDJ_PERF_TAG_CONTROL_CYCLE,
    ZDJ_PERF_TAG_UI_CYCLE,
    ZDJ_PERF_TAG_AUDIO_BUF_CYCLE,
    ZDJ_PERF_TAG_COUNT
} zdj_perf_tag_name_t;

static char * zdj_perf_tag_name[ ZDJ_PERF_TAG_COUNT ] = { 
    "Ctrl",// ZDJ_PERF_TAG_CONTROL_CYCLE,
    "UI",// ZDJ_PERF_TAG_UI_CYCLE,
    "Aud",// ZDJ_PERF_TAG_SOUNDCARD_FAST_CYCLE
};

typedef struct {
    zdj_perf_tag_name_t name;
    uint64_t start;
    uint64_t end;
} zdj_perf_tag_t;

typedef struct {
    uint32_t tag_count;
    uint32_t tag_max;
    zdj_perf_tag_t * tags;
} zdj_perf_state_t;

typedef struct {
    zdj_perf_tag_name_t name;
    uint32_t count;
    uint64_t max_dur;
    uint64_t min_dur;
    uint64_t avg_dur;
    uint32_t max_cadence;
    uint32_t min_cadence;
    uint32_t avg_cadence;
    struct zdj_perf_report_line_t * next;
} zdj_perf_report_line_t;

typedef struct {
    int line_count;
    zdj_perf_report_line_t * lines;
    uint32_t cycle_count;
    uint32_t miss_count;
} zdj_perf_report_t;

// Get raw time for perf tag
uint64_t zdj_perf_time( void );

zdj_error_type_t zdj_perf_init( uint32_t tag_count );
zdj_error_type_t zdj_enable_perf( void );
zdj_error_type_t zdj_disable_perf( void );
zdj_error_type_t zdj_reset_perf( void );

bool zdj_perf_enabled( void );

zdj_perf_tag_t * zdj_new_perf_tag_for_thread( zdj_system_thread_t thread );

zdj_perf_report_t * zdj_new_perf_report( void );
zdj_perf_report_line_t * zdj_perf_report_line_for_name( 
    zdj_perf_report_t * report,
    zdj_perf_tag_name_t name 
);
zdj_error_type_t zdj_perf_report_add_tag( 
    zdj_perf_report_t * report, 
    zdj_perf_tag_t * tag,
    zdj_perf_tag_t * next_tag
);

zdj_perf_report_t * zdj_perf_make_cycle_timing_report( void );

#endif
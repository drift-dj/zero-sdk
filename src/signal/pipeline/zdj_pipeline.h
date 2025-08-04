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

#ifndef ZDJ_PIPELINE_H
#define ZDJ_PIPELINE_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/perf/zdj_perf.h>

#define ZDJ_WAVEFORM_WINDOW_SAMPLE_STRIDE 2048

typedef enum {
    ZDJ_PIPELINE_TIMEBASE_PCM_44100,
    ZDJ_PIPELINE_TIMEBASE_PCM_48000,
    ZDJ_PIPELINE_TIMEBASE_PCM_96000,
    ZDJ_PIPELINE_TIMEBASE_SECONDS,
    ZDJ_PIPELINE_TIMEBASE_BEATS
} zdj_pipeline_timebase_t;

typedef struct {
    double d_val;
    uint32_t u_val;
    int i_val;
    zdj_pipeline_timebase_t timebase;
} zdj_pipeline_mark_t;

typedef enum {
    ZDJ_PIPELINE_BREAK_LOOP,
    ZDJ_PIPELINE_BREAK_SKIP,
    ZDJ_PIPELINE_BREAK_SPLICE,
    ZDJ_PIPELINE_BREAK_START,
    ZDJ_PIPELINE_BREAK_START_FX,
    ZDJ_PIPELINE_BREAK_STOP,
    ZDJ_PIPELINE_BREAK_STOP_FX,
} zdj_pipeline_break_type_t;

typedef struct {
    zdj_pipeline_mark_t mark_in;
    zdj_pipeline_mark_t mark_out;
    zdj_pipeline_break_type_t type;
    bool quantized;
} zdj_pipeline_break_t;

typedef struct {
    // Abstract reference address for window in external data ref's coordinate space.
    // e.g. SEEK_SET offset in file pointer, PCM sample # in WAV file.
    uint64_t ext_ref_addr; 
    zdj_pipeline_mark_t source_mark;
    // Index into buffer/linked list where external reference address is found.
    uint32_t int_ref_addr_index;
    uint32_t source_mark_index;

    // Full width of window
    int len;
    // Count of indexes from int_ref_addr_index to end of buffer/linked list.
    int fwd_len;
    // Count of indexes from int_ref_addr_index to beginning of buffer/linked list.
    int back_len;

    // Index in buffer/linked list of forward extent of valid data.
    // Used when moving window a small amount to infill new data.
    int fwd_valid_index;
    // Index in buffer/linked list of backward extent of valid data.
    // Used when moving window a small amount to infill new data.
    int back_valid_index;
} zdj_pipeline_window_state_t;

typedef struct {
    int data_start;
    int data_end;
} zdj_pipeline_window_state_valid_data_t;

typedef enum {
    ZDJ_PIPELINE_NODE_STATUS_RUNNING,
    ZDJ_PIPELINE_NODE_STATUS_IDLE
} zdj_pipeline_node_status_t;

typedef struct zdj_pipeline_node_t {
    void * state;
    void ( *deinit_state )( struct zdj_pipeline_node_t * );

    zdj_pipeline_window_state_t * window_state;
    zdj_error_type_t ( *move_window )( struct zdj_pipeline_node_t *, int );
    zdj_error_type_t ( *reset_window )( struct zdj_pipeline_node_t *, uint32_t );
    zdj_error_type_t ( *resize_window )( struct zdj_pipeline_node_t *, uint32_t, uint32_t );

    float * ( *get_data )( struct zdj_pipeline_node_t * );

    // Push funcs - Used when front-end code tells pipeline when to process data.
    // e.g. FX processor
    void ( *update_wait )( struct zdj_pipeline_node_t * );
    void ( *update_async )( struct zdj_pipeline_node_t * );

    // Pull callback - Used when internal system tells front-end when pipeline has new data to process 
    // e.g. analog io node - updates are triggered within M7 core
    void ( *update_cb )( struct zdj_pipeline_node_t * );

    pthread_t * thread;

    sem_t * async_wait;
    sem_t * async_ready;

    zdj_error_type_t ( *open )( struct zdj_pipeline_node_t * );
    zdj_error_type_t ( *close )( struct zdj_pipeline_node_t * );
} zdj_pipeline_node_t;

zdj_pipeline_node_t * zdj_new_pipeline_node( void );
zdj_error_type_t zdj_deinit_pipeline_node( zdj_pipeline_node_t * node );

// zdj_error_type_t zdj_pipeline_enable_perf( zdj_pipeline_node_t * node, uint32_t tag_count );
// zdj_error_type_t zdj_pipeline_disable_perf( zdj_pipeline_node_t * node );
// zdj_error_type_t zdj_pipeline_reset_perf( zdj_pipeline_node_t * node );

bool zdj_pipeline_window_state_get_valid_indexes( 
    zdj_pipeline_window_state_t * window_state, 
    zdj_pipeline_window_state_valid_data_t * valid_data 
);
zdj_error_type_t zdj_pipeline_window_state_move( zdj_pipeline_window_state_t * window_state, int offset );
zdj_error_type_t zdj_pipeline_window_state_reset( zdj_pipeline_window_state_t * window_state, uint32_t ext_address );
zdj_error_type_t zdj_pipeline_window_state_resize( 
    zdj_pipeline_window_state_t * window_state,
    uint32_t back_len, 
    uint32_t fwd_len
);
bool zdj_pipeline_window_contains_address_range( 
    zdj_pipeline_window_state_t * window_state, 
    uint32_t start, 
    uint32_t end 
);

void zdj_pipeline_window_print( zdj_pipeline_window_state_t * window_state );

#endif
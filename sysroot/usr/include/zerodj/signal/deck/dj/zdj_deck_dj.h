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

#ifndef ZDJ_DECK_DJ_H
#define ZDJ_DECK_DJ_H

#include <stdbool.h>
#include <pthread.h>

// #include <zerodj/controls/zdj_controls.h>
// #include <zerodj/library/zdj_library.h>
#include <zerodj/signal/deck/zdj_deck.h>
// #include <zerodj/signal/pipeline/zdj_pipeline.h>


typedef enum {
    ZDJ_DECK_TSM_SOURCE_PITCH,
    ZDJ_DECK_TSM_SOURCE_TEMPO,
} zdj_dj_deck_tsm_source_t;

typedef struct {
    zdj_library_song_t * song;
    
    // Internal audio pipeline
    zdj_pipeline_node_t * dsp_node;
    zdj_pipeline_node_t * tsm_pitch_node;
    zdj_pipeline_node_t * tsm_tempo_node;
    zdj_pipeline_node_t * decode_node;

    // TSM engine management
    bool tempo_tsm_enabled;
    zdj_dj_deck_tsm_source_t tsm_source;
    
    // Thread management
    sem_t start_cycle;
    bool thread_ready;
    bool exit_thread;

    // Sync
    double set_bpm;

    // Internal non-threadsafe loop/skip API
    void ( *handle_new_loop_req )( struct zdj_deck_t * );
    void ( *handle_disable_loop_req )( struct zdj_deck_t * );
    void ( *handle_move_loop_req )( struct zdj_deck_t *, int64_t, bool );
    void ( *handle_resize_loop_req )( struct zdj_deck_t *, int64_t, bool );
    void ( *stage_skip_req )( struct zdj_deck_t * );
    void ( *update_skip_req )( struct zdj_deck_t * );
} zdj_dj_deck_state_t;

zdj_error_type_t zdj_new_dj_deck( zdj_deck_t * deck, void * resource );
void zdj_deck_dj_init_soundcard( zdj_deck_t * deck );
void zdj_deck_dj_init_controls( zdj_deck_t * deck );
void zdj_deck_dj_init_transport( zdj_deck_t * deck );
void zdj_deck_dj_init_loop_skip( zdj_deck_t * deck );
void zdj_deck_dj_init_sync( zdj_deck_t * deck );

// void zdj_dj_deck_handle_controls( zdj_deck_t * deck, zdj_control_event_t * event );
void zdj_dj_deck_reset_platter( zdj_deck_platter_t * platter, double addr );
// void zdj_dj_deck_set_sync_bpm( zdj_deck_t * deck, double offset );
// void zdj_dj_deck_offset_sync_bpm( zdj_deck_t * deck, double offset );

// void zdj_deck_new_loop_req( zdj_deck_t * deck, int64_t len, bool quant );
// void zdj_deck_enable_loop_req( zdj_deck_t * deck, zdj_library_cuepoint_t * cuepoint );
// void zdj_deck_disable_loop_req( zdj_deck_t * deck );
// void zdj_deck_move_loop( zdj_deck_t * deck, int64_t distance, bool quant );
// void zdj_deck_resize_loop( zdj_deck_t * deck, int64_t offset, bool quant );

// void zdj_deck_new_loop( zdj_deck_t * deck );
// void zdj_deck_enable_loop( zdj_deck_t * deck, zdj_library_cuepoint_t * cuepoint );
// void zdj_deck_disable_loop( zdj_deck_t * deck );
// void zdj_deck_move_loop( zdj_deck_t * deck, int64_t distance, bool quant );
// void zdj_deck_resize_loop( zdj_deck_t * deck, int64_t offset, bool quant );

// void zdj_deck_new_skip_req( zdj_deck_t * deck, double units );
// void zdj_deck_stage_skip( zdj_deck_t * deck );
// void zdj_deck_update_skip( zdj_deck_t * deck );
// void zdj_deck_new_skip( zdj_deck_t * deck );

#endif
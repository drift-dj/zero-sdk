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

#include <zerodj/signal/deck/zdj_deck.h>

typedef enum {
    ZDJ_DECK_TSM_SOURCE_PITCH,
    ZDJ_DECK_TSM_SOURCE_TEMPO,
} zdj_dj_deck_tsm_source_t;

// Signal from the platter to the deck that a transition
// in TSM mode needs to happen - sync new TSM node's head.
typedef enum {
    ZDJ_DECK_TSM_TX_NONE,
    ZDJ_DECK_TSM_TX_TO_TEMPO,
    ZDJ_DECK_TSM_TX_TO_PITCH
} zdj_dj_deck_tsm_tx_req_t;

typedef struct {
    zdj_library_song_t * song;
    
    // Internal audio pipeline
    zdj_pipeline_node_t * tsm_pitch_node;
    zdj_pipeline_node_t * tsm_tempo_node;
    zdj_pipeline_node_t * decode_node;
    int decode_win_buf_count;

    // TSM engine management
    bool slo_coder;
    bool tempo_tsm_enabled;
    zdj_dj_deck_tsm_source_t tsm_source;
    zdj_dj_deck_tsm_tx_req_t tsm_tx_req;
    
    // Thread management
    sem_t start_cycle;
    bool thread_ready;
    bool exit_thread;

    // Sync
    double set_bpm;
    int sync_mult_ui_counter;
} zdj_dj_deck_state_t;

zdj_error_type_t zdj_new_dj_deck( zdj_deck_t * deck, void * resource, int win_buf_count );
void zdj_deck_dj_init_soundcard( zdj_deck_t * deck );
void zdj_deck_dj_init_controls( zdj_deck_t * deck );
void zdj_deck_dj_init_platter( zdj_deck_t * deck );
void zdj_deck_dj_init_command( zdj_deck_t * deck );
void zdj_deck_dj_init_sync( zdj_deck_t * deck );

void zdj_dj_deck_reset_tsm_nodes( zdj_deck_t * deck );

bool zdj_dj_deck_is_in_tempo_tsm_mode( zdj_deck_t * deck );
bool zdj_dj_deck_is_in_pitch_tsm_mode( zdj_deck_t * deck );

bool zdj_dj_deck_has_beatgrid( zdj_deck_t * deck );

bool zdj_dj_deck_command_request( zdj_deck_t * deck, zdj_deck_control_command_request_state_t req );
bool zdj_dj_deck_command_is_active( zdj_deck_t * deck );
bool zdj_dj_deck_quantize_commands( zdj_deck_t * deck );

void zdj_dj_deck_set_cuepoint( zdj_deck_t * deck );
void zdj_dj_deck_next_cuepoint( zdj_deck_t * deck );
void zdj_dj_deck_play_cuepoint( zdj_deck_t * deck );
void zdj_dj_deck_reset_to_cuepoint( zdj_deck_t * deck );

void zdj_dj_deck_hotcue( zdj_deck_t * deck, int num );

bool zdj_dj_deck_loop_is_enabled( zdj_deck_t * deck );
void zdj_dj_deck_new_loop( zdj_deck_t * deck );
void zdj_dj_deck_enable_loop( zdj_deck_t * deck, zdj_library_cuepoint_t * cuepoint );
void zdj_dj_deck_disable_loop( zdj_deck_t * deck );
void zdj_dj_deck_reset_to_loop_start( zdj_deck_t * deck );
void zdj_dj_deck_move_loop( zdj_deck_t * deck, double val );
void zdj_dj_deck_resize_loop( zdj_deck_t * deck, double req_len );

void zdj_dj_deck_skip( zdj_deck_t * deck, double val );
void zdj_dj_deck_change_skip_length( zdj_deck_t * deck, double val );
void zdj_dj_deck_set_skip_origin( zdj_deck_t * deck );
void zdj_dj_deck_skip_to_origin( zdj_deck_t * deck );
void zdj_dj_deck_skip_to_loop_start( zdj_deck_t * deck );

zdj_deck_control_platter_request_t * zdj_dj_deck_new_platter_request( zdj_deck_t * deck );
bool zdj_dj_deck_platter_can_play( zdj_deck_t * deck );
bool zdj_dj_deck_platter_is_playing( zdj_deck_t * deck );
bool zdj_dj_deck_platter_can_scrub( zdj_deck_t * deck, int dir );
bool zdj_dj_deck_platter_can_nudge( zdj_deck_t * deck );
bool zdj_dj_deck_platter_is_hyperscrubbing( zdj_deck_t * deck );

void zdj_dj_deck_platter_update_scrub_fade( zdj_deck_t * deck );
void zdj_dj_deck_platter_reset_antipop( zdj_deck_t * deck );
void zdj_dj_deck_platter_update_antipop( zdj_deck_t * deck );

void zdj_dj_deck_platter_start_motor( zdj_deck_t * deck, bool spin_up );
void zdj_dj_deck_platter_stop_motor( zdj_deck_t * deck, bool spin_down );

#endif
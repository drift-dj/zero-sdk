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

#include <zerodj/controls/zdj_controls.h>
#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>

typedef enum {
    ZDJ_DECK_TYPE_DJ,
    ZDJ_DECK_TYPE_LIB,
    ZDJ_DECK_TYPE_EXTERNAL,
    ZDJ_DECK_TYPE_CV,
    ZDJ_DECK_TYPE_LOOP,
    ZDJ_DECK_TYPE_PLUGIN,
    ZDJ_DECK_TYPE_TEST
} zdj_deck_type_t;

typedef enum {
    ZDJ_DECK_STATION_NONE,
    ZDJ_DECK_STATION_1,
    ZDJ_DECK_STATION_2,
    ZDJ_DECK_STATION_EXT
} zdj_deck_station_t;

typedef enum {
    ZDJ_DECK_STATUS_NEW, // Just created
    ZDJ_DECK_STATUS_MAKE_PIPELINE, // Allocate mem, setup pipeline
    ZDJ_DECK_STATUS_WAIT_THREAD_AVAIL, // Wait for existing station thread to exit
    ZDJ_DECK_STATUS_WAIT_THREAD_READY, // Wait for new station thread to get running
    ZDJ_DECK_STATUS_RUNNING, // Active
    ZDJ_DECK_STATUS_STOP_TRANSPORT, // Ignore control events and spooldown deck drive/transport
    ZDJ_DECK_STATUS_WAIT_SPOOLDOWN, // Wait for transport to fully stop
    ZDJ_DECK_STATUS_IDLE // Deck manager can delete us
} zdj_deck_status_t;

// Loops crossfade smoothly from start addr to end addr.
// This crossfade can span multiple buffers/update cycles.
// While the crossfade is active, updates to start/end address
// won't take effect until crossfade is over.
// typedef enum {
//     ZDJ_DECK_LOOP_PHASE_INACTIVE,
//     ZDJ_DECK_LOOP_PHASE_CAPTURE,
//     ZDJ_DECK_LOOP_PHASE_RUN,
//     ZDJ_DECK_LOOP_PHASE_CROSSFADING,
//     ZDJ_DECK_LOOP_PHASE_EXIT
// } zdj_deck_control_loop_phase_t;

typedef struct {
    // zdj_deck_control_loop_phase_t phase;
    bool is_enabled;
    int64_t start_pcm_addr;
    int64_t end_pcm_addr;
    int fade_len;
} zdj_deck_control_loop_state_t;

// Beat skip can't be updated while actively crossfading.
// typedef enum {
//     ZDJ_DECK_SKIP_PHASE_INACTIVE, // Idle - do nothing
//     ZDJ_DECK_SKIP_PHASE_STAGED, // Address has been determined, crossfade has not yet started
//     ZDJ_DECK_SKIP_PHASE_CROSSFADING, // Crossfade is running
// } zdj_deck_control_skip_phase_t;

typedef struct {
    // zdj_deck_control_skip_phase_t phase;
    int64_t depart_pcm_addr;
    int64_t arrive_pcm_addr;
    int fade_len;
} zdj_deck_control_skip_state_t;

typedef enum {
    ZDJ_PLATTER_MOTOR_IDLE,
    ZDJ_PLATTER_MOTOR_TO_RUN,
    ZDJ_PLATTER_MOTOR_RUN,
    ZDJ_PLATTER_MOTOR_TO_IDLE,
} zdj_deck_platter_motor_state_t;

typedef struct {
    double head; // Montotonic location of the direct-drive platter.
    // pitch_setting - The physical position of the pitch slider (or value set by beatgrid sync)
    // Stays same whether playback is engaged or not.
    double pitch_setting; 
    double set_rate; // Target speed at which the deck motor should be running.
    double instant_rate; // set_rate, but filtered by simulation params.

    double ramp_rate; // Speed at which instant_rate becomes set_rate.
    zdj_deck_platter_motor_state_t state;
} zdj_deck_platter_motor_t;

typedef enum {
    ZDJ_PLATTER_SLIP_SCRATCH,
    ZDJ_PLATTER_SLIP_NUDGE,
    ZDJ_PLATTER_SLIP_TO_LAM,
    ZDJ_PLATTER_SLIP_LAMINAR,
    ZDJ_PLATTER_SLIP_TO_SCRATCH
} zdj_deck_platter_slip_state_t;

typedef struct {
    // Damped spring sim values
    double mass;
    double spring_k;
    double damp_c;
    double displacement;
    double velocity;
    double sim_duration; // Number of cycles after which sim should stabilize
    double sim_counter;

    // Sim output values
    double offset; 
    double set_val;
    double instant_val;
    int slip_dwell; // Hysteresis - how long after input does platter remain in slip state
    zdj_deck_platter_slip_state_t state;
} zdj_deck_platter_slip_t;


typedef struct {
    double head; // Output in decode-space of platter simulation
} zdj_deck_platter_needle_t;

typedef struct {
    zdj_deck_platter_motor_t motor;
    zdj_deck_platter_slip_t slip;
    zdj_deck_platter_needle_t needle;
    bool nudge_override;
    double nudge_coeff;
    double scratch_coeff;
} zdj_deck_platter_t;

typedef struct zdj_deck_control_state_t { 
    zdj_deck_platter_t platter;
    zdj_deck_control_skip_state_t skip_state;
    zdj_deck_control_loop_state_t loop_state;
    uint8_t control_change_flags[ ZDJ_CONTROL_ID_COUNT ];
} zdj_deck_control_state_t;

typedef struct zdj_deck_t {
    zdj_deck_type_t type;
    zdj_deck_station_t station;
    zdj_deck_status_t status;
    bool safe_to_deinit; // playback is stopped - okay to run teardown.

    // update_state is called from the slow deck_manager thread.
    void ( *update_state )( struct zdj_deck_t * );
    void ( *deinit )( struct zdj_deck_t * );

    // get_edge_data is called from the soundcard's fast cycle thread.
    void ( *get_edge_data )( void *, zdj_pipeline_node_t *, bool );

    // update_controls is called from the Controls thread ~900 kHz.
    void ( *handle_control_event )( struct zdj_deck_t *, zdj_control_event_t * );
    void ( *update_controls )( struct zdj_deck_t * );
    zdj_deck_control_state_t controls;

    // internal state
    void * state;

    // linkage for the controls thread
    struct zdj_deck_t * next;
    struct zdj_deck_t * prev;
} zdj_deck_t;

typedef struct {
    zdj_library_song_t * song;
    // Internal audio pipeline
    zdj_pipeline_node_t * dsp_node;
    zdj_pipeline_node_t * tsm_node;
    zdj_pipeline_node_t * decode_node;
} zdj_cv_deck_state_t;

typedef struct {
    int id;
} zdj_extern_deck_state_t;

typedef struct {
    int id;
} zdj_loop_deck_state_t;

typedef struct {
    int id;
} zdj_plugin_deck_state_t;

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

zdj_deck_t * zdj_new_deck( zdj_deck_type_t type, zdj_deck_station_t station, void * resource );

zdj_error_type_t zdj_new_lib_deck( zdj_deck_t * deck, void * resource );
zdj_error_type_t zdj_new_dj_deck( zdj_deck_t * deck, void * resource );
zdj_error_type_t zdj_new_cv_deck( zdj_deck_t * deck, void * resource );
zdj_error_type_t zdj_new_extern_deck( zdj_deck_t * deck );
zdj_error_type_t zdj_new_test_deck( zdj_deck_t * deck );
zdj_error_type_t zdj_new_live_loop_deck( zdj_deck_t * deck );
zdj_error_type_t zdj_new_plugin_deck( zdj_deck_t * deck, void * resource );

void zdj_deck_init_controls( zdj_deck_t * deck );

void zdj_clear_deck_control_flags( zdj_deck_t * deck );
void zdj_deck_handle_control( zdj_deck_t * deck, zdj_control_event_t * event );
void zdj_deck_update_controls ( zdj_deck_t * deck );

int64_t zdj_deck_get_pcm_addr_for_decode_addr( zdj_deck_t * deck, int64_t decode_addr );

void zdj_deck_new_loop( zdj_deck_t * deck, int64_t len, bool quant );
void zdj_deck_enable_loop( zdj_deck_t * deck, zdj_library_cuepoint_t * cuepoint );
void zdj_deck_disable_loop( zdj_deck_t * deck );
void zdj_deck_move_loop( zdj_deck_t * deck, int64_t distance, bool quant );
void zdj_deck_resize_loop( zdj_deck_t * deck, int64_t offset, bool quant );

void zdj_deck_new_skip( zdj_deck_t * deck, int64_t distance, bool quant );

#endif
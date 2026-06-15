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
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>

typedef enum {
    ZDJ_DECK_TYPE_DJ,
    ZDJ_DECK_TYPE_LIB,
    ZDJ_DECK_TYPE_EXTERNAL,
    ZDJ_DECK_TYPE_XPORT,
    ZDJ_DECK_TYPE_CV,
    ZDJ_DECK_TYPE_LOOP,
    ZDJ_DECK_TYPE_PLUGIN,
    ZDJ_DECK_TYPE_TEST
} zdj_deck_type_t;

typedef enum {
    ZDJ_DECK_STATION_NONE,
    ZDJ_DECK_STATION_1,
    ZDJ_DECK_STATION_2,
    ZDJ_DECK_STATION_EXT,
    ZDJ_DECK_STATION_XPORT
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
typedef enum {
    ZDJ_DECK_LOOP_PHASE_INACTIVE,
    ZDJ_DECK_LOOP_PHASE_ACTIVATE,
    ZDJ_DECK_LOOP_PHASE_MOVE,
    ZDJ_DECK_LOOP_PHASE_RESIZE,
    ZDJ_DECK_LOOP_PHASE_CAPTURE,
    ZDJ_DECK_LOOP_PHASE_RUN,
    ZDJ_DECK_LOOP_PHASE_CROSSFADING,
    ZDJ_DECK_LOOP_PHASE_DEACTIVATE,
    ZDJ_DECK_LOOP_PHASE_EXIT
} zdj_deck_control_loop_phase_t;

typedef struct {
    zdj_deck_control_loop_phase_t phase;
    bool is_enabled;
    double pcm_len;
    double beatgrid_len;
    double start_origin_d;
    double end_origin_d;
    double move_req_len;
    double length_change_req_len;
    int fade_len;
} zdj_deck_control_loop_state_t;

// typedef enum {
//     ZDJ_DECK_HOTCUE_PHASE_INACTIVE, // Idle - do nothing
//     ZDJ_DECK_HOTCUE_PHASE_ACTIVATE, // Request received from control thread
//     ZDJ_DECK_HOTCUE_PHASE_STAGED, // Address has been determined, skip has not been requested
//     ZDJ_DECK_HOTCUE_PHASE_RUN // Skip has been requested
// } zdj_deck_control_hotcue_phase_t;

// typedef struct {
//     zdj_deck_control_hotcue_phase_t phase;
//     double current_target_d;
// } zdj_deck_control_hotcue_state_t;

typedef struct {
    double current_offset;
    double skip_req_len;
    double dest_origin_d;
    bool locked;
} zdj_deck_control_skip_state_t;

typedef enum {
    ZDJ_PLATTER_MOTOR_IDLE,
    ZDJ_PLATTER_MOTOR_SPIN_UP,
    ZDJ_PLATTER_MOTOR_RUN,
    ZDJ_PLATTER_MOTOR_SPIN_DOWN,
} zdj_deck_platter_motor_state_t;

typedef struct {
    // pitch_setting - The physical position of the pitch slider (or value set by beatgrid sync)
    // Stays same whether playback is engaged or not.
    double pitch_setting; 
    bool enabled; // If motor is powered or not.
    double set_rate; // Target speed at which the deck motor should be running.
    double instant_rate; // set_rate, but filtered by simulation params.
    double instant_val; // number of samples to advance

    int cur_spin_up_cycle;
    int spin_up_cycle_count; // Number of sound card buf cycles to finish motor spin up
    int cur_spin_down_cycle;
    int spin_down_cycle_count; // Number of sound card buf cycles to finish motor spin down
    zdj_deck_platter_motor_state_t state;
} zdj_deck_platter_motor_t;

typedef enum {
    ZDJ_PLATTER_MODE_SLIP,
    ZDJ_PLATTER_MODE_LAMINAR
} zdj_deck_platter_slip_state_t;
typedef enum {
    ZDJ_PLATTER_MODE_TX_NONE,
    ZDJ_PLATTER_MODE_TX_TO_SLIP,
    ZDJ_PLATTER_MODE_TX_TO_LAMINAR
} zdj_deck_platter_slip_tx_flag_t;

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
    double tempo_nudge_rate; // bypass the slipmat sim in tempo tsm mode
    int slip_dwell; // Hysteresis - how long after input does platter remain in slip state
    zdj_deck_platter_slip_state_t state;
} zdj_deck_platter_slip_t;

typedef enum {
    ZDJ_DECK_ANTIPOP_STATE_INACTIVE,
    ZDJ_DECK_ANTIPOP_STATE_TO_ZERO,
    ZDJ_DECK_ANTIPOP_STATE_AT_ZERO,
    ZDJ_DECK_ANTIPOP_STATE_FROM_ZERO,
    ZDJ_DECK_ANTIPOP_STATE_DEACTIVATE,
} zdj_deck_platter_antipop_state_t;

typedef struct {
    zdj_deck_platter_antipop_state_t state;
    bool enabled;
    double tracking_val;
    float lowpass_val;
    float slew_limit;
    float start_fade_val;
    float end_fade_val;
    float engage_thresh; // slip.instant val below which AC coupling is active
} zdj_deck_platter_antipop_t;

typedef struct {
    float start_fade_val;
    float end_fade_val;
    float fade_start_rate; // slip.instant_val below which fade coeff = 1.0
    float fade_complete_rate; // slip.instant_val above which fade coeff = 0.0
} zdj_deck_platter_scrub_fade_t;

typedef enum {
    ZDJ_DECK_HYPERSCRUB_INACTIVE,
    ZDJ_DECK_HYPERSCRUB_ACTIVATE,
    ZDJ_DECK_HYPERSCRUB_ACTIVE,
    ZDJ_DECK_HYPERSCRUB_DEACTIVATE
} zdj_deck_platter_hyperscrub_state_t;

typedef struct {
    zdj_deck_platter_hyperscrub_state_t state;
    float exit_thresh;
    float reentry_thresh;
    float ui_rate; // used by playback ui to zoom out while scrubbing
} zdj_deck_platter_hyperscrub_t;

typedef struct {
    double slip_input_samps; // set by control input - used to clamp max scrub rate
    double slip_input_head; // set by control input - slip+motor sim chases this while slipping
    double internal_instant_head; // internal tracking of slip+motor sim
    double internal_instant_head_prev; // internal tracking of slip+motor sim
    double head_move_samps;
    double head_move_rate;
    zdj_deck_platter_motor_t motor;
    zdj_deck_platter_slip_t slip;
    zdj_deck_platter_antipop_t antipop;
    zdj_deck_platter_hyperscrub_t hyperscrub;
    zdj_deck_platter_scrub_fade_t scrub_fade;
    bool scratch_override;
    double nudge_coeff;
    double scratch_coeff;
    float hyperscrub_coeff;
} zdj_deck_platter_t;

typedef struct {
    double trim;
    double cue_trim;
    bool cue_mute;
    double eq_lo;
    double eq_mid;
    double eq_hi;
    double fade;
    double xfade;
    double pan;
} zdj_deck_control_knob_state_t;


typedef struct {
    bool reset_pending;
    bool hotcue_pending;
    bool is_cueing;
    double dest_origin_d;
} zdj_deck_control_cue_state_t;

typedef struct zdj_deck_control_state_t { 
    zdj_deck_platter_t platter;
    zdj_deck_control_cue_state_t cue_state;
    zdj_deck_control_skip_state_t skip_state;
    zdj_deck_control_loop_state_t loop_state;
    // zdj_deck_control_needledrop_state_t needledrop_state;
    zdj_deck_control_knob_state_t knob_state;
    bool discon_quantize;
    double discon_quantize_val;
    // int c_count_q_val;
} zdj_deck_control_state_t;

typedef enum {
    ZDJ_DECK_PLATTER_REQUEST_START_MOTOR,
    ZDJ_DECK_PLATTER_REQUEST_STOP_MOTOR,
    ZDJ_DECK_PLATTER_REQUEST_TOGGLE_MOTOR,
    ZDJ_DECK_PLATTER_REQUEST_SCRUB,
    ZDJ_DECK_PLATTER_REQUEST_SCRUB_ALT_0,
    ZDJ_DECK_PLATTER_REQUEST_SCRUB_ALT_1
} zdj_deck_control_platter_request_type_t;

typedef enum {
    ZDJ_DECK_PLATTER_REQUEST_PHASE_NONE,
    ZDJ_DECK_PLATTER_REQUEST_PHASE_NEW,
    ZDJ_DECK_PLATTER_REQUEST_PHASE_RECEIVED
} zdj_deck_control_platter_request_phase_t;

typedef struct {
    zdj_deck_control_platter_request_type_t type;
    zdj_deck_control_platter_request_phase_t phase;
    bool spin_up;
    bool spin_down;
    int event_i_val;
    double target_coord;
    zdj_decode_addr_coord_t coord;
} zdj_deck_control_platter_request_t;

typedef enum {
    ZDJ_DECK_COMMAND_REQUEST_NONE,
    ZDJ_DECK_COMMAND_REQUEST_TOGGLE_LOOP,
    ZDJ_DECK_COMMAND_REQUEST_MOVE_LOOP,
    ZDJ_DECK_COMMAND_REQUEST_RESIZE_LOOP,
    ZDJ_DECK_COMMAND_REQUEST_JUMP_TO_LOOP_START,
    ZDJ_DECK_COMMAND_REQUEST_SKIP,
    ZDJ_DECK_COMMAND_REQUEST_CHANGE_SKIP_LENGTH,
    ZDJ_DECK_COMMAND_REQUEST_SET_SKIP_ORIGIN,
    ZDJ_DECK_COMMAND_REQUEST_JUMP_TO_SKIP_ORIGIN,
    ZDJ_DECK_COMMAND_REQUEST_SET_CUEPOINT,
    ZDJ_DECK_COMMAND_REQUEST_NEXT_CUEPOINT,
    ZDJ_DECK_COMMAND_REQUEST_CUE_START,
    ZDJ_DECK_COMMAND_REQUEST_CUE_END,
    ZDJ_DECK_COMMAND_REQUEST_HOTCUE,
    ZDJ_DECK_COMMAND_REQUEST_PROCESSING,
    ZDJ_DECK_COMMAND_REQUEST_RECEIVED,
    ZDJ_DECK_COMMAND_REQUEST_COUNT
} zdj_deck_control_command_request_state_t;

typedef struct {
    zdj_deck_control_command_request_state_t state;
    int event_i_val;
    int hotcue_num;
    double d_val;
    int req_sens[ ZDJ_DECK_COMMAND_REQUEST_COUNT ]; // sensitivity settings for req inputs
    int req_sens_count[ ZDJ_DECK_COMMAND_REQUEST_COUNT ]; // sensitivity tracking for req inputs
} zdj_deck_control_command_request_t;

typedef struct zdj_deck_t {
    zdj_deck_type_t type;
    zdj_deck_station_t station;
    zdj_deck_status_t status;
    bool safe_to_deinit; // playback is stopped - okay to run teardown.

    // Deck Manager API
    void ( *update_state )( struct zdj_deck_t * ); // Deck Mgr. thread @ 4Hz
    void ( *begin_teardown )( struct zdj_deck_t * );
    void ( *deinit )( struct zdj_deck_t * );

    // UI API
    void ( *ui_load_cb )( struct zdj_deck_t * );
    void ( *ui_unload_cb )( struct zdj_deck_t * );

    // Soundcard API
    // get/push_edge_data are called from the soundcard's fast cycle thread.
    void ( *get_edge_data )( void *, zdj_pipeline_node_t *, bool );
    void ( *push_edge_data )( void *, zdj_pipeline_node_t *, bool );

    // Transport Control API
    zdj_deck_control_state_t controls;
    void ( *handle_control_event )( struct zdj_deck_t *, zdj_control_event_t * ); // Control thread @800Hz

    // Platter API
    zdj_deck_control_platter_request_t platter_reqs[ 8 ]; // Handle up to 8 reqs in a cycle
    int platter_req_write_index;
    void ( *update_platter_req )( struct zdj_deck_t * ); // Control thread @800Hz
    void ( *update_platter_model )( struct zdj_deck_t * ); // Soundcard thread @buffer rate   
    
    // DEPRECATED
    void ( *update_transport_inputs )( struct zdj_deck_t * deck ); 
    void ( *update_transport_outputs )( struct zdj_deck_t * deck ); 
    // DEPRECATED

    // Command API
    zdj_deck_control_command_request_t command_req;
    void ( *update_command_req )( struct zdj_deck_t * ); // Control thread @800Hz
    bool ( *update_command_models )( struct zdj_deck_t * ); // Soundcard thread @buffer rate

    // Sync API
    bool can_sync;
    double sync_factor;
    void ( *set_sync_bpm )( struct zdj_deck_t *, double );
    void ( *offset_sync_bpm )( struct zdj_deck_t *, double );
    void ( *offset_pitch_setting )( struct zdj_deck_t *, double );
    void ( *request_sync_mult )( struct zdj_deck_t *, float );

    // internal state
    void * state;

    int predelay_counter;

    // linkage for the controls thread
    struct zdj_deck_t * next;
    struct zdj_deck_t * prev;
} zdj_deck_t;

// typedef struct {
//     zdj_library_song_t * song;
//     // Internal audio pipeline
//     zdj_pipeline_node_t * dsp_node;
//     zdj_pipeline_node_t * tsm_node;
//     zdj_pipeline_node_t * decode_node;
// } zdj_cv_deck_state_t;

// typedef struct {
//     zdj_pipeline_node_t * dsp_node;

//     // Thread management
//     sem_t start_cycle;
//     bool thread_ready;
//     bool exit_thread;
// } zdj_ext_deck_state_t;

typedef struct {
    int id;
} zdj_loop_deck_state_t;

// typedef struct {
//     float s1_p;
//     float s1_f;
//     float s2_p;
//     float s2_f;
//     float s3_p;
//     float s3_f;
//     float s4_p;
//     float s4_f;
// } zdj_test_deck_state_t;

zdj_deck_t * zdj_new_deck( 
    zdj_deck_type_t type, 
    zdj_deck_station_t station, 
    void * resource, 
    int win_buf_count 
);

#endif
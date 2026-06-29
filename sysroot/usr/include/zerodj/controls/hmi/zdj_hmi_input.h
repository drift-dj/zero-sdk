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

#ifndef ZDJ_HMI_INPUT_H
#define ZDJ_HMI_INPUT_H

#include <stdbool.h>
#include <zerodj/controls/hmi/zdj_hmi_m7_state_model.h>

typedef enum {
    ZDJ_HMI_ENCO_1_VOL,
    ZDJ_HMI_ENCO_2_JOG,
    ZDJ_HMI_ENCO_3_TONE_1,
    ZDJ_HMI_ENCO_4_TONE_2,
    ZDJ_HMI_ENCO_5_TONE_3,
    ZDJ_HMI_PB_0_HOTCUE,
    ZDJ_HMI_PB_1_PLAY,
    ZDJ_HMI_PB_2_FN_1,
    ZDJ_HMI_PB_3_FN_2,
    ZDJ_HMI_PB_4_FN_3,
    ZDJ_HMI_PB_5_NAV,
    ZDJ_HMI_POT_0_CH_1,
    ZDJ_HMI_POT_1_CH_2,
    ZDJ_HMI_POT_2_XFADE,
    ZDJ_HMI_CONTROL_ID_COUNT
} zdj_hmi_input_id_t;

static char * zdj_hmi_input_name[ ZDJ_HMI_CONTROL_ID_COUNT ] = {
    "Volume",// ZDJ_HMI_ENCO_1_VOL,
    "Jog",// ZDJ_HMI_ENCO_2_JOG,
    "Tone 1",// ZDJ_HMI_ENCO_3_TONE_1,
    "Tone 2",// ZDJ_HMI_ENCO_4_TONE_2,
    "Tone 3",// ZDJ_HMI_ENCO_5_TONE_3,
    "Hotcue",// ZDJ_HMI_PB_0_HOTCUE,
    "Play",// ZDJ_HMI_PB_1_PLAY,
    "F(x) 1",// ZDJ_HMI_PB_2_FN_1,
    "F(x) 2",// ZDJ_HMI_PB_3_FN_2,
    "F(x) 3",// ZDJ_HMI_PB_4_FN_3,
    "Nav",// ZDJ_HMI_PB_5_NAV,
    "Fade 1",// ZDJ_HMI_POT_0_CH_1,
    "Fade 2",// ZDJ_HMI_POT_1_CH_2,
    "XFade"// ZDJ_HMI_POT_2_XFADE,
};

typedef enum {
    ZDJ_HMI_EVENT_PRESS,
    ZDJ_HMI_EVENT_MOD_PRESS,
    ZDJ_HMI_EVENT_RELEASE,
    ZDJ_HMI_EVENT_MOD_RELEASE,
    ZDJ_HMI_EVENT_LONG_PRESS,
    ZDJ_HMI_EVENT_MOD_LONG_PRESS,
    ZDJ_HMI_EVENT_LONG_RELEASE,
    ZDJ_HMI_EVENT_MOD_LONG_RELEASE,
    ZDJ_HMI_EVENT_ADJUST,
    ZDJ_HMI_EVENT_MOD_ADJUST,
    ZDJ_HMI_EVENT_PRESS_ADJUST,
    ZDJ_HMI_EVENT_MOD_PRESS_ADJUST,
    ZDJ_HMI_EVENT_PRESS_ADJUST_RELEASE,
    ZDJ_HMI_EVENT_TYPE_COUNT
} zdj_hmi_input_event_type_t;

static char * zdj_hmi_input_event_name[ ZDJ_HMI_EVENT_TYPE_COUNT ] = {
    "Press",// ZDJ_HMI_EVENT_PRESS,
    "Mod Press",// ZDJ_HMI_EVENT_MOD_RELEASE,
    "Release",// ZDJ_HMI_EVENT_RELEASE,
    "Mod Release",// ZDJ_HMI_EVENT_MOD_RELEASE,
    "Long Press",// ZDJ_HMI_EVENT_LONG_PRESS,
    "Mod Long Press",// ZDJ_HMI_EVENT_MOD_LONG_PRESS
    "Long Release",// ZDJ_HMI_EVENT_LONG_RELEASE,
    "Mod Long Release",// ZDJ_HMI_EVENT_MOD_LONG_RELEASE
    "Adjust",// ZDJ_HMI_EVENT_ADJUST,
    "Mod Adjust",// ZDJ_HMI_EVENT_MOD_ADJUST
    "Press Adjust",// ZDJ_HMI_EVENT_PRESS_ADJUST,
    "Mod Press Adjust", // ZDJ_HMI_EVENT_MOD_PRESS_ADJUST
    "Press Adjust Release"// ZDJ_HMI_EVENT_PRESS_ADJUST_RELEASE
};

typedef struct {
    zdj_hmi_input_id_t id;
    zdj_hmi_input_event_type_t type;
    uint16_t mod_bitmap;
    bool blocked;
    int i_val;
    double d_val;
    bool b_val;
} zdj_hmi_input_event_t;

typedef enum {
    ZDJ_HMI_STATE_IDLE,
    ZDJ_HMI_STATE_PRESS,
    ZDJ_HMI_STATE_MOD_PRESS,
    ZDJ_HMI_STATE_UP,
    ZDJ_HMI_STATE_MOD_UP,
    ZDJ_HMI_STATE_PRESS_TURN,
    ZDJ_HMI_STATE_LONG_PRESS,
    ZDJ_HMI_STATE_MOD_LONG_PRESS,
    ZDJ_HMI_STATE_LONG_PRESS_UP,
    ZDJ_HMI_STATE_MOD_LONG_PRESS_UP,
    ZDJ_HMI_STATE_TURN,
    ZDJ_HMI_STATE_DEBOUNCE,
} zdj_hmi_input_state_type_t;

typedef struct {
    zdj_hmi_input_id_t id;
    zdj_hmi_input_state_type_t current_state;
    bool is_modifier;
    bool is_modified;
    bool press_emitted;
    bool long_press_emitted;
    uint16_t turn_press_timer;
    uint16_t turn_hyst_timer;
    int16_t adjust_hyst_val;
    uint16_t adjust_prev_val;
    uint16_t long_press_timer;
    uint16_t debounce_timer;
} zdj_hmi_input_state_t;

extern volatile zdj_hmi_m7_state_model_t * zdj_hmi_m7_state_model;
extern zdj_hmi_input_state_t ** zdj_hmi_input_states;

// Bitmaps used to build modifier key status for each control
extern uint16_t zdj_hmi_mod_bitmap;

void zdj_control_hmi_input_init( void );
void zdj_control_prepare_hmi_input_scan( void );
void zdj_control_scan_hmi_input( void );
void zdj_control_process_hmi_input( void );
void zdj_control_process_hmi_digital_input( 
    zdj_hmi_input_state_t * input_state, 
    int32_t enco_val, 
    int32_t pb_state 
);
void zdj_control_process_hmi_analog_input( zdj_hmi_input_state_t * input_state, int32_t pot_val );
void zdj_control_transform_hmi_events( void );

#define ZDJ_HMI_INPUT_EVENT_BUF_LEN 24
// Ring buffer for queueing events until input event transform consumes them
extern zdj_hmi_input_event_t zdj_hmi_input_event_buf[ ZDJ_HMI_INPUT_EVENT_BUF_LEN ];
int zdj_get_next_hmi_input_event_ind( void );
extern int zdj_hmi_input_event_buf_write;
extern int zdj_hmi_input_event_buf_read;

int zdj_hmi_input_get_fader_val( zdj_hmi_input_id_t id );


#endif
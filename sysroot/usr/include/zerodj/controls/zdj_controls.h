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

#ifndef ZDJ_CONTROLS_H
#define ZDJ_CONTROLS_H

#include <stdint.h>
#include <stdbool.h>

#include <zerodj/controls/hmi/zdj_hmi_input.h>
#include <zerodj/system/error/zdj_error.h>

typedef enum {
    /////////////////////////
    //  UI Control Events  //
    /////////////////////////
    // Jog Events
    ZDJ_UI_CONTROL_JOG_ADJUST_0, // Normal
    ZDJ_UI_CONTROL_JOG_ADJUST_1, // Press-Adjust
    ZDJ_UI_CONTROL_JOG_ADJUST_2, // Shift Adjust
    ZDJ_UI_CONTROL_JOG_ADJUST_3,
    ZDJ_UI_CONTROL_JOG_ADJUST_4,
    ZDJ_UI_CONTROL_JOG_ADJUST_5,
    ZDJ_UI_CONTROL_JOG_ADJUST_6,
    ZDJ_UI_CONTROL_JOG_ADJUST_7,
    ZDJ_UI_CONTROL_JOG_PRESS_0,
    ZDJ_UI_CONTROL_JOG_PRESS_1, // Long Press
    ZDJ_UI_CONTROL_JOG_PRESS_2,
    ZDJ_UI_CONTROL_JOG_RELEASE_0, // Normal
    ZDJ_UI_CONTROL_JOG_RELEASE_1, // Long Release
    ZDJ_UI_CONTROL_JOG_RELEASE_2, // Shift Release
    ZDJ_UI_CONTROL_JOG_RELEASE_3, // Press-Adjust Release

    // Vol/Out knob events
    ZDJ_UI_CONTROL_OUT_ADJUST_0,
    ZDJ_UI_CONTROL_OUT_ADJUST_1,
    ZDJ_UI_CONTROL_OUT_ADJUST_2,
    ZDJ_UI_CONTROL_OUT_PRESS_0,
    ZDJ_UI_CONTROL_OUT_PRESS_1,
    ZDJ_UI_CONTROL_OUT_PRESS_2,
    ZDJ_UI_CONTROL_OUT_RELEASE_0,
    ZDJ_UI_CONTROL_OUT_RELEASE_1,
    ZDJ_UI_CONTROL_OUT_RELEASE_2,

    // Tone 1 Knob Events
    ZDJ_UI_CONTROL_TONE_1_ADJUST_0,
    ZDJ_UI_CONTROL_TONE_1_ADJUST_1,
    ZDJ_UI_CONTROL_TONE_1_ADJUST_2,
    ZDJ_UI_CONTROL_TONE_1_PRESS_0,
    ZDJ_UI_CONTROL_TONE_1_PRESS_1,
    ZDJ_UI_CONTROL_TONE_1_PRESS_2,
    ZDJ_UI_CONTROL_TONE_1_RELEASE_0,
    ZDJ_UI_CONTROL_TONE_1_RELEASE_1,
    ZDJ_UI_CONTROL_TONE_1_RELEASE_2,

    // Tone 2 Knob Events
    ZDJ_UI_CONTROL_TONE_2_ADJUST_0,
    ZDJ_UI_CONTROL_TONE_2_ADJUST_1,
    ZDJ_UI_CONTROL_TONE_2_ADJUST_2,
    ZDJ_UI_CONTROL_TONE_2_PRESS_0,
    ZDJ_UI_CONTROL_TONE_2_PRESS_1,
    ZDJ_UI_CONTROL_TONE_2_PRESS_2,
    ZDJ_UI_CONTROL_TONE_2_RELEASE_0,
    ZDJ_UI_CONTROL_TONE_2_RELEASE_1,
    ZDJ_UI_CONTROL_TONE_2_RELEASE_2,

    // Tone 3 Knob Events
    ZDJ_UI_CONTROL_TONE_3_ADJUST_0,
    ZDJ_UI_CONTROL_TONE_3_ADJUST_1,
    ZDJ_UI_CONTROL_TONE_3_ADJUST_2,
    ZDJ_UI_CONTROL_TONE_3_PRESS_0,
    ZDJ_UI_CONTROL_TONE_3_PRESS_1,
    ZDJ_UI_CONTROL_TONE_3_PRESS_2,
    ZDJ_UI_CONTROL_TONE_3_RELEASE_0,
    ZDJ_UI_CONTROL_TONE_3_RELEASE_1,
    ZDJ_UI_CONTROL_TONE_3_RELEASE_2,
    
    // Play PB Events
    ZDJ_UI_CONTROL_PLAY_PRESS_0,
    ZDJ_UI_CONTROL_PLAY_PRESS_1,
    ZDJ_UI_CONTROL_PLAY_PRESS_2,
    ZDJ_UI_CONTROL_PLAY_PRESS_3,
    ZDJ_UI_CONTROL_PLAY_RELEASE_0,
    ZDJ_UI_CONTROL_PLAY_RELEASE_1,
    ZDJ_UI_CONTROL_PLAY_RELEASE_2,
    ZDJ_UI_CONTROL_PLAY_RELEASE_3,

    // Hotcue PB Events
    ZDJ_UI_CONTROL_HOTCUE_PRESS_0,
    ZDJ_UI_CONTROL_HOTCUE_PRESS_1,
    ZDJ_UI_CONTROL_HOTCUE_PRESS_2,
    ZDJ_UI_CONTROL_HOTCUE_PRESS_3,
    ZDJ_UI_CONTROL_HOTCUE_RELEASE_0,
    ZDJ_UI_CONTROL_HOTCUE_RELEASE_1,
    ZDJ_UI_CONTROL_HOTCUE_RELEASE_2,
    ZDJ_UI_CONTROL_HOTCUE_RELEASE_3,

    // Nav PB Events
    ZDJ_UI_CONTROL_NAV_PRESS_0,
    ZDJ_UI_CONTROL_NAV_PRESS_1,
    ZDJ_UI_CONTROL_NAV_PRESS_2,
    ZDJ_UI_CONTROL_NAV_PRESS_3,
    ZDJ_UI_CONTROL_NAV_RELEASE_0,
    ZDJ_UI_CONTROL_NAV_RELEASE_1,
    ZDJ_UI_CONTROL_NAV_RELEASE_2,
    ZDJ_UI_CONTROL_NAV_RELEASE_3,

    // Fn 1 PB Events
    ZDJ_UI_CONTROL_FN_1_PRESS_0,
    ZDJ_UI_CONTROL_FN_1_PRESS_1,
    ZDJ_UI_CONTROL_FN_1_PRESS_2,
    ZDJ_UI_CONTROL_FN_1_PRESS_3,
    ZDJ_UI_CONTROL_FN_1_RELEASE_0,
    ZDJ_UI_CONTROL_FN_1_RELEASE_1,
    ZDJ_UI_CONTROL_FN_1_RELEASE_2,
    ZDJ_UI_CONTROL_FN_1_RELEASE_3,

    // Fn 2 PB Events
    ZDJ_UI_CONTROL_FN_2_PRESS_0,
    ZDJ_UI_CONTROL_FN_2_PRESS_1,
    ZDJ_UI_CONTROL_FN_2_PRESS_2,
    ZDJ_UI_CONTROL_FN_2_PRESS_3,
    ZDJ_UI_CONTROL_FN_2_RELEASE_0,
    ZDJ_UI_CONTROL_FN_2_RELEASE_1,
    ZDJ_UI_CONTROL_FN_2_RELEASE_2,
    ZDJ_UI_CONTROL_FN_2_RELEASE_3,

    // Fn 3 PB Events
    ZDJ_UI_CONTROL_FN_3_PRESS_0,
    ZDJ_UI_CONTROL_FN_3_PRESS_1,
    ZDJ_UI_CONTROL_FN_3_PRESS_2,
    ZDJ_UI_CONTROL_FN_3_PRESS_3,
    ZDJ_UI_CONTROL_FN_3_RELEASE_0,
    ZDJ_UI_CONTROL_FN_3_RELEASE_1,
    ZDJ_UI_CONTROL_FN_3_RELEASE_2,
    ZDJ_UI_CONTROL_FN_3_RELEASE_3,

    // Fader Events
    ZDJ_UI_CONTROL_FADE_1_ADJUST_0,
    ZDJ_UI_CONTROL_FADE_1_ADJUST_1,
    ZDJ_UI_CONTROL_FADE_2_ADJUST_0,
    ZDJ_UI_CONTROL_FADE_2_ADJUST_1,
    ZDJ_UI_CONTROL_XFADE_ADJUST_0,
    ZDJ_UI_CONTROL_XFADE_ADJUST_1,



    ///////////////////////////
    //  Deck Control Events  //
    ///////////////////////////
    // Admin Controls
    ZDJ_DECK_CONTROL_LR_VOL,
    ZDJ_DECK_CONTROL_CUE_VOL,
    ZDJ_DECK_CONTROL_XFADE,

    // Deck 1
    ZDJ_DECK_1_CONTROL_FADE,
    ZDJ_DECK_1_CONTROL_TRIM,
    ZDJ_DECK_1_CONTROL_EQ_LO,
    ZDJ_DECK_1_CONTROL_EQ_MID,
    ZDJ_DECK_1_CONTROL_EQ_HI,
    ZDJ_DECK_1_CONTROL_PFL_TRIM,
    ZDJ_DECK_1_CONTROL_PFL_MUTE,
    ZDJ_DECK_1_CONTROL_FX_SELECT,
    ZDJ_DECK_1_CONTROL_FX_0,
    ZDJ_DECK_1_CONTROL_FX_1,
    ZDJ_DECK_1_CONTROL_FX_2,
    ZDJ_DECK_1_CONTROL_FX_3,
    ZDJ_DECK_1_CONTROL_FX_4,
    ZDJ_DECK_1_CONTROL_FX_5,
    ZDJ_DECK_1_CONTROL_SCRUB,
    ZDJ_DECK_1_CONTROL_NUDGE,
    ZDJ_DECK_1_CONTROL_TEMPO,
    ZDJ_DECK_1_CONTROL_TEMPO_FINE,
    ZDJ_DECK_1_CONTROL_PLAY_PAUSE,
    ZDJ_DECK_1_CONTROL_PAUSE,
    ZDJ_DECK_1_CONTROL_HOTCUE_START,
    ZDJ_DECK_1_CONTROL_HOTCUE_END,

    // Deck 2
    ZDJ_DECK_2_CONTROL_FADE,
    ZDJ_DECK_2_CONTROL_TRIM,
    ZDJ_DECK_2_CONTROL_EQ_LO,
    ZDJ_DECK_2_CONTROL_EQ_MID,
    ZDJ_DECK_2_CONTROL_EQ_HI,
    ZDJ_DECK_2_CONTROL_PFL_TRIM,
    ZDJ_DECK_2_CONTROL_PFL_MUTE,
    ZDJ_DECK_2_CONTROL_FX_SELECT,
    ZDJ_DECK_2_CONTROL_FX_0,
    ZDJ_DECK_2_CONTROL_FX_1,
    ZDJ_DECK_2_CONTROL_FX_2,
    ZDJ_DECK_2_CONTROL_FX_3,
    ZDJ_DECK_2_CONTROL_FX_4,
    ZDJ_DECK_2_CONTROL_FX_5,
    ZDJ_DECK_2_CONTROL_SCRUB,
    ZDJ_DECK_2_CONTROL_NUDGE,
    ZDJ_DECK_2_CONTROL_TEMPO,
    ZDJ_DECK_2_CONTROL_TEMPO_FINE,
    ZDJ_DECK_2_CONTROL_PLAY_PAUSE,
    ZDJ_DECK_2_CONTROL_PAUSE,
    ZDJ_DECK_2_CONTROL_HOTCUE_START,
    ZDJ_DECK_2_CONTROL_HOTCUE_END,

    // External Deck
    ZDJ_DECK_EXT_CONTROL_TRIM,
    ZDJ_DECK_EXT_CONTROL_EQ_LO,
    ZDJ_DECK_EXT_CONTROL_EQ_MID,
    ZDJ_DECK_EXT_CONTROL_EQ_HI,
    ZDJ_DECK_EXT_CONTROL_PFL_TRIM,
    ZDJ_DECK_EXT_CONTROL_PFL_MUTE,
    ZDJ_DECK_EXT_CONTROL_FX_SELECT,
    ZDJ_DECK_EXT_CONTROL_FX_0,
    ZDJ_DECK_EXT_CONTROL_FX_1,
    ZDJ_DECK_EXT_CONTROL_FX_2,
    ZDJ_DECK_EXT_CONTROL_FX_3,
    ZDJ_DECK_EXT_CONTROL_FX_4,
    ZDJ_DECK_EXT_CONTROL_FX_5,
    ZDJ_DECK_EXT_CONTROL_NUDGE,
    ZDJ_DECK_EXT_CONTROL_TEMPO,
    ZDJ_DECK_EXT_CONTROL_TEMPO_FINE,
    ZDJ_DECK_EXT_CONTROL_PLAY_PAUSE,
    ZDJ_DECK_EXT_CONTROL_PAUSE,
    ZDJ_DECK_EXT_CONTROL_HOTCUE_START,
    ZDJ_DECK_EXT_CONTROL_HOTCUE_STOP,

    ZDJ_CONTROL_ID_COUNT
} zdj_control_id_t;

typedef struct {
    bool controls[ ZDJ_CONTROL_ID_COUNT ];
} zdj_control_active_state_t;

typedef struct {
    zdj_control_id_t id;
    bool blocked;
    int i_val;
    double d_val;
    bool b_val;
} zdj_control_event_t;

void * zdj_control_cycle_thread_main( void * arg );

extern zdj_control_active_state_t zdj_control_active_state;

#define ZDJ_CONTROL_EVENT_BUF_LEN 24
// Ring buffer for queueing events until audio buf cycle consumes them
extern zdj_control_event_t zdj_deck_event_buf[ ZDJ_CONTROL_EVENT_BUF_LEN ];
int zdj_get_next_deck_event_ind( void );
extern volatile int zdj_deck_event_buf_write;
extern volatile int zdj_deck_event_buf_read;

// Ring buffer for queueing events until ui cycle consumes them
extern zdj_control_event_t zdj_ui_event_buf[ ZDJ_CONTROL_EVENT_BUF_LEN ];
int zdj_get_next_ui_event_ind( void );
extern volatile int zdj_ui_event_buf_write;
extern volatile int zdj_ui_event_buf_read;

zdj_error_type_t zdj_controls_init( void );
zdj_error_type_t zdj_clear_controls( void );

zdj_error_type_t zdj_activate_control( zdj_control_id_t control_id );
zdj_error_type_t zdj_deactivate_control( zdj_control_id_t control_id );

// Mapping System
bool zdj_control_map_hmi_input_event( zdj_hmi_input_event_t * in_e, zdj_control_event_t * c_e );
bool zdj_control_event_is_ui_control( zdj_control_event_t * event );
bool zdj_control_event_is_deck_control( zdj_control_event_t * event );

#endif

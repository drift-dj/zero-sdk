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
    ZDJ_UI_CONTROL_NONE,
    // Jog Events
    ZDJ_UI_CONTROL_JOG_ADJUST_0, // Normal
    ZDJ_UI_CONTROL_JOG_ADJUST_1, // Press-Adjust
    ZDJ_UI_CONTROL_JOG_ADJUST_2, // Shift Adjust
    ZDJ_UI_CONTROL_JOG_ADJUST_3,
    ZDJ_UI_CONTROL_JOG_ADJUST_4,
    ZDJ_UI_CONTROL_JOG_ADJUST_5,
    ZDJ_UI_CONTROL_JOG_ADJUST_6,
    ZDJ_UI_CONTROL_JOG_ADJUST_7,
    ZDJ_UI_CONTROL_JOG_PRESS_0, // Press
    ZDJ_UI_CONTROL_JOG_PRESS_1, // Shift Press
    ZDJ_UI_CONTROL_JOG_PRESS_2, // Long Press
    ZDJ_UI_CONTROL_JOG_PRESS_3, // Shift Long Press
    ZDJ_UI_CONTROL_JOG_RELEASE_0, // Release
    ZDJ_UI_CONTROL_JOG_RELEASE_1, // Shift Release
    ZDJ_UI_CONTROL_JOG_RELEASE_2, // Long Release
    ZDJ_UI_CONTROL_JOG_RELEASE_3, // Shift Long Release
    ZDJ_UI_CONTROL_JOG_RELEASE_4, // Press-Adjust Release

    // Vol/Out knob events
    ZDJ_UI_CONTROL_OUT_ADJUST_0,
    ZDJ_UI_CONTROL_OUT_ADJUST_1,
    ZDJ_UI_CONTROL_OUT_ADJUST_2,
    ZDJ_UI_CONTROL_OUT_PRESS_0, // Press
    ZDJ_UI_CONTROL_OUT_PRESS_1, // Shift Press
    ZDJ_UI_CONTROL_OUT_PRESS_2, // Long Press
    ZDJ_UI_CONTROL_OUT_PRESS_3, // Shift Long Press
    ZDJ_UI_CONTROL_OUT_RELEASE_0, // Release
    ZDJ_UI_CONTROL_OUT_RELEASE_1, // Shift Release
    ZDJ_UI_CONTROL_OUT_RELEASE_2, // Long Release
    ZDJ_UI_CONTROL_OUT_RELEASE_3, // Shift Long Release
    ZDJ_UI_CONTROL_OUT_RELEASE_4, // Press-Adjust Release

    // Tone 1 Knob Events
    ZDJ_UI_CONTROL_TONE_1_ADJUST_0,
    ZDJ_UI_CONTROL_TONE_1_ADJUST_1,
    ZDJ_UI_CONTROL_TONE_1_ADJUST_2,
    ZDJ_UI_CONTROL_TONE_1_PRESS_0, // Press
    ZDJ_UI_CONTROL_TONE_1_PRESS_1, // Shift Press
    ZDJ_UI_CONTROL_TONE_1_PRESS_2, // Long Press
    ZDJ_UI_CONTROL_TONE_1_PRESS_3, // Shift Long Press
    ZDJ_UI_CONTROL_TONE_1_RELEASE_0, // Release
    ZDJ_UI_CONTROL_TONE_1_RELEASE_1, // Shift Release
    ZDJ_UI_CONTROL_TONE_1_RELEASE_2, // Long Release
    ZDJ_UI_CONTROL_TONE_1_RELEASE_3, // Shift Long Release
    ZDJ_UI_CONTROL_TONE_1_RELEASE_4, // Press-Adjust Release

    // Tone 2 Knob Events
    ZDJ_UI_CONTROL_TONE_2_ADJUST_0,
    ZDJ_UI_CONTROL_TONE_2_ADJUST_1,
    ZDJ_UI_CONTROL_TONE_2_ADJUST_2,
    ZDJ_UI_CONTROL_TONE_2_PRESS_0, // Press
    ZDJ_UI_CONTROL_TONE_2_PRESS_1, // Shift Press
    ZDJ_UI_CONTROL_TONE_2_PRESS_2, // Long Press
    ZDJ_UI_CONTROL_TONE_2_PRESS_3, // Shift Long Press
    ZDJ_UI_CONTROL_TONE_2_RELEASE_0, // Release
    ZDJ_UI_CONTROL_TONE_2_RELEASE_1, // Shift Release
    ZDJ_UI_CONTROL_TONE_2_RELEASE_2, // Long Release
    ZDJ_UI_CONTROL_TONE_2_RELEASE_3, // Shift Long Release
    ZDJ_UI_CONTROL_TONE_2_RELEASE_4, // Press-Adjust Release

    // Tone 3 Knob Events
    ZDJ_UI_CONTROL_TONE_3_ADJUST_0,
    ZDJ_UI_CONTROL_TONE_3_ADJUST_1,
    ZDJ_UI_CONTROL_TONE_3_ADJUST_2,
    ZDJ_UI_CONTROL_TONE_3_PRESS_0, // Press
    ZDJ_UI_CONTROL_TONE_3_PRESS_1, // Shift Press
    ZDJ_UI_CONTROL_TONE_3_PRESS_2, // Long Press
    ZDJ_UI_CONTROL_TONE_3_PRESS_3, // Shift Long Press
    ZDJ_UI_CONTROL_TONE_3_RELEASE_0, // Release
    ZDJ_UI_CONTROL_TONE_3_RELEASE_1, // Shift Release
    ZDJ_UI_CONTROL_TONE_3_RELEASE_2, // Long Release
    ZDJ_UI_CONTROL_TONE_3_RELEASE_3, // Shift Long Release
    ZDJ_UI_CONTROL_TONE_3_RELEASE_4, // Press-Adjust Release
    
    // Play PB Events
    ZDJ_UI_CONTROL_PLAY_PRESS_0, // Press
    ZDJ_UI_CONTROL_PLAY_PRESS_1, // Shift Press
    ZDJ_UI_CONTROL_PLAY_PRESS_2, // Long Press
    ZDJ_UI_CONTROL_PLAY_PRESS_3, // Shift Long Press
    ZDJ_UI_CONTROL_PLAY_RELEASE_0, // Release
    ZDJ_UI_CONTROL_PLAY_RELEASE_1, // Shift Release
    ZDJ_UI_CONTROL_PLAY_RELEASE_2, // Long Release
    ZDJ_UI_CONTROL_PLAY_RELEASE_3, // Shift Long Release

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
    ZDJ_DECK_CONTROL_TOGGLE_RECORD,
    ZDJ_DECK_CONTROL_START_RECORD,
    ZDJ_DECK_CONTROL_PAUSE_RECORD,
    ZDJ_DECK_CONTROL_SYNC_TOGGLE,
    ZDJ_DECK_CONTROL_SYNC_ENABLE,
    ZDJ_DECK_CONTROL_SYNC_DISABLE,
    ZDJ_DECK_1_2_BASS_SWAP,

    // Deck 1
    ZDJ_DECK_1_CONTROL_FADE,
    ZDJ_DECK_1_CONTROL_TRIM,
    ZDJ_DECK_1_CONTROL_EQ_LO,
    ZDJ_DECK_1_CONTROL_EQ_MID,
    ZDJ_DECK_1_CONTROL_EQ_HI,
    ZDJ_DECK_1_CONTROL_FILTER_0,
    ZDJ_DECK_1_CONTROL_FILTER_1,
    ZDJ_DECK_1_CONTROL_FILTER_2,
    ZDJ_DECK_1_CONTROL_PFL_TRIM,
    ZDJ_DECK_1_CONTROL_PFL_TOGGLE_MUTE,
    ZDJ_DECK_1_CONTROL_LOOP_TOGGLE,
    ZDJ_DECK_1_CONTROL_LOOP_ON,
    ZDJ_DECK_1_CONTROL_LOOP_OFF,
    ZDJ_DECK_1_CONTROL_LOOP_START,
    ZDJ_DECK_1_CONTROL_LOOP_END,
    ZDJ_DECK_1_CONTROL_LOOP_LENGTH,
    ZDJ_DECK_1_CONTROL_SKIP,
    ZDJ_DECK_1_CONTROL_SKIP_LENGTH,
    ZDJ_DECK_1_CONTROL_SKIP_SET_ORIGIN,
    ZDJ_DECK_1_CONTROL_SKIP_RESET_TO_ORIGIN,
    ZDJ_DECK_1_CONTROL_FX_SELECT,
    ZDJ_DECK_1_CONTROL_FX_0,
    ZDJ_DECK_1_CONTROL_FX_1,
    ZDJ_DECK_1_CONTROL_FX_2,
    ZDJ_DECK_1_CONTROL_FX_3,
    ZDJ_DECK_1_CONTROL_FX_4,
    ZDJ_DECK_1_CONTROL_FX_5,
    ZDJ_DECK_1_CONTROL_SYNC_MULT,
    ZDJ_DECK_1_CONTROL_SCRUB,
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
    ZDJ_DECK_2_CONTROL_FILTER_0,
    ZDJ_DECK_2_CONTROL_FILTER_1,
    ZDJ_DECK_2_CONTROL_FILTER_2,
    ZDJ_DECK_2_CONTROL_PFL_TRIM,
    ZDJ_DECK_2_CONTROL_PFL_TOGGLE_MUTE,
    ZDJ_DECK_2_CONTROL_LOOP_TOGGLE,
    ZDJ_DECK_2_CONTROL_LOOP_ON,
    ZDJ_DECK_2_CONTROL_LOOP_OFF,
    ZDJ_DECK_2_CONTROL_LOOP_START,
    ZDJ_DECK_2_CONTROL_LOOP_END,
    ZDJ_DECK_2_CONTROL_LOOP_LENGTH,
    ZDJ_DECK_2_CONTROL_SKIP,
    ZDJ_DECK_2_CONTROL_SKIP_LENGTH,
    ZDJ_DECK_2_CONTROL_SKIP_SET_ORIGIN,
    ZDJ_DECK_2_CONTROL_SKIP_RESET_TO_ORIGIN,
    ZDJ_DECK_2_CONTROL_FX_SELECT,
    ZDJ_DECK_2_CONTROL_FX_0,
    ZDJ_DECK_2_CONTROL_FX_1,
    ZDJ_DECK_2_CONTROL_FX_2,
    ZDJ_DECK_2_CONTROL_FX_3,
    ZDJ_DECK_2_CONTROL_FX_4,
    ZDJ_DECK_2_CONTROL_FX_5,
    ZDJ_DECK_2_CONTROL_SYNC_MULT,
    ZDJ_DECK_2_CONTROL_SCRUB,
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
    ZDJ_DECK_EXT_CONTROL_FILTER_0,
    ZDJ_DECK_EXT_CONTROL_FILTER_1,
    ZDJ_DECK_EXT_CONTROL_FILTER_2,
    ZDJ_DECK_EXT_CONTROL_PFL_TRIM,
    ZDJ_DECK_EXT_CONTROL_PFL_TOGGLE_MUTE,
    ZDJ_DECK_EXT_CONTROL_LOOP_TOGGLE,
    ZDJ_DECK_EXT_CONTROL_LOOP_ON,
    ZDJ_DECK_EXT_CONTROL_LOOP_OFF,
    ZDJ_DECK_EXT_CONTROL_LOOP_START,
    ZDJ_DECK_EXT_CONTROL_LOOP_END,
    ZDJ_DECK_EXT_CONTROL_LOOP_LENGTH,
    ZDJ_DECK_EXT_CONTROL_SKIP,
    ZDJ_DECK_EXT_CONTROL_SKIP_LENGTH,
    ZDJ_DECK_EXT_CONTROL_SKIP_SET_ORIGIN,
    ZDJ_DECK_EXT_CONTROL_SKIP_RESET_TO_ORIGIN,
    ZDJ_DECK_EXT_CONTROL_FX_SELECT,
    ZDJ_DECK_EXT_CONTROL_FX_0,
    ZDJ_DECK_EXT_CONTROL_FX_1,
    ZDJ_DECK_EXT_CONTROL_FX_2,
    ZDJ_DECK_EXT_CONTROL_FX_3,
    ZDJ_DECK_EXT_CONTROL_FX_4,
    ZDJ_DECK_EXT_CONTROL_FX_5,
    ZDJ_DECK_EXT_CONTROL_SCRUB,
    ZDJ_DECK_EXT_CONTROL_SYNC_MULT,
    ZDJ_DECK_EXT_CONTROL_TEMPO,
    ZDJ_DECK_EXT_CONTROL_TEMPO_FINE,
    ZDJ_DECK_EXT_CONTROL_PLAY_PAUSE,
    ZDJ_DECK_EXT_CONTROL_PAUSE,
    ZDJ_DECK_EXT_CONTROL_HOTCUE_START,
    ZDJ_DECK_EXT_CONTROL_HOTCUE_STOP,

    ZDJ_CONTROL_ID_COUNT
} zdj_control_id_t;

typedef enum {
    ZDJ_CONTROL_MAP_NONE,
    
    ZDJ_CONTROL_MAP_MENU_BASE,
    ZDJ_CONTROL_MAP_MENU_DJ_ROOT,

    ZDJ_CONTROL_MAP_TEXT_INPUT,
    
    ZDJ_CONTROL_MAP_SDK_TEST,

    ZDJ_CONTROL_MAP_SOUNDCARD,

    ZDJ_CONTROL_MAP_LIB_EDIT_SONG,
    ZDJ_CONTROL_MAP_LIB_EDIT_CUEPOINT,
    ZDJ_CONTROL_MAP_LIB_EDIT_BEATGRID,

    ZDJ_CONTROL_MAP_STATION_1_EMPTY,
    ZDJ_CONTROL_MAP_STATION_1_MOM_EQ,
    ZDJ_CONTROL_MAP_STATION_1_EQ,
    ZDJ_CONTROL_MAP_STATION_1_TRIM,
    ZDJ_CONTROL_MAP_STATION_1_LOOP,
    ZDJ_CONTROL_MAP_STATION_1_SYNC,

    ZDJ_CONTROL_MAP_STATION_2_EMPTY,
    ZDJ_CONTROL_MAP_STATION_2_MOM_EQ,
    ZDJ_CONTROL_MAP_STATION_2_EQ,
    ZDJ_CONTROL_MAP_STATION_2_TRIM,
    ZDJ_CONTROL_MAP_STATION_2_LOOP,
    ZDJ_CONTROL_MAP_STATION_2_SYNC,
    
    ZDJ_CONTROL_MAP_STATION_EXT_MOM_EQ,
    ZDJ_CONTROL_MAP_STATION_EXT_EQ,
    ZDJ_CONTROL_MAP_STATION_EXT_TRIM,
    ZDJ_CONTROL_MAP_STATION_EXT_LOOP,
    ZDJ_CONTROL_MAP_STATION_EXT_SYNC,

    ZDJ_CONTROL_MAP_COUNT
} zdj_control_map_id_t;

static char * zdj_control_map_name[ ZDJ_CONTROL_MAP_COUNT ] = {
    "None", // ZDJ_CONTROL_MAP_NONE,
    
    "Menu Base", // ZDJ_CONTROL_MAP_MENU_BASE,
    "Menu DJ Root", // ZDJ_CONTROL_MAP_MENU_DJ_ROOT,

    "Text Input", // ZDJ_CONTROL_MAP_TEXT_INPUT,
    
    "SDK Test", // ZDJ_CONTROL_MAP_SDK_TEST,

    "Soundcard", // ZDJ_CONTROL_MAP_SOUNDCARD,

    "Lib Edit Song", // ZDJ_CONTROL_MAP_LIB_EDIT_SONG,
    "Lib Edit Cuepoint", // ZDJ_CONTROL_MAP_LIB_EDIT_CUEPOINT,
    "Lib Edit Beatgrid", // ZDJ_CONTROL_MAP_LIB_EDIT_BEATGRID,

    "Deck 1 Empty", // ZDJ_CONTROL_MAP_STATION_1_EMPTY,
    "Deck 1 Mom. EQ", // ZDJ_CONTROL_MAP_STATION_1_MOM_EQ,
    "Deck 1 EQ", // ZDJ_CONTROL_MAP_STATION_1_EQ,
    "Deck 1 Trim", // ZDJ_CONTROL_MAP_STATION_1_TRIM,
    "Deck 1 Loop", // ZDJ_CONTROL_MAP_STATION_1_LOOP,
    "Deck 1 Sync", // ZDJ_CONTROL_MAP_STATION_1_SYNC,

    "Deck 2 Empty", // ZDJ_CONTROL_MAP_STATION_2_EMPTY,
    "Deck 2 Mom. EQ", // ZDJ_CONTROL_MAP_STATION_2_MOM_EQ,
    "Deck 2 EQ", // ZDJ_CONTROL_MAP_STATION_2_EQ,
    "Deck 2 Trim", // ZDJ_CONTROL_MAP_STATION_2_TRIM,
    "Deck 2 Loop", // ZDJ_CONTROL_MAP_STATION_2_LOOP,
    "Deck 2 Sync", // ZDJ_CONTROL_MAP_STATION_2_SYNC,
    
    "Deck Ext Mom. EQ", // ZDJ_CONTROL_MAP_STATION_EXT_MOM_EQ,
    "Deck Ext EQ", // ZDJ_CONTROL_MAP_STATION_EXT_EQ,
    "Deck Ext Trim",  // ZDJ_CONTROL_MAP_STATION_EXT_TRIM
    "Deck Ext Loop",  // ZDJ_CONTROL_MAP_STATION_EXT_LOOP
    "Deck Ext Sync"  // ZDJ_CONTROL_MAP_STATION_EXT_SYNC
};

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

typedef void ( *zdj_special_control_cb )( zdj_control_event_t * );
typedef struct {
    zdj_control_id_t id;
    zdj_special_control_cb cb;
    bool active;
} zdj_special_control_handler_t;
extern zdj_special_control_handler_t zdj_special_control_handlers[ 5 ];

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
zdj_error_type_t zdj_deactivate_all_controls( void );

void zdj_activate_control_map( zdj_control_map_id_t map_id );

// Register for event CB outside control map
void zdj_register_special_control_handler( int index, zdj_control_id_t control_id, zdj_special_control_cb cb );

// Mapping System
bool zdj_control_map_hmi_input_event( zdj_hmi_input_event_t * in_e, zdj_control_event_t * c_e );
bool zdj_control_event_is_ui_control( zdj_control_event_t * event );
bool zdj_control_event_is_deck_control( zdj_control_event_t * event );

#endif

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

#ifndef ZDJ_DECK_CONTROL_H
#define ZDJ_DECK_CONTROL_H

#include <stdint.h>
#include <stdbool.h>

// typedef enum {
//     ZDJ_DECK_CONTROL_LR_VOL,
//     ZDJ_DECK_CONTROL_CUE_VOL,
//     ZDJ_DECK_CONTROL_XFADE,

//     ZDJ_DECK_1_CONTROL_FADE,
//     ZDJ_DECK_1_CONTROL_TRIM,
//     ZDJ_DECK_1_CONTROL_EQ_LO,
//     ZDJ_DECK_1_CONTROL_EQ_MID,
//     ZDJ_DECK_1_CONTROL_EQ_HI,
//     ZDJ_DECK_1_CONTROL_PFL_TRIM,
//     ZDJ_DECK_1_CONTROL_PFL_MUTE,
//     ZDJ_DECK_1_CONTROL_FX_SELECT,
//     ZDJ_DECK_1_CONTROL_FX_0,
//     ZDJ_DECK_1_CONTROL_FX_1,
//     ZDJ_DECK_1_CONTROL_FX_2,
//     ZDJ_DECK_1_CONTROL_FX_3,
//     ZDJ_DECK_1_CONTROL_FX_4,
//     ZDJ_DECK_1_CONTROL_FX_5,
//     ZDJ_DECK_1_CONTROL_SCRUB,
//     ZDJ_DECK_1_CONTROL_NUDGE,
//     ZDJ_DECK_1_CONTROL_TEMPO,
//     ZDJ_DECK_1_CONTROL_TEMPO_FINE,
//     ZDJ_DECK_1_CONTROL_PLAY,
//     ZDJ_DECK_1_CONTROL_PAUSE,
//     ZDJ_DECK_1_CONTROL_HOTCUE,

//     ZDJ_DECK_2_CONTROL_FADE,
//     ZDJ_DECK_2_CONTROL_TRIM,
//     ZDJ_DECK_2_CONTROL_EQ_LO,
//     ZDJ_DECK_2_CONTROL_EQ_MID,
//     ZDJ_DECK_2_CONTROL_EQ_HI,
//     ZDJ_DECK_2_CONTROL_PFL_TRIM,
//     ZDJ_DECK_2_CONTROL_PFL_MUTE,
//     ZDJ_DECK_2_CONTROL_FX_SELECT,
//     ZDJ_DECK_2_CONTROL_FX_0,
//     ZDJ_DECK_2_CONTROL_FX_1,
//     ZDJ_DECK_2_CONTROL_FX_2,
//     ZDJ_DECK_2_CONTROL_FX_3,
//     ZDJ_DECK_2_CONTROL_FX_4,
//     ZDJ_DECK_2_CONTROL_FX_5,
//     ZDJ_DECK_2_CONTROL_SCRUB,
//     ZDJ_DECK_2_CONTROL_NUDGE,
//     ZDJ_DECK_2_CONTROL_TEMPO,
//     ZDJ_DECK_2_CONTROL_TEMPO_FINE,
//     ZDJ_DECK_2_CONTROL_PLAY,
//     ZDJ_DECK_2_CONTROL_PAUSE,
//     ZDJ_DECK_2_CONTROL_HOTCUE,

//     ZDJ_DECK_EXT_CONTROL_TRIM,
//     ZDJ_DECK_EXT_CONTROL_EQ_LO,
//     ZDJ_DECK_EXT_CONTROL_EQ_MID,
//     ZDJ_DECK_EXT_CONTROL_EQ_HI,
//     ZDJ_DECK_EXT_CONTROL_PFL_TRIM,
//     ZDJ_DECK_EXT_CONTROL_PFL_MUTE,
//     ZDJ_DECK_EXT_CONTROL_FX_SELECT,
//     ZDJ_DECK_EXT_CONTROL_FX_0,
//     ZDJ_DECK_EXT_CONTROL_FX_1,
//     ZDJ_DECK_EXT_CONTROL_FX_2,
//     ZDJ_DECK_EXT_CONTROL_FX_3,
//     ZDJ_DECK_EXT_CONTROL_FX_4,
//     ZDJ_DECK_EXT_CONTROL_FX_5,
//     ZDJ_DECK_EXT_CONTROL_NUDGE,
//     ZDJ_DECK_EXT_CONTROL_TEMPO,
//     ZDJ_DECK_EXT_CONTROL_TEMPO_FINE,
//     ZDJ_DECK_EXT_CONTROL_PLAY,
//     ZDJ_DECK_EXT_CONTROL_PAUSE,
//     ZDJ_DECK_EXT_CONTROL_HOTCUE
// } zdj_deck_control_type_t;

// typedef struct {
//     zdj_deck_control_type_t type;
//     bool handled;
//     int i_val;
//     double d_val;
//     bool b_val;
//     struct zdj_deck_control_event_t * next;
//     struct zdj_deck_control_event_t * prev;
// } zdj_deck_control_event_t;

// // Head of the linked list of unprocessed events
// // Will be NULL if no events need to be processed
// extern zdj_deck_control_event_t * zdj_deck_control_event_base;
// extern zdj_deck_control_event_t * zdj_deck_control_event_tip;
// // extern volatile atomic_bool zdj_deck_control_has_events;
// extern volatile bool zdj_deck_control_has_events;

// zdj_deck_control_event_t * zdj_deck_control_new_event( zdj_deck_control_type_t type );
// void zdj_deck_control_push_event( zdj_deck_control_event_t * event );
// void zdj_deck_control_post_events( void );

#endif

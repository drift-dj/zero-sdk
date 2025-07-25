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

#ifndef ZDJ_PLAYBACK_CONTROL_H
#define ZDJ_PLAYBACK_CONTROL_H

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

typedef enum {
    ZDJ_PLAYBACK_CONTROL_LR_VOL,
    ZDJ_PLAYBACK_CONTROL_CUE_VOL,
    ZDJ_PLAYBACK_CONTROL_XFADE,

    ZDJ_PLAYBACK_CONTROL_DECK_1_FADE,
    ZDJ_PLAYBACK_CONTROL_DECK_1_TRIM,
    ZDJ_PLAYBACK_CONTROL_DECK_1_EQ_LO,
    ZDJ_PLAYBACK_CONTROL_DECK_1_EQ_MID,
    ZDJ_PLAYBACK_CONTROL_DECK_1_EQ_HI,
    ZDJ_PLAYBACK_CONTROL_DECK_1_PFL_TRIM,
    ZDJ_PLAYBACK_CONTROL_DECK_1_PFL_MUTE,
    ZDJ_PLAYBACK_CONTROL_DECK_1_FX_SELECT,
    ZDJ_PLAYBACK_CONTROL_DECK_1_FX_0,
    ZDJ_PLAYBACK_CONTROL_DECK_1_FX_1,
    ZDJ_PLAYBACK_CONTROL_DECK_1_FX_2,
    ZDJ_PLAYBACK_CONTROL_DECK_1_FX_3,
    ZDJ_PLAYBACK_CONTROL_DECK_1_FX_4,
    ZDJ_PLAYBACK_CONTROL_DECK_1_FX_5,
    ZDJ_PLAYBACK_CONTROL_DECK_1_SCRUB,
    ZDJ_PLAYBACK_CONTROL_DECK_1_NUDGE,
    ZDJ_PLAYBACK_CONTROL_DECK_1_TEMPO,
    ZDJ_PLAYBACK_CONTROL_DECK_1_TEMPO_FINE,
    ZDJ_PLAYBACK_CONTROL_DECK_1_PLAY,
    ZDJ_PLAYBACK_CONTROL_DECK_1_PAUSE,
    ZDJ_PLAYBACK_CONTROL_DECK_1_HOTCUE,

    ZDJ_PLAYBACK_CONTROL_DECK_2_FADE,
    ZDJ_PLAYBACK_CONTROL_DECK_2_TRIM,
    ZDJ_PLAYBACK_CONTROL_DECK_2_EQ_LO,
    ZDJ_PLAYBACK_CONTROL_DECK_2_EQ_MID,
    ZDJ_PLAYBACK_CONTROL_DECK_2_EQ_HI,
    ZDJ_PLAYBACK_CONTROL_DECK_2_PFL_TRIM,
    ZDJ_PLAYBACK_CONTROL_DECK_2_PFL_MUTE,
    ZDJ_PLAYBACK_CONTROL_DECK_2_FX_SELECT,
    ZDJ_PLAYBACK_CONTROL_DECK_2_FX_0,
    ZDJ_PLAYBACK_CONTROL_DECK_2_FX_1,
    ZDJ_PLAYBACK_CONTROL_DECK_2_FX_2,
    ZDJ_PLAYBACK_CONTROL_DECK_2_FX_3,
    ZDJ_PLAYBACK_CONTROL_DECK_2_FX_4,
    ZDJ_PLAYBACK_CONTROL_DECK_2_FX_5,
    ZDJ_PLAYBACK_CONTROL_DECK_2_SCRUB,
    ZDJ_PLAYBACK_CONTROL_DECK_2_NUDGE,
    ZDJ_PLAYBACK_CONTROL_DECK_2_TEMPO,
    ZDJ_PLAYBACK_CONTROL_DECK_2_TEMPO_FINE,
    ZDJ_PLAYBACK_CONTROL_DECK_2_PLAY,
    ZDJ_PLAYBACK_CONTROL_DECK_2_PAUSE,
    ZDJ_PLAYBACK_CONTROL_DECK_2_HOTCUE,

    ZDJ_PLAYBACK_CONTROL_DECK_EXT_TRIM,
    ZDJ_PLAYBACK_CONTROL_DECK_EXT_EQ_LO,
    ZDJ_PLAYBACK_CONTROL_DECK_EXT_EQ_MID,
    ZDJ_PLAYBACK_CONTROL_DECK_EXT_EQ_HI,
    ZDJ_PLAYBACK_CONTROL_DECK_EXT_PFL_TRIM,
    ZDJ_PLAYBACK_CONTROL_DECK_EXT_PFL_MUTE,
    ZDJ_PLAYBACK_CONTROL_DECK_EXT_FX_SELECT,
    ZDJ_PLAYBACK_CONTROL_DECK_EXT_FX_0,
    ZDJ_PLAYBACK_CONTROL_DECK_EXT_FX_1,
    ZDJ_PLAYBACK_CONTROL_DECK_EXT_FX_2,
    ZDJ_PLAYBACK_CONTROL_DECK_EXT_FX_3,
    ZDJ_PLAYBACK_CONTROL_DECK_EXT_FX_4,
    ZDJ_PLAYBACK_CONTROL_DECK_EXT_FX_5,
    ZDJ_PLAYBACK_CONTROL_DECK_EXT_PLAY,
    ZDJ_PLAYBACK_CONTROL_DECK_EXT_PAUSE,
    ZDJ_PLAYBACK_CONTROL_DECK_EXT_HOTCUE
} zdj_playback_control_type_t;

typedef struct {
    zdj_playback_control_type_t type;
    bool handled;
    int i_val;
    double d_val;
    bool b_val;
    struct zdj_playback_control_event_t * next;
    struct zdj_playback_control_event_t * prev;
} zdj_playback_control_event_t;

// Head of the linked list of unprocessed events
// Will be NULL if no events need to be processed
extern zdj_playback_control_event_t * zdj_playback_control_event_base;
extern zdj_playback_control_event_t * zdj_playback_control_event_tip;
// extern volatile atomic_bool zdj_playback_control_has_events;
extern volatile bool zdj_playback_control_has_events;

zdj_playback_control_event_t * zdj_playback_control_new_event( zdj_playback_control_type_t type );
void zdj_playback_control_push_event( zdj_playback_control_event_t * event );
void zdj_playback_control_post_events( void );

#endif

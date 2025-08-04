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

#ifndef ZDJ_UI_CONTROL_H
#define ZDJ_UI_CONTROL_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    ZDJ_UI_CONTROL_JOG,
    ZDJ_UI_CONTROL_TONE_1,
    ZDJ_UI_CONTROL_TONE_2,
    ZDJ_UI_CONTROL_TONE_3,
    ZDJ_UI_CONTROL_NAV,
    ZDJ_UI_CONTROL_FN_1,
    ZDJ_UI_CONTROL_FN_2,
    ZDJ_UI_CONTROL_FN_3
} zdj_ui_control_id_t;

typedef enum {
    ZDJ_UI_CONTROL_AXIS_ADJUST_0,
    ZDJ_UI_CONTROL_AXIS_ADJUST_1,
    ZDJ_UI_CONTROL_AXIS_ADJUST_3,
    ZDJ_UI_CONTROL_AXIS_ADJUST_4,
    ZDJ_UI_CONTROL_AXIS_PRESS_0,
    ZDJ_UI_CONTROL_AXIS_PRESS_1,
    ZDJ_UI_CONTROL_AXIS_PRESS_2,
    ZDJ_UI_CONTROL_AXIS_PRESS_3,
    ZDJ_UI_CONTROL_AXIS_RELEASE_0,
    ZDJ_UI_CONTROL_AXIS_RELEASE_1,
    ZDJ_UI_CONTROL_AXIS_RELEASE_2,
    ZDJ_UI_CONTROL_AXIS_RELEASE_3
} zdj_ui_control_axis_t;

typedef struct {
    zdj_ui_control_id_t id;
    zdj_ui_control_axis_t axis;
    bool handled;
    int i_val;
    double d_val;
    bool b_val;
    struct zdj_control_event_t * next;
    struct zdj_control_event_t * prev;
} zdj_control_event_t;

// Head of the linked list of unprocessed events
// Will be NULL if no events need to be processed
extern zdj_control_event_t * zdj_ui_control_event_base;
extern zdj_control_event_t * zdj_control_event_tip;
// extern volatile atomic_bool zdj_ui_control_has_events;
extern volatile bool zdj_ui_control_has_events;

zdj_control_event_t * zdj_ui_control_new_event( zdj_ui_control_id_t id, zdj_ui_control_axis_t axis );
void zdj_ui_control_push_event( zdj_control_event_t * event );
void zdj_ui_control_post_events( void );

#endif

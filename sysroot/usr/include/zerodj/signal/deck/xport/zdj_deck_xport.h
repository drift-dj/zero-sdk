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

#ifndef ZDJ_DECK_XPORT_H
#define ZDJ_DECK_XPORT_H

#include <stdbool.h>
#include <pthread.h>

#include <zerodj/signal/deck/zdj_deck.h>

typedef enum {
    ZDJ_XPORT_DECK_DIRECTION_OUTPUT,
    ZDJ_XPORT_DECK_DIRECTION_INPUT
} zdj_xport_deck_direction_t;

typedef enum {
    ZDJ_XPORT_DECK_SYNC_MODE_NORMAL,
    ZDJ_XPORT_DECK_SYNC_MODE_HALF,
    ZDJ_XPORT_DECK_SYNC_MODE_DOUBLE,
    ZDJ_XPORT_DECK_SYNC_MODE_OFF,
} zdj_xport_deck_sync_mode_t;

typedef struct {
    // Thread management
    sem_t start_cycle;
    bool thread_ready;
    bool exit_thread;
    int ppqn;
    bool meter_on;
    int meter_counter;
    double set_bpm;
    double transport_d;
    double transport_bg;
    zdj_xport_deck_direction_t direction;
    zdj_xport_deck_sync_mode_t sync_mode;
} zdj_xport_deck_state_t;

zdj_error_type_t zdj_new_xport_deck( zdj_deck_t * deck );
void zdj_deck_xport_init_transport( zdj_deck_t * deck );
void zdj_deck_xport_init_sync( zdj_deck_t * deck );

#endif
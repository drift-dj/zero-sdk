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

#ifndef ZDJ_DECK_MANAGER_H
#define ZDJ_DECK_MANAGER_H

#include <stdbool.h>
#include <pthread.h>

#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/system/error/zdj_error.h>

typedef enum {
    ZDJ_DECK_MGR_REQ_CREATE_DECK,
    ZDJ_DECK_MGR_REQ_REMOVE_DECK
} zdj_deck_manager_request_type_t;

typedef enum {
    ZDJ_DECK_MGR_REQ_STATUS_PENDING,
    ZDJ_DECK_MGR_REQ_STATUS_ACTIVE,
    ZDJ_DECK_MGR_REQ_STATUS_DONE
} zdj_deck_manager_request_status_t;

typedef struct zdj_deck_manager_request_t {
    zdj_deck_t * deck;
    zdj_deck_manager_request_type_t type;
    struct zdj_deck_manager_request_t * next;
} zdj_deck_manager_request_t;

typedef struct {
    zdj_deck_t * station_1;
    zdj_deck_t * station_2;
    zdj_deck_t * station_ext;
    zdj_deck_t * decks;
    uint8_t control_change_flags[ ZDJ_CONTROL_ID_COUNT ];
} zdj_deck_manager_t;

zdj_error_type_t zdj_deck_manager_init( void );
zdj_deck_manager_t * zdj_deck_manager( void );

// Entry-point for creating decks/loading files from UI front-end
zdj_deck_t * zdj_deck_manager_add_deck( 
    zdj_deck_type_t type,
    zdj_deck_station_t station,
    void * resource
);
zdj_error_type_t zdj_deck_manager_remove_deck( zdj_deck_t * deck );
zdj_deck_t * zdj_deck_manager_get_deck_for_station( zdj_deck_station_t station );

// Receive a new batch of deck control events.
void zdj_deck_manager_handle_events( int start_ind, int end_ind );
void zdj_deck_manager_clear_control_flags( zdj_deck_t * deck );
// Update the control models/sims for each active deck
void zdj_deck_manager_control_update_cycle( void );

#endif
#ifndef STATE_H
#define STATE_H

#include <zerodj/library/zdj_library.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/signal/deck/zdj_deck.h>

typedef struct {
    zdj_deck_station_t station;
    zdj_view_t * view;
    zdj_deck_t * deck;
    bool loading;
    zdj_control_map_id_t current_page_map;
    zdj_control_map_id_t mom_exit_page_map;
} station_state_t;

typedef struct {
    zdj_control_map_id_t map;
    handle_control_event_t control_handler;
    station_state_t station_1;
    station_state_t station_2;
    station_state_t station_ext;
    zdj_view_t * menu_stack;
    zdj_view_t * playback_view;
    zdj_view_t * record_modal;
    zdj_view_t * soundcard_modal;
    zdj_view_t * vol_widget;
    zdj_view_t * bpm_view;
    zdj_view_t * battery_view;
    bool library_error;
    char library_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
} ui_state_t;
extern ui_state_t * ui_state;

// zdj_deck_station_t get_station_for_map( zdj_control_map_id_t map );

#endif
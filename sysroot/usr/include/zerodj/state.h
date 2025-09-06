#ifndef STATE_H
#define STATE_H

#include <zerodj/library/zdj_library.h>
#include <zerodj/ui/zdj_ui.h>

typedef struct {
    zdj_control_map_id_t map;
    handle_control_event_t control_handler;
    zdj_view_t * menu_stack;
    bool library_error;
    char library_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
} ui_state_t;
extern ui_state_t * ui_state;

#endif
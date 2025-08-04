#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>

#include <zerodj/controls/ui/zdj_ui_control.h>


zdj_control_event_t * zdj_ui_control_event_base;
zdj_control_event_t * zdj_control_event_tip;
volatile bool zdj_ui_control_has_events;

// Push any queued events over to the playback thread
void zdj_ui_control_post_events( void ) {
    zdj_ui_control_has_events = true;
}

// Create a new event with type/id
zdj_control_event_t * zdj_ui_control_new_event( zdj_ui_control_id_t id, zdj_ui_control_axis_t axis ) {
    zdj_control_event_t * e = calloc( 1, sizeof( zdj_control_event_t ) );
    // e->type = type;
    return e;
}

// Push a new event to the tip of the stack
void zdj_ui_control_push_event( zdj_control_event_t * event ) {
    event->next = event->prev = NULL;
    if( !zdj_ui_control_event_base && !zdj_control_event_tip ) {
        zdj_ui_control_event_base = zdj_control_event_tip = event;
    } else {
        zdj_control_event_tip->next = (struct zdj_control_event_t*)event;
        event->prev = (struct zdj_control_event_t*)zdj_control_event_tip;
        zdj_control_event_tip = event;
    }
}
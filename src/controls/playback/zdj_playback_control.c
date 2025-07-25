#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>

#include <zerodj/controls/hmi/zdj_hmi.h>
#include <zerodj/controls/playback/zdj_playback_control.h>

// volatile atomic_bool zdj_playback_control_has_events;

zdj_playback_control_event_t * zdj_playback_control_event_base;
zdj_playback_control_event_t * zdj_playback_control_event_tip;
volatile bool zdj_playback_control_has_events;


// Push any queued events over to the playback thread
void zdj_playback_control_post_events( void ) {
    zdj_playback_control_has_events = true;
}

// Create a new event with type/id
zdj_playback_control_event_t * zdj_playback_control_new_event( zdj_playback_control_type_t type ) {
    zdj_playback_control_event_t * e = calloc( 1, sizeof( zdj_playback_control_event_t ) );
    e->type = type;
    return e;
}

// Push a new event to the tip of the stack
void zdj_playback_control_push_event( zdj_playback_control_event_t * event ) {
    event->next = event->prev = NULL;
    if( !zdj_playback_control_event_base && !zdj_playback_control_event_tip ) {
        zdj_playback_control_event_base = zdj_playback_control_event_tip = event;
    } else {
        zdj_playback_control_event_tip->next = (struct zdj_playback_control_event_t*)event;
        event->prev = (struct zdj_playback_control_event_t*)zdj_playback_control_event_tip;
        zdj_playback_control_event_tip = event;
    }
}
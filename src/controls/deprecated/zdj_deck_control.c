// #include <stdlib.h>
// #include <stdio.h>
// #include <stdbool.h>
// #include <pthread.h>
// #include <unistd.h>
// #include <stdatomic.h>

// 
// #include <zerodj/controls/deck/zdj_deck_control.h>


// zdj_deck_control_event_t * zdj_deck_control_event_base;
// zdj_deck_control_event_t * zdj_deck_control_event_tip;
// volatile bool zdj_deck_control_has_events;

// // Push any queued events over to the playback thread
// void zdj_deck_control_post_events( void ) {
//     zdj_deck_control_has_events = true;
// }

// // Create a new event with type/id
// zdj_deck_control_event_t * zdj_deck_control_new_event( zdj_deck_control_type_t type ) {
//     zdj_deck_control_event_t * e = calloc( 1, sizeof( zdj_deck_control_event_t ) );
//     e->type = type;
//     return e;
// }

// // Push a new event to the tip of the stack
// void zdj_deck_control_push_event( zdj_deck_control_event_t * event ) {
//     event->next = event->prev = NULL;
//     if( !zdj_deck_control_event_base && !zdj_deck_control_event_tip ) {
//         zdj_deck_control_event_base = zdj_deck_control_event_tip = event;
//     } else {
//         zdj_deck_control_event_tip->next = (struct zdj_deck_control_event_t*)event;
//         event->prev = (struct zdj_deck_control_event_t*)zdj_deck_control_event_tip;
//         zdj_deck_control_event_tip = event;
//     }
// }
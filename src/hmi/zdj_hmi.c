#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <zerodj/hmi/zdj_hmi.h>
#include <zerodj/hmi/input/zdj_hmi_input.h>
#include <zerodj/m7/zdj_m7.h>


void zdj_hmi_init( void ) {
    zdj_hmi_init_input( );
    zdj_hmi_event_base = NULL;
}

void zdj_hmi_activate( void ) {
    // Signal M7 core to begin scanning HMI muxes and collecting state data
    zdj_m7_msg( ZDJ_M7_ACTIVATE_HMI );
}

void zdj_hmi_deactivate( void ) {
    zdj_m7_msg( ZDJ_M7_DEACTIVATE_HMI );
}

// It's possible for multiple apps to run in parallel.
// In that case, only one app should be sending the
// flush_hmi message to m7, else one app will delete 
// the hmi state before the other can read it.
void zdj_hmi_pull_m7_events( bool should_flush ) {
    // Process M7's hmi input model from shared memory
    zdj_hmi_process_input( );

    // Signal M7 to flush a new model into shared memory
    if( should_flush ) {
        zdj_m7_msg( ZDJ_M7_FLUSH_HMI );
    }
}

void zdj_hmi_clear_events( void ) {
    // It's possible to do some post-view_stack event handling here.
    // Re-name this func to something like: post-process events...

    // walk the event list freeing events
    zdj_hmi_input_clear_events( );
}
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include <zerodj/controls/zdj_controls.h>
#include <zerodj/controls/hmi/zdj_hmi_input.h>
#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/perf/zdj_perf.h>
#include <zerodj/system/thread/zdj_thread.h>

zdj_control_active_state_t zdj_control_active_state;

// Increment and bound and return write head.
int zdj_get_next_deck_event_ind( void ) {
    zdj_deck_event_buf_write++;
    zdj_deck_event_buf_write %= ZDJ_CONTROL_EVENT_BUF_LEN;
    return zdj_deck_event_buf_write;
}
zdj_control_event_t zdj_deck_event_buf[ ZDJ_CONTROL_EVENT_BUF_LEN ];
volatile int zdj_deck_event_buf_write;
volatile int zdj_deck_event_buf_read;

// Increment and bound and return write head.
int zdj_get_next_ui_event_ind( void ) {
    zdj_ui_event_buf_write++;
    zdj_ui_event_buf_write %= ZDJ_CONTROL_EVENT_BUF_LEN;
    return zdj_ui_event_buf_write;
}
zdj_control_event_t zdj_ui_event_buf[ ZDJ_CONTROL_EVENT_BUF_LEN ];
volatile int zdj_ui_event_buf_write;
volatile int zdj_ui_event_buf_read;

zdj_error_type_t zdj_controls_init( void ) {
    zdj_control_hmi_input_init( );
    zdj_clear_controls( );
    // Stand up control cycle thread
    zdj_thread_launch_control_cycle( zdj_control_cycle_thread_main, NULL );
}

// Wipe any existing control/hmi input events.
// Reset input mapping states.
zdj_error_type_t zdj_clear_controls( void ) {
    zdj_deck_event_buf_read = 0;
    zdj_deck_event_buf_write = 0;
    memset( zdj_deck_event_buf, 0, sizeof( zdj_control_event_t ) * ZDJ_CONTROL_EVENT_BUF_LEN );
    zdj_ui_event_buf_read = 0;
    zdj_ui_event_buf_write = 0;
    memset( zdj_ui_event_buf, 0, sizeof( zdj_control_event_t ) * ZDJ_CONTROL_EVENT_BUF_LEN );
    zdj_hmi_input_event_buf_read = 0;
    zdj_hmi_input_event_buf_write = 0;
    memset( zdj_hmi_input_event_buf, 0, sizeof( zdj_control_event_t ) * ZDJ_CONTROL_EVENT_BUF_LEN );
}

zdj_error_type_t zdj_activate_control( zdj_control_id_t control_id ) {
    zdj_control_active_state.controls[ control_id ] = true;
}

zdj_error_type_t zdj_deactivate_control( zdj_control_id_t control_id ) {
    zdj_control_active_state.controls[ control_id ] = false;
}

// Increment and bound the write head for hmi_input events
// int zdj_get_next_hmi_input_event_write( void ) {
//     zdj_hmi_input_event_buf_write++;
//     zdj_hmi_input_event_buf_write %= ZDJ_CONTROL_EVENT_BUF_LEN;
//     return zdj_hmi_input_event_buf_write;
// }

void * zdj_control_cycle_thread_main( void * arg ) {

    // Prep Internal HMI Input scan
    zdj_control_prepare_hmi_input_scan( );

    // Set up sleep for ~900Hz cycle time
    // Maybe implement a soft-sleep system here which
    // lengthens sleep time if no events are happening.
    // Speeds back up after events arrive.
    struct timespec frame_sleep = { 0, 400000 };

    while( 1 ) {

        // Sleep until next sample
        nanosleep( &frame_sleep, NULL );

        // Open a tag for the process cycle
        zdj_perf_tag_t * tag = zdj_new_perf_tag_for_thread( ZDJ_SYSTEM_THREAD_CONTROL );
        tag->name = ZDJ_PERF_TAG_CONTROL_CYCLE;
        tag->start = zdj_perf_time( );

        // Run HMI scan cycle
        zdj_control_scan_hmi_input( );
        // Get Pot vals from M7

        // Generate Internal HMI Input events
        zdj_control_process_hmi_input( );

        // Make events from external input sources. (MIDI controllers, etc.)
        // These can map to UI/Deck Control events OR HMI Input events -- allowing
        // external control surfaces to replicate Zero's built-in buttons/knobs.
        // Generate MIDI mapped events
        // zdj_control_process_usb_midi_input( );
        // Generate USB HID mapped events
        // zdj_control_process_usb_hid_input( );
        // Generate Signal mapped events
        // zdj_control_process_signal_input( );

        // Transform HMI Input events into UI/Deck Control events
        zdj_control_transform_hmi_events( );

        // Close the perf tag
        tag->end = zdj_perf_time( );
    }
}

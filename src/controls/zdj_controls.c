#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include <zerodj/controls/zdj_controls.h>
#include <zerodj/controls/hmi/zdj_hmi_input.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/zdj_deck_manager.h>
#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/perf/zdj_perf.h>
#include <zerodj/system/thread/zdj_thread.h>

zdj_control_active_state_t zdj_control_active_state;
zdj_special_control_handler_t zdj_special_control_handlers[ 5 ];

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
    memset( &zdj_special_control_handlers, 0, sizeof( zdj_special_control_handler_t )*5 );
    // Stand up control cycle thread
    zdj_thread_launch_control_cycle( zdj_control_cycle_thread_main, NULL );
    return ZDJ_ERROR_OKAY;
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

zdj_error_type_t zdj_deactivate_all_controls( void ) {
    memset( zdj_control_active_state.controls, 0, sizeof(bool) * ZDJ_CONTROL_ID_COUNT );
}

// Register a special control cb which is called regardles of control map state.
void zdj_register_special_control_handler( int index, zdj_control_id_t control_id, zdj_special_control_cb cb ) {
    zdj_special_control_handlers[ index ].id = control_id;
    zdj_special_control_handlers[ index ].cb = cb;
    zdj_special_control_handlers[ index ].active = true;
    zdj_activate_control( control_id );
}

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
        // zdj_perf_tag_t * tag;
        // if( zdj_perf_enabled( ) ) {
        //     tag = zdj_new_perf_tag_for_thread( ZDJ_SYSTEM_THREAD_CONTROL );
        //     tag->name = ZDJ_PERF_TAG_CONTROL_CYCLE;
        //     tag->start = zdj_perf_time( );
        // }

        // Run HMI scan cycle
        zdj_control_scan_hmi_input( );
        // Get Pot vals from M7

        // Generate Internal HMI Input events
        zdj_control_process_hmi_input( );

        // Make events from external input sources. (MIDI controllers, etc.)
        // These can map directly to UI/Deck Control events OR indirectly as HMI Input events -- 
        // allowing external control surfaces to replicate Zero's built-in buttons/knobs.
        // Generate MIDI mapped events
        // zdj_control_process_usb_midi_input( );
        // Generate USB HID mapped events
        // zdj_control_process_usb_hid_input( );
        // Generate Signal mapped events
        // zdj_control_process_signal_input( );

        // Transform HMI Input events into UI/Deck Control events
        zdj_control_transform_hmi_events( );

        // If there are unhandled deck events in the ring buffer...
        if( zdj_deck_event_buf_read != zdj_deck_event_buf_write ) {
            // ...pass control events into deck_manager.
            zdj_deck_manager_handle_events( zdj_deck_event_buf_read, zdj_deck_event_buf_write );
            // Update the event buf's read head so we don't re-process these events
            zdj_deck_event_buf_read = zdj_deck_event_buf_write;
        }

        // Close the perf tag
        // if( zdj_perf_enabled( ) ) { tag->end = zdj_perf_time( ); }
    }
}

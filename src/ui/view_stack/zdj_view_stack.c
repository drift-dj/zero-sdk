#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view_stack/zdj_view_stack.h>
#include <zerodj/display/zdj_display.h>
#include <zerodj/hmi/zdj_hmi.h>
#include <zerodj/health/zdj_health_type.h>

zdj_view_t * zdj_view_stack_bottom;
zdj_view_t * zdj_view_stack_top;
zdj_rect_t view_stack_frame = {0,0,128,64};


void zdj_view_stack_init( void ) {

    // Init the HMI event system
    zdj_hmi_init( );

    // Reset head

    if( zdj_display_init( ) != 0) {
        printf( "zero-config failed to init display... exiting\n" );
        exit( ZDJ_HEALTH_STATUS_SDL_FAILED );
    }
}

void zdj_view_stack_deinit( void ) {
    zdj_display_release( );
}

void zdj_view_stack_update( void ) {
    // Get a fresh set of events from the M7 HMI system
    zdj_hmi_create_events( true );

    zdj_view_t * view;
    zdj_hmi_event_t * event;
    // Start at stack top, calling handle_hmi as we stride toward bottom.
    // HMI Events are invoked against a view one at a time.
    view = zdj_view_stack_top;
    while( view ) {
        event = zdj_hmi_event_base;
        while( event ) {
            view->handle_hmi_event( view, event );
            event = event->next;
        }
        view = view->down;
    }
    // Trigger the HMI event system to post-process the event stack
    zdj_hmi_clear_events( );

    // Start at stack bottom, calling draw on everything as we stride up to top.
    view = zdj_view_stack_bottom;
    while( view ) {
        view->draw( view, &view_stack_frame );
        view = view->up;
    }

    // Pack pixels into shared buffer and signal M7 to write to display.
    zdj_display_draw( );
}

void zdj_view_stack_push( zdj_view_t * view ) {
    view->up = view->down = NULL;
    if( !zdj_view_stack_bottom && !zdj_view_stack_top ) {
        zdj_view_stack_bottom = zdj_view_stack_top = view;
    } else {
        zdj_view_stack_top->up = view;
        view->down = zdj_view_stack_top;
        zdj_view_stack_top = view;
    }
}

zdj_view_t * zdj_view_stack_pop( void ) {
    return NULL;
}

void zdj_view_stack_remove( zdj_view_t * view ) {

}

zdj_view_t * zdj_view_stack_peek_index( int index ) {
    return NULL;
}
 
zdj_view_t * zdj_view_stack_insert_at( int index ) {
    return NULL;
}

zdj_view_t * zdj_view_stack_insert_above( int index ) {
    return NULL;
}

zdj_view_t * zdj_view_stack_insert_below( int index ) {
    return NULL;
}
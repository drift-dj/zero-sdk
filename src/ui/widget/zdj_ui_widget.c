#include <stdio.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/ui/zdj_ui.h>
// #include <zerodj/ui/view/asset_view/zdj_asset_view.h>
// #include <zerodj/ui/view/file_browser_view/zdj_file_browser_view.h>
// #include <zerodj/ui/view/menu_view/zdj_menu_view.h>
// #include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
// #include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
// #include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>
#include <zerodj/ui/widget/zdj_ui_widget.h>
#include <zerodj/ui/widget/debug/zdj_debug_widget.h>
#include <zerodj/ui/widget/perf/zdj_perf_widget.h>
#include <zerodj/ui/widget/recording/zdj_recording_widget.h>
#include <zerodj/ui/widget/volume/zdj_volume_widget.h>

static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event );

zdj_error_type_t zdj_ui_widget_init( void ) {
    // Init ui panel state
    zdj_widget_state_t * state = calloc( 1, sizeof( zdj_widget_state_t ) );
    zdj_widget_view( )->state = state;
    zdj_widget_view( )->handle_control_event = &_handle_control;

    state->debug_widget = zdj_new_debug_widget( );
    zdj_add_subview( zdj_widget_view( ), state->debug_widget );

    state->perf_widget = zdj_new_perf_widget( );
    zdj_add_subview( zdj_widget_view( ), state->perf_widget );

    state->volume_widget = zdj_new_volume_widget( );
    zdj_add_subview( zdj_widget_view( ), state->volume_widget );

    state->recording_widget = zdj_new_recording_widget( );
    zdj_add_subview( zdj_widget_view( ), state->recording_widget );

    return ZDJ_ERROR_OKAY;
}

static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {
    // printf( "widget view _handle_control\n" );
    zdj_widget_state_t * state = (zdj_widget_state_t*)view->state;
    // If a panel is deployed, send the event down into the panel
    if( _event->id == ZDJ_UI_CONTROL_TOGGLE_DEBUG_WIDGET ) { 
        zdj_debug_widget_state_t * debug_state = (zdj_debug_widget_state_t*)state->debug_widget->state;
        debug_state->toggle( state->debug_widget );
    } else if( _event->id == ZDJ_UI_CONTROL_TOGGLE_PERF_WIDGET ) {
        zdj_perf_widget_state_t * perf_state = (zdj_perf_widget_state_t*)state->perf_widget->state;
        perf_state->toggle( state->perf_widget );
    } else if( _event->id == ZDJ_UI_CONTROL_RECORDING_SAVED ) {
        
    } else if( _event->id == ZDJ_UI_CONTROL_SCREENCAP_SAVED ) {
        
    }
}
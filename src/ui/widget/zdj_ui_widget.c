#include <stdio.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/settings/zdj_settings.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/zdj_view_stack.h>
#include <zerodj/ui/widget/zdj_ui_widget.h>
#include <zerodj/ui/widget/crash/zdj_crash_widget.h>
#include <zerodj/ui/widget/debug/zdj_debug_widget.h>
#include <zerodj/ui/widget/log/zdj_log_widget.h>
#include <zerodj/ui/widget/notify/zdj_notify_widget.h>
#include <zerodj/ui/widget/perf/zdj_perf_widget.h>
#include <zerodj/ui/widget/recording/zdj_recording_widget.h>
#include <zerodj/ui/widget/volume/zdj_volume_widget.h>

static zdj_widget_state_t * _zdj_widget_state;
static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event );

zdj_error_type_t zdj_ui_widget_init( void ) {
    // Init ui panel state
    _zdj_widget_state = calloc( 1, sizeof( zdj_widget_state_t ) );
    zdj_widget_view( )->state = _zdj_widget_state;
    zdj_widget_view( )->handle_control_event = &_handle_control;

    _zdj_widget_state->crash_widget = zdj_new_crash_widget( );
    zdj_add_subview( zdj_widget_view( ), _zdj_widget_state->crash_widget );
    
    _zdj_widget_state->debug_widget = zdj_new_debug_widget( );
    zdj_add_subview( zdj_widget_view( ), _zdj_widget_state->debug_widget );

    _zdj_widget_state->log_widget = zdj_new_log_widget( );
    zdj_add_subview( zdj_widget_view( ), _zdj_widget_state->log_widget );
    // Killing this for now due to unexplored crash
    // Observe show log widget at boot setting
    // if( zdj_setting_get( ZDJ_SETTING_LOG_DEPLOY_AT_BOOT )->b_val ) {
    //     zdj_log_widget_state_t * log_widget_state = (zdj_log_widget_state_t*)_zdj_widget_state->log_widget->state;
    //     log_widget_state->toggle( _zdj_widget_state->log_widget );
    // }

    _zdj_widget_state->notify_widget = zdj_new_notify_widget( );
    zdj_add_subview( zdj_widget_view( ), _zdj_widget_state->notify_widget );
    
    _zdj_widget_state->perf_widget = zdj_new_perf_widget( );
    zdj_add_subview( zdj_widget_view( ), _zdj_widget_state->perf_widget );

    _zdj_widget_state->volume_widget = zdj_new_volume_widget( );
    zdj_add_subview( zdj_widget_view( ), _zdj_widget_state->volume_widget );

    _zdj_widget_state->recording_widget = zdj_new_recording_widget( );
    zdj_add_subview( zdj_widget_view( ), _zdj_widget_state->recording_widget );

    return ZDJ_ERROR_OKAY;
}

static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {
    // If a panel is deployed, send the event down into the panel
    zdj_crash_widget_state_t * crash_state = (zdj_crash_widget_state_t*)_zdj_widget_state->crash_widget->state;
    if( crash_state->deployed ) {
        _event->blocked = true;
        if( _event->id == ZDJ_UI_CONTROL_JOG_RELEASE_0 ||
            _event->id == ZDJ_UI_CONTROL_NAV_RELEASE_0
        ) {
            crash_state->toggle( _zdj_widget_state->crash_widget );
        }
    }
    if( _event->id == ZDJ_UI_CONTROL_TOGGLE_DEBUG_WIDGET ) { 
        zdj_debug_widget_state_t * debug_state = (zdj_debug_widget_state_t*)_zdj_widget_state->debug_widget->state;
        debug_state->toggle( _zdj_widget_state->debug_widget );
    } else if( _event->id == ZDJ_UI_CONTROL_TOGGLE_LOG_WIDGET ) {
        printf( "Toggle Log widget\n" );
        zdj_log_widget_state_t * log_state = (zdj_log_widget_state_t*)_zdj_widget_state->log_widget->state;
        log_state->toggle( _zdj_widget_state->log_widget );
    } else if( _event->id == ZDJ_UI_CONTROL_TOGGLE_PERF_WIDGET ) {
        zdj_perf_widget_state_t * perf_state = (zdj_perf_widget_state_t*)_zdj_widget_state->perf_widget->state;
        perf_state->toggle( _zdj_widget_state->perf_widget );
    }
}

zdj_view_t * zdj_ui_get_notify_widget( void ) {
    return _zdj_widget_state->notify_widget;
}

zdj_view_t * zdj_ui_get_log_widget( void ) {
    return _zdj_widget_state->log_widget;
}

zdj_view_t * zdj_ui_get_recording_widget( void ) {
    return _zdj_widget_state->recording_widget;
}

void zdj_ui_recording_widget_add_clip( void ) {
    zdj_recording_widget_state_t * state = (zdj_recording_widget_state_t*)_zdj_widget_state->recording_widget->state;
    state->has_new_clip = true;
}

// Trigger an update in the soundcard-dependent widgets
void zdj_ui_widget_update_soundcard( void ) {
    if( !_zdj_widget_state ) { return; }

    zdj_volume_widget_state_t * vol_state = (zdj_volume_widget_state_t*)_zdj_widget_state->volume_widget->state;
    if( vol_state ) { vol_state->needs_soundcard_update = true; }

    zdj_recording_widget_state_t * record_state = (zdj_recording_widget_state_t*)_zdj_widget_state->recording_widget->state;
    if( record_state ) { record_state->needs_soundcard_update = true; }
}

// Deploy the crash log widget
void zdj_ui_widget_show_crash_log( void ) {
    if( !_zdj_widget_state ) { return; }
    zdj_crash_widget_state_t * crash_state = (zdj_crash_widget_state_t*)_zdj_widget_state->crash_widget->state;
    if( crash_state ) { crash_state->toggle( _zdj_widget_state->crash_widget ); }
}
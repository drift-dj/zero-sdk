#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/log_view/zdj_log_view.h>
#include <zerodj/ui/view/modal_view/zdj_modal_view.h>
#include <zerodj/ui/view/scroll_view/zdj_scroll_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _zdj_log_view_draw_log( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_log_view_draw_tail( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_log_view_draw_cat( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_log_view_handle_control( zdj_view_t * view, zdj_control_event_t * _event );
static zdj_error_type_t _zdj_log_view_cleanup_str( char * buf, size_t buf_len );

zdj_view_t * zdj_new_log_view( 
    char * log_path, 
    zdj_log_view_type_t type, 
    zdj_view_t * parent_view,
    zdj_rect_t * frame
) {
    printf( "zdj_new_log_view: %d %s\n", type, log_path );

    zdj_view_t * view = zdj_new_modal_view( zdj_modal_rect( ) );
    view->type = ZDJ_VIEW_LOG;
    // browser_view->draw = &_draw;
    if( type == ZDJ_LOG_VIEW_TYPE_TAIL ) {
        view->draw = &_zdj_log_view_draw_tail;
    } else if( type == ZDJ_LOG_VIEW_TYPE_CAT ) {
        view->draw = &_zdj_log_view_draw_cat;
    } else if( type == ZDJ_LOG_VIEW_TYPE_LOG ) {
        view->draw = &_zdj_log_view_draw_log;
    }
    view->handle_control_event = &_zdj_log_view_handle_control;
    view->map = ZDJ_CONTROL_MAP_SETTINGS_PANEL;

    // Add a scroll_view
    zdj_view_t * scroll_view = zdj_new_scroll_view( frame );
    zdj_scroll_view_state_t * scroll_view_state = (zdj_scroll_view_state_t*)scroll_view->state;
    scroll_view_state->scroll_dir = ZDJ_VERTICAL;
    zdj_add_subview( view, scroll_view );

    // Add state
    zdj_log_view_state_t * state = calloc( 1, sizeof( zdj_log_view_state_t ) );
    view->state = state;
    state->needs_layout_update = true;
    state->parent_view = parent_view;
    state->type = type;

    if( type == ZDJ_LOG_VIEW_TYPE_TAIL ) {
        snprintf( state->log_path, sizeof( state->log_path ), "tail -20 %s", log_path );
    } else if( type == ZDJ_LOG_VIEW_TYPE_CAT || type == ZDJ_LOG_VIEW_TYPE_LOG ) {
        snprintf( state->log_path, sizeof( state->log_path ), "cat %s", log_path );
    }

    state->scroll_view = scroll_view;
    state->log_counter = 0;
    state->log_fp = popen( state->log_path, "r" );
    if ( state->log_fp == NULL ) {
        printf( "failed to popen: %s\n", state->log_path );
        return NULL;
    }
    

    return view;
}

static void _zdj_log_view_draw_tail( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_log_view_state_t * state = (zdj_log_view_state_t*)view->state;
    zdj_scroll_view_state_t * scroll_state = (zdj_scroll_view_state_t*)state->scroll_view->state;
    
    // Redraw the log every n seconds
    if( !state->log_fp && state->log_counter++ > 120 ) {
        state->log_counter = 0;

        state->log_fp = popen( state->log_path, "r" );
        // Get next line from log and add it to the scroll_view
        char log_line[ 1024 ];
        zdj_point_t point = { 0, 0 };
        if( fgets( log_line, sizeof( log_line ), state->log_fp ) ) {
            _zdj_log_view_cleanup_str( log_line, sizeof( log_line ) );
            zdj_view_t * label = zdj_new_label_view( log_line, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE );
            label->frame.x = 1;
            label->frame.y = scroll_state->scroll_size.y;
            zdj_scroll_view_add_subview( state->scroll_view, label );
            point.y = scroll_state->scroll_size.y-view->frame.h;
        } else {
            // If we've drained the log, close it and de-ref so we
            // can re-open it at next draw cycle.
            pclose( state->log_fp );
            state->log_fp = NULL;

            // Scroll to last line in tail
            zdj_scroll_view_to_point( state->scroll_view, &point );
        }
    }
}

static void _zdj_log_view_draw_cat( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // printf( "_zdj_log_view_draw_cat\n" );
    zdj_log_view_state_t * state = (zdj_log_view_state_t*)view->state;
    zdj_scroll_view_state_t * scroll_state = (zdj_scroll_view_state_t*)state->scroll_view->state;
    
    if( state->needs_layout_update ) {
        state->log_fp = popen( state->log_path, "r" );

        if( state->log_fp ) {
            // Get next line from log and add it to the scroll_view
            char log_line[ 1024 ];

            while( fgets( log_line, sizeof( log_line ), state->log_fp ) ) {
                _zdj_log_view_cleanup_str( log_line, sizeof( log_line ) );
                zdj_view_t * label = zdj_new_label_view( log_line, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE );
                label->frame.x = 1;
                label->frame.y = scroll_state->scroll_size.y;
                zdj_scroll_view_add_subview( state->scroll_view, label );
            }

            pclose( state->log_fp );
            state->log_fp = NULL;
        }
    }
    state->needs_layout_update = false;
}

static void _zdj_log_view_draw_log( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // printf( "_zdj_log_view_draw_log\n" );
    zdj_log_view_state_t * state = (zdj_log_view_state_t*)view->state;
    zdj_scroll_view_state_t * scroll_state = (zdj_scroll_view_state_t*)state->scroll_view->state;
    
    // If we drained the log last time, try opening it again.
    // Don't immediately re-open the log to avoid overloading the system.
    if( !state->log_fp && state->log_counter++ > 120 ) {
        state->log_fp = popen( state->log_path, "r" );
        state->log_counter = 0;
    }

    if( state->log_fp ) {
        // Get next line from log and add it to the scroll_view
        char log_line[ 1024 ];
        if( fgets( log_line, sizeof( log_line ), state->log_fp ) ) {
            _zdj_log_view_cleanup_str( log_line, sizeof( log_line ) );
            zdj_view_t * label = zdj_new_label_view( log_line, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE );
            label->frame.x = 1;
            label->frame.y = scroll_state->scroll_size.y;
            zdj_scroll_view_add_subview( state->scroll_view, label );
            zdj_point_t point = { 0, scroll_state->scroll_size.y-view->frame.h };
            zdj_scroll_view_to_point( state->scroll_view, &point );
        } else {
            // If we've drained the log, close it and de-ref so we
            // can re-open it at next draw cycle.
            pclose( state->log_fp );
            state->log_fp = NULL;
        }
    }
}

static void _zdj_log_view_handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_control_event_t * e = (zdj_control_event_t *)_event;
    zdj_log_view_state_t * state = (zdj_log_view_state_t*)view->state;
    zdj_scroll_view_state_t * scroll_state = (zdj_scroll_view_state_t*)state->scroll_view->state;

    // printf( "_zdj_log_view_handle_control: %d\n", e->id );

    // Ignore events which have been blocked by layers above this one.
    if( e->blocked ) { return; }
    
    if( e->id == ZDJ_UI_CONTROL_JOG_ADJUST_0 ) {
        e->blocked = true;
        // Scroll the scroll view
        zdj_point_t point;
        point.x = 0;
        point.y = (float)e->i_val;
        zdj_scroll_view_by_point( state->scroll_view, &point );
    } else if( e->id == ZDJ_UI_CONTROL_TONE_2_ADJUST_0 ) {
        e->blocked = true;
        // Scroll the scroll view
        zdj_point_t point;
        point.x = 0;
        point.y = e->i_val * 2;
        zdj_scroll_view_by_point( state->scroll_view, &point );
    } else if( e->id == ZDJ_UI_CONTROL_TONE_1_ADJUST_0 ) {
        e->blocked = true;
        // Scroll the scroll view
        zdj_point_t point;
        point.x = e->i_val * 2;
        point.y = 0;
        zdj_scroll_view_by_point( state->scroll_view, &point );
    } else if( e->id == ZDJ_UI_CONTROL_JOG_RELEASE_0 || e->id == ZDJ_UI_CONTROL_NAV_RELEASE_0 ) {
        e->blocked = true;
        // Pop
        printf( "popping from: %p\n", state->parent_view );
        zdj_pop_subview_of( state->parent_view, true );
    }
}

zdj_error_type_t _zdj_log_view_cleanup_str( char * buf, size_t buf_len ) {
    int strlen = 0;
    for( int i=0; i<buf_len; i++ ) {
        strlen++;
        if( buf[ i ] == '\n' ||
            buf[ i ] == 0x0a ) {
            buf[ i ] = ' ';
        }
        if( buf[ i ] == '\0' ) {
            break;
        }
    }

    // If string has timestamp, trim it out.
    if( !strncmp( buf, "[", 1 ) ) {
        memmove( buf, buf+14, strlen );
    }
    return ZDJ_ERROR_OKAY;
}
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/controls/hmi/zdj_hmi.h>
#include <zerodj/ui/zdj_ui.h>
// #include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/log_view/zdj_log_view.h>
// #include <zerodj/ui/view/menu_view/zdj_menu_view.h>
// #include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
// #include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
// #include <zerodj/ui/view/menu_section_view/zdj_menu_section_view.h>
#include <zerodj/ui/view/scroll_view/zdj_scroll_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _zdj_log_view_draw_tail( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_log_view_draw_cat( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_log_view_handle_hmi( zdj_view_t * view, void * _event );
static zdj_error_type_t _zdj_log_view_cleanup_str( char * buf, size_t buf_len );

zdj_view_t * zdj_new_log_view( char * log_path, zdj_log_view_type_t type, zdj_rect_t * frame ) {
    // printf( "zdj_new_log_view: %s\n", log_path );
    zdj_view_t * view = zdj_new_view( frame );
    view->type = ZDJ_VIEW_BASE;
    if( type == ZDJ_LOG_VIEW_TYPE_TAIL ) {
        view->draw = &_zdj_log_view_draw_tail;
    } else if( type == ZDJ_LOG_VIEW_TYPE_CAT ) {
        view->draw = &_zdj_log_view_draw_cat;
    }
    view->handle_hmi_event = &_zdj_log_view_handle_hmi;

    // Add a scroll_view
    zdj_view_t * scroll_view = zdj_new_scroll_view( frame );
    zdj_scroll_view_state_t * scroll_view_state = (zdj_scroll_view_state_t*)scroll_view->state;
    scroll_view_state->scroll_dir = ZDJ_VERTICAL;
    zdj_add_subview( view, scroll_view );

    // Add state
    zdj_log_view_state_t * state = calloc( 1, sizeof( zdj_log_view_state_t ) );
    view->state = state;
    state->type = type;
    state->log_path = strdup( log_path );
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
            label->frame->x = 1;
            label->frame->y = scroll_state->scroll_size.y;
            zdj_scroll_view_add_subview( state->scroll_view, label );
            point.y = scroll_state->scroll_size.y-view->frame->h;
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
            label->frame->x = 1;
            label->frame->y = scroll_state->scroll_size.y;
            zdj_scroll_view_add_subview( state->scroll_view, label );
            zdj_point_t point = { 0, scroll_state->scroll_size.y-view->frame->h };
            zdj_scroll_view_to_point( state->scroll_view, &point );
        } else {
            // If we've drained the log, close it and de-ref so we
            // can re-open it at next draw cycle.
            pclose( state->log_fp );
            state->log_fp = NULL;
        }
    }
}

static void _zdj_log_view_handle_hmi( zdj_view_t * view, void * _event ) {
    zdj_hmi_event_t * e = (zdj_hmi_event_t *)_event;
    zdj_log_view_state_t * state = (zdj_log_view_state_t*)view->state;
    zdj_scroll_view_state_t * scroll_state = (zdj_scroll_view_state_t*)state->scroll_view->state;

    // Ignore events which have been blocked by layers above this one.
    if( e->blocked ) { return; }
    
    if( e->id == ZDJ_HMI_ENCO_2_JOG ) {
        if( e->type == ZDJ_HMI_EVENT_ADJUST ) {
            e->blocked = true;
            // Scroll the scroll view
            zdj_point_t point;
            point.x = 0;
            point.y = (float)e->i_val;
            zdj_scroll_view_by_point( state->scroll_view, &point );
        }
    } else if( e->id == ZDJ_HMI_ENCO_3_TONE_1 ) {
        if( e->type == ZDJ_HMI_EVENT_ADJUST ) {
            e->blocked = true;
            // Scroll the scroll view
            zdj_point_t point;
            point.x = 0;
            point.y = e->i_val;
            zdj_scroll_view_by_point( state->scroll_view, &point );
        }
    } else if( e->id == ZDJ_HMI_ENCO_4_TONE_2 ) {
        if( e->type == ZDJ_HMI_EVENT_ADJUST ) {
            e->blocked = true;
            // Scroll the scroll view
            zdj_point_t point;
            point.x = e->i_val;
            point.y = 0;
            zdj_scroll_view_by_point( state->scroll_view, &point );
        }
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
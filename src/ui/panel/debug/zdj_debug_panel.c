#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/panel/debug/zdj_debug_panel.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/debug_view/zdj_debug_view.h>
#include <zerodj/ui/view/log_view/zdj_log_view.h>
#include <zerodj/ui/view/scroll_view/zdj_scroll_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _zdj_debug_panel_draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_debug_panel_handle_control( zdj_view_t * view, zdj_control_event_t * _event );
static void _zdj_debug_panel_deploy( zdj_view_t * view );
static void _zdj_debug_panel_retract( zdj_view_t * view );

zdj_view_t * zdj_new_debug_panel( void ) {
    zdj_view_t * view = zdj_new_view( zdj_screen_rect( ) );

    // Add a container view for animations/clipping
    zdj_view_t * container_view = zdj_new_view( zdj_debug_panel_rect( ) );
    zdj_add_subview( view, container_view );
    container_view->type = ZDJ_VIEW_BASE;
    container_view->draw = &_zdj_debug_panel_draw;
    container_view->handle_control_event = &_zdj_debug_panel_handle_control;

    container_view->frame->x = ZDJ_DEBUG_PANEL_WIDTH * -3;
    container_view->frame->y = 0;
    
    container_view->in_anim = zdj_new_anim( ZDJ_ANIM_DEBUG_PANEL_SHOW );
    container_view->out_anim = zdj_new_anim( ZDJ_ANIM_DEBUG_PANEL_HIDE );

    // Add a log_view
    // zdj_view_t * log_view = zdj_new_log_view( 
    //     "cat /sys/kernel/debug/usb/tcpm-0-0052/log", 
    //     ZDJ_LOG_VIEW_TYPE_BUFFER,
    //     zdj_debug_panel_rect( ) 
    // );
    // zdj_view_t * log_view = zdj_new_log_view( 
    //     "dmesg | tail -n 10", 
    //     ZDJ_LOG_VIEW_TYPE_TAIL,
    //     zdj_debug_panel_rect( ) 
    // );
    // zdj_add_subview( container_view, log_view );

    zdj_view_t * debug_view = zdj_new_debug_view( zdj_debug_panel_rect( ) );
    zdj_add_subview( container_view, debug_view );

    // Add state
    zdj_debug_panel_state_t * state = calloc( 1, sizeof( zdj_debug_panel_state_t ) );
    container_view->state = state;
    state->debug_view = debug_view;
    state->event_capture = false;
    
    return view;
}

void _zdj_debug_panel_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_debug_panel_state_t * state = (zdj_debug_panel_state_t*)view->state;

    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_BLACK );

    // if( state->event_capture ) {
    //     rectangleColor( zdj_renderer( ), clip->dst.x-1, clip->dst.y-1, clip->dst.x+clip->dst.w+2, clip->dst.y+clip->dst.h+2, ZDJ_WHITE );
    // }
}

void _zdj_debug_panel_handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_control_event_t * e = (zdj_control_event_t *)_event;
    zdj_debug_panel_state_t * state = (zdj_debug_panel_state_t*)view->state;

    // Ignore events which have been blocked by layers above this one.
    if( e->blocked ) { return; }

    // Capture events for deploy/retract and focus/un-focus.
    if( e->id == ZDJ_UI_CONTROL_FN_1_PRESS_1 ) {
        printf( "_zdj_debug_panel_handle_control\n" );
        e->blocked = true;
        if( !state->deployed ) { 
            // Show debug panel
            _zdj_debug_panel_deploy( view );
        } else {
            // Hide debug panel
            _zdj_debug_panel_retract( view );
        }
    } else if( e->id == ZDJ_UI_CONTROL_TONE_1_RELEASE_0 ) {
        // If we're deployed, capture Tone 1 PB release as a toggle on event_capture.
        // (deployed debug menu always captures Tone 1 PB release)
        if( state->deployed ) {
            printf( "_zdj_debug_panel_handle_control\n" );
            e->blocked = true;
            state->event_capture = !state->event_capture;
        }
    }

    // // If we're currently capturing events, grab menu scroll stuff
    // // and pass it down to the subview stack.
    // if( state->event_capture ) {
    //     if( e->id == ZDJ_UI_CONTROL_JOG_ADJUST_0 ) {
    //         state->debug_view->handle_control_event( state->debug_view, e );
    //         e->blocked = true;
    //     } else if( e->id == ZDJ_UI_CONTROL_TONE_1_ADJUST_0 ) {
    //         state->debug_view->handle_control_event( state->debug_view, _event );
    //         e->blocked = true;
    //     } else if( e->id == ZDJ_UI_CONTROL_TONE_2_ADJUST_0 ) {
    //         state->debug_view->handle_control_event( state->debug_view, _event );
    //         e->blocked = true;
    //     }
    // }
}

void _zdj_debug_panel_deploy( zdj_view_t * view ) {
    zdj_debug_panel_state_t * state = (zdj_debug_panel_state_t*)view->state;
    state->event_capture = true;
    state->deployed = true;

    ((anim_init_t)view->in_anim->init_fn)( 
        view->in_anim, 
        view
    );
    view->anim = view->in_anim;
}

void _zdj_debug_panel_retract( zdj_view_t * view ) {
    zdj_debug_panel_state_t * state = (zdj_debug_panel_state_t*)view->state;
    state->event_capture = false;
    state->deployed = false;

    ((anim_init_t)view->out_anim->init_fn)( 
        view->out_anim, 
        view 
    );
    view->anim = view->out_anim;
}
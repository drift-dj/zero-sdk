#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/controls/hmi/zdj_hmi.h>
#include <zerodj/signal/pipeline/perf/zdj_pipeline_perf.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/panel/perf/zdj_perf_panel.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/scroll_view/zdj_scroll_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _zdj_perf_panel_draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_perf_panel_handle_hmi( zdj_view_t * view, void * _event );
static void _zdj_perf_panel_deploy( zdj_view_t * view );
static void _zdj_perf_panel_retract( zdj_view_t * view );

zdj_view_t * zdj_new_perf_panel( void ) {
    zdj_view_t * view = zdj_new_view( zdj_screen_rect( ) );

    // Add a container view for animations/clipping
    zdj_view_t * container_view = zdj_new_view( &(zdj_rect_t){0,0,60,30} );
    zdj_add_subview( view, container_view );
    container_view->type = ZDJ_VIEW_BASE;
    container_view->draw = &_zdj_perf_panel_draw;
    container_view->handle_hmi_event = &_zdj_perf_panel_handle_hmi;

    container_view->frame->x = 60;
    container_view->frame->y = 0;
    
    container_view->in_anim = zdj_new_anim( ZDJ_ANIM_DEBUG_PANEL_SHOW );
    container_view->out_anim = zdj_new_anim( ZDJ_ANIM_DEBUG_PANEL_SHOW );



    // Add state
    zdj_perf_panel_state_t * state = calloc( 1, sizeof( zdj_perf_panel_state_t ) );
    container_view->state = state;
    
    return view;
}

void _zdj_perf_panel_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_perf_panel_state_t * state = (zdj_perf_panel_state_t*)view->state;

    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_BLACK );

    if( state->event_capture ) {
        rectangleColor( zdj_renderer( ), clip->dst.x-1, clip->dst.y-1, clip->dst.x+clip->dst.w+2, clip->dst.y+clip->dst.h+2, ZDJ_WHITE );
    }
}

void _zdj_perf_panel_handle_hmi( zdj_view_t * view, void * _event ) {
    zdj_hmi_event_t * e = (zdj_hmi_event_t *)_event;
    zdj_perf_panel_state_t * state = (zdj_perf_panel_state_t*)view->state;

    // Ignore events which have been blocked by layers above this one.
    if( e->blocked ) { return; }
    
    // Capture events for deploy/retract and focus/un-focus.
    if( e->id == ZDJ_HMI_PB_3_FN_2 && e->type == ZDJ_HMI_EVENT_LONG_PRESS ) {
        e->blocked = true;
        if( !state->deployed ) { 
            // Show debug panel
            _zdj_perf_panel_deploy( view );
        } else {
            // Hide debug panel
            _zdj_perf_panel_retract( view );
        }
    } else if( e->id == ZDJ_HMI_ENCO_3_TONE_1 && e->type == ZDJ_HMI_EVENT_RELEASE ) {
        // If we're deployed, capture Tone 1 PB release as a toggle on event_capture.
        // (deployed debug menu always captures Tone 1 PB release)
        if( state->deployed ) {
            e->blocked = true;
            state->event_capture = !state->event_capture;
        }
    }

    // If we're currently capturing events, grab menu scroll stuff
    // and pass it down to the subview stack.
    if( state->event_capture ) {
        if( e->id == ZDJ_HMI_ENCO_2_JOG ) {
            if( e->type == ZDJ_HMI_EVENT_ADJUST ) {
                state->log_view->handle_hmi_event( state->log_view, e );
                e->blocked = true;
            }
        } else if( e->id == ZDJ_HMI_ENCO_3_TONE_1) {
            if( e->type == ZDJ_HMI_EVENT_ADJUST ) {
                state->log_view->handle_hmi_event( state->log_view, _event );
                e->blocked = true;
            }
        } else if( e->id == ZDJ_HMI_ENCO_4_TONE_2 ) {
            if( e->type == ZDJ_HMI_EVENT_ADJUST ) {
                state->log_view->handle_hmi_event( state->log_view, _event );
                e->blocked = true;
            }
        }
    }
}

void _zdj_perf_panel_deploy( zdj_view_t * view ) {
    zdj_perf_panel_state_t * state = (zdj_perf_panel_state_t*)view->state;
    state->event_capture = true;
    state->deployed = true;

    ((anim_init_t)view->in_anim->init_fn)( 
        view->in_anim, 
        view
    );
    view->anim = view->in_anim;
}

void _zdj_perf_panel_retract( zdj_view_t * view ) {
    zdj_perf_panel_state_t * state = (zdj_perf_panel_state_t*)view->state;
    state->event_capture = false;
    state->deployed = false;

    ((anim_init_t)view->out_anim->init_fn)( 
        view->out_anim, 
        view 
    );
    view->anim = view->out_anim;
}
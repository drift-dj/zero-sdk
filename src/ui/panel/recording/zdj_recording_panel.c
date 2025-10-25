#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/signal/deck/zdj_deck_manager.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/panel/recording/zdj_recording_panel.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/log_view/zdj_log_view.h>
#include <zerodj/ui/view/scroll_view/zdj_scroll_view.h>
#include <zerodj/ui/view/soundcard_view/zdj_soundcard_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _handle_control( zdj_view_t * view, zdj_control_event_t * event );
static void _init_ui( zdj_view_t * view, zdj_recording_panel_state_t * panel_state );

static void _deploy( zdj_view_t * view );
static void _retract( zdj_view_t * view );

zdj_view_t * zdj_new_recording_panel( void ) {
    // printf( "zdj_new_recording_panel\n" );
    zdj_view_t * view = zdj_new_view( zdj_screen_rect( ) );
    view->type = ZDJ_VIEW_BASE;
    // view->draw = &_draw;

    // Add a container view for animations/clipping
    zdj_view_t * container_view = zdj_new_view( &(zdj_rect_t){ ZDJ_SCREEN_W-40, -7, 40, 6 } );
    zdj_add_subview( view, container_view );
    container_view->type = ZDJ_VIEW_BASE;
    container_view->draw = &_draw;
    container_view->handle_control_event = &_handle_control;

    container_view->frame.x = ZDJ_SCREEN_W-24;
    container_view->frame.y = -7;

    zdj_set_anim( &container_view->in_anim, ZDJ_ANIM_RECORD_PANEL_SHOW );
    zdj_set_anim( &container_view->out_anim, ZDJ_ANIM_RECORD_PANEL_HIDE );

    // Add state
    zdj_recording_panel_state_t * state = calloc( 1, sizeof( zdj_recording_panel_state_t ) );
    container_view->state = state;
    state->ui_init = false;
    state->deploy_timer = 0;
    
    // printf( "zdj_new_recording_panel done\n" );
    return view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_recording_panel_state_t * state = (zdj_recording_panel_state_t*)view->state;
    // Draw box and border
    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_BLACK );

    // This doesn't work since draw isn't called when we're offscreen
    // if( zdj_deck_manager( )->control_change_flags[ ZDJ_DECK_CONTROL_TOGGLE_RECORD ] ) {
    //     zdj_deck_manager( )->control_change_flags[ ZDJ_DECK_CONTROL_TOGGLE_RECORD ] = false;
    //     if( state->deploy_state == ZDJ_RECORD_PANEL_RETRACTED ) { 
    //         printf( "showing mini-meter\n" );
    //         // Show meter view if nothing is on-screen
    //         _deploy( view );
    //         state->deploy_state = ZDJ_RECORD_PANEL_MINI_METER;
    //     }
    //     state->deploy_timer = 0;
    // }
}

static void _handle_control( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_recording_panel_state_t * state = (zdj_recording_panel_state_t*)view->state;

    // Ignore events which have been blocked by layers above this one.
    if( event->blocked ) { return; }
    
    // Capture events for deploy/retract and focus/un-focus.
    if( event->id == ZDJ_UI_CONTROL_FN_1_RELEASE_2 ) {
        printf( "Recording Panel Deploy Toggle\n" );
        event->blocked = true;
        switch ( state->deploy_state ) {
        case ZDJ_RECORD_PANEL_RETRACTED:
            _deploy( view );
            state->deploy_state = ZDJ_RECORD_PANEL_MINI_METER;
            break;
        case ZDJ_RECORD_PANEL_MINI_METER:
            _retract( view );
            // Show soundcard recording page
            state->deploy_state = ZDJ_RECORD_PANEL_OPTIONS_VIEW;
            break;
        case ZDJ_RECORD_PANEL_OPTIONS_VIEW:
            // Hide soundcard recording page
            state->deploy_state = ZDJ_RECORD_PANEL_RETRACTED;
            break;
        
        default:
            break;
        }
    } 
}

static void _deploy( zdj_view_t * view ) {
    zdj_recording_panel_state_t * state = (zdj_recording_panel_state_t*)view->state;

    // Lazy-load the meter
    _init_ui( view, state );
    if( !state->ui_init ){ return; }

    ((anim_init_t)view->in_anim.init_fn)( 
        &view->in_anim, 
        view
    );
    view->anim = &view->in_anim;
}

static void _retract( zdj_view_t * view ) {
    zdj_recording_panel_state_t * state = (zdj_recording_panel_state_t*)view->state;

    ((anim_init_t)view->out_anim.init_fn)( 
        &view->out_anim, 
        view 
    );
    view->anim = &view->out_anim;
}

// Lazy load at deploy to avoid a circular dependency between soundcard and ui
static void _init_ui( zdj_view_t * view, zdj_recording_panel_state_t * panel_state ) {
    // If soundcard isn't up yet, cancel the UI bringup
    if( !zdj_soundcard || panel_state->ui_init ){ return; }

    printf( "init_ui\n" );

    // Add recording bus meter
    panel_state->meter = zdj_new_record_mini_meter_view( );
    panel_state->meter->frame.x = 2;
    panel_state->meter->frame.y = 1;

    zdj_add_subview( view, panel_state->meter );
    panel_state->ui_init = true;
}
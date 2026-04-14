#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/perf/zdj_perf.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/thread_view/zdj_thread_view.h>
#include <zerodj/ui/view/scroll_view/zdj_scroll_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>
#include <zerodj/ui/widget/perf/zdj_perf_widget.h>



static void _toggle( zdj_view_t * view );
static void _deploy( zdj_view_t * view );
static void _retract( zdj_view_t * view );

static void _draw_container( zdj_view_t * view, zdj_view_clip_t * clip );

static void _put_mem( char * str );

zdj_view_t * zdj_new_perf_widget( void ) {
    // printf( "zdj_new_volume_widget\n" );
    zdj_view_t * view = zdj_new_view( zdj_screen_rect( ) );
    view->type = ZDJ_VIEW_BASE;

    // Add a container view for animations/clipping
    zdj_view_t * container_view = zdj_new_view( &(zdj_rect_t){ 0, ZDJ_SCREEN_H+2, 64, 11 } );
    zdj_add_subview( view, container_view );
    container_view->type = ZDJ_VIEW_BASE;
    container_view->draw = &_draw_container;

    zdj_set_anim( &container_view->in_anim, ZDJ_ANIM_PERF_WIDGET_SHOW );
    zdj_set_anim( &container_view->out_anim, ZDJ_ANIM_PERF_WIDGET_HIDE );

    // Add state
    zdj_perf_widget_state_t * state = calloc( 1, sizeof( zdj_perf_widget_state_t ) );
    view->state = state;
    container_view->state = state;
    state->container = container_view;
    state->update_counter = 0;
    state->toggle = &_toggle;
    
    return view;
}

static void _draw_container( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_perf_widget_state_t * state = (zdj_perf_widget_state_t*)view->state;
    // Draw box and border
    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_BLACK );

    if( state->update_counter++ > 30 ) {
        state->update_counter = 0;
        if( state->perf_label ){ zdj_remove_subview_of( view, state->perf_label ); }
        char str[ 64 ];
        
        // Get hz for control thread update
        uint64_t time = zdj_perf_time( ) - state->prev_time;
        state->prev_time = zdj_perf_time( );
        double dur = (double)time / (double)zdj_control_cycle_counter;
        double hz = 1000000000.0 / dur;
        sprintf( str, "%d/%1.4f Hz", zdj_control_cycle_counter, hz );
        zdj_control_cycle_reset = true;

        state->perf_label = zdj_new_label_view( str, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
        zdj_add_subview( view, state->perf_label );
    }
}

static void _toggle( zdj_view_t * view ) {
    zdj_perf_widget_state_t * state = (zdj_perf_widget_state_t*)view->state;
    if( state->deployed ) {
        _retract( view );
    } else {
        _deploy( view );
    }
}

static void _deploy( zdj_view_t * view ) {
    zdj_perf_widget_state_t * state = (zdj_perf_widget_state_t*)view->state;
    state->deployed = true;
    // printf( "debug deploy: %p\n", state->container );

    zdj_control_cycle_reset = true;

    ((anim_init_t)state->container->in_anim.init_fn)( 
        &state->container->in_anim, 
        state->container
    );
    state->container->anim = &state->container->in_anim;

    // printf( "debug deploy done\n" );
}

static void _retract( zdj_view_t * view ) {
    // printf( "debug retract\n" );
    zdj_perf_widget_state_t * state = (zdj_perf_widget_state_t*)view->state;
    state->deployed = false;

    ((anim_init_t)state->container->out_anim.init_fn)( 
        &state->container->out_anim, 
        state->container 
    );
    state->container->anim = &state->container->out_anim;
}

// static void _put_mem( char * str ) {
//     // Build mem size string
//     char cmd[ 256 ];
//     char mem[ 256 ];
//     snprintf( cmd, sizeof( cmd ), "cat /proc/%d/status | grep 'VmSize*'", getpid( ) );
//     char p_res[ 256 ];
//     zdj_fs_get_popen( cmd, p_res );
//     if( strlen( p_res ) < 1 ) { return; }
//     strcpy( str, &p_res[ 8 ] );
// }


// zdj_perf_widget_state_t * _zdj_perf_widget_state;

// static void _zdj_perf_widget_draw( zdj_view_t * view, zdj_view_clip_t * clip );
// static void _zdj_perf_widget_handle_control( zdj_view_t * view, zdj_control_event_t * _event );
// static void _zdj_perf_widget_deploy( zdj_view_t * view );
// static void _zdj_perf_widget_retract( zdj_view_t * view );

// zdj_view_t * zdj_new_perf_widget( void ) {

//     // Init perf here for now
//     zdj_perf_init( 3000 );

//     zdj_view_t * view = zdj_new_view( zdj_screen_rect( ) );
//     view->frame.w = ZDJ_PERF_PANEL_WIDTH;
//     view->frame.h = ZDJ_PERF_PANEL_HEIGHT;

//     // Add a container view for animations/clipping
//     zdj_view_t * container_view = zdj_new_view( zdj_perf_widget_rect( ) );
//     zdj_add_subview( view, container_view );
//     container_view->type = ZDJ_VIEW_BASE;
//     container_view->draw = &_zdj_perf_widget_draw;
//     container_view->handle_control_event = &_zdj_perf_widget_handle_control;

//     container_view->frame.w = ZDJ_PERF_PANEL_WIDTH;
//     container_view->frame.h = ZDJ_PERF_PANEL_HEIGHT;
//     container_view->frame.x = ZDJ_PERF_PANEL_WIDTH * -3;
//     container_view->frame.y = 0;
    
//     zdj_set_anim( &container_view->in_anim, ZDJ_ANIM_DEBUG_PANEL_SHOW );
//     zdj_set_anim( &container_view->out_anim, ZDJ_ANIM_DEBUG_PANEL_HIDE );

//     zdj_view_t * thread_view = zdj_new_thread_view( zdj_perf_widget_rect( ) );
//     zdj_add_subview( container_view, thread_view );

//     // Add state
//     _zdj_perf_widget_state = calloc( 1, sizeof( zdj_perf_widget_state_t ) );
//     container_view->state = _zdj_perf_widget_state;
//     _zdj_perf_widget_state->container_view = container_view;
//     _zdj_perf_widget_state->thread_view = thread_view;
//     _zdj_perf_widget_state->event_capture = false;
    
//     return view;
// }

// void _zdj_perf_widget_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
//     zdj_perf_widget_state_t * state = (zdj_perf_widget_state_t*)view->state;

//     boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_BLACK );

// }

// void _zdj_perf_widget_handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {
//     zdj_control_event_t * e = (zdj_control_event_t *)_event;
//     zdj_perf_widget_state_t * state = (zdj_perf_widget_state_t*)view->state;
//     // Ignore events which have been blocked by layers above this one.
//     if( e->blocked ) { return; }
    
//     // // Capture events for deploy/retract and focus/un-focus.
//     // if( e->id == ZDJ_UI_CONTROL_FN_2_PRESS_1 ) {
//     //     e->blocked = true;
//     //     zdj_perf_widget_toggle( );
//     // } 
// }

// void zdj_perf_widget_toggle( void ) {
//     printf( "zdj_perf_widget_toggle\n" );
//     if( _zdj_perf_widget_state->deployed ) {
//         zdj_perf_widget_retract( );
//     } else {
//         zdj_perf_widget_deploy( );
//     }
// }

// void zdj_perf_widget_deploy( void ) {
//     _zdj_perf_widget_state->event_capture = true;
//     _zdj_perf_widget_state->deployed = true;

//     zdj_enable_perf( );

//     ((anim_init_t)_zdj_perf_widget_state->container_view->in_anim.init_fn)( 
//         &_zdj_perf_widget_state->container_view->in_anim, 
//         _zdj_perf_widget_state->container_view
//     );
//     _zdj_perf_widget_state->container_view->anim = &_zdj_perf_widget_state->container_view->in_anim;
// }

// void zdj_perf_widget_retract( void ) {
//     _zdj_perf_widget_state->event_capture = false;
//     _zdj_perf_widget_state->deployed = false;

//     zdj_disable_perf( );

//     ((anim_init_t)_zdj_perf_widget_state->container_view->out_anim.init_fn)( 
//         &_zdj_perf_widget_state->container_view->out_anim, 
//         _zdj_perf_widget_state->container_view
//     );
//     _zdj_perf_widget_state->container_view->anim = &_zdj_perf_widget_state->container_view->out_anim;
// }
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
#include <zerodj/ui/widget/notify/zdj_notify_widget.h>

zdj_notify_widget_state_t * _zdj_notify_widget_state; 

static void _toggle( zdj_view_t * view );
static void _deploy( zdj_view_t * view );
static void _retract( zdj_view_t * view );

static void _draw_container( zdj_view_t * view, zdj_view_clip_t * clip );

static void _put_mem( char * str );

zdj_view_t * zdj_new_notify_widget( void ) {
    // printf( "zdj_new_volume_widget\n" );
    zdj_view_t * view = zdj_new_view( zdj_screen_rect( ) );
    view->type = ZDJ_VIEW_BASE;

    // Add a container view for animations/clipping
    zdj_view_t * container_view = zdj_new_view( &(zdj_rect_t){ ZDJ_SCREEN_W-50, ZDJ_SCREEN_H+2, 50, 11 } );
    zdj_add_subview( view, container_view );
    container_view->type = ZDJ_VIEW_BASE;
    container_view->draw = &_draw_container;

    zdj_set_anim( &container_view->in_anim, ZDJ_ANIM_NOTIFY_WIDGET_SHOW );
    zdj_set_anim( &container_view->out_anim, ZDJ_ANIM_NOTIFY_WIDGET_HIDE );

    // Add state
    _zdj_notify_widget_state = calloc( 1, sizeof( zdj_notify_widget_state_t ) );
    view->state = _zdj_notify_widget_state;
    container_view->state = _zdj_notify_widget_state;
    _zdj_notify_widget_state->container = container_view;
    _zdj_notify_widget_state->toggle = &_toggle;
    
    return view;
}

void zdj_show_notify_widget( int line_count, char * line_1, char * line_2, char * line_3 ) {
    if( !_zdj_notify_widget_state->deployed ) {
        // Set up labels
        // Deploy
        // _deploy( perf_widget );
    }
}

static void _draw_container( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // Draw box and border
    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_BLACK );
}

static void _toggle( zdj_view_t * view ) {
    if( _zdj_notify_widget_state->deployed ) {
        _retract( view );
    } else {
        _deploy( view );
    }
}

static void _deploy( zdj_view_t * view ) {
    _zdj_notify_widget_state->deployed = true;
    // printf( "debug deploy: %p\n", state->container );

    ((anim_init_t)_zdj_notify_widget_state->container->in_anim.init_fn)( 
        &_zdj_notify_widget_state->container->in_anim, 
        _zdj_notify_widget_state->container
    );
    _zdj_notify_widget_state->container->anim = &_zdj_notify_widget_state->container->in_anim;

    // printf( "debug deploy done\n" );
}

static void _retract( zdj_view_t * view ) {
    // printf( "debug retract\n" );
    _zdj_notify_widget_state->deployed = false;

    ((anim_init_t)_zdj_notify_widget_state->container->out_anim.init_fn)( 
        &_zdj_notify_widget_state->container->out_anim, 
        _zdj_notify_widget_state->container 
    );
    _zdj_notify_widget_state->container->anim = &_zdj_notify_widget_state->container->out_anim;
}

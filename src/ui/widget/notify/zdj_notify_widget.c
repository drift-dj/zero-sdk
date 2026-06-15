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
#include <zerodj/ui/widget/zdj_ui_widget.h>
#include <zerodj/ui/widget/notify/zdj_notify_widget.h>

zdj_notify_widget_state_t * _zdj_notify_widget_state; 

static void _deploy( zdj_view_t * view );
static void _retract( zdj_view_t * view );

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _draw_container( zdj_view_t * view, zdj_view_clip_t * clip );


zdj_view_t * zdj_new_notify_widget( void ) {
    // printf( "zdj_new_volume_widget\n" );
    zdj_view_t * view = zdj_new_view( zdj_screen_rect( ) );
    view->type = ZDJ_VIEW_BASE;
    view->draw = &_draw;

    // Add a container view for animations/clipping
    // zdj_view_t * container_view = zdj_new_view( &(zdj_rect_t){ ZDJ_SCREEN_W-50, ZDJ_SCREEN_H+2, 50, 11 } );
    zdj_view_t * container_view = zdj_new_view( &(zdj_rect_t){ 0, ZDJ_SCREEN_H+2, 50, 11 } );
    zdj_add_subview( view, container_view );
    container_view->type = ZDJ_VIEW_BASE;
    container_view->draw = &_draw_container;

    zdj_set_anim( &container_view->in_anim, ZDJ_ANIM_NOTIFY_WIDGET_SHOW );
    zdj_set_anim( &container_view->out_anim, ZDJ_ANIM_NOTIFY_WIDGET_HIDE );

    // Add state
    _zdj_notify_widget_state = calloc( 1, sizeof( zdj_notify_widget_state_t ) );
    view->state = _zdj_notify_widget_state;
    _zdj_notify_widget_state->container = container_view;
    
    return view;
}

void zdj_show_notify_widget( char * line_1, char * line_2, char * line_3 ) {
    // printf( "zdj_show_notify_widget: %s\n", line_1 );
    zdj_view_t * view = zdj_ui_get_notify_widget( );
    if( !view ){ return; }
    zdj_notify_widget_state_t * state = _zdj_notify_widget_state;
    if( !_zdj_notify_widget_state->deployed ) {
        if( state->label_1 ){ zdj_remove_subview_of( _zdj_notify_widget_state->container, state->label_1 ); }
        if( state->label_2 ){ zdj_remove_subview_of( _zdj_notify_widget_state->container, state->label_2 ); }
        if( state->label_3 ){ zdj_remove_subview_of( _zdj_notify_widget_state->container, state->label_3 ); }
        state->w = 0;
        state->h = 3;

        // Set up labels
        if( line_1 ) {
            state->label_1 = zdj_new_label_view( line_1, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
            state->w = fmax( state->w, state->label_1->frame.w );
            state->h = state->h + 7;
            zdj_add_subview( _zdj_notify_widget_state->container, state->label_1 );
        }
        if( line_2 ) {
            state->label_2 = zdj_new_label_view( line_2, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
            state->w = fmax( state->w, state->label_2->frame.w );
            state->h = state->h + 7;
            zdj_add_subview( _zdj_notify_widget_state->container, state->label_2 );
        }
        if( line_3 ) {
            state->label_3 = zdj_new_label_view( line_3, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
            state->w = fmax( state->w, state->label_3->frame.w );
            state->h = state->h + 7;
            zdj_add_subview( _zdj_notify_widget_state->container, state->label_3 );
        }

        _zdj_notify_widget_state->container->frame.w = state->w;
        _zdj_notify_widget_state->container->frame.h = state->h;

        // Deploy
        _deploy( _zdj_notify_widget_state->container );
    }
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // printf( "note widg _draw\n" );
    if( _zdj_notify_widget_state->deployed &&
        _zdj_notify_widget_state->deploy_counter++ > 100 
    ) {
        _retract( _zdj_notify_widget_state->container );
    }
}

static void _draw_container( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // Draw box and border
    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_BLACK );
}

static void _deploy( zdj_view_t * view ) {
    if( !view ){ return; }
    _zdj_notify_widget_state->deployed = true;
    _zdj_notify_widget_state->deploy_counter = 0;
    // printf( "notify deploy: %p\n", _zdj_notify_widget_state->container );

    ((anim_init_t)_zdj_notify_widget_state->container->in_anim.init_fn)( 
        &_zdj_notify_widget_state->container->in_anim, 
        _zdj_notify_widget_state->container
    );
    _zdj_notify_widget_state->container->anim = &_zdj_notify_widget_state->container->in_anim;

    // printf( "debug deploy done\n" );
}

static void _retract( zdj_view_t * view ) {
    // printf( "notify retract: %p\n", view );
    if( !view ){ return; }
    _zdj_notify_widget_state->deployed = false;

    ((anim_init_t)_zdj_notify_widget_state->container->out_anim.init_fn)( 
        &_zdj_notify_widget_state->container->out_anim, 
        _zdj_notify_widget_state->container 
    );
    _zdj_notify_widget_state->container->anim = &_zdj_notify_widget_state->container->out_anim;
}

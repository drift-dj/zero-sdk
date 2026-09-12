#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/system/log/zdj_log.h>
#include <zerodj/system/settings/zdj_settings.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/widget/sleep/zdj_sleep_widget.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _toggle( zdj_view_t * view );
static void _deploy( zdj_view_t * view );
static void _retract( zdj_view_t * view );

static void _draw_container( zdj_view_t * view, zdj_view_clip_t * clip );

zdj_view_t * zdj_new_log_widget( void ) {
    // printf( "zdj_new_volume_widget\n" );
    zdj_view_t * view = zdj_new_view( zdj_screen_rect( ) );
    view->type = ZDJ_VIEW_BASE;

    // Add a container view for animations/clipping
    zdj_view_t * container_view = zdj_new_view( &(zdj_rect_t){ 10, ZDJ_SCREEN_H + 2, 108, 38 } );
    zdj_add_subview( view, container_view );
    container_view->type = ZDJ_VIEW_BASE;
    container_view->draw = &_draw_container;

    zdj_set_anim( &container_view->in_anim, ZDJ_ANIM_SLEEP_WIDGET_SHOW );
    zdj_set_anim( &container_view->out_anim, ZDJ_ANIM_SLEEP_WIDGET_HIDE );

    // Add state
    zdj_sleep_widget_state_t * state = calloc( 1, sizeof( zdj_sleep_widget_state_t ) );
    view->state = state;
    container_view->state = state;
    state->container = container_view;
    state->update_counter = 0;
    state->toggle = &_toggle;

    return view;
}

static void _draw_container( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_sleep_widget_state_t * state = (zdj_sleep_widget_state_t*)view->state;
    // Draw box and border
    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_BLACK );
}

static void _toggle( zdj_view_t * view ) {
    // printf( "Log Widget Toggle\n" );
    zdj_sleep_widget_state_t * state = (zdj_sleep_widget_state_t*)view->state;
    if( state->deployed ) {
        _retract( view );
        zdj_setting_set_crash_flag( false );
    } else {
        _deploy( view );
    }
}

static void _deploy( zdj_view_t * view ) {
    // printf( "Log Widget Deploy\n" );
    zdj_sleep_widget_state_t * state = (zdj_sleep_widget_state_t*)view->state;
    state->deployed = true;
    // printf( "debug deploy: %p\n", state->container );

    ((anim_init_t)state->container->in_anim.init_fn)( 
        &state->container->in_anim, 
        state->container
    );
    state->container->anim = &state->container->in_anim;
}

static void _retract( zdj_view_t * view ) {
    // printf( "log retract\n" );
    zdj_sleep_widget_state_t * state = (zdj_sleep_widget_state_t*)view->state;
    state->deployed = false;

    ((anim_init_t)state->container->out_anim.init_fn)( 
        &state->container->out_anim, 
        state->container 
    );
    state->container->anim = &state->container->out_anim;
}

bool zdj_log_widget_append_line( char * line ) {
    // If log widget is deployed, add a log line to the display
}
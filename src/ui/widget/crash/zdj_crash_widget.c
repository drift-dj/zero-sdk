#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/system/settings/zdj_settings.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/widget/crash/zdj_crash_widget.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _toggle( zdj_view_t * view );
static void _deploy( zdj_view_t * view );
static void _retract( zdj_view_t * view );

static void _draw_container( zdj_view_t * view, zdj_view_clip_t * clip );

zdj_view_t * zdj_new_crash_widget( void ) {
    // printf( "zdj_new_volume_widget\n" );
    zdj_view_t * view = zdj_new_view( zdj_screen_rect( ) );
    view->type = ZDJ_VIEW_BASE;

    // Add a container view for animations/clipping
    zdj_view_t * container_view = zdj_new_view( &(zdj_rect_t){ 10, ZDJ_SCREEN_H + 2, 108, 38 } );
    zdj_add_subview( view, container_view );
    container_view->type = ZDJ_VIEW_BASE;
    container_view->draw = &_draw_container;

    zdj_set_anim( &container_view->in_anim, ZDJ_ANIM_CRASH_WIDGET_SHOW );
    zdj_set_anim( &container_view->out_anim, ZDJ_ANIM_CRASH_WIDGET_HIDE );

    // Add state
    zdj_crash_widget_state_t * state = calloc( 1, sizeof( zdj_crash_widget_state_t ) );
    view->state = state;
    container_view->state = state;
    state->container = container_view;
    state->update_counter = 0;
    state->toggle = &_toggle;
    
    // Add crash header
    zdj_view_t * border = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_SM_HATCH_TEX ], NULL );
    border->frame.x = 1;
    border->frame.y = 1;
    border->frame.w = container_view->frame.w - 1;
    border->frame.h = 15;
    zdj_add_subview( container_view, border );
    zdj_view_t * bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BLACK ], NULL );
    bg->frame.x = 3;
    bg->frame.y = 3;
    bg->frame.w = container_view->frame.w - 5;
    bg->frame.h = 11;
    zdj_add_subview( container_view, bg );
    zdj_view_t * skull = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_SKULL_ALERT ], NULL );
    skull->frame.x = 4;
    skull->frame.y = 4;
    zdj_add_subview( container_view, skull );

    char title_str[ 128 ];
    sprintf( title_str, "CRASH LOG %03d", zdj_cur_log_num( ZDJ_LOG_TYPE_CRASH ) );
    zdj_view_t * title = zdj_new_label_view( title_str, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    title->frame.x = 17;
    title->frame.y = 4;
    zdj_add_subview( container_view, title );
    
    // Add top 3 lines from log
    char line_1[ 512 ];
    char line_2[ 512 ];
    char line_3[ 512 ];
    zdj_put_cur_log( ZDJ_LOG_TYPE_CRASH, line_1, line_2, line_3 );
    zdj_view_t * line_1_label = zdj_new_ticker_view( line_1, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    line_1_label->frame.x = 2;
    line_1_label->frame.y = 17;
    line_1_label->frame.w = container_view->frame.w - 2;
    line_1_label->frame.h = 8;
    zdj_add_subview( container_view, line_1_label );
    zdj_view_t * line_2_label = zdj_new_ticker_view( line_2, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    line_2_label->frame.x = 2;
    line_2_label->frame.y = 24;
    line_2_label->frame.w = container_view->frame.w - 2;
    line_2_label->frame.h = 8;
    zdj_add_subview( container_view, line_2_label );
    zdj_view_t * line_3_label = zdj_new_ticker_view( line_3, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    line_3_label->frame.x = 2;
    line_3_label->frame.y = 31;
    line_3_label->frame.w = container_view->frame.w - 2;
    line_3_label->frame.h = 8;
    zdj_add_subview( container_view, line_3_label );

    return view;
}

static void _draw_container( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_crash_widget_state_t * state = (zdj_crash_widget_state_t*)view->state;
    // Draw box and border
    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_BLACK );
}

static void _toggle( zdj_view_t * view ) {
    // printf( "Crash Widget Toggle\n" );
    zdj_crash_widget_state_t * state = (zdj_crash_widget_state_t*)view->state;
    if( state->deployed ) {
        _retract( view );
        zdj_setting_set_crash_flag( false );
    } else {
        _deploy( view );
    }
}

static void _deploy( zdj_view_t * view ) {
    // printf( "Crash Widget Deploy\n" );
    zdj_crash_widget_state_t * state = (zdj_crash_widget_state_t*)view->state;
    state->deployed = true;
    // printf( "debug deploy: %p\n", state->container );

    ((anim_init_t)state->container->in_anim.init_fn)( 
        &state->container->in_anim, 
        state->container
    );
    state->container->anim = &state->container->in_anim;
}

static void _retract( zdj_view_t * view ) {
    // printf( "debug retract\n" );
    zdj_crash_widget_state_t * state = (zdj_crash_widget_state_t*)view->state;
    state->deployed = false;

    ((anim_init_t)state->container->out_anim.init_fn)( 
        &state->container->out_anim, 
        state->container 
    );
    state->container->anim = &state->container->out_anim;
}

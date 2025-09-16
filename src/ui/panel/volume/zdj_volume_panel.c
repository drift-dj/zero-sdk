#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/signal/deck/zdj_deck_controls.h>
#include <zerodj/signal/deck/zdj_deck_manager.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/panel/volume/zdj_volume_panel.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/log_view/zdj_log_view.h>
#include <zerodj/ui/view/scroll_view/zdj_scroll_view.h>
#include <zerodj/ui/view/soundcard_view/zdj_soundcard_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _deploy( zdj_view_t * view, zdj_volume_panel_state_t * panel_state );
static void _retract( zdj_view_t * view, zdj_volume_panel_state_t * panel_state );

static void _draw_container( zdj_view_t * view, zdj_view_clip_t * clip );

static void _init_ui( zdj_view_t * view, zdj_volume_panel_state_t * panel_state );

zdj_view_t * zdj_new_volume_panel( void ) {
    // printf( "zdj_new_volume_panel\n" );
    zdj_view_t * view = zdj_new_view( zdj_screen_rect( ) );
    view->type = ZDJ_VIEW_BASE;
    view->draw = &_draw;

    // Add a container view for animations/clipping
    zdj_view_t * container_view = zdj_new_view( &(zdj_rect_t){ ZDJ_SCREEN_W+3, 5, 14, 48 } );
    zdj_add_subview( view, container_view );
    container_view->type = ZDJ_VIEW_BASE;
    container_view->draw = &_draw_container;

    container_view->frame.x = ZDJ_SCREEN_W+3;
    container_view->frame.y = 5;

    zdj_set_anim( &container_view->in_anim, ZDJ_ANIM_VOLUME_PANEL_SHOW );
    zdj_set_anim( &container_view->out_anim, ZDJ_ANIM_VOLUME_PANEL_HIDE );

    // Add state
    zdj_volume_panel_state_t * state = calloc( 1, sizeof( zdj_volume_panel_state_t ) );
    view->state = state;
    state->ui_init = false;
    state->deploy_timer = 0;
    state->container = container_view;
    
    // printf( "zdj_new_volume_panel done\n" );
    return view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_volume_panel_state_t * state = (zdj_volume_panel_state_t*)view->state;

    // Check for a change in the main volume knob and restart the timer
    if( zdj_deck_manager( )->control_change_flags[ ZDJ_DECK_CONTROL_LR_VOL ] ) {
        printf( "vol change\n" );
        zdj_deck_manager( )->control_change_flags[ ZDJ_DECK_CONTROL_LR_VOL ] = false;
        if( !state->deployed ) { _deploy( state->container, state ); }
        state->deploy_timer = 0;
    }

    if( state->deploy_timer < 100 ) {
        state->deploy_timer++;
    } else if( state->deploy_timer == 100 ) {
        state->deploy_timer++;
        if( state->deployed ){ _retract( state->container, state ); }
    }
}

static void _draw_container( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // Draw box and border
    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_BLACK );
}

static void _deploy( zdj_view_t * view, zdj_volume_panel_state_t * panel_state ) {
    printf( "volume deploy\n" );

    // Lazy-load the meter
    _init_ui( view, panel_state );
    if( !panel_state->ui_init ){ return; }

    panel_state->deployed = true;

    ((anim_init_t)view->in_anim.init_fn)( 
        &view->in_anim, 
        view
    );
    view->anim = &view->in_anim;
}

static void _retract( zdj_view_t * view, zdj_volume_panel_state_t * panel_state ) {
    printf( "volume retract\n" );
    panel_state->deployed = false;

    ((anim_init_t)view->out_anim.init_fn)( 
        &view->out_anim, 
        view 
    );
    view->anim = &view->out_anim;
}

// Lazy load at deploy to avoid a circular dependency between soundcard and ui
static void _init_ui( zdj_view_t * view, zdj_volume_panel_state_t * panel_state ) {
    // If soundcard isn't up yet, cancel the UI bringup
    if( !zdj_soundcard || panel_state->ui_init ){ return; }

    printf( "init_ui\n" );

    // Add LR bus meter
    zdj_soundcard_node_t * lr_node = zdj_soundcard_get_node_for_name( 
        zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS 
    );
    panel_state->meter = zdj_new_audio_stereo_meter_view( 
        lr_node, 
        ZDJ_SOUNDCARD_LABEL_MAIN_BUS, 
        false 
    );
    panel_state->meter->frame.x = 2;
    panel_state->meter->frame.y = 2;

    zdj_soundcard_node_config_context_t * context = zdj_soundcard_new_node_config_context( );
    context->soundcard = zdj_soundcard;
    context->node = lr_node;
    zdj_soundcard_meter_state_t * meter_state = (zdj_soundcard_meter_state_t*)panel_state->meter->state;
    meter_state->config_context = context;
    meter_state->is_hilite = true;

    zdj_add_subview( panel_state->container, panel_state->meter );
    panel_state->ui_init = true;
}
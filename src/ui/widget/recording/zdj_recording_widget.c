#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/signal/deck/zdj_deck_manager.h>
#include <zerodj/signal/pipeline/node/audio/record/zdj_audio_record_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/widget/recording/zdj_recording_widget.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/log_view/zdj_log_view.h>
#include <zerodj/ui/view/scroll_view/zdj_scroll_view.h>
#include <zerodj/ui/panel/soundcard/zdj_soundcard_panel.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _deploy( zdj_view_t * view, zdj_recording_widget_state_t * widget_state );
static void _retract( zdj_view_t * view, zdj_recording_widget_state_t * widget_state );

static void _draw_container( zdj_view_t * view, zdj_view_clip_t * clip );

static void _init_ui( zdj_view_t * view, zdj_recording_widget_state_t * widget_state );

zdj_view_t * zdj_new_recording_widget( void ) {
    // printf( "zdj_new_volume_widget\n" );
    zdj_view_t * view = zdj_new_view( zdj_screen_rect( ) );
    view->type = ZDJ_VIEW_BASE;
    view->draw = &_draw;

    // Add a container view for animations/clipping
    zdj_view_t * container_view = zdj_new_view( &(zdj_rect_t){ ZDJ_SCREEN_W-34, 1, 34, 6 } );
    zdj_add_subview( view, container_view );
    container_view->type = ZDJ_VIEW_BASE;
    container_view->draw = &_draw_container;

    container_view->frame.x = ZDJ_SCREEN_W+3;

    zdj_set_anim( &container_view->in_anim, ZDJ_ANIM_RECORD_WIDGET_SHOW );
    zdj_set_anim( &container_view->out_anim, ZDJ_ANIM_RECORD_WIDGET_HIDE );

    // Add state
    zdj_recording_widget_state_t * state = calloc( 1, sizeof( zdj_recording_widget_state_t ) );
    view->state = state;
    state->ui_init = false;
    state->deploy_timer = 0;
    state->container = container_view;
    
    // printf( "zdj_new_volume_widget done\n" );
    return view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_recording_widget_state_t * state = (zdj_recording_widget_state_t*)view->state;

    if( state->needs_soundcard_update ) {
        printf( "updating recording widget\n" );
        if( state->container->subviews ) { zdj_remove_all_subviews_of( state->container ); }
        state->ui_init = false;
        state->needs_soundcard_update = false;
    }
    
    zdj_audio_record_node_state_t * recording_state = (zdj_audio_record_node_state_t*)zdj_soundcard->recording_node->state;

    
    if( recording_state->status == ZDJ_AUDIO_RECORD_ACTIVE && !state->deployed ) {
        _deploy( state->container, state );
        state->deploy_timer = 0; // Set widget up to hide when recording stops
    } else if( recording_state->status != ZDJ_AUDIO_RECORD_ACTIVE ) {
        // Check for a change in the record volume knob and restart the timer
        if( zdj_deck_manager( )->control_change_flags[ ZDJ_DECK_CONTROL_RECORD_VOL ] ) {
            // printf( "vol change\n" );
            zdj_deck_manager( )->control_change_flags[ ZDJ_DECK_CONTROL_RECORD_VOL ] = false;
            if( !state->deployed ) { _deploy( state->container, state ); }
            state->deploy_timer = 0;
            state->vol_change_timer = 0;
        }

        if( state->deploy_timer < 100 ) {
            state->deploy_timer++;
        } else if( state->deploy_timer == 100 ) {
            state->deploy_timer++;
            if( state->deployed ){ _retract( state->container, state ); }
        }
    }

    if( recording_state->status == ZDJ_AUDIO_RECORD_ACTIVE ) {
        // Show meter on record volume change and peak
        if( zdj_deck_manager( )->control_change_flags[ ZDJ_DECK_CONTROL_RECORD_VOL ] ) {
            zdj_deck_manager( )->control_change_flags[ ZDJ_DECK_CONTROL_RECORD_VOL ] = false;
            // printf( "vol change\n" );
            state->vol_change_timer = 0;
        }
        if( state->icon ){ state->icon->frame.y = 0; }
        if( state->meter){ 
            state->meter->frame.x = 0; 
            if( state->vol_change_timer < 150 ) {
                state->vol_change_timer++;
                state->meter->frame.y = 0;
            } else {
                state->meter->frame.y = -10;
            }
        }
    } else {
        if( state->icon ){ state->icon->frame.y = -6; }
        if( state->meter){ state->meter->frame.x = 5; }
    }
}

static void _draw_container( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // Draw box and border
    // boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_BLACK );
}

static void _deploy( zdj_view_t * view, zdj_recording_widget_state_t * widget_state ) {
    // printf( "volume deploy\n" );

    // Lazy-load the meter
    _init_ui( view, widget_state );
    if( !widget_state->ui_init ){ return; }

    widget_state->deployed = true;

    ((anim_init_t)view->in_anim.init_fn)( 
        &view->in_anim, 
        view
    );
    view->anim = &view->in_anim;
}

static void _retract( zdj_view_t * view, zdj_recording_widget_state_t * widget_state ) {
    // printf( "volume retract\n" );
    widget_state->deployed = false;

    ((anim_init_t)view->out_anim.init_fn)( 
        &view->out_anim, 
        view 
    );
    view->anim = &view->out_anim;
}

// Lazy load at deploy to avoid a circular dependency between soundcard and ui
static void _init_ui( zdj_view_t * view, zdj_recording_widget_state_t * widget_state ) {
    // If soundcard isn't up yet, cancel the UI bringup
    if( !zdj_soundcard || widget_state->ui_init ){ return; }

    // Add LR bus meter
    zdj_soundcard_node_t * lr_node = zdj_soundcard_get_node_for_name( 
        zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS 
    );
    widget_state->meter = zdj_new_record_mini_meter_view( );

    zdj_soundcard_node_config_context_t * context = zdj_soundcard_new_node_config_context( );
    context->soundcard = zdj_soundcard;
    context->node = lr_node;
    zdj_soundcard_meter_state_t * meter_state = (zdj_soundcard_meter_state_t*)widget_state->meter->state;
    meter_state->config_context = context;
    meter_state->is_hilite = true;

    zdj_add_subview( widget_state->container, widget_state->meter );

    // Add Time Label
    // widget_state->time_label = zdj_new_label_view( " ", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    // widget_state->time_label->frame.x = 25 - widget_state->time_label->frame.w;
    // widget_state->time_label->frame.y = 5;
    // zdj_add_subview( widget_state->container, widget_state->time_label );

    // Add recording icon
    widget_state->icon = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_RECORD_BUS ], NULL );
    widget_state->icon->frame.x = view->frame.w - 8;
    zdj_add_subview( widget_state->container, widget_state->icon );

    widget_state->ui_init = true;
}
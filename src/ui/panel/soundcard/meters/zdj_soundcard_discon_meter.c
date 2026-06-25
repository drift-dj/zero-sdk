#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/node/analysis/meter/zdj_meter_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/panel/soundcard/zdj_soundcard_panel.h>
#include <zerodj/ui/panel/soundcard/meters/zdj_soundcard_meter.h>
#include <zerodj/ui/panel/soundcard/options/zdj_soundcard_options.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event );
static void _deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_discon_stereo_meter_view( 
    zdj_soundcard_node_t * node, 
    zdj_soundcard_meter_label_t label,
    bool show_detail 
) {
    zdj_view_t * meter_view = zdj_new_view( &(zdj_rect_t){0,0,15,55} );
    meter_view->type = ZDJ_VIEW_MENU_ITEM;
    meter_view->draw = &_draw;
    meter_view->handle_control_event = &_handle_control;
    meter_view->deinit_state = &_deinit_state;

    // Add a state instance
    // Note that zdj_soundcard_meter_state_t is an extension of menu_item_view_state.
    // This means it behaves like a normal item but has some extra storage for our stuff.
    zdj_soundcard_meter_state_t * state = calloc( 1, sizeof( zdj_soundcard_meter_state_t ) );
    state->needs_layout_update = true;
    state->handles_hmi = true;
    meter_view->state = state;

    // Add meter
    zdj_view_t * meter_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_DISCON_STEREO ], NULL );
    meter_bg->frame.x = -1;
    meter_bg->frame.y = 8;
    zdj_add_subview( meter_view, meter_bg );

    // Add detail
    zdj_view_t * detail = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_DETAIL ], NULL );
    detail->frame.x = 2;
    detail->frame.y = 46;
    detail->frame.w = 0;
    state->detail = detail;
    zdj_add_subview( meter_view, detail );

    // Add label
    zdj_view_t * meter_label = zdj_new_asset_view( 
        &zdj_ui_assets[ zdj_meter_asset_for_label( label ) ], 
        NULL 
    );
    zdj_add_subview( meter_view, meter_label );

    return meter_view;
}

zdj_view_t * zdj_new_discon_mono_meter_view( 
    zdj_soundcard_node_t * node, 
    zdj_soundcard_meter_label_t label,
    bool show_detail 
) {
    zdj_view_t * meter_view = zdj_new_view( &(zdj_rect_t){0,0,13,55} );
    meter_view->type = ZDJ_VIEW_MENU_ITEM;
    meter_view->draw = &_draw;
    meter_view->handle_control_event = &_handle_control;
    meter_view->deinit_state = &_deinit_state;

    // Add a state instance
    zdj_soundcard_meter_state_t * state = calloc( 1, sizeof( zdj_soundcard_meter_state_t ) );
    state->handles_hmi = true;
    meter_view->state = state;

    // Add meter
    zdj_view_t * meter_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_DISCON_MONO ], NULL );
    meter_bg->frame.x = -1;
    meter_bg->frame.y = 11;
    zdj_add_subview( meter_view, meter_bg );

    // Add label
    zdj_view_t * meter_label = zdj_new_asset_view( 
        &zdj_ui_assets[ zdj_meter_asset_for_label( label ) ], 
        NULL 
    );
    meter_label->frame.x = -1;
    zdj_add_subview( meter_view, meter_label );

    // Add detail
    zdj_view_t * detail = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_DETAIL ], NULL );
    detail->frame.x = 0;
    detail->frame.y = 46;
    detail->frame.w = 0;
    state->detail = detail;
    zdj_add_subview( meter_view, detail );

    return meter_view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_soundcard_meter_state_t * state = (zdj_soundcard_meter_state_t*)view->state;
    if( state->is_hilite ) {
        state->detail->frame.w = 7;
    } else {
        state->detail->frame.w = 0;
    }
}

static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_control_event_t * e = (zdj_control_event_t *)_event;
    zdj_soundcard_meter_state_t * state = (zdj_soundcard_meter_state_t*)view->state;
    zdj_soundcard_node_config_context_t * context = state->config_context;
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)context->options_view_state;

    if(e->id == ZDJ_UI_CONTROL_JOG_RELEASE_0 ) {
        zdj_push_subview( zdj_panel_view( ), zdj_new_soundcard_options( context ), true );
        e->blocked = true;
    }

    // Ignore events which have been blocked by layers above this one.
    if( e->blocked ) { return; }
}

static void _deinit_state( zdj_view_t * view ) {
    zdj_soundcard_meter_state_t * state = (zdj_soundcard_meter_state_t*)view->state;
}
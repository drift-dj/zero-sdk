#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/controls/hmi/zdj_hmi.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/node/analysis/meter/zdj_meter_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/soundcard_view/zdj_soundcard_view.h>
#include <zerodj/ui/view/soundcard_view/meters/zdj_soundcard_meter.h>
#include <zerodj/ui/view/soundcard_view/options/zdj_soundcard_options.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _zdj_clock_meter_draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_clock_meter_handle_hmi( zdj_view_t * view, void * _event );
static void _zdj_clock_meter_deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_clock_meter_view( 
    zdj_soundcard_node_t * node, 
    zdj_soundcard_meter_label_t label,
    bool show_detail 
) {
    zdj_view_t * clock_meter_view = zdj_new_view( &(zdj_rect_t){0,0,12,40} );
    clock_meter_view->type = ZDJ_VIEW_MENU_ITEM;
    clock_meter_view->draw = &_zdj_clock_meter_draw;
    clock_meter_view->handle_hmi_event = &_zdj_clock_meter_handle_hmi;
    clock_meter_view->deinit_state = &_zdj_clock_meter_deinit_state;

    // Add a state instance
    zdj_soundcard_meter_state_t * state = calloc( 1, sizeof( zdj_soundcard_meter_state_t ) );
    clock_meter_view->state = state;

    // Add clock counter
    zdj_view_t * clock_counter = zdj_new_asset_view( 
        &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_CLOCK_COUNT_3 ], 
        NULL 
    );
    clock_counter->frame->x = 0;
    clock_counter->frame->y = 36;
    state->clock_counter = clock_counter;
    zdj_add_subview( clock_meter_view, clock_counter );

    // Add clock pulse
    zdj_view_t * clock_pulse = zdj_new_asset_view( 
        &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_CLOCK_PULSE ], 
        NULL 
    );
    clock_pulse->frame->x = 0;
    clock_pulse->frame->y = 44;
    state->clock_pulse = clock_pulse;
    zdj_add_subview( clock_meter_view, clock_pulse );

    // Add BPM
    zdj_view_t * bpm = zdj_new_label_vert_view( "124.3", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    bpm->frame->y = 25;
    bpm->frame->x = -2;
    zdj_add_subview( clock_meter_view, bpm );

    // Add label
    zdj_view_t * meter_label = zdj_new_asset_view( 
        &zdj_ui_assets[ zdj_meter_asset_for_label( label ) ], 
        NULL 
    );
    meter_label->frame->x = -1;
    zdj_add_subview( clock_meter_view, meter_label );

    // Add detail
    zdj_view_t * detail = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_DETAIL ], NULL );
    detail->frame->x = 0;
    detail->frame->y = 43;
    detail->frame->w = 0;
    state->detail = detail;
    zdj_add_subview( clock_meter_view, detail );

    return clock_meter_view;
}

void _zdj_clock_meter_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );
    zdj_soundcard_meter_state_t * state = (zdj_soundcard_meter_state_t*)view->state;
    zdj_soundcard_node_config_context_t * config = state->config_context;
    if( state->is_hilite ) {
        state->detail->frame->w = 7;
    } else {
        state->detail->frame->w = 0;
    }
}

void _zdj_clock_meter_handle_hmi( zdj_view_t * view, void * _event ) {
    zdj_hmi_event_t * e = (zdj_hmi_event_t *)_event;
    zdj_soundcard_meter_state_t * state = (zdj_soundcard_meter_state_t*)view->state;
    zdj_soundcard_node_config_context_t * context = state->config_context;
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)context->options_view_state;
    
    if(e->id == ZDJ_HMI_ENCO_2_JOG && e->type == ZDJ_HMI_EVENT_RELEASE ) {
        printf( "show detail\n" );
        zdj_push_subview( zdj_root_view( ), zdj_new_soundcard_options( context ), true );
    }

    // Ignore events which have been blocked by layers above this one.
    if( e->blocked ) { return; }

    if( (e->id == ZDJ_HMI_ENCO_3_TONE_1 && e->type == ZDJ_HMI_EVENT_RELEASE) ) {
        // Prevent views/menus below this one from getting jog wheel events
        e->blocked = true;

        printf( "toggle mute\n" );
        // if( !zdj_soundcard_node_name_is_output( context->node->name ) ) {
            context->node->mute = !context->node->mute;
            if( options_state ){ options_state->needs_layout_update = true; }
        // }
        
    }
}

void _zdj_clock_meter_deinit_state( zdj_view_t * view ) {
    
}

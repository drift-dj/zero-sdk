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
#include <zerodj/ui/view/soundcard_view/zdj_soundcard_view.h>
#include <zerodj/ui/view/soundcard_view/meters/zdj_soundcard_meter.h>
#include <zerodj/ui/view/soundcard_view/options/zdj_soundcard_options.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _zdj_audio_meter_draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_audio_meter_handle_hmi( zdj_view_t * view, void * _event );
static void _zdj_audio_meter_deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_audio_stereo_meter_view( 
    zdj_soundcard_node_t * node, 
    zdj_soundcard_meter_label_t label,
    bool show_detail 
) {
    zdj_view_t * audio_meter_view = zdj_new_view( &(zdj_rect_t){0,0,15,41} );
    audio_meter_view->type = ZDJ_VIEW_MENU_ITEM;
    audio_meter_view->draw = &_zdj_audio_meter_draw;
    audio_meter_view->handle_hmi_event = &_zdj_audio_meter_handle_hmi;
    audio_meter_view->deinit_state = &_zdj_audio_meter_deinit_state;

    // Add a state instance
    // Note that zdj_soundcard_meter_state_t is an extension of menu_item_view_state.
    // This means it behaves like a normal item but has some extra storage for our stuff.
    zdj_soundcard_meter_state_t * state = calloc( 1, sizeof( zdj_soundcard_meter_state_t ) );
    state->data = calloc( 1, sizeof( zdj_ui_data_t ) );
    state->needs_layout_update = true;
    state->handles_hmi = true;
    audio_meter_view->state = state;

    // Add meter
    zdj_view_t * meter_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_STEREO_METER ], NULL );
    zdj_add_subview( audio_meter_view, meter_bg );

    // Add meter covers and store for updates
    zdj_view_t * meter_cover_l = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BLACK ], NULL );
    meter_cover_l->frame->w = 5;
    // meter_cover_l->frame->h = 0;
    meter_cover_l->frame->h = 46;
    zdj_add_subview( audio_meter_view, meter_cover_l );
    state->meter_cover_l = meter_cover_l;

    zdj_view_t * meter_cover_r = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BLACK ], NULL );
    meter_cover_r->frame->x = 5;
    meter_cover_r->frame->w = 6;
    // meter_cover_r->frame->h = 0;
    meter_cover_r->frame->h = 46;
    zdj_add_subview( audio_meter_view, meter_cover_r );
    state->meter_cover_r = meter_cover_r;

    // Add label
    zdj_view_t * meter_label = zdj_new_asset_view( 
        &zdj_ui_assets[ zdj_meter_asset_for_label( label ) ], 
        NULL 
    );
    zdj_add_subview( audio_meter_view, meter_label );

    // Add fader
    zdj_view_t * fader = zdj_new_view( &(zdj_rect_t){0,9,11,7} );
    zdj_view_t * fader_body = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_FADER_BODY ], NULL );
    zdj_add_subview( fader, fader_body );
    zdj_view_t * fader_edge = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_FADER_EDGE ], NULL );
    fader_edge->frame->x = 3;
    fader_edge->frame->y = 1;
    zdj_add_subview( fader, fader_edge );
    zdj_view_t * fader_index = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_FADER_INDEX ], NULL );
    fader_index->frame->x = 4;
    fader_index->frame->y = 3;
    zdj_add_subview( fader, fader_index );
    state->fader = fader;
    zdj_add_subview( audio_meter_view, fader );

    // Add mute cover
    zdj_view_t * mute_cover = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_MUTE_STEREO ], NULL );
    mute_cover->frame->x = 0;
    mute_cover->frame->y = 8;
    state->mute_cover = mute_cover;
    zdj_add_subview( audio_meter_view, mute_cover );

    // Add detail
    zdj_view_t * detail = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_DETAIL ], NULL );
    detail->frame->x = 2;
    detail->frame->y = 43;
    detail->frame->w = 0;
    state->detail = detail;
    zdj_add_subview( audio_meter_view, detail );

    return audio_meter_view;
}

zdj_view_t * zdj_new_audio_mono_meter_view( 
    zdj_soundcard_node_t * node, 
    zdj_soundcard_meter_label_t label,
    bool show_detail 
) {
    zdj_view_t * audio_meter_view = zdj_new_view( &(zdj_rect_t){0,0,12,40} );
    audio_meter_view->type = ZDJ_VIEW_MENU_ITEM;
    audio_meter_view->draw = &_zdj_audio_meter_draw;
    audio_meter_view->handle_hmi_event = &_zdj_audio_meter_handle_hmi;
    audio_meter_view->deinit_state = &_zdj_audio_meter_deinit_state;

    // Add a state instance
    zdj_soundcard_meter_state_t * state = calloc( 1, sizeof( zdj_soundcard_meter_state_t ) );
    audio_meter_view->state = state;

    // Add meter
    zdj_view_t * meter_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_MONO_METER ], NULL );
    zdj_add_subview( audio_meter_view, meter_bg );

    // Add meter covers and store for updates
    zdj_view_t * meter_cover_l = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BLACK ], NULL );
    meter_cover_l->frame->w = 12;
    meter_cover_l->frame->h = 46;
    zdj_add_subview( audio_meter_view, meter_cover_l );
    state->meter_cover_l = meter_cover_l;

    // Add label
    zdj_view_t * meter_label = zdj_new_asset_view( 
        &zdj_ui_assets[ zdj_meter_asset_for_label( label ) ], 
        NULL 
    );
    meter_label->frame->x = -1;
    zdj_add_subview( audio_meter_view, meter_label );

    // Add fader
    zdj_view_t * fader = zdj_new_view( &(zdj_rect_t){0,9,11,7} );
    zdj_view_t * fader_body = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_FADER_BODY ], NULL );
    zdj_add_subview( fader, fader_body );
    zdj_view_t * fader_edge = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_FADER_EDGE ], NULL );
    fader_edge->frame->x = 3;
    fader_edge->frame->y = 1;
    zdj_add_subview( fader, fader_edge );
    zdj_view_t * fader_index = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_FADER_INDEX ], NULL );
    fader_index->frame->x = 4;
    fader_index->frame->y = 3;
    fader_index->frame->w = 5;
    zdj_add_subview( fader, fader_index );
    state->fader = fader;
    zdj_add_subview( audio_meter_view, fader );

    // Add mute cover
    zdj_view_t * mute_cover = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_MUTE_MONO ], NULL );
    mute_cover->frame->x = 1;
    mute_cover->frame->y = 8;
    state->mute_cover = mute_cover;
    zdj_add_subview( audio_meter_view, mute_cover );

    // Add detail
    zdj_view_t * detail = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_DETAIL ], NULL );
    detail->frame->x = 1;
    detail->frame->y = 43;
    detail->frame->w = 0;
    state->detail = detail;
    zdj_add_subview( audio_meter_view, detail );

    return audio_meter_view;
}

void _zdj_audio_meter_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_soundcard_meter_state_t * state = (zdj_soundcard_meter_state_t*)view->state;
    zdj_soundcard_node_config_context_t * config = state->config_context;

    if( state->is_hilite ) {
        state->fader->frame->w = 12;
        state->detail->frame->w = 7;
        state->fader->frame->y = ((state->config_context->node->gain / 255.0f) * 36) + 3;
    } else {
        state->fader->frame->w = 0;
        state->detail->frame->w = 0;
    }
    if( config->node->mute ) { 
        state->mute_cover->frame->w = 11; 
        state->fader->frame->w = 0;
    } else { 
        state->mute_cover->frame->w = 0; 
    }
}

void _zdj_audio_meter_handle_hmi( zdj_view_t * view, void * _event ) {
    zdj_hmi_event_t * e = (zdj_hmi_event_t *)_event;
    zdj_soundcard_meter_state_t * state = (zdj_soundcard_meter_state_t*)view->state;
    zdj_soundcard_node_config_context_t * context = state->config_context;
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)context->options_view_state;
    
    if(e->id == ZDJ_HMI_ENCO_2_JOG && e->type == ZDJ_HMI_EVENT_RELEASE ) {
        zdj_push_subview( zdj_root_view( ), zdj_new_soundcard_options( context ), true );
    }

    // Ignore events which have been blocked by layers above this one.
    if( e->blocked ) { return; }

    if( (e->id == ZDJ_HMI_ENCO_2_JOG && e->type == ZDJ_HMI_EVENT_PRESS_ADJUST) ||
        (e->id == ZDJ_HMI_ENCO_3_TONE_1 && e->type == ZDJ_HMI_EVENT_ADJUST) 
    ) {
        // Prevent views/menus below this one from getting jog wheel events
        e->blocked = true;

        context->node->gain += e->i_val * -2;
        if( context->node->gain > 255 ) { context->node->gain = 255; }
        if( context->node->gain < 0 ) { context->node->gain = 0; }
        if( options_state ){ options_state->needs_layout_update = true; }
    } else if( (e->id == ZDJ_HMI_ENCO_3_TONE_1 && e->type == ZDJ_HMI_EVENT_RELEASE) ) {
        // Prevent views/menus below this one from getting jog wheel events
        e->blocked = true;

        printf( "toggle mute\n" );
        // if( !zdj_soundcard_node_name_is_output( context->node->name ) ) {
            context->node->mute = !context->node->mute;
            if( options_state ){ options_state->needs_layout_update = true; }
        // }
        
    } else if( (e->id == ZDJ_HMI_ENCO_4_TONE_2 && e->type == ZDJ_HMI_EVENT_ADJUST) ) {
        // Prevent views/menus below this one from getting jog wheel events
        e->blocked = true;

        if( !context->node->stereo ) {
            context->node->pan += e->i_val * -2;
            if( context->node->pan > 127 ) { context->node->pan = 127; }
            if( context->node->pan < -127 ) { context->node->pan = -127; }
            if( options_state ){ options_state->needs_layout_update = true; }
        }
    }
}

void _zdj_audio_meter_deinit_state( zdj_view_t * view ) {
    
}


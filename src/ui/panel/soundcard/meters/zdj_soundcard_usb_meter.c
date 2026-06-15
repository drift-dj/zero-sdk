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

zdj_view_t * zdj_new_usb_stereo_meter_view( 
    zdj_soundcard_node_t * node, 
    zdj_soundcard_meter_label_t label,
    bool show_detail 
) {
    // printf( "zdj_new_audio_stereo_meter_view\n" );
    zdj_view_t * audio_meter_view = zdj_new_view( &(zdj_rect_t){0,0,15,55} );
    audio_meter_view->type = ZDJ_VIEW_MENU_ITEM;
    audio_meter_view->draw = &_draw;
    audio_meter_view->handle_control_event = &_handle_control;
    audio_meter_view->deinit_state = &_deinit_state;

    // Add a state instance
    // Note that zdj_soundcard_meter_state_t is an extension of menu_item_view_state.
    // This means it behaves like a normal item but has some extra storage for our stuff.
    zdj_soundcard_meter_state_t * state = calloc( 1, sizeof( zdj_soundcard_meter_state_t ) );
    state->needs_layout_update = true;
    state->handles_hmi = true;
    audio_meter_view->state = state;

    // Add meter
    zdj_view_t * meter_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_STEREO_METER ], NULL );
    meter_bg->frame.x = -1;
    meter_bg->frame.y = 8;
    zdj_add_subview( audio_meter_view, meter_bg );

    // Add meter covers and store for updates
    zdj_view_t * meter_cover_l = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BLACK ], NULL );
    meter_cover_l->frame.w = 6;
    // meter_cover_l->frame.h = 0;
    meter_cover_l->frame.y = 9;
    meter_cover_l->frame.h = 43;
    zdj_add_subview( audio_meter_view, meter_cover_l );
    state->meter_cover_l = meter_cover_l;

    zdj_view_t * meter_cover_r = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BLACK ], NULL );
    meter_cover_r->frame.x = 6;
    meter_cover_r->frame.y = 9;
    meter_cover_r->frame.w = 5;
    // meter_cover_r->frame.h = 0;
    meter_cover_r->frame.h = 43;
    zdj_add_subview( audio_meter_view, meter_cover_r );
    state->meter_cover_r = meter_cover_r;
    

    // Add mute cover
    zdj_view_t * mute_cover = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_MUTE_STEREO ], NULL );
    mute_cover->frame.x = 0;
    mute_cover->frame.y = 9;
    state->mute_cover = mute_cover;
    zdj_add_subview( audio_meter_view, mute_cover );

    // Add detail
    zdj_view_t * detail = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_DETAIL ], NULL );
    detail->frame.x = 2;
    detail->frame.y = 46;
    detail->frame.w = 0;
    state->detail = detail;
    zdj_add_subview( audio_meter_view, detail );

    // Add label
    zdj_view_t * meter_label = zdj_new_asset_view( 
        &zdj_ui_assets[ zdj_meter_asset_for_label( label ) ], 
        NULL 
    );
    zdj_add_subview( audio_meter_view, meter_label );

    // Add fader
    if( zdj_soundcard_node_name_should_show_fader( node->name ) ) {
        // printf( "adding fader for: %s\n", zdj_soundcard_node_name[ node->name ] );
        zdj_view_t * fader = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_FADER_BODY ], NULL );
        fader->frame.x = -1;
        fader->frame.y = -10;
        state->fader = fader;
        zdj_add_subview( audio_meter_view, fader );
    }
    
    return audio_meter_view;
}

zdj_view_t * zdj_new_usb_mono_meter_view( 
    zdj_soundcard_node_t * node, 
    zdj_soundcard_meter_label_t label,
    bool show_detail 
) {
    // printf( "zdj_new_audio_mono_meter_view: \n" );
    zdj_view_t * audio_meter_view = zdj_new_view( &(zdj_rect_t){0,0,12,55} );
    audio_meter_view->type = ZDJ_VIEW_MENU_ITEM;
    audio_meter_view->draw = &_draw;
    audio_meter_view->handle_control_event = &_handle_control;
    audio_meter_view->deinit_state = &_deinit_state;

    // Add a state instance
    zdj_soundcard_meter_state_t * state = calloc( 1, sizeof( zdj_soundcard_meter_state_t ) );
    state->handles_hmi = true;
    audio_meter_view->state = state;

    // Add meter
    zdj_view_t * meter_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_MONO_METER ], NULL );
    meter_bg->frame.x = -1;
    meter_bg->frame.y = 8;
    zdj_add_subview( audio_meter_view, meter_bg );

    // Add meter covers and store for updates
    zdj_view_t * meter_cover_l = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BLACK ], NULL );
    
    meter_cover_l->frame.y = 9;
    meter_cover_l->frame.w = 11;
    meter_cover_l->frame.h = 43;
    zdj_add_subview( audio_meter_view, meter_cover_l );
    state->meter_cover_l = meter_cover_l;

    // Add label
    zdj_view_t * meter_label = zdj_new_asset_view( 
        &zdj_ui_assets[ zdj_meter_asset_for_label( label ) ], 
        NULL 
    );
    meter_label->frame.x = -1;
    zdj_add_subview( audio_meter_view, meter_label );

    // Add fader
    if( zdj_soundcard_node_name_should_show_fader( node->name ) ) {
    // if( node->dsp_dto ) {
        zdj_view_t * fader = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_FADER_BODY ], NULL );
        fader->frame.x = -1;
        fader->frame.y = -10;
        state->fader = fader;
        zdj_add_subview( audio_meter_view, fader );
    }

    // Add mute cover
    zdj_view_t * mute_cover = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_MUTE_MONO ], NULL );
    mute_cover->frame.x = 1;
    mute_cover->frame.y = 9;
    state->mute_cover = mute_cover;
    zdj_add_subview( audio_meter_view, mute_cover );

    // Add detail
    zdj_view_t * detail = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_DETAIL ], NULL );
    detail->frame.x = 1;
    detail->frame.y = 46;
    detail->frame.w = 0;
    state->detail = detail;
    zdj_add_subview( audio_meter_view, detail );

    return audio_meter_view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // printf( "_zdj_usb_meter draw\n" );
    zdj_soundcard_meter_state_t * state = (zdj_soundcard_meter_state_t*)view->state;
    zdj_soundcard_node_config_context_t * config = state->config_context;

    zdj_pipeline_node_t * meter_pipe = config->node->meter_pipe;
    zdj_meter_node_state_t * meter_state = (zdj_meter_node_state_t*)meter_pipe->state;
    float meter_l_h = (meter_state->instant_val_0) * 43;
    float meter_r_h = (meter_state->instant_val_1) * 43;
    // printf( "%1.1f/%1.1f - %1.0f/%1.0f\n", 
    //     meter_state->lowpass_val_0, 
    //     meter_state->instant_val_1, 
    //     meter_l_h, meter_r_h 
    // );

    if( state->is_hilite ) {
        state->detail->frame.w = 7;
        if( state->fader && state->config_context->node->dsp_dto ) {
            if( config->node->stereo ) { state->fader->frame.w = 13; } 
            else { state->fader->frame.w = 10; }
            double fader_val = state->config_context->node->dsp_dto->get_gain_display_val( 
                state->config_context->node->dsp_dto->gain_model,
                state->config_context->node->dsp_dto->gain
            );
            state->fader->frame.y = (32 - (fader_val * 32)) + 10;
        }
    } else {
        if( state->fader && state->config_context->node->dsp_dto ) {
            if( config->node->stereo ) { state->fader->frame.w = 13; } 
            else { state->fader->frame.w = 10; }
        }
        state->detail->frame.w = 0;
        state->fader->frame.w = 0;
    }
    if( config->node->mute ) { 
        state->mute_cover->frame.w = 11; 
        // state->fader->frame.w = 0;
        if( state->fader && state->config_context->node->dsp_dto ) {
            state->fader->frame.w = 0;
        }
    } else { 
        state->mute_cover->frame.w = 0; 
        state->meter_cover_l->frame.h = 43 - meter_l_h;
        if( config->node->stereo ) {
            state->meter_cover_r->frame.h = 43 - meter_r_h;
        }
    }
    // printf( "_zdj_audio_meter_draw done\n" );
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

    if( e->id == ZDJ_UI_CONTROL_JOG_ADJUST_1 || e->id == ZDJ_UI_CONTROL_TONE_1_ADJUST_0 ) {
        // Prevent views/menus below this one from getting jog wheel events
        e->blocked = true;
        if( context->node->dsp_dto ) {
            context->node->dsp_dto->adjust_gain( context->node, e->i_val );
        }
        if( options_state ){ options_state->needs_layout_update = true; }
    } else if( e->id == ZDJ_UI_CONTROL_TONE_1_RELEASE_0 ) {
        // Prevent views/menus below this one from getting jog wheel events
        e->blocked = true;

        printf( "toggle mute\n" );
        // if( !zdj_soundcard_node_name_is_output( context->node->name ) ) {
            context->node->mute = !context->node->mute;
            if( options_state ){ options_state->needs_layout_update = true; }
        // }
        
    } else if( e->id == ZDJ_UI_CONTROL_TONE_2_ADJUST_0 ) {
        // Prevent views/menus below this one from getting jog wheel events
        e->blocked = true;

        if( !context->node->stereo && context->node->dsp_dto ) {
            context->node->pan += e->i_val * -2;
            if( context->node->pan > 127 ) { context->node->pan = 127; }
            if( context->node->pan < -127 ) { context->node->pan = -127; }
            if( options_state ){ options_state->needs_layout_update = true; }
        }
    }
}

static void _deinit_state( zdj_view_t * view ) {
    zdj_soundcard_meter_state_t * state = (zdj_soundcard_meter_state_t*)view->state;
}


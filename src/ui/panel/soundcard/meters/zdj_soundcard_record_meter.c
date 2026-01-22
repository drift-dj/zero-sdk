#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/node/analysis/meter/zdj_meter_node.h>
#include <zerodj/signal/pipeline/node/audio/record/zdj_audio_record_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/panel/soundcard/zdj_soundcard_panel.h>
#include <zerodj/ui/panel/soundcard/meters/zdj_soundcard_meter.h>
#include <zerodj/ui/panel/soundcard/options/zdj_soundcard_options.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_record_meter_view( void ) {
    // printf( "zdj_new_audio_stereo_meter_view\n" );
    zdj_view_t * audio_meter_view = zdj_new_view( &(zdj_rect_t){0,0,115,15} );
    audio_meter_view->type = ZDJ_VIEW_BASE;
    audio_meter_view->draw = &_draw;
    audio_meter_view->deinit_state = &_deinit_state;

    // Add a state instance
    // Note that zdj_soundcard_meter_state_t is an extension of menu_item_view_state.
    // This means it behaves like a normal item but has some extra storage for our stuff.
    zdj_soundcard_meter_state_t * state = calloc( 1, sizeof( zdj_soundcard_meter_state_t ) );
    audio_meter_view->state = state;

    zdj_view_t * meter_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_RECORD_METER ], NULL );
    meter_bg->frame.x = 3;
    meter_bg->frame.y = 1;
    zdj_add_subview( audio_meter_view, meter_bg );

    // Add meter covers and store for updates
    zdj_view_t * meter_cover_l = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BLACK ], NULL );
    meter_cover_l->frame.x = 3;
    meter_cover_l->frame.w = 111;
    // meter_cover_l->frame.w = 0;
    meter_cover_l->frame.y = 1;
    meter_cover_l->frame.h = 5;
    zdj_add_subview( audio_meter_view, meter_cover_l );
    state->meter_cover_l = meter_cover_l;

    zdj_view_t * meter_cover_r = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BLACK ], NULL );
    meter_cover_r->frame.x = 3;
    meter_cover_r->frame.w = 111;
    // meter_cover_r->frame.w = 0;
    meter_cover_r->frame.y = 9;
    meter_cover_r->frame.h = 5;
    zdj_add_subview( audio_meter_view, meter_cover_r );
    state->meter_cover_r = meter_cover_r;

    // Add fader
    zdj_view_t * fader = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_RECORD_METER_FADER ], NULL );
    state->fader = fader;
    zdj_add_subview( audio_meter_view, fader );

    return audio_meter_view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // printf( "meter draw\n" );
    zdj_soundcard_meter_state_t * view_state = (zdj_soundcard_meter_state_t*)view->state;
    zdj_soundcard_node_t * node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS );
    zdj_pipeline_node_t * meter_pipe = node->meter_pipe;
    zdj_meter_node_state_t * meter_state = (zdj_meter_node_state_t*)meter_pipe->state;
    
    float meter_l = (meter_state->instant_val_0) * 112;
    float meter_r = (meter_state->instant_val_1) * 112;
    
    view_state->meter_cover_l->frame.x = meter_l + 3;
    view_state->meter_cover_l->frame.w = 112 - meter_l;

    view_state->meter_cover_r->frame.x = meter_r + 3;
    view_state->meter_cover_r->frame.w = 112 - meter_r;

    view_state->fader->frame.x = node->dsp_dto->gain * 111;
    // view_state->fader->frame.x = 80;
}



void _deinit_state( zdj_view_t * view ) {
    
}


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
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

typedef struct {

} zdj_soundcard_record_mini_meter_state_t;

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_record_mini_meter_view( void ) {
    zdj_view_t * audio_meter_view = zdj_new_view( &(zdj_rect_t){0,0,28,11} );
    audio_meter_view->type = ZDJ_VIEW_BASE;
    audio_meter_view->draw = &_draw;
    audio_meter_view->deinit_state = &_deinit_state;

    // Add a state instance
    // Note that zdj_soundcard_meter_state_t is an extension of menu_item_view_state.
    // This means it behaves like a normal item but has some extra storage for our stuff.
    zdj_soundcard_meter_state_t * state = calloc( 1, sizeof( zdj_soundcard_meter_state_t ) );
    audio_meter_view->state = state;

    // Note that we're hacking the existing views to store the meter bits here
    // BG
    state->normal_view = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DJ_KNOB_BG ], NULL );
    state->normal_view->frame.w = audio_meter_view->frame.w;
    zdj_add_subview( audio_meter_view, state->normal_view );

    // Meter tape
    state->meter_cover_l = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DJ_KNOB_VAL ], NULL );
    state->meter_cover_l->frame.w = 0;
    zdj_add_subview( audio_meter_view, state->meter_cover_l );

    // Clip alert
    state->hilite_view = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DJ_METER_ALERT_HORIZ ], NULL );
    state->hilite_view->frame.w = 0;
    zdj_add_subview( audio_meter_view, state->hilite_view );

    // Fader
    state->fader = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DJ_MINI_FADER_HORIZ ], NULL );
    state->fader->frame.y = 2;
    state->fader->frame.h = 3;
    zdj_add_subview( audio_meter_view, state->fader );


    return audio_meter_view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    if( zdj_soundcard->state != ZDJ_SOUNDCARD_STATE_RUNNING ) { return; }

    zdj_soundcard_meter_state_t * view_state = (zdj_soundcard_meter_state_t*)view->state;
    zdj_soundcard_node_t * node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS );
    zdj_pipeline_node_t * meter_pipe = node->meter_pipe;
    zdj_meter_node_state_t * meter_state = (zdj_meter_node_state_t*)meter_pipe->state;
    zdj_audio_record_node_state_t * recording_state = (zdj_audio_record_node_state_t*)zdj_soundcard->recording_node->state;

    float meter_val = fmax(meter_state->instant_val_0, meter_state->instant_val_1) * 28;
    view_state->meter_cover_l->frame.w = meter_val;

    bool show_full_ui = false;

    if( meter_state->has_ol_0_0 || meter_state->has_ol_0_1 ) {
        view_state->hilite_view->frame.w = view->frame.w;
    } else {
        view_state->hilite_view->frame.w = 0;
    }

    // Fader
    double fader_val = node->dsp_dto->get_gain_display_val( 
        node->dsp_dto->gain_model,
        node->dsp_dto->gain
    );
    view_state->fader->frame.x = (fader_val * view->frame.w) - 2;
}

void _deinit_state( zdj_view_t * view ) {
    
}


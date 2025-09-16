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
#include <zerodj/ui/view/soundcard_view/zdj_soundcard_view.h>
#include <zerodj/ui/view/soundcard_view/meters/zdj_soundcard_meter.h>
#include <zerodj/ui/view/soundcard_view/options/zdj_soundcard_options.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_record_mini_meter_view( void ) {
    // printf( "zdj_new_audio_stereo_meter_view\n" );
    zdj_view_t * audio_meter_view = zdj_new_view( &(zdj_rect_t){0,0,24,4} );
    audio_meter_view->type = ZDJ_VIEW_BASE;
    audio_meter_view->draw = &_draw;
    audio_meter_view->deinit_state = &_deinit_state;

    // Add a state instance
    // Note that zdj_soundcard_meter_state_t is an extension of menu_item_view_state.
    // This means it behaves like a normal item but has some extra storage for our stuff.
    zdj_soundcard_meter_state_t * state = calloc( 1, sizeof( zdj_soundcard_meter_state_t ) );

    return audio_meter_view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // printf( "mini meter draw\n" );
    zdj_soundcard_node_t * node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS );
    zdj_pipeline_node_t * meter_pipe = node->meter_pipe;
    zdj_meter_node_state_t * meter_state = (zdj_meter_node_state_t*)meter_pipe->state;
    float meter_l = (meter_state->instant_val_0) * 40;
    float meter_r = (meter_state->instant_val_1) * 40;
    
    boxColor( zdj_renderer( ), ZDJ_SCREEN_W-meter_l, clip->dst.y+1, ZDJ_SCREEN_W, clip->dst.y+2, ZDJ_WHITE );
    boxColor( zdj_renderer( ), ZDJ_SCREEN_W-meter_r, clip->dst.y+3, ZDJ_SCREEN_W, clip->dst.y+4, ZDJ_WHITE );

    zdj_pipeline_node_t * recording_node = zdj_soundcard->recording_node;
    zdj_audio_record_node_state_t * recording_state = (zdj_audio_record_node_state_t*)recording_node->state;
    if( recording_state->status != ZDJ_AUDIO_RECORD_ACTIVE ) {
        lineColor( zdj_renderer( ), 119, clip->dst.y+5, 124, clip->dst.y, ZDJ_BLACK );
        lineColor( zdj_renderer( ), 120, clip->dst.y+5, 125, clip->dst.y, ZDJ_WHITE );
        lineColor( zdj_renderer( ), 121, clip->dst.y+5, 126, clip->dst.y, ZDJ_BLACK );
        lineColor( zdj_renderer( ), 122, clip->dst.y+5, 127, clip->dst.y, ZDJ_WHITE );
        lineColor( zdj_renderer( ), 123, clip->dst.y+5, 128, clip->dst.y, ZDJ_BLACK );
    }
}



void _deinit_state( zdj_view_t * view ) {
    
}


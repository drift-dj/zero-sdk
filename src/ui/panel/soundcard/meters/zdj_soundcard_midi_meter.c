#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>


#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/panel/soundcard/zdj_soundcard_panel.h>
#include <zerodj/ui/panel/soundcard/meters/zdj_soundcard_meter.h>
#include <zerodj/ui/view/zdj_view_stack.h>

void _zdj_midi_meter_draw( zdj_view_t * view, zdj_view_clip_t * clip );
void _zdj_midi_meter_deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_midi_meter_view( 
    zdj_soundcard_node_t * node, 
    zdj_soundcard_meter_label_t label,
    bool show_detail 
) {
    zdj_view_t * midi_meter_view = zdj_new_view( zdj_modal_rect( ) );
    midi_meter_view->type = ZDJ_VIEW_SOUNDCARD;
    midi_meter_view->draw = &_zdj_midi_meter_draw;
    midi_meter_view->deinit_state = &_zdj_midi_meter_deinit_state;

    // Add a state instance
    zdj_soundcard_meter_state_t * state = calloc( 1, sizeof( zdj_soundcard_meter_state_t ) );
    midi_meter_view->state = state;

    return midi_meter_view;
}

void _zdj_midi_meter_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );
}

void _zdj_midi_meter_deinit_state( zdj_view_t * view ) {
    
}

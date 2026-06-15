#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/nudge_view/zdj_nudge_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_nudge_view( zdj_deck_t * deck ) {
    // Build the label's view
    zdj_view_t * view = zdj_new_view( &(zdj_rect_t){0,0,ZDJ_SCREEN_W,5} );
    view->type = ZDJ_VIEW_NUDGE;
    view->draw = &_draw;
    view->deinit_state = &_deinit_state;

    zdj_nudge_state_t * nudge_state = calloc( 1, sizeof( zdj_nudge_state_t ) );
    nudge_state->deck = deck;
    view->state = (void*)nudge_state;

    nudge_state->arrow_l = zdj_new_noclip_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DJ_NUDGE_ARROW_L ], NULL );
    nudge_state->arrow_l->frame.x = -20;
    zdj_add_subview( view, nudge_state->arrow_l );

    nudge_state->arrow_r = zdj_new_noclip_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DJ_NUDGE_ARROW_R ], NULL );
    nudge_state->arrow_r->frame.x = -20;
    zdj_add_subview( view, nudge_state->arrow_r );
    
    return view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_nudge_state_t * nudge_state = (zdj_nudge_state_t*)view->state;  
    
    // Hide if nudge is not active
    if( fabs( nudge_state->deck->controls.platter.slip.tempo_nudge_rate ) < 5.0 ) {
        nudge_state->arrow_l->frame.x = -20;
        nudge_state->arrow_r->frame.x = -20;
    
    // Calculate x based on nudge velocity
    } else {
        double arrow_x = nudge_state->deck->controls.platter.slip.tempo_nudge_rate * 0.1;
        if( arrow_x > 0 ) {
            nudge_state->arrow_r->frame.x = (view->frame.w / 2) + arrow_x;
            nudge_state->arrow_l->frame.x = -10;
        } else {
            nudge_state->arrow_r->frame.x = -10;
            nudge_state->arrow_l->frame.x = (view->frame.w / 2) + arrow_x - 5;
        }
    }
}

static void _deinit_state( zdj_view_t * view ) {
    zdj_nudge_state_t * state = (zdj_nudge_state_t*)view->state;
    free( state );
    view->state = NULL;
}
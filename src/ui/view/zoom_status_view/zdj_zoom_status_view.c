#include <stdlib.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/zoom_status_view/zdj_zoom_status_view.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_zoom_status_view( zdj_rect_t * frame ) {
    zdj_view_t * view = zdj_new_view( frame );
    view->draw = &_draw;
    view->deinit_state = &_deinit_state;

    zdj_zoom_status_view_state_t * zoom_state = calloc( 1, sizeof( zdj_zoom_status_view_state_t ) );
    zoom_state->zoom_ratio = 0.0;
    zoom_state->center_ratio = 0.0;
    zoom_state->has_valid_display = false;
    view->state = zoom_state;

    // Add zoom status
    zdj_view_t * zoom_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_SM_HATCH_TEX ], NULL );
    zoom_bg->frame.x = 3;
    zoom_bg->frame.w = view->frame.w - 6;
    zoom_bg->frame.h = view->frame.h;
    zdj_add_subview( view, zoom_bg );

    zdj_view_t * status_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BLACK ], NULL );
    status_bg->frame.x = 2;
    status_bg->frame.w = 3;
    status_bg->frame.h = view->frame.h;
    zdj_add_subview( view, status_bg );
    zoom_state->status_bg = status_bg;
    zdj_view_t * status_indicator = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_WHITE ], NULL );
    status_indicator->frame.x = 3;
    status_indicator->frame.w = 1;
    status_indicator->frame.h = view->frame.h;
    zdj_add_subview( view, status_indicator );
    zoom_state->status_indicator = status_indicator;

    
    zdj_view_t * left_zoom_bar = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_WHITE ], NULL );
    left_zoom_bar->frame.w = 2;
    left_zoom_bar->frame.h = view->frame.h;
    zdj_add_subview( view, left_zoom_bar );
    zdj_view_t * left_blackout_bar = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BLACK ], NULL );
    left_blackout_bar->frame.w = 1;
    left_blackout_bar->frame.h = view->frame.h;
    left_blackout_bar->frame.x = 2;
    zdj_add_subview( view, left_blackout_bar );
    zdj_view_t * right_zoom_bar = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_WHITE ], NULL );
    right_zoom_bar->frame.x = view->frame.w-2;
    right_zoom_bar->frame.w = 2;
    right_zoom_bar->frame.h = view->frame.h;
    zdj_add_subview( view, right_zoom_bar );
    zdj_view_t * right_blackout_bar = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BLACK ], NULL );
    right_blackout_bar->frame.w = 1;
    right_blackout_bar->frame.h = view->frame.h;
    right_blackout_bar->frame.x = view->frame.w-3;
    zdj_add_subview( view, right_blackout_bar );

    return view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_zoom_status_view_state_t * zoom_state = (zdj_zoom_status_view_state_t*)view->state;
    
    if( !zoom_state->has_valid_display ) {
        double indicator_w = ceil(zoom_state->zoom_ratio * ((double)view->frame.w-7));
        double indicator_x = zoom_state->center_ratio * ((double)view->frame.w-7);
        zoom_state->status_indicator->frame.w = indicator_w;
        zoom_state->status_indicator->frame.x = indicator_x - (indicator_w/2) + 3;
        zoom_state->status_bg->frame.w = indicator_w + 2;
        zoom_state->status_bg->frame.x = indicator_x - (indicator_w/2) + 2;
    }
}

static void _deinit_state( zdj_view_t * view ) {
    zdj_zoom_status_view_state_t * state = (zdj_zoom_status_view_state_t*)view->state;
    free( state );
    view->state = NULL;
}
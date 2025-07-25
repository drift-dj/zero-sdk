#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>

void zdj_menu_item_icon_init_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    // Clear out the normal/hilite views' subviews
    if( state->hilite_view ) { 
        zdj_remove_all_subviews_of( state->hilite_view ); 
    } else {
        state->hilite_view = zdj_new_view( NULL );
        zdj_add_subview( view, state->hilite_view );
        state->hilite_view->frame->w = view->frame->w;
        state->hilite_view->frame->h = view->frame->h;
    }
    if( state->normal_view ) { 
        zdj_remove_all_subviews_of( state->normal_view );
    } else {
        state->normal_view = zdj_new_view( NULL );
        zdj_add_subview( view, state->normal_view );
        state->normal_view->frame->w = view->frame->w;
        state->normal_view->frame->h = view->frame->h;
    }

    // Setup normal view
    zdj_view_t * icon = zdj_new_asset_view( &zdj_ui_assets[ state->icon ], NULL );
    zdj_add_subview( state->normal_view, icon );
    icon->frame->x = 7;
    icon->frame->y = 3;

    zdj_view_t * title_ticker = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_CENTER, ZDJ_SDL_WHITE );
    zdj_add_subview( state->normal_view, title_ticker );
    title_ticker->frame->x = 1;
    title_ticker->frame->y = 15;
    title_ticker->frame->w = view->frame->w;
    title_ticker->frame->h = 10;

    // Setup hilite view
    zdj_view_t * icon_hi = zdj_new_asset_view( &zdj_ui_assets[ state->icon_hi ], NULL );
    zdj_add_subview( state->hilite_view, icon_hi );
    icon_hi->frame->x = 7;
    icon_hi->frame->y = 3;

    zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_8_L ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg );
    hilite_bg->frame->y = 16;
    hilite_bg->frame->w = view->frame->w;
    zdj_view_t * hilite_bg_r = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_8_R ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg_r );
    hilite_bg_r->frame->y = 16;
    hilite_bg_r->frame->x = view->frame->w-1;

    zdj_view_t * title_ticker_hilite = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_CENTER, ZDJ_SDL_BLACK );
    zdj_add_subview( state->hilite_view, title_ticker_hilite );
    title_ticker_hilite->frame->x = 1;
    title_ticker_hilite->frame->y = 15;
    title_ticker_hilite->frame->w = view->frame->w;
    title_ticker_hilite->frame->h = view->frame->h;

    state->needs_layout_init = false;
}
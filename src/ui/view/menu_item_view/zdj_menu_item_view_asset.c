#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>

void zdj_menu_item_asset_init_layout( zdj_view_t * view ) {
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
    if( state->icon ) {
        zdj_view_t * icon = zdj_new_asset_view( &zdj_ui_assets[ state->icon ], NULL );
        zdj_add_subview( state->normal_view, icon );
        // icon->frame->x = 7;
        // icon->frame->y = 3;
    }

    // Setup hilite view
    // if( state->icon_hi ) {
        zdj_view_t * icon_hi = zdj_new_asset_view( &zdj_ui_assets[ state->icon_hi ], NULL );
        zdj_add_subview( state->hilite_view, icon_hi );
        // icon_hi->frame->x = 7;
        // icon_hi->frame->y = 3;
    // }

    state->needs_layout_init = false;
}
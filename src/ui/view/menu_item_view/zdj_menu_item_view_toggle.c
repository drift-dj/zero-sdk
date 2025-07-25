#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>


void zdj_menu_item_toggle_init_layout( zdj_view_t * view ) {
    // printf( "zdj_menu_item_toggle_init_layout\n" );
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

    view->frame->h = 7;
    bool toggle_state = state->data->b_val;
    
    // Setup normal view
    zdj_view_t * title_ticker_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE );
    zdj_add_subview( state->normal_view, title_ticker_norm );
    title_ticker_norm->frame->x = 1;
    title_ticker_norm->frame->w = view->frame->w - 8;
    title_ticker_norm->frame->h = view->frame->h;
    title_ticker_norm->frame->y = -1;
    
    // Setup toggle state
    zdj_view_t * toggle;
    if( toggle_state ) { 
        toggle = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_TOGGLE_ON ], NULL );
    } else {
        toggle = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_TOGGLE_OFF ], NULL );
    }
    zdj_add_subview( state->normal_view, toggle );
    toggle->frame->x = view->frame->w - 6;
    toggle->frame->y = 1;


    
    // Setup hilite view
    zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_L ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg );
    hilite_bg->frame->y = 0;
    hilite_bg->frame->w = view->frame->w;
    zdj_view_t * hilite_bg_r = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_R ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg_r );
    hilite_bg_r->frame->y = 0;
    hilite_bg_r->frame->x = view->frame->w-1;

    zdj_view_t * title_ticker_hilite = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_BLACK );
    zdj_ticker_state_t * ticker_state = (zdj_ticker_state_t*)title_ticker_hilite->state;
    zdj_add_subview( state->hilite_view, title_ticker_hilite );
    title_ticker_hilite->frame->x = 1;
    title_ticker_hilite->frame->w = view->frame->w - 8;
    title_ticker_hilite->frame->h = view->frame->h;
    title_ticker_hilite->frame->y = -1;

    // Adjust hilite frame based on ticker's frame
    hilite_bg->frame->w = view->frame->w;
    hilite_bg->frame->x = 0;
    // Setup toggle hilite state
    zdj_view_t * toggle_hi;
    if( toggle_state ) { 
        toggle_hi = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_TOGGLE_ON_HI ], NULL );
    } else {
        toggle_hi = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_TOGGLE_OFF_HI ], NULL );
    }
    zdj_add_subview( state->hilite_view, toggle_hi );
    toggle_hi->frame->x = view->frame->w - 6;
    toggle_hi->frame->y = 1;

    state->needs_layout_init = false;
}

void zdj_menu_item_toggle_update_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    state->needs_layout_update = false;
}
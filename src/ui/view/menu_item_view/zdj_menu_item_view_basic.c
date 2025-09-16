#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>

void zdj_menu_item_basic_l_init_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    // Clear out the normal/hilite views' subviews
    if( state->hilite_view ) { 
        zdj_remove_all_subviews_of( state->hilite_view ); 
    } else {
        state->hilite_view = zdj_new_view( NULL );
        zdj_add_subview( view, state->hilite_view );
        state->hilite_view->frame.w = view->frame.w;
        state->hilite_view->frame.h = view->frame.h;
    }
    if( state->normal_view ) { 
        zdj_remove_all_subviews_of( state->normal_view );
    } else {
        state->normal_view = zdj_new_view( NULL );
        zdj_add_subview( view, state->normal_view );
        state->normal_view->frame.w = view->frame.w;
        state->normal_view->frame.h = view->frame.h;
    }

    view->frame.h = 8;
    
    // Setup normal view
    zdj_view_t * title_ticker_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE );
    zdj_add_subview( state->normal_view, title_ticker_norm );
    title_ticker_norm->frame.x = ZDJ_MENU_ITEM_MARGIN_L;
    title_ticker_norm->frame.y = -1;
    title_ticker_norm->frame.w = view->frame.w - ZDJ_MENU_ITEM_MARGIN_L - ZDJ_MENU_ITEM_MARGIN_R;
    title_ticker_norm->frame.h = view->frame.h;
    
    // Setup hilite view
    zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_L ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg );
    hilite_bg->frame.y = 0;
    hilite_bg->frame.w = view->frame.w;
    hilite_bg->frame.h = 8;
    zdj_view_t * hilite_bg_r = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_R ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg_r );
    hilite_bg_r->frame.y = 0;
    hilite_bg_r->frame.x = view->frame.w-2;
    hilite_bg_r->frame.h = 8;

    zdj_view_t * title_ticker_hilite = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    zdj_ticker_state_t * ticker_state = (zdj_ticker_state_t*)title_ticker_hilite->state;
    zdj_add_subview( state->hilite_view, title_ticker_hilite );
    title_ticker_hilite->frame.x = ZDJ_MENU_ITEM_MARGIN_L;
    title_ticker_hilite->frame.y = -1;
    title_ticker_hilite->frame.w = view->frame.w - ZDJ_MENU_ITEM_MARGIN_L - ZDJ_MENU_ITEM_MARGIN_R;
    title_ticker_hilite->frame.h = view->frame.h;

    // Adjust hilite frame based on ticker's frame
    hilite_bg->frame.w = (int)fmin( view->frame.w, zdj_ticker_view_get_text_w( title_ticker_hilite ))+1;
    hilite_bg->frame.x = 0;

    state->needs_layout_init = false;
}

void zdj_menu_item_basic_r_init_layout( zdj_view_t * view ) {
    // printf( "zdj_menu_item_basic_r_init_layout\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    // Clear out the normal/hilite views' subviews
    if( state->hilite_view ) { 
        zdj_remove_all_subviews_of( state->hilite_view ); 
    } else {
        state->hilite_view = zdj_new_view( NULL );
        zdj_add_subview( view, state->hilite_view );
        state->hilite_view->frame.w = view->frame.w;
        state->hilite_view->frame.h = view->frame.h;
    }
    if( state->normal_view ) { 
        zdj_remove_all_subviews_of( state->normal_view );
    } else {
        state->normal_view = zdj_new_view( NULL );
        zdj_add_subview( view, state->normal_view );
        state->normal_view->frame.w = view->frame.w;
        state->normal_view->frame.h = view->frame.h;
    }

    view->frame.h = 8;
    
    // Setup normal view
    zdj_view_t * title_ticker_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE );
    zdj_add_subview( state->normal_view, title_ticker_norm );
    title_ticker_norm->frame.x = ZDJ_MENU_ITEM_MARGIN_L;
    title_ticker_norm->frame.y = -1;
    title_ticker_norm->frame.w = view->frame.w - ZDJ_MENU_ITEM_MARGIN_L - ZDJ_MENU_ITEM_MARGIN_R;
    title_ticker_norm->frame.h = view->frame.h;

    // Setup hilite view
    zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_L ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg );
    hilite_bg->frame.y = 0;
    hilite_bg->frame.w = view->frame.w;
    hilite_bg->frame.h = 8;
    zdj_view_t * hilite_bg_r = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_R ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg_r );
    hilite_bg_r->frame.y = 0;
    hilite_bg_r->frame.x = view->frame.w-2;
    hilite_bg_r->frame.h = 8;

    zdj_view_t * title_ticker_hilite = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_BLACK );
    zdj_ticker_state_t * ticker_state = (zdj_ticker_state_t*)title_ticker_hilite->state;
    zdj_add_subview( state->hilite_view, title_ticker_hilite );
    title_ticker_hilite->frame.x = ZDJ_MENU_ITEM_MARGIN_L;
    title_ticker_hilite->frame.y = -1;
    title_ticker_hilite->frame.w = view->frame.w - ZDJ_MENU_ITEM_MARGIN_L - ZDJ_MENU_ITEM_MARGIN_R;
    title_ticker_hilite->frame.h = view->frame.h;

    // Adjust hilite frame based on ticker's frame
    hilite_bg->frame.w = (int)fmin( view->frame.w, zdj_ticker_view_get_text_w( title_ticker_hilite ))+1;
    hilite_bg->frame.x = view->frame.w - hilite_bg->frame.w - 2;

    state->needs_layout_init = false;
}



void zdj_menu_item_basic_launch_r_init_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    // Clear out the normal/hilite views' subviews
    if( state->hilite_view ) { 
        zdj_remove_all_subviews_of( state->hilite_view ); 
    } else {
        state->hilite_view = zdj_new_view( NULL );
        zdj_add_subview( view, state->hilite_view );
        state->hilite_view->frame.w = view->frame.w;
        state->hilite_view->frame.h = view->frame.h;
    }
    if( state->normal_view ) { 
        zdj_remove_all_subviews_of( state->normal_view );
    } else {
        state->normal_view = zdj_new_view( NULL );
        zdj_add_subview( view, state->normal_view );
        state->normal_view->frame.w = view->frame.w;
        state->normal_view->frame.h = view->frame.h;
    }

    view->frame.h = 8;
    
    // Setup normal view
    zdj_view_t * title_ticker_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE );
    zdj_add_subview( state->normal_view, title_ticker_norm );
    title_ticker_norm->frame.x = ZDJ_MENU_ITEM_MARGIN_L;
    title_ticker_norm->frame.y = -1;
    title_ticker_norm->frame.w = view->frame.w - ZDJ_MENU_ITEM_MARGIN_L - ZDJ_MENU_ITEM_MARGIN_R;
    title_ticker_norm->frame.h = view->frame.h;

    zdj_view_t * launch = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_UP ], NULL );
    zdj_add_subview( state->normal_view, launch );
    launch->frame.x = view->frame.w - zdj_ticker_view_get_text_w( title_ticker_norm ) - 7;
    launch->frame.y = 0;
    
    // Setup hilite view
    zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_L ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg );
    hilite_bg->frame.y = 0;
    hilite_bg->frame.w = view->frame.w;
    hilite_bg->frame.h = 8;
    zdj_view_t * hilite_bg_r = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_R ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg_r );
    hilite_bg_r->frame.y = 0;
    hilite_bg_r->frame.x = view->frame.w-2;
    hilite_bg_r->frame.h = 8;

    zdj_view_t * launch_hi = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_UP_HI ], NULL );
    zdj_add_subview( state->hilite_view, launch_hi );
    launch_hi->frame.x = view->frame.w - zdj_ticker_view_get_text_w( title_ticker_norm ) - 7;
    launch_hi->frame.y = 0;

    zdj_view_t * title_ticker_hilite = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_BLACK );
    zdj_ticker_state_t * ticker_state = (zdj_ticker_state_t*)title_ticker_hilite->state;
    zdj_add_subview( state->hilite_view, title_ticker_hilite );
    title_ticker_hilite->frame.x = ZDJ_MENU_ITEM_MARGIN_L;
    title_ticker_hilite->frame.y = -1;
    title_ticker_hilite->frame.w = view->frame.w - ZDJ_MENU_ITEM_MARGIN_L - ZDJ_MENU_ITEM_MARGIN_R;
    title_ticker_hilite->frame.h = view->frame.h;

    // Adjust hilite frame based on ticker's frame
    hilite_bg->frame.w = (int)fmin( view->frame.w, zdj_ticker_view_get_text_w( title_ticker_hilite ))+7;
    hilite_bg->frame.x = view->frame.w - hilite_bg->frame.w - 2;

    state->needs_layout_init = false;
}
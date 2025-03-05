#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>

void zdj_menu_item_nohilite_l_update_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    // Clear out the normal/hilite views' subviews
    zdj_remove_all_subviews_of( state->hilite_view );
    zdj_remove_all_subviews_of( state->normal_view );

    state->normal_view->frame->w = view->frame->w;
    state->normal_view->frame->h = view->frame->h;
    state->hilite_view->frame->w = view->frame->w;
    state->hilite_view->frame->h = view->frame->h;
    
    // Setup normal view
    zdj_view_t * title_ticker_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( state->normal_view, title_ticker_norm );
    title_ticker_norm->frame->x = ZDJ_MENU_ITEM_MARGIN_L;
    title_ticker_norm->frame->w = view->frame->w - ZDJ_MENU_ITEM_MARGIN_L - ZDJ_MENU_ITEM_MARGIN_R;
    title_ticker_norm->frame->h = view->frame->h;
    
    // Setup hilite view
    zdj_view_t * title_ticker_hi = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( state->hilite_view, title_ticker_hi );
    title_ticker_hi->frame->x = ZDJ_MENU_ITEM_MARGIN_L;
    title_ticker_hi->frame->w = view->frame->w - ZDJ_MENU_ITEM_MARGIN_L - ZDJ_MENU_ITEM_MARGIN_R;
    title_ticker_hi->frame->h = view->frame->h;

    state->has_valid_display = true;
}

void zdj_menu_item_nohilite_r_update_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    // Clear out the normal/hilite views' subviews
    zdj_remove_all_subviews_of( state->hilite_view );
    zdj_remove_all_subviews_of( state->normal_view );

    state->normal_view->frame->w = view->frame->w;
    state->normal_view->frame->h = view->frame->h;
    state->hilite_view->frame->w = view->frame->w;
    state->hilite_view->frame->h = view->frame->h;
    
    // Setup normal view
    zdj_view_t * title_ticker_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE );
    zdj_add_subview( state->normal_view, title_ticker_norm );
    title_ticker_norm->frame->x = ZDJ_MENU_ITEM_MARGIN_L;
    title_ticker_norm->frame->w = view->frame->w - ZDJ_MENU_ITEM_MARGIN_L - ZDJ_MENU_ITEM_MARGIN_R;
    title_ticker_norm->frame->h = view->frame->h;
    
    // Setup hilite view
    zdj_view_t * title_ticker_hi = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE );
    zdj_add_subview( state->hilite_view, title_ticker_hi );
    title_ticker_hi->frame->x = ZDJ_MENU_ITEM_MARGIN_L;
    title_ticker_hi->frame->w = view->frame->w - ZDJ_MENU_ITEM_MARGIN_L - ZDJ_MENU_ITEM_MARGIN_R;
    title_ticker_hi->frame->h = view->frame->h;

    state->has_valid_display = true;
}
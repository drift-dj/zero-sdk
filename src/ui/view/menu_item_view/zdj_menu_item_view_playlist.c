#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>

static void _draw_edit_edit( zdj_view_t * view );
static void _draw_edit_delete( zdj_view_t * view );
static void _draw_edit_move( zdj_view_t * view );
static void _draw_edit_moving( zdj_view_t * view );
static void _draw_edit_done( zdj_view_t * view );

static void _enter_edit_mode( zdj_view_t * item );
static void _exit_edit_mode( zdj_view_t * item );

void zdj_menu_item_playlist_init_layout( zdj_view_t * view ) {
    // printf( "zdj_menu_item_playlist_init_layout\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    state->enter_edit_mode = _enter_edit_mode;
    state->exit_edit_mode = _exit_edit_mode;

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

void zdj_menu_item_playlist_update_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    // printf( "zdj_menu_item_playlist_update_layout\n" );
    if( state->edit_action == ZDJ_MENU_ITEM_ACTION_END_MOVE ) {
        _draw_edit_moving( view );
    } else {
        int _edit_option_index = (int)round(state->edit_option_index);
        switch( _edit_option_index ) {
            case 0:
                state->edit_action = ZDJ_MENU_ITEM_ACTION_DELETE;
                _draw_edit_delete( view );
                break;
            case 1:
                state->edit_action = ZDJ_MENU_ITEM_ACTION_EDIT;
                _draw_edit_edit( view );
                break;
            case 2:
                state->edit_action = ZDJ_MENU_ITEM_ACTION_START_MOVE;
                _draw_edit_move( view );
                break;
            case 3:
                state->edit_action = ZDJ_MENU_ITEM_ACTION_DONE;
                _draw_edit_done( view );
                break;
        }
    }
    state->needs_layout_update = false;
}

static void _draw_edit_edit( zdj_view_t * view ) {
    // printf( "_draw_edit_edit\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_remove_all_subviews_of( state->hilite_view );
    zdj_remove_all_subviews_of( state->normal_view );

    // Setup normal view
    zdj_view_t * title_ticker_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE );
    zdj_add_subview( state->hilite_view, title_ticker_norm );
    title_ticker_norm->frame.x = ZDJ_MENU_ITEM_MARGIN_L;
    title_ticker_norm->frame.y = -1;
    title_ticker_norm->frame.w = view->frame.w - ZDJ_MENU_ITEM_MARGIN_L - ZDJ_MENU_ITEM_MARGIN_R - 17;
    title_ticker_norm->frame.h = view->frame.h;

    zdj_view_t * btn = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MENU_EDIT ], NULL );
    zdj_add_subview( state->hilite_view, btn );
    btn->frame.y = 1;
    btn->frame.x = view->frame.w - 17;
}

static void _draw_edit_delete( zdj_view_t * view ) {
    // printf( "_draw_edit_delete\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_remove_all_subviews_of( state->hilite_view );
    zdj_remove_all_subviews_of( state->normal_view );

    // Setup normal view
    zdj_view_t * title_ticker_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE );
    zdj_add_subview( state->hilite_view, title_ticker_norm );
    title_ticker_norm->frame.x = ZDJ_MENU_ITEM_MARGIN_L;
    title_ticker_norm->frame.y = -1;
    title_ticker_norm->frame.w = view->frame.w - ZDJ_MENU_ITEM_MARGIN_L - ZDJ_MENU_ITEM_MARGIN_R - 10;
    title_ticker_norm->frame.h = view->frame.h;

    zdj_view_t * btn = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MENU_DELETE ], NULL );
    zdj_add_subview( state->hilite_view, btn );
    btn->frame.y = 1;
    btn->frame.x = view->frame.w - 10;
}

static void _draw_edit_move( zdj_view_t * view ) {
    // printf( "_draw_edit_move\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_remove_all_subviews_of( state->hilite_view );
    zdj_remove_all_subviews_of( state->normal_view );

    // Setup normal view
    zdj_view_t * title_ticker_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE );
    zdj_add_subview( state->hilite_view, title_ticker_norm );
    title_ticker_norm->frame.x = ZDJ_MENU_ITEM_MARGIN_L;
    title_ticker_norm->frame.y = -1;
    title_ticker_norm->frame.w = view->frame.w - ZDJ_MENU_ITEM_MARGIN_L - ZDJ_MENU_ITEM_MARGIN_R - 22;
    title_ticker_norm->frame.h = view->frame.h;

    zdj_view_t * btn = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MENU_MOVE ], NULL );
    zdj_add_subview( state->hilite_view, btn );
    btn->frame.y = 1;
    btn->frame.x = view->frame.w - 22;
}

static void _draw_edit_moving( zdj_view_t * view ) {
    // printf( "_draw_edit_moving\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_remove_all_subviews_of( state->hilite_view );
    zdj_remove_all_subviews_of( state->normal_view );

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
    title_ticker_hilite->frame.w = view->frame.w - ZDJ_MENU_ITEM_MARGIN_L - ZDJ_MENU_ITEM_MARGIN_R - 8;
    title_ticker_hilite->frame.h = view->frame.h;

    // Adjust hilite frame based on ticker's frame
    hilite_bg->frame.w = (int)fmin( view->frame.w, zdj_ticker_view_get_text_w( title_ticker_hilite ))+9;
    hilite_bg->frame.x = view->frame.w - hilite_bg->frame.w - 2;

    zdj_view_t * btn = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MENU_MOVING ], NULL );
    zdj_add_subview( state->hilite_view, btn );
    btn->frame.y = 1;
    btn->frame.x = view->frame.w - 8;
}

static void _draw_edit_done( zdj_view_t * view ) {
    // printf( "_draw_edit_done\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_remove_all_subviews_of( state->hilite_view );
    zdj_remove_all_subviews_of( state->normal_view );

    // Setup normal view
    zdj_view_t * title_ticker_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE );
    zdj_add_subview( state->hilite_view, title_ticker_norm );
    title_ticker_norm->frame.x = ZDJ_MENU_ITEM_MARGIN_L;
    title_ticker_norm->frame.y = -1;
    title_ticker_norm->frame.w = view->frame.w - ZDJ_MENU_ITEM_MARGIN_L - ZDJ_MENU_ITEM_MARGIN_R - 12;
    title_ticker_norm->frame.h = view->frame.h;

    zdj_view_t * btn = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MENU_OKAY ], NULL );
    zdj_add_subview( state->hilite_view, btn );
    btn->frame.y = 1;
    btn->frame.x = view->frame.w - 11;
}

static void _enter_edit_mode( zdj_view_t * item ) {
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;
    
    // Update layout to first edit option state
    item_state->edit_active = true;
    item_state->edit_option_index = 0.0f;
    item_state->needs_layout_update = true;
}

static void _exit_edit_mode( zdj_view_t * item ) {
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;
    
    // Update layout to normal state
    item_state->edit_active = false;
    item_state->needs_layout_init = true;    
    item_state->edit_action = ZDJ_MENU_ITEM_ACTION_SELECT;
}
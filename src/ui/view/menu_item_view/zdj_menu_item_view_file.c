#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/deck/zdj_deck_manager.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/progress_bar_view/zdj_progress_bar_view.h>

static void _draw_base( zdj_view_t * view );
static void _draw_open( zdj_view_t * view );
static void _draw_duplicate( zdj_view_t * view );
static void _draw_move( zdj_view_t * view );
static void _draw_delete( zdj_view_t * view );

static void _enter_edit_mode( zdj_view_t * item );
static void _exit_edit_mode( zdj_view_t * item );

void zdj_menu_item_file_init_layout( zdj_view_t * view ) {
    // printf( "zdj_menu_item_file_init_layout\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    state->enter_edit_mode = _enter_edit_mode;
    state->exit_edit_mode = _exit_edit_mode;
    state->scroll_to_exit_edit_mode = true;

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
    zdj_view_t * title_norm = zdj_new_label_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( state->normal_view, title_norm );
    title_norm->frame.x = 1;
    title_norm->frame.y = -1;

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
    title_ticker_hilite->frame.x = 1;
    title_ticker_hilite->frame.y = -1;
    title_ticker_hilite->frame.w = view->frame.w;
    title_ticker_hilite->frame.h = view->frame.h;

    state->needs_layout_init = false;
}

void zdj_menu_item_file_update_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    int _edit_option_index = (int)round(state->edit_option_index);
    // printf( "zdj_menu_item_file_update_layout: %d\n", _edit_option_index );
    switch( _edit_option_index ) {
        case 0:
            break;
        case 1:
            state->edit_action = ZDJ_MENU_ITEM_ACTION_DONE;
            _draw_base( view );
            break;
        case 2:
        case 3:
            state->edit_action = ZDJ_MENU_ITEM_ACTION_OPEN;
            state->action = ZDJ_MENU_ITEM_ACTION_FILE_OPEN;
            _draw_open( view );
            break;
        case 4:
        case 5:
            state->edit_action = ZDJ_MENU_ITEM_ACTION_DUPLICATE;
            state->action = ZDJ_MENU_ITEM_ACTION_FILE_DUPLICATE;
            _draw_duplicate( view );
            break;
        case 6:
        case 7:
            state->edit_action = ZDJ_MENU_ITEM_ACTION_MOVE_FILE;
            state->action = ZDJ_MENU_ITEM_ACTION_FILE_MOVE;
            _draw_move( view );
            break;
        case 8:
        case 9:
            state->edit_action = ZDJ_MENU_ITEM_ACTION_DELETE;
            state->action = ZDJ_MENU_ITEM_ACTION_FILE_DELETE;
            _draw_delete( view );
            break;
        case 10:
            state->edit_action = ZDJ_MENU_ITEM_ACTION_DONE;
            _draw_base( view );
            break;
        case 11:
            break;
    }

    state->needs_layout_update = false;
}


static void _draw_base( zdj_view_t * view ) {
    // printf( "_draw_base\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_remove_all_subviews_of( state->hilite_view );
    zdj_remove_all_subviews_of( state->normal_view );

    // Setup normal view
    zdj_view_t * title_ticker_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( state->normal_view, title_ticker_norm );
    title_ticker_norm->frame.x = 1;
    title_ticker_norm->frame.y = -1;
    title_ticker_norm->frame.w = view->frame.w;
    title_ticker_norm->frame.h = view->frame.h;
    
    // Setup hilite view
    zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_L ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg );
    hilite_bg->frame.y = 0;
    hilite_bg->frame.w = view->frame.w;
    hilite_bg->frame.h = 8;

    zdj_view_t * title_ticker_hilite = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    zdj_ticker_state_t * ticker_state = (zdj_ticker_state_t*)title_ticker_hilite->state;
    zdj_add_subview( state->hilite_view, title_ticker_hilite );
    title_ticker_hilite->frame.x = 1;
    title_ticker_hilite->frame.y = -1;
    title_ticker_hilite->frame.w = view->frame.w;
    title_ticker_hilite->frame.h = view->frame.h;
}

static void _draw_open( zdj_view_t * view ) {
    // printf( "_draw_open\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_remove_all_subviews_of( state->hilite_view );
    zdj_remove_all_subviews_of( state->normal_view );

    // Setup normal view
    zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_L ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg );
    hilite_bg->frame.y = 0;
    hilite_bg->frame.w = view->frame.w;
    hilite_bg->frame.h = 8;

    zdj_view_t * title_ticker_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    zdj_add_subview( state->hilite_view, title_ticker_norm );
    title_ticker_norm->frame.x = 12;
    title_ticker_norm->frame.y = -1;
    title_ticker_norm->frame.w = view->frame.w - 12;
    title_ticker_norm->frame.h = view->frame.h;

    zdj_view_t * icon= zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_FILE_OPEN ], NULL );
    icon->frame.x = -1;
    icon->frame.y = -2;
    zdj_add_subview( state->hilite_view, icon );
}

static void _draw_duplicate( zdj_view_t * view ) {
    // printf( "_draw_duplicate\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_remove_all_subviews_of( state->hilite_view );
    zdj_remove_all_subviews_of( state->normal_view );

    // Setup normal view
    zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_L ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg );
    hilite_bg->frame.y = 0;
    hilite_bg->frame.w = view->frame.w;
    hilite_bg->frame.h = 8;

    zdj_view_t * title_ticker_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    zdj_add_subview( state->hilite_view, title_ticker_norm );
    title_ticker_norm->frame.x = 10;
    title_ticker_norm->frame.y = -1;
    title_ticker_norm->frame.w = view->frame.w - 10;
    title_ticker_norm->frame.h = view->frame.h;

    zdj_view_t * icon= zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_FILE_DUPLICATE], NULL );
    icon->frame.x = -1;
    icon->frame.y = -2;
    zdj_add_subview( state->hilite_view, icon );
}

static void _draw_move( zdj_view_t * view ) {
    // printf( "_draw_move\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_remove_all_subviews_of( state->hilite_view );
    zdj_remove_all_subviews_of( state->normal_view );

    // Setup normal view
    zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_L ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg );
    hilite_bg->frame.y = 0;
    hilite_bg->frame.w = view->frame.w;
    hilite_bg->frame.h = 8;

    zdj_view_t * title_ticker_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    zdj_add_subview( state->hilite_view, title_ticker_norm );
    title_ticker_norm->frame.x = 10;
    title_ticker_norm->frame.y = -1;
    title_ticker_norm->frame.w = view->frame.w - 10;
    title_ticker_norm->frame.h = view->frame.h;

    zdj_view_t * btn = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_FILE_MOVE ], NULL );
    btn->frame.x = -1;
    btn->frame.y = -2;
    zdj_add_subview( state->hilite_view, btn );
}

static void _draw_delete( zdj_view_t * view ) {
    // printf( "_draw_delete\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_remove_all_subviews_of( state->hilite_view );
    zdj_remove_all_subviews_of( state->normal_view );

    // Setup normal view
    zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_L ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg );
    hilite_bg->frame.y = 0;
    hilite_bg->frame.w = view->frame.w;
    hilite_bg->frame.h = 8;

    zdj_view_t * title_ticker_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    zdj_add_subview( state->hilite_view, title_ticker_norm );
    title_ticker_norm->frame.x = 12;
    title_ticker_norm->frame.y = -1;
    title_ticker_norm->frame.w = view->frame.w - 12;
    title_ticker_norm->frame.h = view->frame.h;

    zdj_view_t * btn = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_DELETE ], NULL );
    btn->frame.x = -1;
    btn->frame.y = -2;
    zdj_add_subview( state->hilite_view, btn );
}

static void _enter_edit_mode( zdj_view_t * item ) {
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;

    // Update layout to first edit option state
    item_state->edit_option_index = 2;
    item_state->edit_active = true;
    item_state->needs_layout_update = true;

}

static void _exit_edit_mode( zdj_view_t * item ) {
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;
    
    // Update layout to normal state
    item_state->edit_active = false;
    item_state->needs_layout_init = true;    
    item_state->edit_option_index = 1.0;
    item_state->edit_action = ZDJ_MENU_ITEM_ACTION_SELECT;
}
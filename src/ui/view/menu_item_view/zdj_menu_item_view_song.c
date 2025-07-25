#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/progress_bar_view/zdj_progress_bar_view.h>

void zdj_menu_item_song_import_init_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    // // Determine layout based on song import state
    // zdj_library_song_t * song = (zdj_library_song_t*)state->data->ptr;
    // if( !song || !song->catalog || !song->catalog->title ){ 
    //     state->normal_view = zdj_new_ticker_view( "Error", ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE );
    // } else {
    //     state->normal_view = zdj_new_ticker_view( song->catalog->title, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE );
    // }

    // Add title ticker
    state->normal_view = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE );
    zdj_add_subview( view, state->normal_view );
    state->normal_view->frame->x = ZDJ_MENU_ITEM_MARGIN_L;
    state->normal_view->frame->w = view->frame->w - ZDJ_MENU_ITEM_MARGIN_L - ZDJ_MENU_ITEM_MARGIN_R;
    state->normal_view->frame->h = view->frame->h;

    state->needs_layout_init = false;
}

void zdj_menu_item_song_import_update_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    state->normal_view->frame->w = view->frame->w - 35;

    if( !state->hilite_view ) {
        state->hilite_view = zdj_new_progress_bar_view( &(zdj_rect_t){ view->frame->w-28,1,24,4 }, ZDJ_PROGRESS_BAR_VIEW_NORMAL );
        zdj_add_subview( view, state->hilite_view );
    }

    zdj_library_song_t * song = (zdj_library_song_t*)state->data->ptr;
    if( state->hilite_view && song ) {
        zdj_progress_bar_view_state_t * progress_state = (zdj_progress_bar_view_state_t*)state->hilite_view->state;
        progress_state->val = song->analysis_progress;
    }
    
    state->needs_layout_update = false;
}
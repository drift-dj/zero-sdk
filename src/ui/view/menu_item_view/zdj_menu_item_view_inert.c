#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>

void zdj_menu_item_inert_init_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    // Clear out the normal/hilite views' subviews
    zdj_remove_all_subviews_of( view );

    // Setup normal view
    zdj_view_t * title_ticker = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE );
    zdj_add_subview( view, title_ticker );
    title_ticker->frame->w = view->frame->w;
    title_ticker->frame->h = view->frame->h;

    state->needs_layout_init = false;
}

void zdj_menu_item_inert_data_init_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    // Clear out the normal/hilite views' subviews
    zdj_remove_all_subviews_of( view );

    // Setup normal view
    state->title_view = zdj_new_label_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( view, state->title_view );

    // Add divider
    state->div_view = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_WIDE_H_DIV ], NULL );
    zdj_add_subview( view, state->div_view );
    state->div_view->frame->y = 4;
    state->div_view->frame->x = state->title_view->frame->w + 3;
    state->div_view->frame->w = view->frame->w - state->title_view->frame->w;

    state->needs_layout_init = false;
}

void zdj_menu_item_inert_data_update_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    if( state->data_view ) { zdj_remove_subview_of( view, state->data_view ); }

    char data_str[ 64 ];
    snprintf( data_str, sizeof( data_str ), "%d Songs", *(int*)state->data->ptr );

    state->data_view = zdj_new_label_view( data_str, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE );
    zdj_add_subview( view, state->data_view );
    state->data_view->frame->x = view->frame->w - state->data_view->frame->w;
    state->data_view->frame->y = 0;

    state->div_view->frame->w = view->frame->w - state->title_view->frame->w - state->data_view->frame->w - 3;

    state->needs_layout_update = false;
}

void zdj_menu_item_inert_status_init_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    // Clear out the normal/hilite views' subviews
    zdj_remove_all_subviews_of( view );

    // Setup normal view
    state->title_view = zdj_new_label_view( state->data->c_val, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( view, state->title_view );

    state->needs_layout_init = false;
}

void zdj_menu_item_inert_status_update_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    // Clear out the normal/hilite views' subviews
    zdj_remove_all_subviews_of( view );

    // Setup normal view
    state->title_view = zdj_new_label_view( state->data->c_val, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( view, state->title_view );

    state->needs_layout_update = false;
}
#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>

void zdj_menu_item_data_l_update_layout( zdj_view_t * view ) { }

void zdj_menu_item_data_r_update_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    // Clear out the normal/hilite views' subviews
    zdj_remove_subviews_of( state->hilite_view );
    zdj_remove_subviews_of( state->normal_view );

    state->normal_view->frame->w = view->frame->w;
    state->normal_view->frame->h = view->frame->h;
    state->hilite_view->frame->w = view->frame->w;
    state->hilite_view->frame->h = view->frame->h;

    // Pin title label to the left edge + margin
    zdj_view_t * title_label_norm = zdj_new_label_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( state->normal_view, title_label_norm );
    title_label_norm->frame->x = 5;
    title_label_norm->frame->y = 1;

    // Get value from data instance
    char * data_val = state->data->c_val;
    if( !data_val ) {
        data_val = "No Data";
    }
    zdj_view_t * data_label_norm = zdj_new_label_view( data_val, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( state->normal_view, data_label_norm );
    data_label_norm->frame->x = view->frame->w - data_label_norm->frame->w;
    data_label_norm->frame->y = 1;
    
    // Add dots
    zdj_view_t * divider = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_WIDE_H_DIV ], NULL );
    zdj_add_subview( state->normal_view, divider );
    divider->frame->h = 1;
    divider->frame->y = 5;
    divider->frame->w = view->frame->w - title_label_norm->frame->w - data_label_norm->frame->w - 9;
    divider->frame->x = title_label_norm->frame->x + title_label_norm->frame->w + 2;

    
    // Setup hilite view
    zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_WHITE ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg );

    // Pin title label to the left edge + margin
    zdj_view_t * title_label_hi = zdj_new_label_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    zdj_add_subview( state->hilite_view, title_label_hi );
    title_label_hi->frame->x = 5;
    title_label_hi->frame->y = 1;

    // Get value from data instance
    zdj_view_t * data_label_hi = zdj_new_label_view( data_val, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    zdj_add_subview( state->hilite_view, data_label_hi );
    data_label_hi->frame->x = view->frame->w - data_label_hi->frame->w;
    data_label_hi->frame->y = 1;

    // Adjust hilite frame based on ticker's frame
    hilite_bg->frame->w = view->frame->w - 5;
    hilite_bg->frame->x = view->frame->w - hilite_bg->frame->w;
    hilite_bg->frame->h = view->frame->h;

    state->has_valid_display = true;
}


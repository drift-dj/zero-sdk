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
    title_ticker->frame.w = view->frame.w;
    title_ticker->frame.h = view->frame.h;

    state->needs_layout_init = false;
}

void zdj_menu_item_inert_data_init_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    // Clear out the normal/hilite views' subviews
    zdj_remove_all_subviews_of( view );

    view->frame.h = 7;
    
    // Pin title label to the left edge + margin
    zdj_view_t * title_label_norm = zdj_new_label_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( view, title_label_norm );
    title_label_norm->frame.x = 6;
    title_label_norm->frame.y = -1;

    zdj_view_t * data_label_norm = zdj_new_label_view( " ", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( view, data_label_norm );
    data_label_norm->frame.x = view->frame.w - data_label_norm->frame.w;
    data_label_norm->frame.y = -1;
    
    // Add dots
    zdj_view_t * divider = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MID_H_DIV ], NULL );
    zdj_add_subview( view, divider );
    divider->frame.h = 1;
    divider->frame.y = 3;
    divider->frame.w = view->frame.w - title_label_norm->frame.w - data_label_norm->frame.w - 12;
    divider->frame.x = title_label_norm->frame.x + title_label_norm->frame.w + 2;

    // // Setup normal view
    // state->title_view = zdj_new_label_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    // zdj_add_subview( view, state->title_view );

    // // Add divider
    // state->div_view = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_WIDE_H_DIV ], NULL );
    // zdj_add_subview( view, state->div_view );
    // state->div_view->frame.y = 4;
    // state->div_view->frame.x = state->title_view->frame.w + 3;
    // state->div_view->frame.w = view->frame.w - state->title_view->frame.w;

    state->needs_layout_init = false;
}

void zdj_menu_item_inert_data_update_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    zdj_remove_all_subviews_of( view );
    
    view->frame.h = 7;

    // Build data string
    char data_str[ 256 ];
    switch ( state->data_type ) {
    case ZDJ_MENU_ITEM_DATA_TYPE_CHAR:
        snprintf( data_str, sizeof( data_str ), "%s%s%s", 
            state->data_prefix,
            state->data.c_val,
            state->data_suffix
        );
        break;
    case ZDJ_MENU_ITEM_DATA_TYPE_BOOL:
        snprintf( data_str, sizeof( data_str ), "%s%s%s", 
            state->data_prefix,
            ( state->data.b_val ) ? "Yes" : "No",
            state->data_suffix
        );
        break;
    case ZDJ_MENU_ITEM_DATA_TYPE_INT:
        snprintf( data_str, sizeof( data_str ), "%s%d%s", 
            state->data_prefix,
            state->data.i_val,
            state->data_suffix
        );
        break;
    case ZDJ_MENU_ITEM_DATA_TYPE_DOUBLE_0:
        snprintf( data_str, sizeof( data_str ), "%s%1.0f%s", 
            state->data_prefix,
            state->data.f_val,
            state->data_suffix
        );
        break;
    case ZDJ_MENU_ITEM_DATA_TYPE_DOUBLE_1:
        snprintf( data_str, sizeof( data_str ), "%s%1.0f%s", 
            state->data_prefix,
            state->data.f_val,
            state->data_suffix
        );
        break;
    case ZDJ_MENU_ITEM_DATA_TYPE_DOUBLE_2:
        snprintf( data_str, sizeof( data_str ), "%s%1.0f%s", 
            state->data_prefix,
            state->data.f_val,
            state->data_suffix
        );
        break;
    }
    
    // Pin title label to the left edge + margin
    zdj_view_t * title_label_norm = zdj_new_label_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( view, title_label_norm );
    title_label_norm->frame.x = 6;
    title_label_norm->frame.y = -1;

    zdj_view_t * data_label_norm = zdj_new_label_view( data_str, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( view, data_label_norm );
    data_label_norm->frame.x = view->frame.w - data_label_norm->frame.w;
    data_label_norm->frame.y = -1;
    
    // Add dots
    zdj_view_t * divider = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MID_H_DIV ], NULL );
    zdj_add_subview( view, divider );
    divider->frame.h = 1;
    divider->frame.y = 3;
    divider->frame.w = view->frame.w - title_label_norm->frame.w - data_label_norm->frame.w - 12;
    divider->frame.x = title_label_norm->frame.x + title_label_norm->frame.w + 2;

    state->needs_layout_update = false;
}

void zdj_menu_item_inert_status_init_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    // Clear out the normal/hilite views' subviews
    zdj_remove_all_subviews_of( view );

    // Setup normal view
    state->title_view = zdj_new_label_view( state->data.c_val, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( view, state->title_view );

    state->needs_layout_init = false;
}

void zdj_menu_item_inert_status_update_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    // Clear out the normal/hilite views' subviews
    zdj_remove_all_subviews_of( view );

    // Setup normal view
    state->title_view = zdj_new_label_view( state->data.c_val, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( view, state->title_view );

    state->needs_layout_update = false;
}
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

void zdj_menu_item_data_l_init_layout( zdj_view_t * view ) { }

void zdj_menu_item_data_r_init_layout( zdj_view_t * view ) {

    // printf( "data item layout init\n" );
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
        snprintf( data_str, sizeof( data_str ), "%s%1.1f%s", 
            state->data_prefix,
            state->data.f_val,
            state->data_suffix
        );
        break;
    case ZDJ_MENU_ITEM_DATA_TYPE_DOUBLE_2:
        snprintf( data_str, sizeof( data_str ), "%s%1.2f%s", 
            state->data_prefix,
            state->data.f_val,
            state->data_suffix
        );
        break;
    }
    
    // Pin title label to the left edge + margin
    zdj_view_t * title_label_norm = zdj_new_label_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( state->normal_view, title_label_norm );
    title_label_norm->frame.x = 1;
    title_label_norm->frame.y = -1;

    zdj_view_t * data_label_norm = zdj_new_label_view( data_str, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( state->normal_view, data_label_norm );
    // zdj_menu_item_view_state_t * data_label_state = (zdj_menu_item_view_state_t*)data_label_norm->state;
    // float data_label_w = data_label_state->title_view
    data_label_norm->frame.x = view->frame.w - data_label_norm->frame.w;
    data_label_norm->frame.y = -1;
    
    // Add dots
    zdj_view_t * divider = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_WIDE_H_DIV ], NULL );
    zdj_add_subview( state->normal_view, divider );
    divider->frame.h = 1;
    divider->frame.y = 3;
    divider->frame.w = view->frame.w - title_label_norm->frame.w - data_label_norm->frame.w - 4;
    divider->frame.x = title_label_norm->frame.x + title_label_norm->frame.w + 2;

    // Setup hilite view
    zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_L ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg );
    hilite_bg->frame.y = 0;
    hilite_bg->frame.w = view->frame.w;
    zdj_view_t * hilite_bg_r = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_R ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg_r );
    hilite_bg_r->frame.y = 0;
    hilite_bg_r->frame.x = view->frame.w-1;

    // Pin title label to the left edge + margin
    zdj_view_t * title_label_hi = zdj_new_label_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    zdj_add_subview( state->hilite_view, title_label_hi );
    title_label_hi->frame.x = 1;
    title_label_hi->frame.y = -1;

    // Get value from data instance
    zdj_view_t * data_label_hi = zdj_new_label_view( data_str, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    zdj_add_subview( state->hilite_view, data_label_hi );
    data_label_hi->frame.x = view->frame.w - data_label_hi->frame.w;
    data_label_hi->frame.y = -1;

    // Add dots
    zdj_view_t * divider_hi = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_WIDE_H_DIV_HI ], NULL );
    zdj_add_subview( state->hilite_view, divider_hi );
    divider_hi->frame.h = 1;
    divider_hi->frame.y = 3;
    divider_hi->frame.w = view->frame.w - title_label_hi->frame.w - data_label_hi->frame.w - 4;
    divider_hi->frame.x = title_label_hi->frame.x + title_label_hi->frame.w + 2;

    // Adjust hilite frame based on ticker's frame
    hilite_bg->frame.w = view->frame.w;
    hilite_bg->frame.x = view->frame.w - hilite_bg->frame.w;
    hilite_bg->frame.h = view->frame.h;
    
    state->needs_layout_init = false;
}

void zdj_menu_item_data_l_update_layout( zdj_view_t * view ) {

}

void zdj_menu_item_data_r_update_layout( zdj_view_t * view ) {

}
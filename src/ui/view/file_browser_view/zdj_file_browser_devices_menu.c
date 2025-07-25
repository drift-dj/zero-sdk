#include <stdio.h>
#include <dirent.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/controls/hmi/zdj_hmi.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/file_browser_view/zdj_file_browser_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

void _update_zero_layout( zdj_view_t * view );
void _update_msd_layout( zdj_view_t * view );

// Show all media devices currently connected
zdj_view_t * zdj_new_device_browser_menu( zdj_view_t * browser, zdj_rect_t * frame ) {
    zdj_file_browser_view_state_t * browser_state = (zdj_file_browser_view_state_t *)browser->state;

    // Update the header's path display
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)browser_state->header_view->state;
    header_state->title = "Media";
    header_state->has_valid_display = false;

    // Make menu
    zdj_view_t * menu = zdj_new_menu_view( ZDJ_HORIZONTAL, frame );
    menu->frame->x = 0;
    menu->frame->y = 0;
    

    // Add Zero item
    zdj_view_t * zero_item = zdj_new_menu_item( "Zero", ZDJ_MENU_ITEM_LAYOUT_ICON );
    zdj_menu_item_view_state_t * zero_state = (zdj_menu_item_view_state_t*)zero_item->state;
    zero_state->action = ZDJ_MENU_ITEM_ACTION_DIR_ENTER;
    zero_state->link = "/media/internal/";
    zero_state->data->ptr = browser;
    zero_state->handles_hmi = true;
    zero_item->handle_hmi_event = &zdj_file_browser_item_hmi_delegate;
    zero_item->frame->x = 8;
    zero_item->frame->y = 12;
    zero_item->frame->w = 30;
    zero_item->frame->h = 25;
    zdj_menu_view_add_item( menu, zero_item );

    // Add attached MSDs
    zdj_view_t * msd_item = zdj_new_menu_item( "MSD", ZDJ_MENU_ITEM_LAYOUT_ICON );
    zdj_menu_item_view_state_t * msd_state = (zdj_menu_item_view_state_t*)msd_item->state;
    msd_state->action = ZDJ_MENU_ITEM_ACTION_DIR_ENTER;
    msd_state->link = "/media/internal/";
    msd_state->data->ptr = browser;
    msd_state->handles_hmi = true;
    msd_item->handle_hmi_event = &zdj_file_browser_item_hmi_delegate;
    msd_item->frame->x = 50;
    msd_item->frame->y = 12;
    msd_item->frame->w = 30;
    msd_item->frame->h = 25;
    zdj_menu_view_add_item( menu, msd_item );

    return menu;
}

// void _update_zero_layout( zdj_view_t * view ) {
//     zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

//     // Clear out the normal/hilite views' subviews
//     zdj_remove_all_subviews_of( state->hilite_view );
//     zdj_remove_all_subviews_of( state->normal_view );

//     state->normal_view->frame->w = view->frame->w;
//     state->normal_view->frame->h = view->frame->h;
//     state->hilite_view->frame->w = view->frame->w;
//     state->hilite_view->frame->h = view->frame->h;
    
//     // Setup normal view
//     zdj_view_t * title_label = zdj_new_label_view( "Zero", ZDJ_FONT_6, ZDJ_JUSTIFY_CENTER, ZDJ_SDL_WHITE );
//     zdj_add_subview( state->normal_view, title_label );
//     title_label->frame->x = 4;
//     title_label->frame->y = 15;

//     zdj_view_t * icon = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_ZERO ], NULL );
//     zdj_add_subview( state->normal_view, icon );
//     icon->frame->x = 4;
//     icon->frame->y = 3;

//     // Setup hilite view
//     zdj_view_t * icon_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_11_L ], NULL );
//     zdj_add_subview( state->hilite_view, icon_bg );
//     icon_bg->frame->x = 2;
//     icon_bg->frame->y = 2;
//     icon_bg->frame->w = 19;
//     zdj_view_t * icon_bg_r = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_11_R ], NULL );
//     zdj_add_subview( state->hilite_view, icon_bg_r );
//     icon_bg_r->frame->x = 20;
//     icon_bg_r->frame->y = 2;

//     zdj_view_t * title_label_hi = zdj_new_label_view( "Zero", ZDJ_FONT_6, ZDJ_JUSTIFY_CENTER, ZDJ_SDL_WHITE );
//     zdj_add_subview( state->hilite_view, title_label_hi );
//     title_label_hi->frame->x = 4;
//     title_label_hi->frame->y = 15;

//     zdj_view_t * icon_hi = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_ZERO_HI ], NULL );
//     zdj_add_subview( state->hilite_view, icon_hi );
//     icon_hi->frame->x = 4;
//     icon_hi->frame->y = 3;

//     state->has_valid_display = true;
// }

// void _update_msd_layout( zdj_view_t * view ) {
//     zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

//     // Clear out the normal/hilite views' subviews
//     zdj_remove_all_subviews_of( state->hilite_view );
//     zdj_remove_all_subviews_of( state->normal_view );

//     state->normal_view->frame->w = view->frame->w;
//     state->normal_view->frame->h = view->frame->h;
//     state->hilite_view->frame->w = view->frame->w;
//     state->hilite_view->frame->h = view->frame->h;
    
//     // Setup normal view
//     zdj_view_t * title_label = zdj_new_label_view( "Untitled", ZDJ_FONT_6, ZDJ_JUSTIFY_CENTER, ZDJ_SDL_WHITE );
//     zdj_add_subview( state->normal_view, title_label );
//     title_label->frame->x = 0;
//     title_label->frame->y = 15;

//     zdj_view_t * icon = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DRIVE ], NULL );
//     zdj_add_subview( state->normal_view, icon );
//     icon->frame->x = 7;
//     icon->frame->y = 2;

//     // Setup hilite view
//     zdj_view_t * icon_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_12_L ], NULL );
//     zdj_add_subview( state->hilite_view, icon_bg );
//     icon_bg->frame->x = 5;
//     icon_bg->frame->y = 1;
//     icon_bg->frame->w = 15;
//     zdj_view_t * icon_bg_r = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_12_R ], NULL );
//     zdj_add_subview( state->hilite_view, icon_bg_r );
//     icon_bg_r->frame->x = 20;
//     icon_bg_r->frame->y = 1;

//     zdj_view_t * title_label_hi = zdj_new_label_view( "Untitled", ZDJ_FONT_6, ZDJ_JUSTIFY_CENTER, ZDJ_SDL_WHITE );
//     zdj_add_subview( state->hilite_view, title_label_hi );
//     title_label_hi->frame->x = 0;
//     title_label_hi->frame->y = 15;

//     zdj_view_t * icon_hi = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DRIVE_HI ], NULL );
//     zdj_add_subview( state->hilite_view, icon_hi );
//     icon_hi->frame->x = 7;
//     icon_hi->frame->y = 2;

//     state->has_valid_display = true;
// }
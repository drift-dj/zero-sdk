#include <stdio.h>
#include <string.h>
#include <sys/statvfs.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>

void zdj_menu_item_browser_device_init_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    view->frame.h = 52;
    // Clear out the normal/hilite views' subviews
    if( state->normal_view ) { 
        zdj_remove_all_subviews_of( state->normal_view );
    } else {
        state->normal_view = zdj_new_view( NULL );
        zdj_add_subview( view, state->normal_view );
        state->normal_view->frame.w = view->frame.w;
        state->normal_view->frame.h = view->frame.h;
    }
    if( state->hilite_view ) { 
        zdj_remove_all_subviews_of( state->hilite_view ); 
    } else {
        state->hilite_view = zdj_new_view( NULL );
        zdj_add_subview( view, state->hilite_view );
        state->hilite_view->frame.w = view->frame.w;
        state->hilite_view->frame.h = view->frame.h;
    }

    // Get sizes
    struct statvfs stat;
    uint64_t total = 1;
    uint64_t avail = 1;
    if ( statvfs( state->link, &stat ) == 0 ) {
        total = ((uint64_t)stat.f_blocks * stat.f_frsize) / 1000000;
        avail = ((uint64_t)stat.f_bavail * stat.f_frsize) / 1000000;
    }

    // printf( "statvfs: %s %lu %lu\n", state->link, total, avail );
    

    // Setup normal view
    zdj_view_t * icon;
    zdj_view_t * icon_hi;
    switch ( state->data.i_val ) {
    case ZDJ_MENU_ITEM_BROWSER_DEVICE_TYPE_ZERO:
        icon = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_ZERO ], NULL );
        icon_hi = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_ZERO ], NULL );
        break;
    case ZDJ_MENU_ITEM_BROWSER_DEVICE_TYPE_LINUX:
        icon = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_PROMPT ], NULL );
        icon_hi = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_PROMPT ], NULL );
        break;
    case ZDJ_MENU_ITEM_BROWSER_DEVICE_TYPE_MSD:
        icon = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_USB ], NULL );
        icon_hi = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_USB ], NULL );
        break;
    }
    zdj_add_subview( state->normal_view, icon );
    icon->frame.y = 15 - icon->frame.h;
    zdj_add_subview( state->hilite_view, icon_hi );
    icon_hi->frame.y = 15 - icon_hi->frame.h;

    
    zdj_view_t * title_norm = zdj_new_label_view( state->title, ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( view, title_norm );
    title_norm->frame.y = 15;

    // zdj_view_t * title_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    // // zdj_add_subview( state->normal_view, title_norm );
    // zdj_add_subview( view, title_norm );
    // title_norm->frame.x = 0;
    // title_norm->frame.y = 15;
    // title_norm->frame.w = view->frame.w;
    // title_norm->frame.h = 8;

    char size_str [ 32 ];
    char avail_str [ 32 ];
    if( total > 999 ) {
        sprintf( size_str, "%1.1f/%1.1f", (float)(avail) / 1000.0, (float)total / 1000.0 );
        strcpy( avail_str, "GB AVAIL" );
    } else {
        sprintf( size_str, "%lu/%lu", (avail), total );
        strcpy( avail_str, "MB AVAIL" );
    }
    
    zdj_view_t * size_norm = zdj_new_label_view( size_str, ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    // zdj_add_subview( state->normal_view, size_norm );
    zdj_add_subview( view, size_norm );
    size_norm->frame.y = 25;

    zdj_view_t * avail_norm = zdj_new_label_view( avail_str, ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    // zdj_add_subview( state->normal_view, avail_norm );
    zdj_add_subview( view, avail_norm );
    avail_norm->frame.y = 32;

    zdj_view_t * size_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_SM_HATCH_TEX ], NULL );
    size_bg->frame.y = 41;
    size_bg->frame.w = avail_norm->frame.w;
    size_bg->frame.h = 3;
    // zdj_add_subview( state->normal_view, size_bg );
    zdj_add_subview( view, size_bg );

    zdj_view_t * size = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_WHITE ], NULL );
    size->frame.y = 41;
    size->frame.w = round( avail_norm->frame.w * ((float)(total - avail) / (float)total ) );
    size->frame.h = 3;
    // zdj_add_subview( state->normal_view, size );
    zdj_add_subview( view, size );


    // Setup hilite view
    // zdj_view_t * open_hi = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_UP_RIGHT_LG ], NULL );
    zdj_view_t * open_hi = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_OPEN ], NULL );
    zdj_add_subview( state->hilite_view, open_hi );
    open_hi->frame.x = icon->frame.w - 4;
    open_hi->frame.y = icon->frame.y - 2;
    // open_hi->frame.x = 10;
    // open_hi->frame.y = 2;

    // zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_L ], NULL );
    // zdj_add_subview( state->hilite_view, hilite_bg );
    // hilite_bg->frame.y = 16;
    // // hilite_bg->frame.w = view->frame.w;
    // // zdj_view_t * hilite_bg_r = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_8_R ], NULL );
    // // zdj_add_subview( state->hilite_view, hilite_bg_r );
    // // hilite_bg_r->frame.y = 16;
    // // hilite_bg_r->frame.x = view->frame.w-1;

    // zdj_view_t * title_ticker_hilite = zdj_new_ticker_view( state->title, ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_CENTER, ZDJ_SDL_BLACK );
    // zdj_add_subview( state->hilite_view, title_ticker_hilite );
    // title_ticker_hilite->frame.x = 1;
    // title_ticker_hilite->frame.y = 15;
    // // title_ticker_hilite->frame.w = view->frame.w;
    // title_ticker_hilite->frame.h = 9;

    // if( title_norm->frame.w < view->frame.w ) {
    //     title_norm->frame.x = round((view->frame.w / 2) - (title_norm->frame.w / 2));
    //     title_ticker_hilite->frame.x = title_norm->frame.x;
    //     title_ticker_hilite->frame.w = title_norm->frame.w + 10;
    //     hilite_bg->frame.x = title_norm->frame.x - 1;
    //     hilite_bg->frame.w = title_norm->frame.w + 1;
    // } else {
    //     title_ticker_hilite->frame.w = view->frame.w;
    //     hilite_bg->frame.w = view->frame.w;
    // }

    state->needs_layout_init = false;
}
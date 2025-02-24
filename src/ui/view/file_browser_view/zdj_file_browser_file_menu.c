#include <stdio.h>
#include <dirent.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/hmi/zdj_hmi.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/file_browser_view/zdj_file_browser_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

void _zdj_file_browser_add_dir_item_to_menu( zdj_view_t * browser, zdj_view_t * menu, char * dirname, char * path );
void _zdj_file_browser_add_file_item_to_menu( zdj_view_t * browser, zdj_view_t * menu, char * filename );

void _zdj_file_browser_nav_up_layout( zdj_view_t * view );
void _zdj_file_browser_dir_select_layout( zdj_view_t * view );

char * _zdj_file_browser_parent_dir( char * dir );

zdj_view_t * zdj_new_file_browser_menu_for_path( 
    zdj_view_t * browser,
    zdj_rect_t * frame, 
    char * path, 
    bool read_only,
    bool allow_nav,
    zdj_file_browser_select_type_t select_type 
) {
    zdj_file_browser_view_state_t * browser_state = (zdj_file_browser_view_state_t *)browser->state;

    // Update the header's path display
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)browser_state->header_view->state;
    header_state->title = strdup( path );
    header_state->has_valid_display = false;

    zdj_view_t * menu = zdj_new_menu_view( ZDJ_VERTICAL, frame );
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)menu->state;
    // We need to use menu's back button scroll index to show/hide browser's cancel button.
    menu_state->has_back = true;

    zdj_view_t * nav_up;
    zdj_view_t * dir_select;
    zdj_view_t * add_dir;
    // Add Top options
    if( allow_nav ) {

        // Add and 'up one dir' item 
        zdj_view_t * nav_up = zdj_new_menu_item( "Back" );
        zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)nav_up->state;
        state->action = ZDJ_MENU_ITEM_ACTION_DIR_BACK;
        state->update_layout = &_zdj_file_browser_nav_up_layout;
        state->link = _zdj_file_browser_parent_dir( path );
        // Nav buttons need a reference to parent browser for inserting menus
        state->data->ptr = browser;
        nav_up->handle_hmi_event = &zdj_file_browser_item_hmi_delegate;
        nav_up->frame->x = 1;
        nav_up->frame->y = 3;
        nav_up->frame->w = 15;
        nav_up->frame->h = 12;
        zdj_menu_view_add_item( menu, nav_up );
 
        // Add a 'select this dir' item
        if( select_type == ZDJ_FILE_BROWSER_SELECT_TYPE_DIR ) {
            zdj_view_t * dir_select = zdj_new_menu_item( "Save here" );
            zdj_menu_item_view_state_t * dir_select_state = (zdj_menu_item_view_state_t*)dir_select->state;
            dir_select_state->action = ZDJ_MENU_ITEM_ACTION_DIR_BACK;
            dir_select_state->update_layout = &_zdj_file_browser_dir_select_layout;
            dir_select->handle_hmi_event = &zdj_file_browser_item_hmi_delegate;
            dir_select->frame->x = 67;
            dir_select->frame->y = 3;
            dir_select->frame->w = 60;
            dir_select->frame->h = 12;
            zdj_menu_view_add_item( menu, dir_select );
        }
    }
    if( !read_only ) {
        // Add a 'new dir' item
    }

    // Scan dir contents, adding menu items
    DIR * dir = opendir( path );
    struct dirent * entry;
    int id = 0;
    if ( dir ) {
        // Look at every entry in dir.
        while ( ( entry = readdir( dir ) ) != NULL ) {
            if( entry->d_name[ 0 ] == '.' ) continue;
            if ( entry->d_type == DT_DIR ) {
                char dir_path[ 2048 ];
                snprintf( dir_path, sizeof( dir_path ), "%s/%s", path, entry->d_name );
                _zdj_file_browser_add_dir_item_to_menu( browser, menu, entry->d_name, dir_path );
            } else {
                _zdj_file_browser_add_file_item_to_menu( browser, menu, entry->d_name );
            }
        }
        closedir( dir );
    }

    return menu;
}

void _zdj_file_browser_add_dir_item_to_menu( zdj_view_t * browser, zdj_view_t * menu, char * dirname, char * path ) {
    zdj_view_t * item = zdj_new_menu_item( dirname );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)item->state;
    item->handle_hmi_event = &zdj_file_browser_item_hmi_delegate;
    state->update_layout = zdj_menu_item_update_for_layout( ZDJ_MENU_ITEM_LAYOUT_DIR_R );
    state->action = ZDJ_MENU_ITEM_ACTION_DIR_ENTER;
    state->link = strdup( path );
    state->data->ptr = browser;
    zdj_menu_view_add_item( menu, item );
}

void _zdj_file_browser_add_file_item_to_menu( zdj_view_t * browser, zdj_view_t * menu, char * filename ) {
    zdj_view_t * item = zdj_new_menu_item( strdup(filename) );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)item->state;
    item->handle_hmi_event = &zdj_file_browser_item_hmi_delegate;
    state->update_layout = zdj_menu_item_update_for_layout( ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
    state->action = ZDJ_MENU_ITEM_ACTION_FILE_SELECT;
    state->link = strdup( filename );
    state->data->ptr = browser;
    zdj_menu_view_add_item( menu, item );
}

void _zdj_file_browser_nav_up_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    // Clear out the normal/hilite views' subviews
    zdj_remove_subviews_of( state->hilite_view );
    zdj_remove_subviews_of( state->normal_view );

    state->normal_view->frame->w = view->frame->w;
    state->normal_view->frame->h = view->frame->h;
    state->hilite_view->frame->w = view->frame->w;
    state->hilite_view->frame->h = view->frame->h;

    zdj_view_t * dir_up_icon = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DIR_UP ], NULL );
    zdj_add_subview( state->normal_view, dir_up_icon );
    dir_up_icon->frame->x = 1;
    dir_up_icon->frame->y = 1;
    dir_up_icon->frame->w = zdj_ui_assets[ ZDJ_UI_ASSET_DIR_UP ].w;
    dir_up_icon->frame->h = zdj_ui_assets[ ZDJ_UI_ASSET_DIR_UP ].h;
    
    // // Setup hilite view
    zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_WHITE ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg );

    zdj_view_t * dir_up_icon_hi = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DIR_UP_HI ], NULL );
    zdj_add_subview( state->hilite_view, dir_up_icon_hi );
    dir_up_icon_hi->frame->x = 1;
    dir_up_icon_hi->frame->y = 1;
    dir_up_icon_hi->frame->w = zdj_ui_assets[ ZDJ_UI_ASSET_DIR_UP_HI ].w;
    dir_up_icon_hi->frame->h = zdj_ui_assets[ ZDJ_UI_ASSET_DIR_UP_HI ].h;

    // Adjust hilite frame based on ticker's frame
    hilite_bg->frame->w = view->frame->w+1;
    hilite_bg->frame->x = 0;
    hilite_bg->frame->h = 10;

    state->has_valid_display = true;
}

void _zdj_file_browser_dir_select_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    // Clear out the normal/hilite views' subviews
    zdj_remove_subviews_of( state->hilite_view );
    zdj_remove_subviews_of( state->normal_view );

    state->normal_view->frame->w = view->frame->w;
    state->normal_view->frame->h = view->frame->h;
    state->hilite_view->frame->w = view->frame->w;
    state->hilite_view->frame->h = view->frame->h;

    zdj_view_t * install_icon = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DIR_ADD ], NULL );
    zdj_add_subview( state->normal_view, install_icon );
    install_icon->frame->x = view->frame->w - 13;
    install_icon->frame->y = 1;
    install_icon->frame->w = zdj_ui_assets[ ZDJ_UI_ASSET_DIR_ADD ].w;
    install_icon->frame->h = zdj_ui_assets[ ZDJ_UI_ASSET_DIR_ADD ].h;
    
    // // Setup hilite view
    zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_WHITE ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg );

    zdj_view_t * install_icon_hi = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DIR_ADD_HI ], NULL );
    zdj_add_subview( state->hilite_view, install_icon_hi );
    install_icon_hi->frame->x = view->frame->w - 13;
    install_icon_hi->frame->y = 1;
    install_icon_hi->frame->w = zdj_ui_assets[ ZDJ_UI_ASSET_DIR_ADD_HI ].w;
    install_icon_hi->frame->h = zdj_ui_assets[ ZDJ_UI_ASSET_DIR_ADD_HI ].h;

    zdj_view_t * title_ticker_hilite = zdj_new_ticker_view( "Save file here", ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_BLACK );
    zdj_add_subview( state->hilite_view, title_ticker_hilite );
    title_ticker_hilite->frame->x = 1;
    title_ticker_hilite->frame->y = 0;
    title_ticker_hilite->frame->w = view->frame->w - 15;
    title_ticker_hilite->frame->h = view->frame->h;

    // Adjust hilite frame based on ticker's frame
    hilite_bg->frame->w = view->frame->w;
    hilite_bg->frame->x = 0;
    hilite_bg->frame->h = 10;

    state->has_valid_display = true;
}

char * _zdj_file_browser_parent_dir( char * dir ) {
    char * new_dir = strdup( dir );
    char * pch;
    pch = strrchr( new_dir, '/' );
    new_dir[ pch-new_dir ] = '\0';
    return new_dir;
}
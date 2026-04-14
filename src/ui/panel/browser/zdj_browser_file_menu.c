#include <stdio.h>
#include <dirent.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/panel/browser/zdj_browser_panel.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _add_dir_item_to_menu( zdj_view_t * browser, zdj_view_t * menu, char * dirname, char * path );
static void _add_file_item_to_menu( zdj_view_t * browser, zdj_view_t * menu, char * filepath );

static void _add_chrome_to_menu( 
    zdj_view_t * browser, 
    zdj_view_t * menu, 
    char * path,
    char * select_dir_title 
);

static void _parent_dir( char * input, char * output );

zdj_view_t * zdj_new_browser_panel_file_menu_for_path( 
    zdj_view_t * browser,
    zdj_rect_t * frame, 
    char * path, 
    char * select_dir_title
) {
    // printf( "zdj_new_browser_panel_file_menu_for_path\n" );
    zdj_browser_panel_state_t * browser_state = (zdj_browser_panel_state_t *)browser->state;
    zdj_view_t * menu = zdj_new_menu_view( ZDJ_VERTICAL, frame );
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)menu->state;
    // We need to use menu's back button scroll index to show/hide browser's cancel button.
    menu_state->has_back = false;
    menu_state->header_view = browser_state->header_view;
    menu->map = ZDJ_CONTROL_MAP_SETTINGS_PANEL;

    zdj_view_t * nav_up;
    zdj_view_t * dir_select;
    zdj_view_t * add_dir;

    // Add Chrome
    _add_chrome_to_menu( browser, menu, path, select_dir_title );
    // Add divider between header items and dir items
    zdj_view_t * div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MID_H_DIV ], NULL );
    zdj_menu_view_add_item( menu, div );

    zdj_menu_view_add_padding( menu, 2 );

    // Scan dir contents, adding menu items
    DIR * dir = opendir( path );
    // printf( "file browser open dir %s: %p\n", path, dir );
    struct dirent * entry;
    int id = 0;
    if ( dir ) {
        // Look at every entry in dir.
        while ( ( entry = readdir( dir ) ) != NULL ) {
            // printf( "file browser menu entry: %s\n", entry->d_name );
            if( !browser_state->show_hidden ) {
                if( entry->d_name[ 0 ] == '.' ) continue;
            }
            char dir_path[ 2048 ];
            snprintf( dir_path, sizeof( dir_path ), "%s/%s", path, entry->d_name );
            if ( entry->d_type == DT_DIR ) {
                _add_dir_item_to_menu( browser, menu, entry->d_name, dir_path );
            } else {
                _add_file_item_to_menu( browser, menu, dir_path );
            }
        }
        closedir( dir );
    }

    zdj_menu_view_add_padding( menu, 2 );

    return menu;
}

static void _add_dir_item_to_menu( zdj_view_t * browser, zdj_view_t * menu, char * dirname, char * path ) {
    zdj_view_t * item = zdj_new_menu_item( dirname, ZDJ_MENU_ITEM_LAYOUT_DIR );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)item->state;
    item->handle_control_event = &zdj_browser_panel_item_hmi_delegate;
    state->action = ZDJ_MENU_ITEM_ACTION_DIR_ENTER;
    strcpy( state->link, path );
    state->data.ptr = browser;
    zdj_menu_view_add_item( menu, item );
}

static void _add_file_item_to_menu( zdj_view_t * browser, zdj_view_t * menu, char * filepath ) {
    // printf( "_add_file_item_to_menu: %s\n", filepath );
    zdj_view_t * item = zdj_new_menu_item( basename( filepath ), ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)item->state;
    item->handle_control_event = &zdj_browser_panel_item_hmi_delegate;
    state->action = ZDJ_MENU_ITEM_ACTION_FILE_SELECT;
    strcpy( state->link, filepath );
    state->data.ptr = browser;
    zdj_menu_view_add_item( menu, item );
}

static void _parent_dir( char * input, char * output ) {
    strcpy( output, input );
    char * pch;
    pch = strrchr( output, '/' );
    output[ pch-output ] = '\0';
}

static void _add_chrome_to_menu( 
    zdj_view_t * browser, 
    zdj_view_t * menu, 
    char * path,
    char * select_dir_title
) {
    zdj_browser_panel_state_t * browser_state = (zdj_browser_panel_state_t *)browser->state;

    // Add and 'up one dir' item 
    zdj_view_t * nav_up = zdj_new_menu_item( "Back", ZDJ_MENU_ITEM_LAYOUT_DIR_UP );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)nav_up->state;
    state->action = ZDJ_MENU_ITEM_ACTION_DIR_BACK;
    _parent_dir( path, state->link );
    // Nav buttons need a reference to parent browser for inserting menus
    state->data.ptr = browser;
    nav_up->handle_control_event = &zdj_browser_panel_item_hmi_delegate;
    nav_up->frame.x = 1;
    nav_up->frame.y = 1;
    nav_up->frame.w = 14;
    // nav_up->frame.h = 10;
    zdj_menu_view_add_item( menu, nav_up );

    // Add a 'select this dir' item
    // zdj_view_t * dir_select = zdj_new_menu_item( select_dir_title, ZDJ_MENU_ITEM_LAYOUT_DIR_SELECT );
    // zdj_menu_item_view_state_t * dir_select_state = (zdj_menu_item_view_state_t*)dir_select->state;
    // dir_select_state->action = ZDJ_MENU_ITEM_ACTION_DIR_SELECT;
    // dir_select_state->data.ptr = browser;
    // strcpy( dir_select_state->link, path );
    // dir_select->handle_control_event = &zdj_browser_panel_item_hmi_delegate;
    // dir_select->frame.y = 1;
    // zdj_menu_view_add_item( menu, dir_select );
}
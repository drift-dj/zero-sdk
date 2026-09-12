#include <stdlib.h>
#include <stdio.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/zdj_data_type.h>
#include <zerodj/controls/zdj_controls.h>
#include <zerodj/system/installer/zdj_installer.h>
#include <zerodj/system/registry/zdj_registry.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/panel/zdj_ui_panel.h>
#include <zerodj/ui/panel/settings/zdj_settings_panel.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/dialog_view/zdj_dialog_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/menu_section_view/zdj_menu_section_view.h>
#include <zerodj/ui/view/modal_view/zdj_modal_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>


static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_back( zdj_view_t * view );

static void _refresh_menu( zdj_view_t * view );

static void _launch_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _update_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _remove_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _plugins_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _error_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _startup_btn( zdj_view_t * view, zdj_control_event_t * _event );

static void _startup_exit( zdj_view_t * view, void * data, bool selection );

zdj_view_t * zdj_new_settings_app_panel( void (*cb)(void*), zdj_install_t * install ) {
    zdj_view_t * view = zdj_new_modal_view( zdj_modal_rect( ) );
    view->draw = &_draw;
    view->handle_control_event = &_handle_control;
    view->map = ZDJ_CONTROL_MAP_SETTINGS_PANEL;

    zdj_settings_panel_state_t * state = calloc( 1, sizeof( zdj_settings_panel_state_t ) );
    state->needs_layout_update = true;
    state->exit_cb = cb;
    state->data = install;
    view->state = state;

    // Make menu
    zdj_view_t * menu = zdj_new_menu_view( ZDJ_VERTICAL, zdj_modal_rect( ) );
    zdj_add_subview( view, menu );
    menu->frame.x = 0;
    menu->frame.y = 0;
    menu->frame.w = ZDJ_MODAL_WIDTH;
    menu->frame.h = ZDJ_MODAL_HEIGHT;
    state->menu = menu;
    
    // Set up header
    zdj_view_t * menu_header = zdj_new_menu_header( 
        "Settings",
        " ",
        ZDJ_MENU_HEADER_STYLE_NORMAL,
        ZDJ_MENU_HEADER_BACK_STYLE_BACK
    );
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_header->state;
    header_state->handle_back = &_handle_back;
    zdj_menu_view_add_header( menu, menu_header );
    
    return view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_settings_panel_state_t * state = (zdj_settings_panel_state_t*)view->state;

    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );

    if( state->needs_layout_update ) { _refresh_menu( view ); }
}


static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {
    // Ignore events which have been blocked by layers above this one.
    if( _event->blocked ) { return; }

    zdj_settings_panel_state_t * state = (zdj_settings_panel_state_t*)view->state;
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu->state;

    if( (_event->id == ZDJ_UI_CONTROL_JOG_RELEASE_0 &&
        menu_state->scroll_index == -1) ||
        _event->id == ZDJ_UI_CONTROL_NAV_RELEASE_0
    ) {
        printf( "app_view back_btn\n" );
        // Dump the top view on the stack (this view)
        zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
        zdj_pop_subview_of( panel_state->settings_panel, true );
        _event->blocked = true;
        // Return immediately since we're being dismissed
        return;
    } else { 
        // Send events down into the subview stack
        zdj_settings_panel_state_t * state = (zdj_settings_panel_state_t*)view->state;
        state->menu->handle_control_event( state->menu, _event );
    }

    _event->blocked = true;
}

static void _handle_back( zdj_view_t * view ) {
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_pop_subview_of( panel_state->settings_panel, true );
}

static void _refresh_menu( zdj_view_t * view ) {
    zdj_settings_panel_state_t * state = (zdj_settings_panel_state_t*)view->state;
    zdj_install_t * install = (zdj_install_t*)state->data;

    zdj_menu_view_remove_all_subviews( state->menu );

    // Add an app launch button
    zdj_view_t * app_launch_btn = zdj_new_menu_item( install->display_name, ZDJ_MENU_ITEM_LAYOUT_LAUNCH_BIG );
    zdj_menu_item_view_state_t * app_launch_state = (zdj_menu_item_view_state_t*)app_launch_btn->state;
    app_launch_state->data.ptr = install;
    strcpy( app_launch_state->data.c_val, "launch" );
    app_launch_btn->handle_control_event = &_launch_btn;
    app_launch_btn->frame.x = 3;
    app_launch_btn->frame.y = 3;
    app_launch_btn->frame.h = 20;
    zdj_menu_view_add_item( state->menu, app_launch_btn );
    app_launch_btn->frame.h = 20;

    // Add version str
    char version_str[ 64 ];
    snprintf( version_str, sizeof( version_str ), "%s  (%d.%d.%d)", 
        install->version.desc,    
        install->version.major,
        install->version.minor,
        install->version.hotfix
    );
    
    printf( "version: %s\n", version_str );
    zdj_view_t * version = zdj_new_data_menu_item( 
        "Version", 
        ZDJ_MENU_ITEM_LAYOUT_INERT_DATA,
        ZDJ_MENU_ITEM_DATA_TYPE_CHAR,
        NULL,
        NULL 
    );
    zdj_menu_item_view_state_t * version_state = (zdj_menu_item_view_state_t*)version->state;
    strcpy( version_state->data.c_val, version_str );
    zdj_menu_view_add_item( state->menu, version );
    version->frame.y = view->frame.h - 36;
    
    
    // Add remove button
    zdj_view_t * remove_btn = zdj_new_menu_item( "Remove", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    remove_btn->handle_control_event = &_remove_btn;
    zdj_menu_view_add_item( state->menu, remove_btn );
    remove_btn->frame.y = view->frame.h - 27;

    // Check if app is set to startup
    bool is_startup = zdj_registry_install_is_normal_req( install );

    // Add startup selection button
    zdj_view_t * startup_btn = zdj_new_menu_item( "Launch App at Startup", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    startup_btn->handle_control_event = &_startup_btn;
    zdj_menu_view_add_item( state->menu, startup_btn );
    startup_btn->frame.y = view->frame.h - 18;
    zdj_menu_item_view_state_t * startup_state = (zdj_menu_item_view_state_t*)startup_btn->state;
    startup_state->data.ptr = install;
    startup_state->data.b_val = zdj_registry_install_is_normal_req( install );

    state->needs_layout_update = false;
}

static void _launch_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    printf( "launch_btn\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_install_t * install = state->data.ptr;
    char * registry_name = install->registry_name;
    zdj_launch_req_t * req = zdj_registry_create_launch_req( 
        registry_name,
        "zero-config"
    );
    zdj_registry_commit_launch_req( req );
    exit( 0 );
}

static void _update_btn( zdj_view_t * view, zdj_control_event_t * _event ) {

}

static void _remove_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_install_t * install = state->data.ptr;
    // cfg_add_remove_confirm_view( (zdj_install_t*)state->data->ptr );
}

static void _plugins_btn( zdj_view_t * view, zdj_control_event_t * _event ) {

}

static void _error_btn( zdj_view_t * view, zdj_control_event_t * _event ) {

}

static void _startup_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    // Launch startup confirm dialog
    zdj_view_t * dialog = zdj_new_dialog_view( 
        ZDJ_DIALOG_VIEW_TYPE_OKAY_CANCEL,
        "Confirm",
        "This app will launch at",
        "boot. Are you sure?"
    );
    zdj_dialog_view_state_t * dialog_state = (zdj_dialog_view_state_t*)dialog->state;
    dialog_state->handle_dialog_exit = _startup_exit;
    dialog_state->selection_data = view;

    printf( "dialog: %p, %p, %p\n", dialog, dialog_state->handle_dialog_exit, dialog_state->selection_data );

    // zdj_push_subview( zdj_root_view( ), dialog, true );
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_push_subview( panel_state->settings_panel, dialog, true );
}

static void _startup_exit( zdj_view_t * view, void * data, bool selection ) {
    printf( "_startup_exit\n" );
    if( selection == true ) {
        // printf( "0: %p\n", data );
        zdj_view_t * startup_item = (zdj_view_t *)data;
        zdj_menu_item_view_state_t * startup_state = (zdj_menu_item_view_state_t*)startup_item->state;
        zdj_install_t * install = (zdj_install_t *)startup_state->data.ptr;
        // printf( "1: %p, %p, %p\n", startup_item, startup_state, install );
        zdj_registry_set_normal_req( install );
        // Refresh the startup button with its new value
        startup_state->data.b_val = true;
        startup_state->needs_layout_init = true;
        // printf( "2\n" );
    }
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_pop_subview_of( panel_state->settings_panel, true );
    // printf( "_startup_exit done\n" );
}
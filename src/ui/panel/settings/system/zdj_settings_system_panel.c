#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/reboot.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/installer/zdj_installer.h>
#include <zerodj/system/registry/zdj_registry.h>
#include <zerodj/system/settings/zdj_settings.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/panel/zdj_ui_panel.h>
#include <zerodj/ui/panel/settings/zdj_settings_panel.h>
#include <zerodj/ui/panel/settings/system/zdj_settings_system_panel.h>
#include <zerodj/ui/view/file_browser_view/zdj_file_browser_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/menu_section_view/zdj_menu_section_view.h>
#include <zerodj/ui/view/modal_view/zdj_modal_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_back( zdj_view_t * menu_view );
static void _refresh_menu( zdj_view_t * view );

static void _install_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _browser_exit( zdj_view_t * browser, zdj_file_browser_exit_context_t * context );

static void _subview_exit( void * data );

static void _reset_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _reboot_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _usb_boot_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _os_btn( zdj_view_t * view, zdj_control_event_t * event );

zdj_view_t * zdj_new_settings_system_panel( void (*cb)(void*) ) {
    zdj_view_t * view = zdj_new_modal_view( zdj_modal_rect( ) );
    view->draw = &_draw;
    view->handle_control_event = &_handle_control;
    view->map = ZDJ_CONTROL_MAP_SETTINGS_PANEL;

    zdj_settings_panel_state_t * state = calloc( 1, sizeof( zdj_settings_panel_state_t ) );
    state->needs_layout_update = true;
    state->exit_cb = cb;
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
    // printf( "system panel _handle_control\n" );
    // Ignore events which have been blocked by layers above this one.
    if( _event->blocked ) { return; }

    // Send events down into the top subview
    zdj_view_t * subview = zdj_view_stack_top_subview_of( view );
    subview->handle_control_event( subview, _event );

    _event->blocked = true;
}

static void _handle_back( zdj_view_t * menu_view ) {
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_pop_subview_of( panel_state->settings_panel, true );
}

static void _refresh_menu( zdj_view_t * view ) {
    zdj_settings_panel_state_t * state = (zdj_settings_panel_state_t*)view->state;

    zdj_menu_view_remove_all_subviews( state->menu );

    // System Section
    zdj_menu_view_add_padding( state->menu, 3 );
    zdj_menu_view_add_section( state->menu, zdj_new_menu_section( "System" ) );
    // Reboot
    zdj_view_t * reboot_btn = zdj_new_menu_item( "Reboot", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    reboot_btn->handle_control_event = &_reboot_btn;
    zdj_menu_view_add_item( state->menu, reboot_btn );
    // System Reset
    zdj_view_t * reset_btn = zdj_new_menu_item( "Factory Reset", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    reset_btn->handle_control_event = &_reset_btn;
    zdj_menu_item_view_state_t * reset_state = (zdj_menu_item_view_state_t*)reset_btn->state;
    reset_state->data.ptr = view;
    zdj_menu_view_add_item( state->menu, reset_btn );

    // USB Section
    zdj_menu_view_add_padding( state->menu, 3 );
    zdj_menu_view_add_section( state->menu, zdj_new_menu_section( "USB" ) );
    // Boot behavior
    zdj_view_t * usb_boot_btn = zdj_new_data_menu_item( 
        "Boot Mode", ZDJ_MENU_ITEM_LAYOUT_DATA_R, ZDJ_MENU_ITEM_DATA_TYPE_CHAR, NULL, NULL 
    );
    usb_boot_btn->handle_control_event = &_usb_boot_btn;
    zdj_menu_item_view_state_t * usb_boot_state = (zdj_menu_item_view_state_t*)usb_boot_btn->state;
    usb_boot_state->data.ptr = view;
    switch ( zdj_setting_get( ZDJ_SETTING_USB_INIT_OPTION )->i_val ) {
        case ZDJ_SETTING_USB_INIT_OFFLINE: strcpy( usb_boot_state->data.c_val, "Offline" ); break;
        case ZDJ_SETTING_USB_INIT_HOST: strcpy( usb_boot_state->data.c_val, "Host" ); break;
        case ZDJ_SETTING_USB_INIT_GADGET: strcpy( usb_boot_state->data.c_val, "Gadget" ); break;
        case ZDJ_SETTING_USB_INIT_PREVIOUS: strcpy( usb_boot_state->data.c_val, "Retain" ); break;
    }
    zdj_menu_view_add_item( state->menu, usb_boot_btn );

    // Software Section
    zdj_menu_view_add_padding( state->menu, 3 );
    zdj_menu_view_add_section( state->menu, zdj_new_menu_section( "Software" ) );

    zdj_view_t * install_btn = zdj_new_menu_item( "+ Install Apps", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    install_btn->handle_control_event = _install_btn;
    zdj_menu_view_add_item( state->menu, install_btn );

    zdj_view_t * os_btn = zdj_new_menu_item( "DriftOS", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    zdj_menu_item_view_state_t * os_state = (zdj_menu_item_view_state_t*)os_btn->state;
    os_state->data.ptr = view;
    os_btn->handle_control_event = _os_btn;
    zdj_menu_view_add_item( state->menu, os_btn ); 
    
    state->needs_layout_update = false;
}

static void _subview_exit( void * data ) {

}

static void _install_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    printf( "_add_update_btn\n" );
    zdj_view_t * browser = zdj_new_file_browser_view( 
        zdj_modal_rect( ), "/media/internal/installers", 
        true, 
        true, 
        ZDJ_FILE_BROWSER_TYPE_SELECT_FILE, 
        "Scan",
        false 
    );
    if( !browser ) {
        printf( "Unable to open browser -- exiting\n" );
        exit( 1 );
    }
    
    // Add a select callback
    zdj_file_browser_view_state_t * browser_state = (zdj_file_browser_view_state_t *)browser->state;
    browser_state->handle_file_browser_exit = &_browser_exit;
    // Add the menu to the top of the stack
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_push_subview( panel_state->settings_panel, browser, true );
}

static void _browser_exit( zdj_view_t * browser, zdj_file_browser_exit_context_t * context ) {
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_installer_t * installer;
    if( context->status == ZDJ_FILE_BROWSER_EXIT_STATUS_CANCEL ) {
        // Cancel simply pops the browser off the stack.
        zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
        zdj_pop_subview_of( panel_state->settings_panel, true );
    } else if( context->status == ZDJ_FILE_BROWSER_EXIT_STATUS_SELECT ) {
        // Attempt to make installer from selected path.
        // If selected path is an installer, push an installer detail view.
        installer = zdj_installer_for_filepath( context->filepath );
        
        if( installer ) {
            zdj_view_t * installer_view = zdj_new_settings_installer_panel( &_subview_exit, installer );
            zdj_push_subview_behind( 
                panel_state->settings_panel, 
                zdj_view_stack_top_subview_of( panel_state->settings_panel ),
                installer_view,
                true
            );
        } else {
            printf( "Alert for invalid installer\n" );
            // cfg_add_alert_view( INVALID_INSTALLER );
        }
    }
}

static void _reset_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_menu_item_view_state_t * btn_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * system_panel = (zdj_view_t *)btn_state->data.ptr;

    zdj_view_t * reset_panel = zdj_new_settings_reset_panel( );
    zdj_push_subview( system_panel, reset_panel, true );
}

static void _reboot_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    sync( );
    reboot( RB_AUTOBOOT );
}

static void _usb_boot_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    int usb_boot_setting = zdj_setting_get( ZDJ_SETTING_USB_INIT_OPTION )->i_val;
    switch ( usb_boot_setting ) {
        case ZDJ_SETTING_USB_INIT_OFFLINE: 
            zdj_setting_set_int( ZDJ_SETTING_USB_INIT_OPTION, ZDJ_SETTING_USB_INIT_HOST ); 
            break;
        case ZDJ_SETTING_USB_INIT_HOST: 
            zdj_setting_set_int( ZDJ_SETTING_USB_INIT_OPTION, ZDJ_SETTING_USB_INIT_GADGET ); 
            break;
        case ZDJ_SETTING_USB_INIT_GADGET: 
            zdj_setting_set_int( ZDJ_SETTING_USB_INIT_OPTION, ZDJ_SETTING_USB_INIT_PREVIOUS ); 
            break;
        case ZDJ_SETTING_USB_INIT_PREVIOUS: 
            zdj_setting_set_int( ZDJ_SETTING_USB_INIT_OPTION, ZDJ_SETTING_USB_INIT_OFFLINE ); 
            break;
    }

    zdj_menu_item_view_state_t * btn_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * system_panel = (zdj_view_t *)btn_state->data.ptr;
    zdj_settings_panel_state_t * system_panel_state = (zdj_settings_panel_state_t*)system_panel->state;
    system_panel_state->needs_layout_update = true;
}

static void _os_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_menu_item_view_state_t * btn_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * system_panel = (zdj_view_t *)btn_state->data.ptr;
    zdj_settings_panel_state_t * system_panel_state = (zdj_settings_panel_state_t*)system_panel->state;
    zdj_view_t * os_panel = zdj_new_settings_os_panel( &_subview_exit );
    system_panel_state->event_target = os_panel;
    zdj_push_subview( system_panel, os_panel, true );
}
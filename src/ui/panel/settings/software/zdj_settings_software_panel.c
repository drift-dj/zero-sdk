#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/installer/zdj_installer.h>
#include <zerodj/system/registry/zdj_registry.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/panel/zdj_ui_panel.h>
#include <zerodj/ui/panel/settings/zdj_settings_panel.h>
#include <zerodj/ui/panel/settings/software/zdj_settings_software_panel.h>
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

static void _app_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _os_btn( zdj_view_t * view, zdj_control_event_t * event );

zdj_view_t * zdj_new_settings_software_panel( void (*cb)(void*) ) {
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
    // Ignore events which have been blocked by layers above this one.
    if( _event->blocked ) { return; }

    zdj_settings_panel_state_t * state = (zdj_settings_panel_state_t*)view->state;
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu->state;

    if( (_event->id == ZDJ_UI_CONTROL_JOG_RELEASE_0 &&
        menu_state->scroll_index == -1) ||
        _event->id == ZDJ_UI_CONTROL_NAV_RELEASE_0
    ) {
        printf( "usb_status_view back_btn\n" );
        // Dump the top view on the stack (this view)
        zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
        zdj_pop_subview_of( panel_state->settings_panel, true );
        _event->blocked = true;
        // Return immediately since we're being dismissed
        return;
    } else { 
        // Send events down into the subview stack
        zdj_view_t * subview = zdj_view_stack_top_subview_of( view );
        if( subview->handle_control_event ){ subview->handle_control_event( subview, _event ); }
        // zdj_settings_panel_state_t * state = (zdj_settings_panel_state_t*)view->state;
        // state->menu->handle_control_event( state->menu, _event );
    }

    _event->blocked = true;
}

static void _handle_back( zdj_view_t * menu_view ) {
    printf( "_handle_back\n" );
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_pop_subview_of( panel_state->settings_panel, true );
}

static void _refresh_menu( zdj_view_t * view ) {
    zdj_settings_panel_state_t * state = (zdj_settings_panel_state_t*)view->state;

    zdj_menu_view_remove_all_subviews( state->menu );



    zdj_view_t * install_btn = zdj_new_menu_item( "+ Install Apps", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    install_btn->handle_control_event = _install_btn;
    zdj_menu_view_add_item( state->menu, install_btn );

    // Loop thru registry, adding menu items for each installed app
    zdj_install_t * install = zdj_registry_installs( );
    while( install ) {
        if( !strcmp( install->category, "music" ) || !strcmp( install->category, "util" ) ) {
            char * app_name = strdup( install->display_name );
            zdj_view_t * item = zdj_new_menu_item( app_name, ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
            item->handle_control_event = &_app_btn;
            zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;
            strcpy( item_state->link, app_name );
            item_state->data.ptr = view;
            strcpy( item_state->data.c_val, install->registry_name );
            zdj_menu_view_add_item( state->menu, item );
        }
        
        install = install->next;
    }

    zdj_view_t * os_btn = zdj_new_menu_item( "DriftOS", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    zdj_menu_item_view_state_t * os_state = (zdj_menu_item_view_state_t*)os_btn->state;
    os_state->data.ptr = view;
    os_btn->handle_control_event = _os_btn;
    zdj_menu_view_add_item( state->menu, os_btn );
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
    printf( "_browser_exit!: %p, %d, %s\n", browser, context->status, context->filepath );
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
        
        printf( "installer: %p path: %s\n", installer, context->filepath );
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

static void _app_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_menu_item_view_state_t * btn_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_install_t * install = zdj_registry_install_for_name( btn_state->data.c_val );

    zdj_view_t * software_panel = (zdj_view_t *)btn_state->data.ptr;
    zdj_settings_software_panel_state_t * software_panel_state = (zdj_settings_software_panel_state_t*)software_panel->state;
    zdj_view_t * app_panel = zdj_new_settings_app_panel( &_subview_exit, install );
    software_panel_state->event_target = app_panel;
    zdj_push_subview( software_panel, app_panel, true );
}

static void _os_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    printf( "_os_btn\n" );
    zdj_menu_item_view_state_t * btn_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * software_panel = (zdj_view_t *)btn_state->data.ptr;
    zdj_settings_software_panel_state_t * software_panel_state = (zdj_settings_software_panel_state_t*)software_panel->state;
    zdj_view_t * os_panel = zdj_new_settings_os_panel( &_subview_exit );
    software_panel_state->event_target = os_panel;
    zdj_push_subview( software_panel, os_panel, true );
}
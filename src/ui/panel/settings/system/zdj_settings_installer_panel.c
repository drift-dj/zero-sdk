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

typedef struct {
    bool view_needs_refresh;
    void ( *exit_cb )( void* );
} settings_installer_state_t;

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _deinit_state( zdj_view_t * view );

static void _handle_back( zdj_view_t * view );
static void _handle_control( zdj_view_t * view, zdj_control_event_t * event );

static void _install_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _install_dialog_exit( zdj_view_t * view, void * data, bool selection );

static void _installer_manifest_item_cb( zdj_installer_manifest_item_t * manifest_item, void * data );

zdj_view_t * zdj_new_settings_installer_panel( void (*cb)(void*), zdj_installer_t * installer ) {

    bool valid_installer = true;
    char tmp_str[2048];

    // Pre-validate installer display data
    char registry_name[ 64 ];
    char display_name[ 64 ];
    bool is_update;
    zdj_install_t * current_install;
    char current_version_string[ 64 ];
    char installer_version_string[ 64 ];
    char manifest_totals[ 64 ];
    char size_suffix[ 16 ];

    // registry name
    strcpy( registry_name, installer->install.registry_name );
    current_install = zdj_registry_install_for_name( registry_name );

    // display name
    strcpy( display_name, installer->install.display_name );

    printf( "current version: %p\n", current_install );

    // is_update/current install version
    if( current_install ) {
        is_update = true;
        snprintf( current_version_string, sizeof( current_version_string ), "%s  (%d.%d.%d)", 
            current_install->version.desc,    
            current_install->version.major,
            current_install->version.minor,
            current_install->version.hotfix
        );
    } // Don't validate current_install

    // new install version 
    snprintf( installer_version_string, sizeof( installer_version_string ), "%s  (%d.%d.%d)",
        installer->install.version.desc,
        installer->install.version.major,
        installer->install.version.minor,
        installer->install.version.hotfix
    );

    // Build file count/file size string
    int file_count = zdj_installer_file_count( installer );
    if( !file_count ) { valid_installer = false; }
    
    // Make a filesize in KB/MB depending on size
    float file_size = (float)installer->data_offsets.binary_length / 1000.0f;
    strcpy( size_suffix, "kB" );
    if( file_size > 1000.0f ) { 
        file_size /= 1000.0f;
        strcpy( size_suffix, "MB" );
    }
    snprintf( manifest_totals, sizeof( manifest_totals ), "%d  files   %1.1f %s",
        file_count,
        file_size,
        size_suffix
    );

    // Make view
    zdj_view_t * view = zdj_new_modal_view( zdj_modal_rect( ) );
    view->frame.x = ZDJ_MODAL_X;
    view->frame.y = ZDJ_SCREEN_H;
    view->map = ZDJ_CONTROL_MAP_MENU_BASE;

    // Make menu
    zdj_view_t * menu = zdj_new_menu_view( ZDJ_VERTICAL, zdj_modal_rect( ) );
    zdj_add_subview( view, menu );
    menu->frame.x = 0;
    menu->frame.y = 0;
    menu->frame.w = ZDJ_MODAL_WIDTH;
    menu->frame.h = ZDJ_MODAL_HEIGHT;

    zdj_view_t * menu_header = zdj_new_menu_header( 
        "Install",
        display_name,
        ZDJ_MENU_HEADER_STYLE_NORMAL,
        ZDJ_MENU_HEADER_BACK_STYLE_CANCEL
    );
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_header->state;
    header_state->handle_back = &_handle_back;
    zdj_menu_view_add_header( menu, menu_header );

    settings_installer_state_t * state = calloc( 1, sizeof( settings_installer_state_t ) );
    state->view_needs_refresh = false;
    state->exit_cb = cb;
    view->state = state;

    // Add an app launch button
    zdj_view_t * install_btn = zdj_new_menu_item( display_name, ZDJ_MENU_ITEM_LAYOUT_LAUNCH_BIG );
    zdj_menu_item_view_state_t * install_state = (zdj_menu_item_view_state_t*)install_btn->state;
    install_state->data.ptr = installer;
    strcpy( install_state->data.c_val, "install" );
    install_btn->handle_control_event = &_install_btn;
    install_btn->frame.x = 3;
    install_btn->frame.y = 3;
    install_btn->frame.h = 20;
    zdj_menu_view_add_item( menu, install_btn );
    install_btn->frame.h = 20;
    
    // new install version 
    zdj_view_t * installer_version_label = zdj_new_label_view( installer_version_string, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    installer_version_label->frame.x = 4;
    installer_version_label->frame.y = 25;
    zdj_menu_view_add_item( menu, installer_version_label );

    zdj_menu_view_add_padding( menu, 3 );

    // Files section
    snprintf( manifest_totals, sizeof( manifest_totals ), "%d files %1.1f%s",
        file_count,
        file_size,
        size_suffix
    );
    zdj_view_t * manifest_totals_section = zdj_new_menu_section( manifest_totals );
    zdj_menu_view_add_section( menu, manifest_totals_section );
    
    // Add non-hilite menu items for each file in manifest
    zdj_installer_iterate_manifest( installer, _installer_manifest_item_cb, menu );

    // Version info section
    zdj_menu_view_add_padding( menu, 3 );


    // If there's an existing install, show some version info
    if( current_install ) {
        zdj_view_t * version_info_section = zdj_new_menu_section( "Version Info" );
        zdj_menu_view_add_section( menu, version_info_section );
        // Current Install
        zdj_view_t * current_install_item = zdj_new_data_menu_item( "Current", ZDJ_MENU_ITEM_LAYOUT_INERT_DATA, ZDJ_MENU_ITEM_DATA_TYPE_CHAR, NULL, NULL );
        zdj_menu_item_view_state_t * current_install_state = (zdj_menu_item_view_state_t*)current_install_item->state;
        current_install_state->needs_layout_update = true;
        strcpy( current_install_state->data.c_val, current_version_string );
        zdj_menu_view_add_item( menu, current_install_item );
        // New Install
        zdj_view_t * new_install_item = zdj_new_data_menu_item( "Installer", ZDJ_MENU_ITEM_LAYOUT_INERT_DATA, ZDJ_MENU_ITEM_DATA_TYPE_CHAR, NULL, NULL );
        zdj_menu_item_view_state_t * new_install_state = (zdj_menu_item_view_state_t*)new_install_item->state;
        new_install_state->needs_layout_update = true;
        strcpy( new_install_state->data.c_val, installer_version_string );
        zdj_menu_view_add_item( menu, new_install_item );
    }

    // Zero the scroll index
    zdj_menu_view_set_scroll_index( menu, 0 );

    return view;    
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {

}

static void _deinit_state( zdj_view_t * view ) {

}

static void _handle_back( zdj_view_t * view ){ 
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_pop_subview_of( panel_state->settings_panel, true );
}

static void _install_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_menu_item_view_state_t * btn_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_install_t * install = btn_state->data.ptr;
    char version_str[ 32 ];
    snprintf( version_str, sizeof( version_str ), "%d.%d.%d",    
        install->version.major,
        install->version.minor,
        install->version.hotfix
    );
    char dialog_line_1[ 64 ];
    snprintf( dialog_line_1, sizeof( dialog_line_1 ), "This will install v. %s", version_str );
    char dialog_line_2[ 64 ];
    snprintf( dialog_line_2, sizeof( dialog_line_2 ), "of the %s app.", install->display_name );
    // Launch drop lib confirm dialog
    zdj_view_t * dialog = zdj_new_dialog_view( 
        ZDJ_DIALOG_VIEW_TYPE_OKAY_CANCEL,
        "Confirm",
        dialog_line_1,
        dialog_line_2
    );
    zdj_dialog_view_state_t * dialog_state = (zdj_dialog_view_state_t*)dialog->state;
    dialog_state->handle_dialog_exit = &_install_dialog_exit;
    dialog_state->selection_data = view;
    
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_push_subview( panel_state->settings_panel, dialog, true );
}

static void _install_dialog_exit( zdj_view_t * view, void * data, bool selection ) {
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    // printf( "_install_dialog_exit %d\n", selection );
    if( selection ) {
        printf( "launch installer\n" );
        
        // Write install_req to disk
        FILE * fd = fopen( ZDJ_INSTALLER_REQUEST_PATH, "w" );
        if( !fd ) { zdj_pop_subview_of( panel_state->settings_panel, true ); }
        printf( "RE_ENABLED INSTALLER WRITE!!!!\n" );
        // fwrite( ui_state->installer_path, sizeof( ui_state->installer_path ), 1, fd );
        fclose( fd );
        
        // Launch zero-install
        zdj_launch_req_t * req = zdj_registry_create_launch_req( 
            "zero-install",
            "zero-config"
        );
        zdj_registry_commit_launch_req( req );
        exit( 0 );
    } else {
        zdj_pop_subview_of( panel_state->settings_panel, true );
    }
}

// Make a new menu item for each item in the manifest db
static void _installer_manifest_item_cb( zdj_installer_manifest_item_t * manifest_item, void * data ) {
    zdj_view_t * menu_view = (zdj_view_t*)data;
    zdj_view_t * menu_item = zdj_new_menu_item( manifest_item->dest_path, ZDJ_MENU_ITEM_LAYOUT_INERT );
    zdj_menu_view_add_item( menu_view, menu_item );
}
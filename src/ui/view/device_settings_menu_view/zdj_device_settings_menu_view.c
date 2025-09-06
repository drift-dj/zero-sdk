#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/system/usb/zdj_usb.h>
#include <zerodj/ui/zdj_ui.h>
// #include <zerodj/ui/anim/zdj_anim.h>
// #include <zerodj/ui/asset/zdj_ui_asset.h>
// #include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/device_settings_menu_view/zdj_device_settings_menu_view.h>
#include <zerodj/ui/view/dialog_view/zdj_dialog_view.h>
// #include <zerodj/ui/view/file_browser_view/zdj_file_browser_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/menu_section_view/zdj_menu_section_view.h>
#include <zerodj/ui/view/soundcard_view/zdj_soundcard_view.h>
// #include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
// #include <zerodj/ui/view/zdj_view_stack.h>
#include <zerodj/ui/view/usb_drive_view/zdj_usb_drive_view.h>
#include <zerodj/ui/view/usb_status_view/zdj_usb_status_view.h>


static void _handle_usb_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_usb_drive_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_soundcard_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_drop_lib_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _drop_library_dialog_exit( zdj_view_t * view, void * data, bool selection );
static void _handle_dumper_btn( zdj_view_t * view, zdj_control_event_t * _event );

zdj_view_t * zdj_new_device_settings_menu( zdj_rect_t * frame ) {
    zdj_view_t * menu = zdj_new_menu_view( ZDJ_VERTICAL, frame );

    zdj_menu_view_state_t * menu_state = ( zdj_menu_view_state_t* )menu->state;
    menu_state->needs_layout_update = true;

    // Add menu header
    zdj_view_t * header = zdj_new_menu_header( 
        "Device Settings",
        " ",
        ZDJ_MENU_HEADER_STYLE_NORMAL,
        ZDJ_MENU_HEADER_BACK_STYLE_BACK
    );
    zdj_menu_view_add_header( menu, header );

    // Soundcard
    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Soundcard" ) );

    zdj_view_t * soundcard_btn = zdj_new_menu_item( "Open Soundcard", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
    soundcard_btn->handle_control_event = &_handle_soundcard_btn;
    zdj_menu_view_add_item( menu, soundcard_btn );

    // USB
    zdj_menu_view_add_section( menu, zdj_new_menu_section( "USB" ) );

    zdj_view_t * usb_status;
    if( zdj_usb_status->mode == ZDJ_USB_MODE_HOST ) {
        // If we're in USB host mode and have MSD devices, show.
        usb_status = zdj_new_menu_item( "Connected Drive(s)", ZDJ_MENU_ITEM_LAYOUT_INERT_STATUS );
        zdj_menu_item_view_state_t * usb_status_state = (zdj_menu_item_view_state_t*)usb_status->state;

        // Add USB Settings item
        zdj_view_t * usb_btn = zdj_new_menu_item( "USB", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
        usb_btn->handle_control_event = &_handle_usb_btn;
        zdj_menu_view_add_item( menu, usb_btn );
    } else if( zdj_usb_status->mode == ZDJ_USB_MODE_GADGET ) {
        // If we're in USB device mode, show Drive launch button.
        zdj_view_t * usb_status = zdj_new_menu_item( "Transfer Mode", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
        usb_status->handle_control_event = &_handle_usb_drive_btn;
        zdj_menu_view_add_item( menu, usb_status );

        // Add USB Settings item
        zdj_view_t * usb_btn = zdj_new_menu_item( "USB", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
        usb_btn->handle_control_event = &_handle_usb_btn;
        zdj_menu_view_add_item( menu, usb_btn );
    } else {
        // If we're offline, show offline status.
        zdj_view_t * usb_btn = zdj_new_menu_item( "USB (Offline)", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
        usb_btn->handle_control_event = &_handle_usb_btn;
        zdj_menu_view_add_item( menu, usb_btn );
    }

    // Danger
    zdj_menu_view_add_section( menu, zdj_new_menu_section( "DANGER" ) );

    // Add 'DB Dumper item'
    zdj_view_t * dumper_btn = zdj_new_menu_item( "DB Dumper", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
    dumper_btn->handle_control_event = &_handle_dumper_btn;
    zdj_menu_view_add_item( menu, dumper_btn );
    
    // Add 'drop lib tables item'
    zdj_view_t * drop_btn = zdj_new_menu_item( "Drop Library Tables", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
    drop_btn->handle_control_event = &_handle_drop_lib_btn;
    zdj_menu_view_add_item( menu, drop_btn );

    return menu;
}

static void _handle_usb_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_push_subview( zdj_root_view( ), zdj_new_usb_status_view( ), true );
}

static void _handle_usb_drive_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_view_t * drive_view = zdj_new_usb_drive_view( );
    zdj_push_subview( zdj_root_view( ), drive_view, true );
}

static void _handle_soundcard_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_view_t * soundcard_view = zdj_new_soundcard_view( zdj_soundcard );
    zdj_push_subview( zdj_root_view( ), soundcard_view, true );
}

// Drop Lib Tables
static void _handle_drop_lib_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    // Launch drop lib confirm dialog
    zdj_view_t * dialog = zdj_new_dialog_view( 
        ZDJ_DIALOG_VIEW_TYPE_OKAY,
        "Confirm",
        "XXX DROP LIBRARY TABLES XXX",
        "Please do not do this."
    );
    zdj_dialog_view_state_t * dialog_state = (zdj_dialog_view_state_t*)dialog->state;
    dialog_state->handle_dialog_exit = &_drop_library_dialog_exit;
    dialog_state->selection_data = view;
    zdj_push_subview( zdj_root_view( ), dialog, true );
}

static void _drop_library_dialog_exit( zdj_view_t * view, void * data, bool selection ) {
    // printf( "_drop_library_dialog_exit %d\n", selection );
    zdj_library_reset_db( );
    zdj_pop_subview_of( zdj_root_view( ), true );
}

// Copy lib to shared drive and launch drive mode
static void _handle_dumper_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_fs_remove_dir( "/media/internal/zdj_dump" );
    zdj_fs_mkdir_p( "/media/internal/zdj_dump" );
    // Copy lib/import DBs to the data_dump dir
    zdj_fs_copy_file( ZDJ_LIBRARY_DB_PATH, "/media/internal/zdj_dump/zero.db", true );
    zdj_fs_copy_file( ZDJ_LIBRARY_IMPORT_DB_PATH, "/media/internal/zdj_dump/zero_import.db", true );
    sync( );

    // Launch drive mode
    zdj_view_t * drive_view = zdj_new_usb_drive_view( );
    zdj_push_subview( zdj_root_view( ), drive_view, true );
}
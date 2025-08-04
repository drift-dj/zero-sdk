#include <stdio.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>


#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/view/zdj_view_stack.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/menu_section_view/zdj_menu_section_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/usb_status_view/zdj_usb_status_view.h>
#include <zerodj/system/usb/zdj_usb.h>

void _zdj_usb_status_view_host_update_layout( zdj_view_t * view );

void zdj_usb_status_view_host_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)view->state;

    // Catch a switch from zero-usb backend which requires a reboot.
    if( zdj_usb_status->requires_reboot ) {
        // Switch to reboot required view.
        view->draw = &zdj_usb_status_view_reboot_draw;
        state->needs_layout_update = true;
        return;
    }

    // While in host mode, we're periodically polling for attached devices
    if( state->frame_counter++ > 100 ) {
        // Count lines in devices output.
        int line_count = 0;
        FILE * fp = popen( "cat /sys/kernel/debug/usb/devices", "r" );
        if ( fp == NULL ) {
            printf( "couldn't open usb devices\n" );
        } else {
            char line[ 256 ];
            while( fgets( line, sizeof( line ), fp ) ) {
                line_count++;
            }
        }
        pclose( fp );

        // If line_count doesn't match last time, flag.
        if( line_count != state->devices_line_count ) {
            state->devices_line_count = line_count;
            state->needs_layout_update = true;
        }
    }

    if( state->needs_layout_update ) { _zdj_usb_status_view_host_update_layout( view ); }
}

void _zdj_usb_status_view_host_update_layout( zdj_view_t * view ) {
    // printf( "zdj_usb_status_view_build_host_layout\n" );
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)view->state;
    zdj_view_t * menu = state->menu_view;
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu_view->state;

    // Clear the old menu.
    zdj_menu_view_remove_all_items( state->menu_view );

    // Get all attached devices
    zdj_usb_attached_devices_t * attached = zdj_usb_get_attached_devices( );

    // Set header title
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_state->header_view->state;
    header_state->name = "USB Mode";
    header_state->title = "Host";
    header_state->has_valid_display = false;

    // Add zero icon
    zdj_view_t * zero = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_ZERO ], NULL );
    zero->frame->x = 14;
    zero->frame->y = 8;
    zdj_menu_view_add_item( menu, zero );

    // Add divider
    zdj_view_t * div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_NAR_H_DIV ], NULL );
    div->frame->x = 31;
    div->frame->y = 12;
    div->frame->w = 53;
    zdj_menu_view_add_item( menu, div );

    zdj_view_t * box;
    zdj_view_t * stat_label;
    if ( attached->count > 0 ) {
        // Add status box
        box = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BOX_1 ], NULL );
        box->frame->x = 86;
        box->frame->y = 3;
        zdj_menu_view_add_item( menu, box );

        // Add status label
        char device_count[ 8 ];
        snprintf( device_count, sizeof( device_count ), "%d", attached->count );
        stat_label = zdj_new_label_view( device_count, ZDJ_FONT_12, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
        stat_label->frame->x = 93;
        stat_label->frame->y = 2;
        zdj_menu_view_add_item( menu, stat_label );

        zdj_menu_view_add_padding( menu, 8 );

        // Add Devices Section
        zdj_menu_view_add_section( menu, zdj_new_menu_section( "Attached Devices" ) );

        // Scan for attached devices
        zdj_usb_device_t * device = attached->devices;
        while( device ) {
            zdj_view_t * attached_device = zdj_new_menu_item( device->name_user, ZDJ_MENU_ITEM_LAYOUT_DATA_R );
            // attached_device->handle_control_event = &zdj_usb_status_view_handle_device_mode_btn;
            zdj_menu_item_view_state_t * attached_state = (zdj_menu_item_view_state_t*)attached_device->state;
            char type_str[ 256 ];
            snprintf( type_str, sizeof( type_str ), "%s%s%s", 
                device->has_audio ? "audio" : " ",
                device->has_hid ? "controller" : " ",
                device->has_msd ? "drive" : " "
            );
            attached_state->data->c_val = strdup( type_str );
            zdj_menu_view_add_item( menu, attached_device );

            device = device->next;
        }
    } else {
        // Add empty status box
        box = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DOTTED_BOX_1 ], NULL );
        box->frame->x = 86;
        box->frame->y = 3;
        zdj_menu_view_add_item( menu, box );

        zdj_menu_view_add_padding( menu, 8 );
    }

    // Add Mode Switch Section
    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Mode Switch" ) );

    // Add Enable Device
    zdj_view_t * device_btn = zdj_new_menu_item( "Device Mode", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
    device_btn->handle_control_event = &zdj_usb_status_view_handle_device_mode_btn;
    zdj_menu_item_view_state_t * device_state = (zdj_menu_item_view_state_t*)device_btn->state;
    device_state->data->ptr = view;
    zdj_menu_view_add_item( menu, device_btn );

    // Add Offline
    zdj_view_t * offline_btn = zdj_new_menu_item( "Offline", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
    offline_btn->handle_control_event = &zdj_usb_status_view_handle_offline_btn;
    zdj_menu_item_view_state_t * offline_state = (zdj_menu_item_view_state_t*)offline_btn->state;
    offline_state->data->ptr = view;
    zdj_menu_view_add_item( menu, offline_btn );

    state->needs_layout_update = false;
}
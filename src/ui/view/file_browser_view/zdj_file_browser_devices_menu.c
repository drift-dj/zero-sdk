#include <stdio.h>
#include <dirent.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/usb/zdj_usb.h>
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
    strcpy( header_state->title, "Media" );
    header_state->has_valid_display = false;

    // Make menu
    zdj_view_t * menu = zdj_new_menu_view( ZDJ_HORIZONTAL, frame );
    menu->frame.x = 0;
    menu->frame.y = 0;

    zdj_refresh_device_browser_menu( browser, menu );

    return menu;
}

void zdj_refresh_device_browser_menu( zdj_view_t * browser, zdj_view_t * menu ) {
    // printf( "zdj_refresh_device_browser_menu\n" );
    zdj_menu_view_remove_all_subviews( menu );

    int x = 9;

    // Add Zero item
    zdj_view_t * zero_item = zdj_new_browser_device_menu_item( 
        "Zero", "/media/internal/", ZDJ_MENU_ITEM_BROWSER_DEVICE_TYPE_ZERO 
    );
    zdj_menu_item_view_state_t * zero_state = (zdj_menu_item_view_state_t*)zero_item->state;
    zero_state->action = ZDJ_MENU_ITEM_ACTION_DIR_ENTER;
    zero_state->data.ptr = browser;
    zero_state->handles_hmi = true;
    zero_item->handle_control_event = &zdj_file_browser_item_hmi_delegate;
    zero_item->frame.x = x;
    zero_item->frame.y = 5;
    zero_item->frame.w = 40;
    zdj_menu_view_add_item( menu, zero_item );
    x += 50;

    // If we're in USB host mode
    // Look for an attached MSD drive
    // zdj_usb_attached_devices_t * attached = zdj_usb_get_attached_devices( ZDJ_USB_TYPE_MSD );
    if( !zdj_usb_state || zdj_usb_state->host_state.attached.count < 1 ) { return; }

    // Add attached MSDs
    for( int i=0; i<zdj_usb_state->host_state.attached.count; i++ ) {
        zdj_view_t * msd_item = zdj_new_browser_device_menu_item( 
            zdj_usb_state->host_state.attached.devices[ i ].name_user,
            zdj_usb_state->host_state.attached.devices[ i ].mount_path, 
            ZDJ_MENU_ITEM_BROWSER_DEVICE_TYPE_MSD 
        );
        zdj_menu_item_view_state_t * msd_state = (zdj_menu_item_view_state_t*)msd_item->state;
        msd_state->action = ZDJ_MENU_ITEM_ACTION_DIR_ENTER;
        msd_state->data.ptr = browser;
        msd_state->handles_hmi = true;
        msd_item->handle_control_event = &zdj_file_browser_item_hmi_delegate;
        msd_item->frame.x = x;
        msd_item->frame.y = 5;
        msd_item->frame.w = 40;
        zdj_menu_view_add_item( menu, msd_item );

        x += 50;
    }
}
#include <stdio.h>
#include <dirent.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/usb/zdj_usb.h>
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


// Show all media devices currently connected
zdj_view_t * zdj_new_browser_panel_device_menu( zdj_view_t * browser, zdj_rect_t * frame ) {
    zdj_browser_panel_state_t * browser_state = (zdj_browser_panel_state_t *)browser->state;

    // Make menu
    zdj_view_t * menu = zdj_new_menu_view( ZDJ_HORIZONTAL, frame );
    menu->frame.x = 0;
    menu->frame.y = 0;

    menu->map = ZDJ_CONTROL_MAP_SETTINGS_PANEL;

    zdj_browser_panel_refresh_devices_menu( browser, menu );

    return menu;
}


void zdj_browser_panel_refresh_devices_menu( zdj_view_t * browser, zdj_view_t * menu ) {
    printf( "zdj_refresh_device_browser_menu\n" );
    
    if( zdj_usb_state != NULL ) { zdj_usb_update_mode_from_sysfs( zdj_usb_state ); }
    
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
    zero_item->handle_control_event = &zdj_browser_panel_item_hmi_delegate;
    zero_item->frame.x = x;
    zero_item->frame.y = 5;
    zero_item->frame.w = 40;
    zdj_menu_view_add_item( menu, zero_item );
    x += 50;

    // Show rootfs if configured
    zdj_view_t * linux_item = zdj_new_browser_device_menu_item( 
        "Linux", "/", ZDJ_MENU_ITEM_BROWSER_DEVICE_TYPE_LINUX 
    );
    zdj_menu_item_view_state_t * linux_state = (zdj_menu_item_view_state_t*)linux_item->state;
    linux_state->action = ZDJ_MENU_ITEM_ACTION_DIR_ENTER;
    linux_state->data.ptr = browser;
    linux_state->handles_hmi = true;
    linux_item->handle_control_event = &zdj_browser_panel_item_hmi_delegate;
    linux_item->frame.x = x;
    linux_item->frame.y = 5;
    linux_item->frame.w = 40;
    zdj_menu_view_add_item( menu, linux_item );
    x += 50;



    // If we're in USB host mode
    // Look for an attached MSD drive
    // if( !zdj_usb_state ) { 
    //     printf( "Browser panel found no USB\n" );
    //     return; 
    // }

    // if( zdj_usb_state->host_state.attached.count < 1 ) { 
    //     printf( "Browser panel found no attached\n" );
    //     return; 
    // }

    if( zdj_usb_state != NULL &&
        zdj_usb_state->host_state.attached.count > 0
    ) {
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
            msd_item->handle_control_event = &zdj_browser_panel_item_hmi_delegate;
            msd_item->frame.x = x;
            msd_item->frame.y = 5;
            msd_item->frame.w = 40;
            zdj_menu_view_add_item( menu, msd_item );

            x += 50;
        }
    }

    // zdj_view_t * extra_item = zdj_new_browser_device_menu_item( 
    //     "Xtra", "/", ZDJ_MENU_ITEM_BROWSER_DEVICE_TYPE_MSD 
    // );
    // zdj_menu_item_view_state_t * extra_state = (zdj_menu_item_view_state_t*)extra_item->state;
    // extra_state->action = ZDJ_MENU_ITEM_ACTION_DIR_ENTER;
    // extra_state->data.ptr = browser;
    // extra_state->handles_hmi = true;
    // extra_item->handle_control_event = &zdj_browser_panel_item_hmi_delegate;
    // extra_item->frame.x = x;
    // extra_item->frame.y = 5;
    // extra_item->frame.w = 40;
    // zdj_menu_view_add_item( menu, extra_item );
}
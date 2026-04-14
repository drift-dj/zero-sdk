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

void _zdj_usb_status_view_offline_update_layout( zdj_view_t * view );

void zdj_usb_status_view_offline_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)view->state;

    // Catch a switch from zero-usb backend which requires a reboot.
    // if( zdj_usb_status->requires_reboot ) {
    //     // Switch to reboot required view.
    //     view->draw = &zdj_usb_status_view_reboot_draw;
    //     state->needs_layout_update = true;
    //     return;
    // }

    if( state->needs_layout_update ) { _zdj_usb_status_view_offline_update_layout( view ); }
}

void _zdj_usb_status_view_offline_update_layout( zdj_view_t * view ) {
    printf( "zdj_usb_status_view_build_system_offline_layout\n" );
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)view->state;
    zdj_view_t * menu = state->menu_view;
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu_view->state;

    // Clear the old menu.
    zdj_menu_view_remove_all_items( state->menu_view );
    
    // Set header title
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_state->header_view->state;
    strcpy( header_state->name, "USB Mode" );
    strcpy( header_state->title, "Offline" );
    header_state->has_valid_display = false;

    // Add zero icon
    zdj_view_t * zero = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_ZERO ], NULL );
    zero->frame.x = 14;
    zero->frame.y = 8;
    zdj_menu_view_add_item( menu, zero );

    // Add usb icon
    zdj_view_t * usb = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_USB ], NULL );
    usb->frame.x = 70;
    usb->frame.y = 8;
    zdj_menu_view_add_item( menu, usb );
    
    // Add offline label
    zdj_view_t * offline = zdj_new_label_view( "Offline", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    offline->frame.x = 84;
    offline->frame.y = 11;
    zdj_menu_view_add_item( menu, offline );

    // Add sleep icon
    zdj_view_t * snooze = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_SNOOZE ], NULL );
    snooze->frame.x = 77;
    snooze->frame.y = 5;
    zdj_menu_view_add_item( menu, snooze );

    // Add divider
    zdj_view_t * div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_NAR_H_DIV ], NULL );
    div->frame.x = 31;
    div->frame.y = 12;
    div->frame.w = 38;
    zdj_menu_view_add_item( menu, div );

    zdj_menu_view_add_padding( menu, 12 );

    // Add Enable Section
    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Enable" ) );

    // // Add Enable Host
    // zdj_view_t * host_btn = zdj_new_menu_item( "Host Mode", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
    // host_btn->handle_control_event = &zdj_usb_status_view_handle_host_mode_btn;
    // zdj_menu_item_view_state_t * host_state = (zdj_menu_item_view_state_t*)host_btn->state;
    // host_state->data.ptr = view;
    // zdj_menu_view_add_item( menu, host_btn );

    // Add Enable Device
    zdj_view_t * device_btn = zdj_new_menu_item( "Device Mode", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
    device_btn->handle_control_event = &zdj_usb_status_view_handle_device_mode_btn;
    zdj_menu_item_view_state_t * device_state = (zdj_menu_item_view_state_t*)device_btn->state;
    device_state->data.ptr = view;
    zdj_menu_view_add_item( menu, device_btn );

    state->needs_layout_update = false;
}
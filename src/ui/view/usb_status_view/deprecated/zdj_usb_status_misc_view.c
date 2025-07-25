#include <stdio.h>
#include <unistd.h>
#include <sys/reboot.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/controls/hmi/zdj_hmi.h>
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
#include <zerodj/ui/view/progress_bar_view/zdj_progress_bar_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/usb_status_view/zdj_usb_status_view.h>
#include <zerodj/system/usb/zdj_usb.h>

static void _handle_reboot_btn( zdj_view_t * view, void * _event );

void zdj_usb_status_view_build_host_error_layout( zdj_view_t * view ) {
    printf( "zdj_usb_status_view_build_host_error_layout\n" );
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)view->state;
    zdj_view_t * menu = state->menu_view;
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu_view->state;

    // Set header title
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_state->header_view->state;
    header_state->name = "USB Mode";
    header_state->title = "Error";
    header_state->has_valid_display = false;

    // Add zero icon
    zdj_view_t * zero = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_ZERO ], NULL );
    zero->frame->x = 14;
    zero->frame->y = 8;
    zdj_menu_view_add_item( menu, zero );

    // Add usb icon
    zdj_view_t * usb = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_USB ], NULL );
    usb->frame->x = 77;
    usb->frame->y = 8;
    zdj_menu_view_add_item( menu, usb );
    
    // Add error label
    zdj_view_t * error = zdj_new_label_view( "Error", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    error->frame->x = 84;
    error->frame->y = 11;
    zdj_menu_view_add_item( menu, error );

    // Add error icon
    zdj_view_t * exclaim = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_EXCLAIM_SM ], NULL );
    exclaim->frame->x = 87;
    exclaim->frame->y = 5;
    zdj_menu_view_add_item( menu, exclaim );

    // Add divider
    zdj_view_t * div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_NAR_H_DIV ], NULL );
    div->frame->x = 31;
    div->frame->y = 12;
    div->frame->w = 43;
    zdj_menu_view_add_item( menu, div );

    zdj_menu_view_add_padding( menu, 12 );

    // Add Error Section
    // TODO: Make error-specific category string
    zdj_menu_view_add_section( 
        menu, 
        zdj_new_menu_section( "Error" ) 
    );

    // Add Error Type
    // TODO: Make error-specific error string
    // zdj_view_t * host_btn = zdj_new_menu_item( 
    //     zdj_usb_mode_change_name[ zdj_usb_current_mode_status( )->change_state ], 
    //     ZDJ_MENU_ITEM_LAYOUT_INERT 
    // );
    // host_btn->handle_hmi_event = &zdj_usb_status_view_handle_host_mode_btn;
    // zdj_menu_view_add_item( menu, host_btn );
}

void zdj_usb_status_view_build_device_error_layout( zdj_view_t * view ) {
    printf( "zdj_usb_status_view_build_device_error_layout\n" );
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)view->state;
    zdj_view_t * menu = state->menu_view;
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu_view->state;

    // Set header title
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_state->header_view->state;
    header_state->name = "USB Mode";
    header_state->title = "Error";
    header_state->has_valid_display = false;

    // Add zero icon
    zdj_view_t * zero = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_ZERO ], NULL );
    zero->frame->x = 14;
    zero->frame->y = 8;
    zdj_menu_view_add_item( menu, zero );

    // Add usb icon
    zdj_view_t * usb = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_USB ], NULL );
    usb->frame->x = 77;
    usb->frame->y = 8;
    zdj_menu_view_add_item( menu, usb );
    
    // Add error label
    zdj_view_t * error = zdj_new_label_view( "Error", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    error->frame->x = 84;
    error->frame->y = 11;
    zdj_menu_view_add_item( menu, error );

    // Add error icon
    zdj_view_t * exclaim = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_EXCLAIM_SM ], NULL );
    exclaim->frame->x = 87;
    exclaim->frame->y = 5;
    zdj_menu_view_add_item( menu, exclaim );

    // Add divider
    zdj_view_t * div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_NAR_H_DIV ], NULL );
    div->frame->x = 31;
    div->frame->y = 12;
    div->frame->w = 43;
    zdj_menu_view_add_item( menu, div );

    zdj_menu_view_add_padding( menu, 12 );

    // Add Error Section
    // TODO: Make error-specific category string
    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Error" ) );

    // Add Error Type
    // TODO: Make error-specific error string
    // zdj_view_t * host_btn = zdj_new_menu_item( 
    //     zdj_usb_mode_change_name[ zdj_usb_current_mode_status( )->change_state ], 
    //     ZDJ_MENU_ITEM_LAYOUT_INERT 
    // );
    // host_btn->handle_hmi_event = &zdj_usb_status_view_handle_host_mode_btn;
    // zdj_menu_view_add_item( menu, host_btn );
}

void zdj_usb_status_view_build_system_error_layout( zdj_view_t * view ) {
    printf( "zdj_usb_status_view_build_system_error_layout\n" );
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)view->state;
    zdj_view_t * menu = state->menu_view;
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu_view->state;

    // Set header title
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_state->header_view->state;
    header_state->name = "USB Mode";
    header_state->title = "System Error";
    header_state->has_valid_display = false;

    // Add zero icon
    zdj_view_t * zero = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_ZERO ], NULL );
    zero->frame->x = 14;
    zero->frame->y = 8;
    zdj_menu_view_add_item( menu, zero );

    // Add usb icon
    zdj_view_t * usb = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_USB ], NULL );
    usb->frame->x = 77;
    usb->frame->y = 8;
    zdj_menu_view_add_item( menu, usb );
    
    // Add error label
    zdj_view_t * error = zdj_new_label_view( "Error", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    error->frame->x = 91;
    error->frame->y = 11;
    zdj_menu_view_add_item( menu, error );

    // Add error icon
    zdj_view_t * exclaim = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_EXCLAIM_SM ], NULL );
    exclaim->frame->x = 86;
    exclaim->frame->y = 6;
    zdj_menu_view_add_item( menu, exclaim );

    // Add divider
    zdj_view_t * div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_NAR_H_DIV ], NULL );
    div->frame->x = 31;
    div->frame->y = 12;
    div->frame->w = 43;
    zdj_menu_view_add_item( menu, div );

    zdj_menu_view_add_padding( menu, 12 );

    // Add Error Section
    // TODO: Make error-specific category string
    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Error" ) );

    // Add Error Type
    // zdj_view_t * error_type = zdj_new_menu_item( 
    //     zdj_usb_mode_change_name[ zdj_usb_current_mode_status( )->change_state ], 
    //     ZDJ_MENU_ITEM_LAYOUT_INERT 
    // );
    // zdj_menu_view_add_item( menu, error_type );

    // // Add Reboot Btn
    // zdj_view_t * reboot_btn = zdj_new_menu_item( "Reboot", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
    // reboot_btn->handle_hmi_event = &_handle_reboot_btn;
    // zdj_menu_view_add_item( menu, reboot_btn );
}

void zdj_usb_status_view_build_system_offline_layout( zdj_view_t * view ) {
    printf( "zdj_usb_status_view_build_system_offline_layout\n" );
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)view->state;
    zdj_view_t * menu = state->menu_view;
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu_view->state;

    // Set header title
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_state->header_view->state;
    header_state->name = "USB Mode";
    header_state->title = "Offline";
    header_state->has_valid_display = false;

    // Add zero icon
    zdj_view_t * zero = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_ZERO ], NULL );
    zero->frame->x = 14;
    zero->frame->y = 8;
    zdj_menu_view_add_item( menu, zero );

    // Add usb icon
    zdj_view_t * usb = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_USB ], NULL );
    usb->frame->x = 70;
    usb->frame->y = 8;
    zdj_menu_view_add_item( menu, usb );
    
    // Add offline label
    zdj_view_t * offline = zdj_new_label_view( "Offline", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    offline->frame->x = 84;
    offline->frame->y = 11;
    zdj_menu_view_add_item( menu, offline );

    // Add sleep icon
    zdj_view_t * snooze = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_SNOOZE ], NULL );
    snooze->frame->x = 77;
    snooze->frame->y = 5;
    zdj_menu_view_add_item( menu, snooze );

    // Add divider
    zdj_view_t * div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_NAR_H_DIV ], NULL );
    div->frame->x = 31;
    div->frame->y = 12;
    div->frame->w = 38;
    zdj_menu_view_add_item( menu, div );

    zdj_menu_view_add_padding( menu, 12 );

    // Add Enable Section
    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Enable" ) );

    // Add Enable Host
    zdj_view_t * host_btn = zdj_new_menu_item( "Host Mode", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
    host_btn->handle_hmi_event = &zdj_usb_status_view_handle_host_mode_btn;
    zdj_menu_item_view_state_t * host_state = (zdj_menu_item_view_state_t*)host_btn->state;
    host_state->data->ptr = view;
    zdj_menu_view_add_item( menu, host_btn );

    // Add Enable Device
    zdj_view_t * device_btn = zdj_new_menu_item( "Device Mode", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
    device_btn->handle_hmi_event = &zdj_usb_status_view_handle_device_mode_btn;
    zdj_menu_item_view_state_t * device_state = (zdj_menu_item_view_state_t*)device_btn->state;
    device_state->data->ptr = view;
    zdj_menu_view_add_item( menu, device_btn );
}

void zdj_usb_status_view_build_processing_layout( zdj_view_t * view ) {
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)view->state;
    zdj_view_t * menu = state->menu_view;
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu_view->state;

    // Clear the old menu.
    zdj_menu_view_remove_all_items( menu );

    // Set header title
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_state->header_view->state;
    header_state->name = "USB Mode";
    header_state->title = "Switching";
    header_state->has_valid_display = false;

    // Add progress view
    zdj_view_t * progress_bar = zdj_new_progress_bar_view( &(zdj_rect_t){ 11,8,96,6 }, ZDJ_PROGRESS_BAR_VIEW_WAIT );
    zdj_menu_view_add_item( menu, progress_bar );

    // Add processing labels
    zdj_view_t * processing = zdj_new_label_view( state->transition_title_1, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    processing->frame->x = 9;
    processing->frame->y = 18;
    zdj_menu_view_add_item( menu, processing );
    zdj_view_t * processing_2 = zdj_new_label_view( state->transition_title_2, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    processing_2->frame->x = 9;
    zdj_menu_view_add_item( menu, processing_2 );
}

void _handle_reboot_btn( zdj_view_t * view, void * _event ) {
    printf( "attempting reboot\n" );
    sync( );
    reboot( RB_AUTOBOOT );
}
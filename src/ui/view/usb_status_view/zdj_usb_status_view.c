#include <stdio.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <sys/reboot.h>
#include <pthread.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>


#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/panel/zdj_ui_panel.h>
#include <zerodj/ui/view/zdj_view_stack.h>
#include <zerodj/ui/view/dialog_view/zdj_dialog_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/usb_drive_view/zdj_usb_drive_view.h>
#include <zerodj/ui/view/usb_status_view/zdj_usb_status_view.h>
#include <zerodj/system/usb/zdj_usb.h>

// static void _zdj_usb_status_view_update_layout( zdj_view_t * view );
static void _handle_control( zdj_view_t * menu_stack, zdj_control_event_t * _event );
static void _deinit_state( zdj_view_t * usb_status_view );

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _update_layout( zdj_view_t * view );

static void _drive_mode( zdj_view_t * view, zdj_control_event_t * _event );
static void _host_mode( zdj_view_t * view, zdj_control_event_t * _event );
static void _device_mode( zdj_view_t * view, zdj_control_event_t * _event );
static void _offline_mode( zdj_view_t * view, zdj_control_event_t * _event );
static void _reboot( zdj_view_t * view, zdj_control_event_t * _event );

zdj_view_t * zdj_new_usb_status_view( void ) {
    printf( "zdj_new_usb_status_view\n" );
    zdj_view_t * usb_status_view = zdj_new_view( zdj_modal_rect( ) );
    usb_status_view->type = ZDJ_VIEW_MODAL;
    usb_status_view->handle_control_event = &_handle_control;
    usb_status_view->deinit_state = &_deinit_state;
    usb_status_view->draw = &_draw;
    usb_status_view->frame.x = ZDJ_MODAL_X;
    usb_status_view->frame.y = ZDJ_SCREEN_H+2;
    zdj_set_anim( &usb_status_view->in_anim, ZDJ_ANIM_MODAL_SHOW );
    zdj_set_anim( &usb_status_view->out_anim, ZDJ_ANIM_MODAL_HIDE );

    // Add a state instance
    zdj_usb_status_view_state_t * state = calloc( 1, sizeof( zdj_usb_status_view_state_t ) );
    usb_status_view->state = state;
    state->needs_layout_update = true;
    state->frame_counter = 0;

    // Add menu
    zdj_view_t * _menu = zdj_new_menu_view( ZDJ_VERTICAL, zdj_modal_rect( ) );
    zdj_add_subview( usb_status_view, _menu );
    _menu->frame.x = 0;
    _menu->frame.y = 0;
    _menu->frame.w = ZDJ_MODAL_WIDTH;
    _menu->frame.h = ZDJ_MODAL_HEIGHT;
    state->menu_view = _menu;

    // Add header
    zdj_view_t * menu_header = zdj_new_menu_header( 
        "USB Status",
        "",
        ZDJ_MENU_HEADER_STYLE_NORMAL,
        ZDJ_MENU_HEADER_BACK_STYLE_BACK
    );
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_header->state;
    zdj_menu_view_add_header( _menu, menu_header );

    // Add Offline
    zdj_view_t * offline_btn = zdj_new_menu_item( "Offline Mode", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    offline_btn->handle_control_event = &_offline_mode;
    zdj_menu_view_add_item( _menu, offline_btn );

    // Add Host
    zdj_view_t * host_btn = zdj_new_menu_item( "Host Mode", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    host_btn->handle_control_event = &_host_mode;
    zdj_menu_view_add_item( _menu, host_btn );

    // Add Device
    zdj_view_t * device_btn = zdj_new_menu_item( "Device Mode", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    device_btn->handle_control_event = &_device_mode;
    zdj_menu_view_add_item( _menu, device_btn );

    // Add Drive
    zdj_view_t * drive_btn = zdj_new_menu_item( "Drive Mode", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    drive_btn->handle_control_event = &_drive_mode;
    zdj_menu_view_add_item( _menu, drive_btn );


    zdj_view_t * reboot_btn = zdj_new_menu_item( "Reboot", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    reboot_btn->handle_control_event = &_reboot;
    zdj_menu_view_add_item( _menu, reboot_btn );

    return usb_status_view;
}

void _handle_control( zdj_view_t * usb_status_view, zdj_control_event_t * _event ) {
    zdj_control_event_t * e = (zdj_control_event_t *)_event;
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)usb_status_view->state;
    
    // printf( "usb status handle_control\n" );
    // Ignore events which have been blocked by layers above this one.
    if( e->blocked ) { return; }

    // Catch a header back button press, or send events into the menu.
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu_view->state;
    
    if( (e->id == ZDJ_UI_CONTROL_JOG_RELEASE_0 &&
        menu_state->scroll_index == -1) ||
        e->id == ZDJ_UI_CONTROL_NAV_RELEASE_0
    ) {
        printf( "usb_status_view back_btn\n" );
        // Dump the top view on the stack (this view)
        zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
        zdj_pop_subview_of( panel_state->settings_panel, true );
        e->blocked = true;
        // Return immediately since we're being dismissed
        return;
    } else {
        // Send events down into the subview stack
        state->menu_view->handle_control_event( state->menu_view, _event );
    }
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {

}
static void _update_layout( zdj_view_t * view ) {

}

static void _deinit_state( zdj_view_t * usb_status_view ) {
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)usb_status_view->state;
    free( state );
    usb_status_view->state = NULL;
}

static void _drive_mode( zdj_view_t * view, zdj_control_event_t * _event ) { 
    // zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;

    // zdj_view_t * drive_view = zdj_new_usb_drive_view( panel_state->settings_panel );
    // zdj_push_subview( panel_state->settings_panel, drive_view, true );

    zdj_usb_mode_state_t req;
    memset( &req, 0, sizeof( zdj_usb_mode_state_t ) );
    req.mode = ZDJ_USB_MODE_GADGET;
    req.gadget_config.mass_storage = true;
    req.gadget_config.shell = true;
    zdj_usb_enable_mode( &req );
}

static void _host_mode( zdj_view_t * view, zdj_control_event_t * _event ) { 
    // zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    // zdj_view_t * status_view = (zdj_view_t*)state->data.ptr;
    // zdj_usb_status_view_state_t * status_state = (zdj_usb_status_view_state_t*)status_view->state;

    zdj_usb_mode_state_t req;
    memset( &req, 0, sizeof( zdj_usb_mode_state_t ) );
    req.mode = ZDJ_USB_MODE_HOST;
    zdj_usb_enable_mode( &req );
}

static void _device_mode( zdj_view_t * view, zdj_control_event_t * _event ) {
    // zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    // zdj_view_t * status_view = (zdj_view_t*)state->data.ptr;
    // zdj_usb_status_view_state_t * status_state = (zdj_usb_status_view_state_t*)status_view->state;

    zdj_usb_mode_state_t req;
    memset( &req, 0, sizeof( zdj_usb_mode_state_t ) );
    req.mode = ZDJ_USB_MODE_GADGET;
    req.gadget_config.shell = true;
    zdj_usb_enable_mode( &req );
}

static void _offline_mode( zdj_view_t * view, zdj_control_event_t * _event ) {
    // zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    // zdj_view_t * status_view = (zdj_view_t*)state->data.ptr;
    // zdj_usb_status_view_state_t * status_state = (zdj_usb_status_view_state_t*)status_view->state;

    zdj_usb_mode_state_t req;
    memset( &req, 0, sizeof( zdj_usb_mode_state_t ) );
    req.mode = ZDJ_USB_MODE_OFFLINE;
    zdj_usb_enable_mode( &req );
}

static void _reboot( zdj_view_t * view, zdj_control_event_t * _event ) {
    reboot( RB_AUTOBOOT );
}

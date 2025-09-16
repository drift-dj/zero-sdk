#include <stdio.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <pthread.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>


#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/view/zdj_view_stack.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/usb_status_view/zdj_usb_status_view.h>
#include <zerodj/system/usb/zdj_usb.h>

static void _zdj_usb_status_view_update_layout( zdj_view_t * view );
static void _zdj_usb_status_view_handle_control( zdj_view_t * menu_stack, zdj_control_event_t * _event );
static void _zdj_usb_status_view_deinit_state( zdj_view_t * usb_status_view );

static void * _zdj_usb_status_view_inotify_thread_main( void * arg );

zdj_view_t * zdj_new_usb_status_view( void ) {
    printf( "zdj_new_usb_status_view\n" );
    zdj_view_t * usb_status_view = zdj_new_view( zdj_modal_rect( ) );
    usb_status_view->type = ZDJ_VIEW_MODAL;
    usb_status_view->handle_control_event = _zdj_usb_status_view_handle_control;
    usb_status_view->deinit_state = &_zdj_usb_status_view_deinit_state;
    usb_status_view->frame.x = ZDJ_MODAL_X;
    usb_status_view->frame.y = ZDJ_SCREEN_H+2;
    // usb_status_view->in_anim = zdj_new_anim( ZDJ_ANIM_MODAL_SHOW );
    // usb_status_view->out_anim = zdj_new_anim( ZDJ_ANIM_MODAL_HIDE );
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

    // Add view based on USB mode
    if( zdj_usb_status->requires_reboot ) {
        usb_status_view->draw = &zdj_usb_status_view_reboot_draw;
    } else if( zdj_usb_status->mode == ZDJ_USB_MODE_HOST ) {
        usb_status_view->draw = &zdj_usb_status_view_host_draw;
    } else if( zdj_usb_status->mode == ZDJ_USB_MODE_GADGET ) {
        usb_status_view->draw = &zdj_usb_status_view_device_draw;
    } else {
        usb_status_view->draw = &zdj_usb_status_view_offline_draw;
    }

    return usb_status_view;
}

void _zdj_usb_status_view_handle_control( zdj_view_t * usb_status_view, zdj_control_event_t * _event ) {
    zdj_control_event_t * e = (zdj_control_event_t *)_event;
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)usb_status_view->state;
    
    // Ignore events which have been blocked by layers above this one.
    if( e->blocked ) { return; }

    // Catch a header back button press, or send events into the menu.
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu_view->state;
    
    // if( e->id == ZDJ_HMI_ENCO_2_JOG &&
    //     e->type == ZDJ_HMI_EVENT_RELEASE &&
    //     menu_state->scroll_index == -1
    // ) {
    if( e->id == ZDJ_UI_CONTROL_JOG_RELEASE_0 &&
        menu_state->scroll_index == -1
    ) {
        // Dump the top view on the stack (this view)
        zdj_pop_subview_of( zdj_root_view( ), true );
        e->blocked = true;
        // Return immediately since we're being dismissed
        return;
    } else {
        // Send events down into the subview stack
        state->menu_view->handle_control_event( state->menu_view, _event );
    }

    // Send events down into the subview stack
    // zdj_view_t * top_subview = zdj_view_stack_top_subview_of( usb_status_view );
}

void _zdj_usb_status_view_deinit_state( zdj_view_t * usb_status_view ) {
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)usb_status_view->state;
    free( state );
    usb_status_view->state = NULL;
}

void zdj_usb_status_view_handle_host_mode_btn( zdj_view_t * view, zdj_control_event_t * _event ) { 
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * status_view = (zdj_view_t*)state->data->ptr;
    zdj_usb_status_view_state_t * status_state = (zdj_usb_status_view_state_t*)status_view->state;

    zdj_usb_mode_request_t request;
    // Copy over pre-existing gadget state
    request.mode = ZDJ_USB_MODE_HOST;
    request.gadget_config.hid = zdj_usb_status->gadget_config.hid;
    request.gadget_config.mass_storage = zdj_usb_status->gadget_config.mass_storage;
    request.gadget_config.midi = zdj_usb_status->gadget_config.midi;
    request.gadget_config.shell = zdj_usb_status->gadget_config.shell;
    request.gadget_config.uac2 = zdj_usb_status->gadget_config.uac2;
    request.submode = zdj_usb_submode_for_gadget_config( &request.gadget_config );
    zdj_usb_request_mode_switch( &request );
}

void zdj_usb_status_view_handle_device_mode_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * status_view = (zdj_view_t*)state->data->ptr;
    zdj_usb_status_view_state_t * status_state = (zdj_usb_status_view_state_t*)status_view->state;

    // Mode switch will require a reboot.
    // Pop a dialog to confirm.

    zdj_usb_mode_request_t request;
    // Copy over pre-existing gadget state
    request.mode = ZDJ_USB_MODE_GADGET;
    request.gadget_config.hid = zdj_usb_status->gadget_config.hid;
    request.gadget_config.mass_storage = zdj_usb_status->gadget_config.mass_storage;
    request.gadget_config.midi = zdj_usb_status->gadget_config.midi;
    request.gadget_config.shell = zdj_usb_status->gadget_config.shell;
    request.gadget_config.uac2 = zdj_usb_status->gadget_config.uac2;
    // Ensure at least a shell gadget is available
    if( !request.gadget_config.hid &&
        !request.gadget_config.mass_storage &&
        !request.gadget_config.midi &&
        !request.gadget_config.shell &&
        !request.gadget_config.uac2
    ) {
        request.gadget_config.shell = true;
    }
    request.submode = zdj_usb_submode_for_gadget_config( &request.gadget_config );
    zdj_usb_request_mode_switch( &request );
}

void zdj_usb_status_view_handle_offline_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * status_view = (zdj_view_t*)state->data->ptr;
    zdj_usb_status_view_state_t * status_state = (zdj_usb_status_view_state_t*)status_view->state;
    zdj_usb_mode_request_t request;
    // Copy over pre-existing gadget state
    request.mode = ZDJ_USB_MODE_OFFLINE;
    request.submode = ZDJ_USB_SUBMODE_OFFLINE;
    zdj_usb_request_mode_switch( &request );
}
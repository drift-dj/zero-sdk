#include <stdio.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <pthread.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/controls/hmi/zdj_hmi.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/view/zdj_view_stack.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/usb_status_view/zdj_usb_status_view.h>
#include <zerodj/usb/zdj_usb.h>

static void _zdj_usb_status_view_draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_usb_status_view_update_layout( zdj_view_t * view );
static void _zdj_usb_status_view_handle_hmi( zdj_view_t * menu_stack, void * _event );
static void _zdj_usb_status_view_deinit_state( zdj_view_t * usb_status_view );

static void * _zdj_usb_status_view_inotify_thread_main( void * arg );

zdj_view_t * zdj_new_usb_status_view( void ) {
    zdj_view_t * usb_status_view = zdj_new_view( zdj_modal_rect( ) );
    usb_status_view->type = ZDJ_VIEW_MODAL;
    usb_status_view->draw = &_zdj_usb_status_view_draw;
    usb_status_view->handle_hmi_event = _zdj_usb_status_view_handle_hmi;
    usb_status_view->deinit_state = &_zdj_usb_status_view_deinit_state;

    usb_status_view->frame->x = ZDJ_MODAL_X;
    usb_status_view->frame->y = ZDJ_SCREEN_H+2;
    
    usb_status_view->in_anim = zdj_new_anim( ZDJ_ANIM_MODAL_SHOW );
    usb_status_view->out_anim = zdj_new_anim( ZDJ_ANIM_MODAL_HIDE );

    // Add a state instance
    zdj_usb_status_view_state_t * state = calloc( 1, sizeof( zdj_usb_status_view_state_t ) );
    usb_status_view->state = state;
    state->needs_layout_update = true;
    state->frame_counter = 0;

    // Add menu
    zdj_view_t * _menu = zdj_new_menu_view( ZDJ_VERTICAL, zdj_modal_rect( ) );
    zdj_add_subview( usb_status_view, _menu );
    _menu->frame->x = 0;
    _menu->frame->y = 0;
    _menu->frame->w = ZDJ_MODAL_WIDTH;
    _menu->frame->h = ZDJ_MODAL_HEIGHT;
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

    return usb_status_view;
}


// Drop in a dotted BG to obscure the views below
void _zdj_usb_status_view_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)view->state;

    // During gadget mode transition, show a wait progress bar until
    // usb status has a frontend update.
    if( zdj_usb_status->has_frontend_update ) {
        zdj_usb_status->has_frontend_update = false;
        state->needs_layout_update = true;
    }

    // While in host mode, we're periodically polling for attached devices
    if( zdj_usb_has_sysfs_devices_update( ) ) {
        printf( "Updating layout for sysfs devices\n" );
        state->needs_layout_update = true;
    }
    // While in device mode, we're periodically polling for port partner
    if( zdj_usb_has_port_partner_update( ) ) {
        printf( "Updating layout for port partner\n" );
        state->needs_layout_update = true;
    }

    if( state->needs_layout_update ) { _zdj_usb_status_view_update_layout( view ); }
}

void _zdj_usb_status_view_update_layout( zdj_view_t * view ) {
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)view->state;
    // zdj_usb_mode_status_t * status = zdj_usb_current_mode_status( );

    // Clear the old menu.
    zdj_menu_view_remove_all_items( state->menu_view );

    printf( "_zdj_usb_status_view_update_layout: %s\n", zdj_usb_mode_name[ zdj_usb_status->mode ] );

    switch ( zdj_usb_status->mode ) {
    case ZDJ_USB_MODE_ERROR:
        zdj_usb_status_view_build_system_error_layout( view );
        break;
    case ZDJ_USB_MODE_OFFLINE:
    case ZDJ_USB_MODE_UNKNOWN:
        zdj_usb_status_view_build_system_offline_layout( view );
        break;
    case ZDJ_USB_MODE_HOST:
        zdj_usb_status_view_build_host_layout( view );
        break;
    case ZDJ_USB_MODE_GADGET:
        zdj_usb_status_view_build_device_layout( view );
        break;
    default:
        break;
    }

    state->needs_layout_update = false;
}

void _zdj_usb_status_view_handle_hmi( zdj_view_t * usb_status_view, void * _event ) {
    zdj_hmi_event_t * e = (zdj_hmi_event_t *)_event;
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)usb_status_view->state;
    
    // Ignore events which have been blocked by layers above this one.
    if( e->blocked ) { return; }

    // Catch a header back button press, or send events into the menu.
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu_view->state;
    
    if( e->id == ZDJ_HMI_ENCO_2_JOG &&
        e->type == ZDJ_HMI_EVENT_RELEASE &&
        menu_state->scroll_index == -1
    ) {
        // Dump the top view on the stack (this view)
        zdj_pop_subview_of( zdj_root_view( ), true );
        e->blocked = true;
        // Return immediately since we're being dismissed
        return;
    } else {
        // Send events down into the subview stack
        state->menu_view->handle_hmi_event( state->menu_view, _event );
    }

    // Send events down into the subview stack
    // zdj_view_t * top_subview = zdj_view_stack_top_subview_of( usb_status_view );
}

void _zdj_usb_status_view_deinit_state( zdj_view_t * usb_status_view ) {
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)usb_status_view->state;
    free( state );
    usb_status_view->state = NULL;
}

void zdj_usb_status_view_handle_host_mode_btn( zdj_view_t * view, void * _event ) { 
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * status_view = (zdj_view_t*)state->data->ptr;
    zdj_usb_status_view_state_t * status_state = (zdj_usb_status_view_state_t*)status_view->state;

    // Mode switch will require a reboot.
    // Pop a dialog to confirm.
    if( zdj_usb_status->mode == ZDJ_USB_MODE_GADGET ||
        zdj_usb_status->mode == ZDJ_USB_MODE_OFFLINE 
    ) {
        
    }

    // // Request a mode change
    // zdj_usb_mode_request_t request;
    // request.mode = ZDJ_USB_MODE_HOST;
    // zdj_usb_request_mode( &request );

    // // Start the transition check flow
    // status_state->transition = true;
    // status_state->transition_counter = 0;
    // strcpy( status_state->transition_title_1, "Switching to Host Mode" );
    // strcpy( status_state->transition_title_2, " " );

    // // Put the UI into processing mode
    // zdj_usb_status_view_build_processing_layout( status_view );

    // // Start periodic scan for new attached devices
    // zdj_usb_start_sysfs_devices_poll( );
    // // Stop periodic scan for port partner
    // zdj_usb_stop_port_partner_poll( );
}

void zdj_usb_status_view_handle_device_mode_btn( zdj_view_t * view, void * _event ) {
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
    request.submode = zdj_usb_submode_for_gadget_config( &request.gadget_config );
    zdj_usb_request_mode_switch( &request );



    // // Request a mode change to last device mode
    // zdj_usb_gadget_config_t config;
    // memset( &config, 0, sizeof( zdj_usb_gadget_config_t ) );
    // // config.uac2 = true;
    // config.shell = true;
    // zdj_usb_reset_gadget( &config );

    // // Start the transition check flow
    // status_state->transition = true;
    // status_state->transition_counter = 0;

    strcpy( status_state->transition_title_1, "Switching to Device Mode:" );
    strcpy( 
        status_state->transition_title_2, 
        zdj_usb_submode_name[ request.submode ]
    );

    // Put the UI into processing mode
    zdj_usb_status_view_build_processing_layout( status_view );

    // // Stop periodic scan for new attached devices
    // zdj_usb_stop_sysfs_devices_poll( );
    // // Start periodic scan for port partner
    // zdj_usb_start_port_partner_poll( );
}

void zdj_usb_status_view_handle_offline_btn( zdj_view_t * view, void * _event ) {
//     zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
//     zdj_view_t * status_view = (zdj_view_t*)state->data->ptr;
//     zdj_usb_status_view_state_t * status_state = (zdj_usb_status_view_state_t*)status_view->state;

//     // Request a mode change
//     zdj_usb_mode_request_t request;
//     request.mode = ZDJ_USB_MODE_OFFLINE;
//     zdj_usb_request_mode( &request );

//     // Start the transition check flow
//     status_state->transition = true;
//     status_state->transition_counter = 0;
//     strcpy( status_state->transition_title_1, "Disabling USB" );
//     strcpy( status_state->transition_title_2, " " );

//     // Put the UI into processing mode
//     zdj_usb_status_view_build_processing_layout( status_view );

//     // Stop periodic scan for new attached devices
//     zdj_usb_stop_sysfs_devices_poll( );
//     // Stop periodic scan for port partner
//     zdj_usb_stop_port_partner_poll( );
}
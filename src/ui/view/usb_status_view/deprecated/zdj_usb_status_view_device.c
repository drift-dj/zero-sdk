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
#include <zerodj/ui/view/progress_bar_view/zdj_progress_bar_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/usb_status_view/zdj_usb_status_view.h>
#include <zerodj/system/usb/zdj_usb.h>

static void _zdj_usb_status_view_device_update_layout( zdj_view_t * view );
static void _zdj_usb_status_view_device_processing_update_layout( zdj_view_t * view );
static void _zdj_usb_status_view_device_error_update_layout( zdj_view_t * view );

static void _handle_uac_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_midi_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_drive_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_controller_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_shell_btn( zdj_view_t * view, zdj_control_event_t * _event );


void zdj_usb_status_view_device_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)view->state;

    // // During gadget mode transition, show a wait progress bar until
    // // usb status has a frontend update.
    // if( zdj_usb_status->has_frontend_update ) {
    //     zdj_usb_status->has_frontend_update = false;
    //     state->needs_layout_update = true;
    // }

    // Catch a switch from zero-usb backend which requires a reboot.
    // if( zdj_usb_status->requires_reboot ) {
    //     // Switch to reboot required view.
    //     view->draw = &zdj_usb_status_view_reboot_draw;
    //     state->needs_layout_update = true;
    //     return;
    // }

    // Periodically check if we have a port partner change
    if( state->frame_counter++ > 80 ) {
        state->frame_counter = 0;
        FILE * fp = popen( "ls /sys/class/typec | grep -i partner", "r" );
        if ( fp != NULL ) {
            char line[ 256 ];
            fgets( line, sizeof( line ), fp );
            // printf( "/typec:%s\n", line );
            if( strstr( line, "partner" ) ) { 
                // Partner exists
                if( state->has_partner == false ) {
                    state->has_partner_update = true;
                }
                state->has_partner = true;
            } else {
                // Partner does not exist
                if( state->has_partner == true ) {
                    state->has_partner_update = true;
                }
                state->has_partner = false;
            }
        }
        pclose( fp );

        if( state->has_partner_update ) {
            state->has_partner_update = false;
            state->needs_layout_update = true;
        }
    }

    if( state->needs_layout_update ) { _zdj_usb_status_view_device_update_layout( view ); }
}

// Draw fn for use when USB system is switching gadget modes.
// This takes a few seconds while zero-usb app runs scripts.
void zdj_usb_status_view_device_processing_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)view->state;

    // // If there's a usb status update from the zero-usb backend,
    // // check the state and exit processing mode.
    // if( zdj_usb_status->has_frontend_update ) {
    //     zdj_usb_status->has_frontend_update = false;

    //     // The zero-usb backend advances switch_state out of 'running' when
    //     // it has completed the script cycle.
    //     // Check for error state and update draw pointer as appropriate
    //     if( zdj_usb_status->switch_state == ZDJ_USB_SUBMODE_SWITCH_SUCCESS ) {
    //         view->draw = zdj_usb_status_view_device_draw;
    //         state->needs_layout_update = true;
    //     } else if( zdj_usb_status->switch_state > ZDJ_USB_SUBMODE_SWITCH_SUCCESS ) {
    //         view->draw = zdj_usb_status_view_device_error_draw;
    //         state->needs_layout_update = true;
    //     }
    // }
}

// Draw fn for use when USB system has errored out in Gadget mode.
void zdj_usb_status_view_device_error_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)view->state;

    if( state->needs_layout_update ) { _zdj_usb_status_view_device_update_layout( view ); }
}

void _zdj_usb_status_view_device_update_layout( zdj_view_t * view ) {
    printf( "_zdj_usb_status_view_device_update_layout: %s\n", zdj_usb_mode_name[ zdj_usb_state->mode_state.mode ] );
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)view->state;
    zdj_view_t * menu = state->menu_view;
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu_view->state;

    // Clear the old menu.
    zdj_menu_view_remove_all_items( state->menu_view );
    
     // Set header title
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_state->header_view->state;
    strcpy( header_state->name, "USB Mode" );
    strcpy( header_state->title, "Device" );
    header_state->has_valid_display = false;

    zdj_view_t * box;
    zdj_view_t * stat_label;
    if ( state->has_partner ) {
        // Add status box
        box = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BOX_1 ], NULL );
        box->frame.x = 16;
        box->frame.y = 3;
        zdj_menu_view_add_item( menu, box );

        // Add status label
        stat_label = zdj_new_label_view( "...", ZDJ_FONT_12, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
        stat_label->frame.x = 20;
        stat_label->frame.y = 2;
        zdj_menu_view_add_item( menu, stat_label );
    } else {
        // Add empty status box
        box = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DOTTED_BOX_1 ], NULL );
        box->frame.x = 16;
        box->frame.y = 3;
        zdj_menu_view_add_item( menu, box );
    }

    // Add zero icon
    zdj_view_t * zero = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_ZERO ], NULL );
    zero->frame.x = 88;
    zero->frame.y = 8;
    zdj_menu_view_add_item( menu, zero );

    // Add divider
    zdj_view_t * div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_NAR_H_DIV ], NULL );
    div->frame.x = 37;
    div->frame.y = 12;
    div->frame.w = 50;
    zdj_menu_view_add_item( menu, div );

    zdj_menu_view_add_padding( menu, 12 );

    // Add Services Section
    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Services" ) );

    // Add Service Buttons
    zdj_view_t * uac_btn = zdj_new_menu_item( "Audio (UAC2)", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    uac_btn->handle_control_event = &_handle_uac_btn;
    zdj_menu_item_view_state_t * uac_state = (zdj_menu_item_view_state_t*)uac_btn->state;
    uac_state->data.b_val = ( access( "/sys/kernel/config/usb_gadget/g1/functions/uac2.usb0", F_OK ) == 0 );
    uac_state->data.ptr = view;
    zdj_menu_view_add_item( menu, uac_btn );

    zdj_view_t * midi_btn = zdj_new_menu_item( "MIDI", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    midi_btn->handle_control_event = &_handle_midi_btn;
    zdj_menu_item_view_state_t * midi_state = (zdj_menu_item_view_state_t*)midi_btn->state;
    midi_state->data.b_val = ( access( "/sys/kernel/config/usb_gadget/g1/functions/midi.usb0", F_OK ) == 0 );
    midi_state->data.ptr = view;
    zdj_menu_view_add_item( menu, midi_btn );

    zdj_view_t * drive_btn = zdj_new_menu_item( "Drive", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    drive_btn->handle_control_event = &_handle_drive_btn;
    zdj_menu_item_view_state_t * drive_state = (zdj_menu_item_view_state_t*)drive_btn->state;
    drive_state->data.b_val = ( access( "/sys/kernel/config/usb_gadget/g1/functions/mass_storage.0", F_OK ) == 0 );
    drive_state->data.ptr = view;
    zdj_menu_view_add_item( menu, drive_btn );

    zdj_view_t * controller_btn = zdj_new_menu_item( "Controller", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    // toggle_btn->handle_control_event = &_handle_host_mode_btn;
    // zdj_menu_item_view_state_t * uac_state = (zdj_menu_item_view_state_t*)uac_btn->state;
    // uac_state->data->b_val = access( "/sys/kernel/config/usb_gadget/g1/functions/uac2.usb0", F_OK );
    zdj_menu_view_add_item( menu, controller_btn );

    zdj_view_t * shell_btn = zdj_new_menu_item( "Shell", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    shell_btn->handle_control_event = &_handle_shell_btn;
    zdj_menu_item_view_state_t * shell_state = (zdj_menu_item_view_state_t*)shell_btn->state;
    shell_state->data.b_val = ( access( "/sys/kernel/config/usb_gadget/g1/functions/acm.ttyGS0", F_OK ) == 0 );
    shell_state->data.ptr = view;
    zdj_menu_view_add_item( menu, shell_btn );


    // Add Mode Switch Section
    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Mode Switch" ) );

    // Add Drive Mode
    zdj_view_t * drive_mode_btn = zdj_new_menu_item( "Drive Mode", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    drive_mode_btn->handle_control_event = &zdj_usb_status_view_handle_drive_mode_btn;
    zdj_menu_item_view_state_t * drive_mode_state = (zdj_menu_item_view_state_t*)drive_mode_btn->state;
    drive_mode_state->data.ptr = view;
    zdj_menu_view_add_item( menu, drive_mode_btn );
    
    // // Add Enable Host
    // zdj_view_t * host_btn = zdj_new_menu_item( "Host Mode", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    // host_btn->handle_control_event = &zdj_usb_status_view_handle_host_mode_btn;
    // zdj_menu_item_view_state_t * host_state = (zdj_menu_item_view_state_t*)host_btn->state;
    // host_state->data.ptr = view;
    // zdj_menu_view_add_item( menu, host_btn );

    // Add Offline
    zdj_view_t * offline_btn = zdj_new_menu_item( "Offline", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    offline_btn->handle_control_event = &zdj_usb_status_view_handle_offline_btn;
    zdj_menu_item_view_state_t * offline_state = (zdj_menu_item_view_state_t*)offline_btn->state;
    offline_state->data.ptr = view;
    zdj_menu_view_add_item( menu, offline_btn );

    state->needs_layout_update = false;
}

static void _zdj_usb_status_view_device_processing_update_layout( zdj_view_t * view ) {
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)view->state;
    zdj_view_t * menu = state->menu_view;
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu_view->state;

    // Clear the old menu.
    zdj_menu_view_remove_all_items( menu );

    // Set header title
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_state->header_view->state;
    strcpy( header_state->name, "USB Mode" );
    strcpy( header_state->title, "Switching" );
    header_state->has_valid_display = false;

    // Add progress view
    zdj_view_t * progress_bar = zdj_new_progress_bar_view( &(zdj_rect_t){ 11,8,96,6 }, ZDJ_PROGRESS_BAR_VIEW_WAIT );
    zdj_menu_view_add_item( menu, progress_bar );

    // Add processing labels
    zdj_view_t * processing = zdj_new_label_view( "Switching to USB Gadget Mode", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    processing->frame.x = 9;
    processing->frame.y = 18;
    zdj_menu_view_add_item( menu, processing );

    zdj_view_t * processing_2 = zdj_new_label_view( zdj_usb_submode_name[ zdj_usb_state->mode_state.submode ], ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    processing_2->frame.x = 9;
    zdj_menu_view_add_item( menu, processing_2 );
}

static void _zdj_usb_status_view_device_error_update_layout( zdj_view_t * view ) {
    printf( "zdj_usb_status_view_build_system_error_layout\n" );
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)view->state;
    zdj_view_t * menu = state->menu_view;
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu_view->state;

    // Set header title
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_state->header_view->state;
    strcpy( header_state->name, "USB Mode" );
    strcpy( header_state->title, "System Error" );
    header_state->has_valid_display = false;

    // Add zero icon
    zdj_view_t * zero = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_ZERO ], NULL );
    zero->frame.x = 14;
    zero->frame.y = 8;
    zdj_menu_view_add_item( menu, zero );

    // Add usb icon
    zdj_view_t * usb = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_USB ], NULL );
    usb->frame.x = 77;
    usb->frame.y = 8;
    zdj_menu_view_add_item( menu, usb );
    
    // Add error label
    zdj_view_t * error = zdj_new_label_view( "Error", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    error->frame.x = 91;
    error->frame.y = 11;
    zdj_menu_view_add_item( menu, error );

    // Add error icon
    zdj_view_t * exclaim = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_EXCLAIM_SM ], NULL );
    exclaim->frame.x = 86;
    exclaim->frame.y = 6;
    zdj_menu_view_add_item( menu, exclaim );

    // Add divider
    zdj_view_t * div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_NAR_H_DIV ], NULL );
    div->frame.x = 31;
    div->frame.y = 12;
    div->frame.w = 43;
    zdj_menu_view_add_item( menu, div );

    zdj_menu_view_add_padding( menu, 12 );

    // Add Error Section
    // TODO: Make error-specific category string
    // zdj_menu_view_add_section( menu, zdj_new_menu_section( "Error" ) );

    // Add Error Type
    // zdj_view_t * error_type = zdj_new_menu_item( 
    //     zdj_usb_mode_change_name[ zdj_usb_current_mode_status( )->change_state ], 
    //     ZDJ_MENU_ITEM_LAYOUT_INERT 
    // );
    // zdj_menu_view_add_item( menu, error_type );

    // // Add Reboot Btn
    // zdj_view_t * reboot_btn = zdj_new_menu_item( "Reboot", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
    // reboot_btn->handle_control_event = &_handle_reboot_btn;
    // zdj_menu_view_add_item( menu, reboot_btn );
}

static void _handle_uac_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    // zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    // zdj_view_t * status_view = (zdj_view_t*)state->data.ptr;
    // zdj_usb_status_view_state_t * status_state = (zdj_usb_status_view_state_t*)status_view->state;

    // zdj_usb_state->mode_state.gadget_config.uac2 = !zdj_usb_state->mode_state.gadget_config.uac2;

    // zdj_usb_mode_state_t request;
    // request.mode = ZDJ_USB_MODE_GADGET;
    // request.gadget_config.uac2 = zdj_usb_state->mode_state.gadget_config.uac2;
    // request.gadget_config.midi = zdj_usb_state->mode_state.gadget_config.midi;
    // request.gadget_config.mass_storage = zdj_usb_state->mode_state.gadget_config.mass_storage;
    // request.gadget_config.hid = zdj_usb_state->mode_state.gadget_config.hid;
    // request.gadget_config.shell = zdj_usb_state->mode_state.gadget_config.shell;
    // request.submode = zdj_usb_submode_for_gadget_config( &request.gadget_config );
    
    // // Ensure we don't attempt to switch to current mode
    // if( zdj_usb_request_mode_switch( &request ) != ZDJ_ERROR_BAD_COMMAND ) {
    //     // Put the UI into processing mode
    //     _zdj_usb_status_view_device_processing_update_layout( status_view );
    //     view->draw = zdj_usb_status_view_device_processing_draw;
    // } else {
    //     zdj_usb_state->mode_state.gadget_config.uac2 = !zdj_usb_state->mode_state.gadget_config.uac2;
    // }
}

static void _handle_midi_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    // zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    // zdj_view_t * status_view = (zdj_view_t*)state->data.ptr;
    // zdj_usb_status_view_state_t * status_state = (zdj_usb_status_view_state_t*)status_view->state;

    // zdj_usb_state->mode_state.gadget_config.midi = !zdj_usb_state->mode_state.gadget_config.midi;

    // zdj_usb_mode_state_t request;
    // request.mode = ZDJ_USB_MODE_GADGET;
    // request.gadget_config.uac2 = zdj_usb_state->mode_state.gadget_config.uac2;
    // request.gadget_config.midi = zdj_usb_state->mode_state.gadget_config.midi;
    // request.gadget_config.mass_storage = zdj_usb_state->mode_state.gadget_config.mass_storage;
    // request.gadget_config.hid = zdj_usb_state->mode_state.gadget_config.hid;
    // request.gadget_config.shell = zdj_usb_state->mode_state.gadget_config.shell;
    // request.submode = zdj_usb_submode_for_gadget_config( &request.gadget_config );
    // zdj_usb_request_mode_switch( &request );

    // // // Put the UI into processing mode
    // // _zdj_usb_status_view_device_processing_update_layout( status_view );
    // // view->draw = zdj_usb_status_view_device_processing_draw;

    // // Ensure we don't attempt to switch to current mode
    // if( zdj_usb_request_mode_switch( &request ) != ZDJ_ERROR_BAD_COMMAND ) {
    //     // Put the UI into processing mode
    //     _zdj_usb_status_view_device_processing_update_layout( status_view );
    //     view->draw = zdj_usb_status_view_device_processing_draw;
    // } else {
    //     zdj_usb_state->mode_state.gadget_config.midi = !zdj_usb_state->mode_state.gadget_config.midi;
    // }
}

static void _handle_drive_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    // zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    // zdj_view_t * status_view = (zdj_view_t*)state->data.ptr;
    // zdj_usb_status_view_state_t * status_state = (zdj_usb_status_view_state_t*)status_view->state;

    // zdj_usb_state->mode_state.gadget_config.mass_storage = !zdj_usb_state->mode_state.gadget_config.mass_storage;

    // zdj_usb_mode_state_t request;
    // request.mode = ZDJ_USB_MODE_GADGET;
    // request.gadget_config.uac2 = zdj_usb_state->mode_state.gadget_config.uac2;
    // request.gadget_config.midi = zdj_usb_state->mode_state.gadget_config.midi;
    // request.gadget_config.mass_storage = zdj_usb_state->mode_state.gadget_config.mass_storage;
    // request.gadget_config.hid = zdj_usb_state->mode_state.gadget_config.hid;
    // request.gadget_config.shell = zdj_usb_state->mode_state.gadget_config.shell;
    // request.submode = zdj_usb_submode_for_gadget_config( &request.gadget_config );
    // zdj_usb_request_mode_switch( &request );

    // // // Put the UI into processing mode
    // // _zdj_usb_status_view_device_processing_update_layout( status_view );
    // // view->draw = zdj_usb_status_view_device_processing_draw;

    // // Ensure we don't attempt to switch to current mode
    // if( zdj_usb_request_mode_switch( &request ) != ZDJ_ERROR_BAD_COMMAND ) {
    //     // Put the UI into processing mode
    //     _zdj_usb_status_view_device_processing_update_layout( status_view );
    //     view->draw = zdj_usb_status_view_device_processing_draw;
    // } else {
    //     zdj_usb_state->mode_state.gadget_config.mass_storage = !zdj_usb_state->mode_state.gadget_config.mass_storage;
    // }
}

static void _handle_controller_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    // zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    // zdj_view_t * status_view = (zdj_view_t*)state->data.ptr;
    // zdj_usb_status_view_state_t * status_state = (zdj_usb_status_view_state_t*)status_view->state;

    // zdj_usb_state->mode_state.gadget_config.hid = !zdj_usb_state->mode_state.gadget_config.hid;

    // zdj_usb_mode_state_t request;
    // request.mode = ZDJ_USB_MODE_GADGET;
    // request.gadget_config.uac2 = zdj_usb_state->mode_state.gadget_config.uac2;
    // request.gadget_config.midi = zdj_usb_state->mode_state.gadget_config.midi;
    // request.gadget_config.mass_storage = zdj_usb_state->mode_state.gadget_config.mass_storage;
    // request.gadget_config.hid = zdj_usb_state->mode_state.gadget_config.hid;
    // request.gadget_config.shell = zdj_usb_state->mode_state.gadget_config.shell;
    // request.submode = zdj_usb_submode_for_gadget_config( &request.gadget_config );
    // zdj_usb_request_mode_switch( &request );

    // // // Put the UI into processing mode
    // // _zdj_usb_status_view_device_processing_update_layout( status_view );
    // // view->draw = zdj_usb_status_view_device_processing_draw;

    // // Ensure we don't attempt to switch to current mode
    // if( zdj_usb_request_mode_switch( &request ) != ZDJ_ERROR_BAD_COMMAND ) {
    //     // Put the UI into processing mode
    //     _zdj_usb_status_view_device_processing_update_layout( status_view );
    //     view->draw = zdj_usb_status_view_device_processing_draw;
    // } else {
    //     zdj_usb_state->mode_state.gadget_config.hid = !zdj_usb_state->mode_state.gadget_config.hid;
    // }
}

static void _handle_shell_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    // zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    // zdj_view_t * status_view = (zdj_view_t*)state->data.ptr;
    // zdj_usb_status_view_state_t * status_state = (zdj_usb_status_view_state_t*)status_view->state;

    // zdj_usb_state->mode_state.gadget_config.shell = !zdj_usb_state->mode_state.gadget_config.shell;

    // zdj_usb_mode_state_t request;
    // request.mode = ZDJ_USB_MODE_GADGET;
    // request.gadget_config.uac2 = zdj_usb_state->mode_state.gadget_config.uac2;
    // request.gadget_config.midi = zdj_usb_state->mode_state.gadget_config.midi;
    // request.gadget_config.mass_storage = zdj_usb_state->mode_state.gadget_config.mass_storage;
    // request.gadget_config.hid = zdj_usb_state->mode_state.gadget_config.hid;
    // request.gadget_config.shell = zdj_usb_state->mode_state.gadget_config.shell;
    // request.submode = zdj_usb_submode_for_gadget_config( &request.gadget_config );
    // zdj_usb_request_mode_switch( &request );

    // // // Put the UI into processing mode
    // // _zdj_usb_status_view_device_processing_update_layout( status_view );
    // // view->draw = zdj_usb_status_view_device_processing_draw;

    // // Ensure we don't attempt to switch to current mode
    // if( zdj_usb_request_mode_switch( &request ) != ZDJ_ERROR_BAD_COMMAND ) {
    //     // Put the UI into processing mode
    //     _zdj_usb_status_view_device_processing_update_layout( status_view );
    //     view->draw = zdj_usb_status_view_device_processing_draw;
    // } else {
    //     zdj_usb_state->mode_state.gadget_config.shell = !zdj_usb_state->mode_state.gadget_config.shell;
    // }
}
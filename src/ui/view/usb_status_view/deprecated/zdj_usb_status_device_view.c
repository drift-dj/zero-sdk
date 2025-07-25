#include <stdio.h>
#include <unistd.h>

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
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/usb_status_view/zdj_usb_status_view.h>
#include <zerodj/usb/zdj_usb.h>

static void _handle_uac_btn( zdj_view_t * view, void * _event );
static void _handle_midi_btn( zdj_view_t * view, void * _event );
static void _handle_drive_btn( zdj_view_t * view, void * _event );
static void _handle_controller_btn( zdj_view_t * view, void * _event );
static void _handle_shell_btn( zdj_view_t * view, void * _event );

void zdj_usb_status_view_build_device_layout( zdj_view_t * view ) {
    printf( "zdj_usb_status_view_build_device_layout\n" );
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)view->state;
    zdj_view_t * menu = state->menu_view;
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu_view->state;

    // Set header title
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_state->header_view->state;
    header_state->name = "USB Mode";
    header_state->title = "Device";
    header_state->has_valid_display = false;

    zdj_view_t * box;
    zdj_view_t * stat_label;
    if ( zdj_usb_has_port_partner( ) ) {
        // Add status box
        box = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BOX_1 ], NULL );
        box->frame->x = 16;
        box->frame->y = 3;
        zdj_menu_view_add_item( menu, box );

        // Add status label
        stat_label = zdj_new_label_view( "...", ZDJ_FONT_12, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
        stat_label->frame->x = 20;
        stat_label->frame->y = 2;
        zdj_menu_view_add_item( menu, stat_label );
    } else {
        // Add empty status box
        box = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DOTTED_BOX_1 ], NULL );
        box->frame->x = 16;
        box->frame->y = 3;
        zdj_menu_view_add_item( menu, box );
    }

    // Add zero icon
    zdj_view_t * zero = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_ZERO ], NULL );
    zero->frame->x = 88;
    zero->frame->y = 8;
    zdj_menu_view_add_item( menu, zero );

    // Add divider
    zdj_view_t * div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_NAR_H_DIV ], NULL );
    div->frame->x = 37;
    div->frame->y = 12;
    div->frame->w = 50;
    zdj_menu_view_add_item( menu, div );

    zdj_menu_view_add_padding( menu, 12 );

    // Add Services Section
    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Services" ) );

    // Get current gadget config to supply toggle states
    // zdj_usb_gadget_config_t * config = zdj_usb_current_gadget_config( );

    // Add Service Buttons
    zdj_view_t * uac_btn = zdj_new_menu_item( "Audio (UAC2)", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    uac_btn->handle_hmi_event = &_handle_uac_btn;
    zdj_menu_item_view_state_t * uac_state = (zdj_menu_item_view_state_t*)uac_btn->state;
    uac_state->data->b_val = zdj_usb_status->gadget_config.uac2;
    uac_state->data->ptr = view;
    zdj_menu_view_add_item( menu, uac_btn );

    zdj_view_t * midi_btn = zdj_new_menu_item( "MIDI", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    midi_btn->handle_hmi_event = &_handle_midi_btn;
    zdj_menu_item_view_state_t * midi_state = (zdj_menu_item_view_state_t*)midi_btn->state;
    midi_state->data->b_val = zdj_usb_status->gadget_config.midi;
    midi_state->data->ptr = view;
    zdj_menu_view_add_item( menu, midi_btn );

    zdj_view_t * drive_btn = zdj_new_menu_item( "Drive", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    drive_btn->handle_hmi_event = &_handle_drive_btn;
    zdj_menu_item_view_state_t * drive_state = (zdj_menu_item_view_state_t*)drive_btn->state;
    drive_state->data->b_val = zdj_usb_status->gadget_config.mass_storage;
    drive_state->data->ptr = view;
    zdj_menu_view_add_item( menu, drive_btn );

    zdj_view_t * controller_btn = zdj_new_menu_item( "Controller", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    // toggle_btn->handle_hmi_event = &_handle_host_mode_btn;
    // zdj_menu_item_view_state_t * uac_state = (zdj_menu_item_view_state_t*)uac_btn->state;
    // uac_state->data->b_val = access( "/sys/kernel/config/usb_gadget/g1/functions/uac2.usb0", F_OK );
    zdj_menu_view_add_item( menu, controller_btn );

    zdj_view_t * shell_btn = zdj_new_menu_item( "Shell", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    shell_btn->handle_hmi_event = &_handle_shell_btn;
    zdj_menu_item_view_state_t * shell_state = (zdj_menu_item_view_state_t*)shell_btn->state;
    shell_state->data->b_val = zdj_usb_status->gadget_config.shell;
    shell_state->data->ptr = view;
    zdj_menu_view_add_item( menu, shell_btn );


    // Add Mode Switch Section
    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Mode Switch" ) );

    // Add Enable Host
    zdj_view_t * host_btn = zdj_new_menu_item( "Host Mode", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
    host_btn->handle_hmi_event = &zdj_usb_status_view_handle_host_mode_btn;
    zdj_menu_item_view_state_t * host_state = (zdj_menu_item_view_state_t*)host_btn->state;
    host_state->data->ptr = view;
    zdj_menu_view_add_item( menu, host_btn );

    // Add Offline
    zdj_view_t * offline_btn = zdj_new_menu_item( "Offline", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
    offline_btn->handle_hmi_event = &zdj_usb_status_view_handle_offline_btn;
    zdj_menu_item_view_state_t * offline_state = (zdj_menu_item_view_state_t*)offline_btn->state;
    offline_state->data->ptr = view;
    zdj_menu_view_add_item( menu, offline_btn );
}

static void _handle_uac_btn( zdj_view_t * view, void * _event ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * status_view = (zdj_view_t*)state->data->ptr;
    zdj_usb_status_view_state_t * status_state = (zdj_usb_status_view_state_t*)status_view->state;

    // Update and restart gadget
    // zdj_usb_gadget_config_t * config = zdj_usb_current_gadget_config( );
    // config->uac2 = !config->uac2;
    // zdj_usb_reset_gadget( config );

    // // Start the transition check flow
    // status_state->transition = true;
    // status_state->transition_counter = 0;
    // strcpy( status_state->transition_title_1, "Resetting Device Mode:" );
    // strcpy( 
    //     status_state->transition_title_2, 
    //     zdj_usb_mode_name[ zdj_usb_mode_for_config( config ) ]
    // );

    // Put the UI into processing mode
    zdj_usb_status_view_build_processing_layout( status_view );
}

static void _handle_midi_btn( zdj_view_t * view, void * _event ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * status_view = (zdj_view_t*)state->data->ptr;
    zdj_usb_status_view_state_t * status_state = (zdj_usb_status_view_state_t*)status_view->state;

    // Update and restart gadget
    // zdj_usb_gadget_config_t * config = zdj_usb_current_gadget_config( );
    // config->midi = !config->midi;
    // zdj_usb_reset_gadget( config );

    // // Start the transition check flow
    // status_state->transition = true;
    // status_state->transition_counter = 0;
    // strcpy( status_state->transition_title_1, "Resetting Device Mode:" );
    // strcpy( 
    //     status_state->transition_title_2, 
    //     zdj_usb_mode_name[ zdj_usb_mode_for_config( config ) ]
    // );

    // Put the UI into processing mode
    zdj_usb_status_view_build_processing_layout( status_view );
}

static void _handle_drive_btn( zdj_view_t * view, void * _event ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * status_view = (zdj_view_t*)state->data->ptr;
    zdj_usb_status_view_state_t * status_state = (zdj_usb_status_view_state_t*)status_view->state;

    // // Update and restart gadget
    // zdj_usb_gadget_config_t * config = zdj_usb_current_gadget_config( );
    // config->mass_storage = !config->mass_storage;
    // zdj_usb_reset_gadget( config );

    // // Start the transition check flow
    // status_state->transition = true;
    // status_state->transition_counter = 0;
    // strcpy( status_state->transition_title_1, "Resetting Device Mode:" );
    // strcpy( 
    //     status_state->transition_title_2, 
    //     zdj_usb_mode_name[ zdj_usb_mode_for_config( config ) ]
    // );

    // Put the UI into processing mode
    zdj_usb_status_view_build_processing_layout( status_view );
}

static void _handle_controller_btn( zdj_view_t * view, void * _event ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * status_view = (zdj_view_t*)state->data->ptr;
    zdj_usb_status_view_state_t * status_state = (zdj_usb_status_view_state_t*)status_view->state;

    // // Update and restart gadget
    // zdj_usb_gadget_config_t * config = zdj_usb_current_gadget_config( );
    // config->hid = !config->hid;
    // zdj_usb_reset_gadget( config );

    // // Start the transition check flow
    // status_state->transition = true;
    // status_state->transition_counter = 0;
    // strcpy( status_state->transition_title_1, "Resetting Device Mode:" );
    // strcpy( 
    //     status_state->transition_title_2, 
    //     zdj_usb_mode_name[ zdj_usb_mode_for_config( config ) ]
    // );

    // Put the UI into processing mode
    zdj_usb_status_view_build_processing_layout( status_view );
}

static void _handle_shell_btn( zdj_view_t * view, void * _event ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * status_view = (zdj_view_t*)state->data->ptr;
    zdj_usb_status_view_state_t * status_state = (zdj_usb_status_view_state_t*)status_view->state;

    // // Update and restart gadget
    // zdj_usb_gadget_config_t * config = zdj_usb_current_gadget_config( );
    // config->shell = !config->shell;
    // zdj_usb_reset_gadget( config );

    // // Start the transition check flow
    // status_state->transition = true;
    // status_state->transition_counter = 0;
    // strcpy( status_state->transition_title_1, "Resetting Device Mode:" );
    // strcpy( 
    //     status_state->transition_title_2, 
    //     zdj_usb_mode_name[ zdj_usb_mode_for_config( config ) ]
    // );

    // Put the UI into processing mode
    zdj_usb_status_view_build_processing_layout( status_view );
}
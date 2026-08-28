#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/usb/zdj_usb.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/panel/zdj_ui_panel.h>
#include <zerodj/ui/panel/usb/zdj_usb_panel.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/modal_view/zdj_modal_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/progress_bar_view/zdj_progress_bar_view.h>
#include <zerodj/ui/view/usb_drive_view/zdj_usb_drive_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

zdj_usb_panel_state_t * _zdj_usb_panel_state;

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_back( zdj_view_t * menu_view );
static void _refresh_menu( zdj_view_t * view );
static void _subview_exit( void * data );

static void _add_processing_state( zdj_view_t * menu );
static void _add_offline_switch_state( zdj_view_t * menu );
static void _add_offline_body( zdj_view_t * menu );
static void _add_host_switch_state( zdj_view_t * menu );
static void _add_host_body( zdj_view_t * menu );
static void _add_gadget_switch_state( zdj_view_t * menu );
static void _add_gadget_body( zdj_view_t * menu );

static void _host_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _gadget_btn( zdj_view_t * view, zdj_control_event_t * event );

static void _uac_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _shell_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _midi_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _hid_btn( zdj_view_t * view, zdj_control_event_t * event );

static void _drive_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _drive_dialog_cb( void );
static void _offline_btn( zdj_view_t * view, zdj_control_event_t * event );

zdj_view_t * zdj_new_usb_panel( void ) {
    zdj_view_t * view = zdj_new_modal_view( zdj_modal_rect( ) );
    view->draw = &_draw;
    view->handle_control_event = &_handle_control;
    view->map = ZDJ_CONTROL_MAP_SETTINGS_PANEL;

    _zdj_usb_panel_state = calloc( 1, sizeof( zdj_usb_panel_state_t ) );
    _zdj_usb_panel_state->needs_layout_update = true;
    view->state = _zdj_usb_panel_state;

    // Make menu
    zdj_view_t * menu = zdj_new_menu_view( ZDJ_VERTICAL, zdj_modal_rect( ) );
    zdj_add_subview( view, menu );
    menu->frame.x = 0;
    menu->frame.y = 0;
    menu->frame.w = ZDJ_MODAL_WIDTH;
    menu->frame.h = ZDJ_MODAL_HEIGHT;
    _zdj_usb_panel_state->menu = menu;
    
    // Set up header
    zdj_view_t * menu_header = zdj_new_menu_header( 
        "Settings",
        " ",
        ZDJ_MENU_HEADER_STYLE_NORMAL,
        ZDJ_MENU_HEADER_BACK_STYLE_NONE
    );
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_header->state;
    header_state->handle_back = &_handle_back;
    zdj_menu_view_add_header( menu, menu_header );

    _zdj_usb_panel_state->overlay = zdj_ui_panel_new_overlay( "USB" );
    zdj_add_subview( view, _zdj_usb_panel_state->overlay );
    
    return view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // printf( "usb panel draw\n" );
    zdj_usb_panel_state_t * state = (zdj_usb_panel_state_t*)view->state;

    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );

    if( state->overlay_counter > 0 ) { state->overlay->frame.x = 0; state->overlay_counter--; }
    else { state->overlay->frame.x = 129; }

    // Exit mode switch state
    if( state->mode_switch_running && 
        zdj_usb_state->switch_data.state != ZDJ_USB_SUBMODE_SWITCH_RUNNING 
    ) {
        state->mode_switch_running = false;
        state->needs_layout_update = true;
    }

    // Update the state during switch
    if( zdj_usb_state->switch_data.has_update ) {
        state->needs_layout_update = true;
    }

    // If we're in host mode, poll the attached devices count for updates
    if( zdj_usb_state->mode_state.mode == ZDJ_USB_MODE_HOST && state->host_poll_counter-- < 0 ) {
        // printf( "host panel poll\n" );
        state->host_poll_counter = zdj_ui_msec_to_frames( 1000 );
        // if( zdj_usb_host_has_devices_update( ) ) { 
        if( zdj_usb_state->host_state.has_usb_panel_update ) { 
            zdj_usb_state->host_state.has_usb_panel_update = false;
            printf( "found updated host devices\n" );
            state->needs_layout_update = true; 
        }

    }

    if( state->needs_layout_update ) { _refresh_menu( view ); }
    // printf( "usb panel draw done\n" );
}

static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {
    // printf( "usb _handle_control\n" );
    // Ignore events which have been blocked by layers above this one.
    if( _event->blocked ) { return; }

    zdj_usb_panel_state_t * state = (zdj_usb_panel_state_t*)view->state;

    // Send events down into the subview stack
    if( _zdj_usb_panel_state->usb_drive_mode_dialog ) { 
        _zdj_usb_panel_state->usb_drive_mode_dialog->handle_control_event( 
            _zdj_usb_panel_state->usb_drive_mode_dialog, 
            _event 
        );
    } else {
        state->menu->handle_control_event( state->menu, _event );
    }

    _event->blocked = true;
}

static void _handle_back( zdj_view_t * menu_view ) {
    printf( "_handle_back\n" );
    zdj_ui_panel_toggle( );
}

static void _refresh_menu( zdj_view_t * view ) {
    // zdj_usb_panel_state_t * state = (zdj_usb_panel_state_t*)view->state;

    zdj_menu_view_remove_all_subviews( _zdj_usb_panel_state->menu );

    // If the USB system is switching modes, show the processing state.
    if( zdj_usb_state->switch_data.state == ZDJ_USB_SUBMODE_SWITCH_RUNNING ) {
        _add_processing_state( _zdj_usb_panel_state->menu );
    } else {
        // Get USB state
        zdj_usb_update_mode_from_sysfs( zdj_usb_state );
        zdj_usb_state->switch_data.has_update = false;
        switch ( zdj_usb_state->mode_state.mode ) {
            case ZDJ_USB_MODE_OFFLINE:
                _add_offline_switch_state( _zdj_usb_panel_state->menu );
                _add_offline_body( _zdj_usb_panel_state->menu );
                // zdj_usb_state->has_switch_update = false;
                break;
            case ZDJ_USB_MODE_HOST:
                _add_host_switch_state( _zdj_usb_panel_state->menu );
                _add_host_body( _zdj_usb_panel_state->menu );
                // zdj_usb_state->has_switch_update = false;
                break;
            case ZDJ_USB_MODE_GADGET:
                _add_gadget_switch_state( _zdj_usb_panel_state->menu );
                _add_gadget_body( _zdj_usb_panel_state->menu );
                // zdj_usb_state->has_switch_update = false;
                break;
            default: break;
        }
    }

    _zdj_usb_panel_state->needs_layout_update = false;
}

static void _subview_exit( void * data ) {

}

// static void _ui_btn( zdj_view_t * view, zdj_control_event_t * event ) {
//     printf( "ui_btn\n" );
//     zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
//     zdj_push_subview( panel_state->usb_panel, zdj_new_settings_ui_panel( ), true );
// }


static void _add_processing_state( zdj_view_t * menu ) {
    zdj_usb_state->switch_data.has_update = false;
    zdj_view_t * progress_bar = zdj_new_progress_bar_view( &(zdj_rect_t){ 8,9,111,6 }, ZDJ_PROGRESS_BAR_VIEW_WAIT );
    zdj_menu_view_add_item( menu, progress_bar );

    zdj_view_t * line = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_WHITE ], NULL );
    line->frame.x = 0;
    line->frame.y = 26;
    line->frame.w = 128;
    line->frame.h = 1;
    zdj_menu_view_add_item( menu, line ); 

    zdj_view_t * label_1 = zdj_new_label_view( zdj_usb_state->switch_data.switch_str_1, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    label_1->frame.y = 33;
    zdj_menu_view_add_item( menu, label_1 );
    zdj_view_t * label_2 = zdj_new_label_view( zdj_usb_state->switch_data.switch_str_2, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    label_2->frame.y = 42;
    zdj_menu_view_add_item( menu, label_2 );
}


static void _add_offline_switch_state( zdj_view_t * menu ) {
    zdj_view_t * host_btn = zdj_new_menu_item( "Host", ZDJ_MENU_ITEM_LAYOUT_BASIC_LG );
    host_btn->frame.x = 6;
    host_btn->frame.y = 5;
    host_btn->handle_control_event = &_host_btn;
    zdj_menu_item_view_state_t * host_state = (zdj_menu_item_view_state_t*)host_btn->state;
    host_state->needs_layout_init = true;
    zdj_menu_view_add_item( menu, host_btn );
    host_btn->frame.h = 15;

    zdj_view_t * mode_switch = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_SW_LG_OFF ], NULL );
    mode_switch->frame.x = 44;
    mode_switch->frame.y = 5;
    zdj_menu_view_add_item( menu, mode_switch );

    zdj_view_t * gadget_btn = zdj_new_menu_item( "Gadget", ZDJ_MENU_ITEM_LAYOUT_BASIC_LG );
    gadget_btn->frame.x = 75;
    gadget_btn->frame.y = 5;
    gadget_btn->handle_control_event = &_gadget_btn;
    zdj_menu_item_view_state_t * gadget_state = (zdj_menu_item_view_state_t*)gadget_btn->state;
    gadget_state->needs_layout_init = true;
    zdj_menu_view_add_item( menu, gadget_btn );
    gadget_btn->frame.h = 15;

    zdj_view_t * line = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_WHITE ], NULL );
    line->frame.x = 0;
    line->frame.y = 26;
    line->frame.w = 128;
    line->frame.h = 1;
    zdj_menu_view_add_item( menu, line ); 
}

static void _add_offline_body( zdj_view_t * menu ) {
    zdj_view_t * label_1 = zdj_new_label_view( "USB Offline", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    label_1->frame.x = 64 - (label_1->frame.w / 2);
    label_1->frame.y = 39;
    zdj_menu_view_add_item( menu, label_1 );
}

static void _add_host_switch_state( zdj_view_t * menu ) {
    zdj_view_t * host_btn = zdj_new_menu_item( "Host", ZDJ_MENU_ITEM_LAYOUT_BASIC_LG );
    host_btn->frame.x = 6;
    host_btn->frame.y = 5;
    zdj_menu_item_view_state_t * host_state = (zdj_menu_item_view_state_t*)host_btn->state;
    host_state->needs_layout_init = true;
    zdj_menu_view_add_item( menu, host_btn );
    host_btn->frame.h = 15;

    zdj_view_t * block = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MENU_BG ], NULL );
    block->frame.x = host_btn->frame.x;
    block->frame.y = host_btn->frame.y;
    block->frame.w = host_btn->frame.w;
    block->frame.h = host_btn->frame.h;
    zdj_menu_view_add_item( menu, block );  

    zdj_view_t * mode_switch = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_SW_LG_L ], NULL );
    mode_switch->frame.x = 44;
    mode_switch->frame.y = 5;
    zdj_menu_view_add_item( menu, mode_switch );

    zdj_view_t * gadget_btn = zdj_new_menu_item( "Gadget", ZDJ_MENU_ITEM_LAYOUT_BASIC_LG );
    gadget_btn->frame.x = 75;
    gadget_btn->frame.y = 5;
    gadget_btn->handle_control_event = &_gadget_btn;
    zdj_menu_item_view_state_t * gadget_state = (zdj_menu_item_view_state_t*)gadget_btn->state;
    gadget_state->needs_layout_init = true;
    zdj_menu_view_add_item( menu, gadget_btn );
    gadget_btn->frame.h = 15;

    zdj_view_t * line = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_WHITE ], NULL );
    line->frame.x = 0;
    line->frame.y = 26;
    line->frame.w = 128;
    line->frame.h = 1;
    zdj_menu_view_add_item( menu, line ); 
}

static void _add_host_body( zdj_view_t * menu ) {
    printf( "add host body\n" );
    
    zdj_usb_attached_devices_t * attached = &zdj_usb_state->host_state.attached;
    printf( "Attached: %p\n", attached );

    if ( attached && attached->count > 0 ) {
        // Scan for attached devices
        printf( "adding devices\n" );
        zdj_usb_device_t * device = attached->devices;
        while( device ) {
            printf( "adding device\n" );
            zdj_menu_view_add_padding( menu, 4 );
            zdj_view_t * attached_device = zdj_new_usb_device_menu_item( device );
            zdj_menu_view_add_item( menu, attached_device );
            printf( "done\n" );

            device = device->next;
        }
    } else {
        zdj_view_t * label_1 = zdj_new_label_view( "No Attached Devices", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
        label_1->frame.x = 64 - (label_1->frame.w / 2);
        label_1->frame.y = 39;
        zdj_menu_view_add_item( menu, label_1 );
    }

}

static void _add_gadget_switch_state( zdj_view_t * menu ) {
    zdj_view_t * host_btn = zdj_new_menu_item( "Host", ZDJ_MENU_ITEM_LAYOUT_BASIC_LG );
    host_btn->frame.x = 6;
    host_btn->frame.y = 5;
    host_btn->handle_control_event = &_host_btn;
    zdj_menu_item_view_state_t * host_state = (zdj_menu_item_view_state_t*)host_btn->state;
    host_state->needs_layout_init = true;
    zdj_menu_view_add_item( menu, host_btn );
    host_btn->frame.h = 15;

    zdj_view_t * mode_switch = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_SW_LG_R ], NULL );
    mode_switch->frame.x = 44;
    mode_switch->frame.y = 5;
    zdj_menu_view_add_item( menu, mode_switch );

    zdj_view_t * gadget_btn = zdj_new_menu_item( "Gadget", ZDJ_MENU_ITEM_LAYOUT_BASIC_LG );
    gadget_btn->frame.x = 75;
    gadget_btn->frame.y = 5;
    zdj_menu_item_view_state_t * gadget_state = (zdj_menu_item_view_state_t*)gadget_btn->state;
    gadget_state->needs_layout_init = true;
    zdj_menu_view_add_item( menu, gadget_btn );
    gadget_btn->frame.h = 15;

    zdj_view_t * block = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MENU_BG ], NULL );
    block->frame.x = gadget_btn->frame.x;
    block->frame.y = gadget_btn->frame.y;
    block->frame.w = gadget_btn->frame.w;
    block->frame.h = gadget_btn->frame.h;
    zdj_menu_view_add_item( menu, block );  


    zdj_view_t * line = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_WHITE ], NULL );
    line->frame.x = 0;
    line->frame.y = 26;
    line->frame.w = 128;
    line->frame.h = 1;
    zdj_menu_view_add_item( menu, line );  
}

static void _add_gadget_body( zdj_view_t * menu ) {
    zdj_view_t * uac_btn = zdj_new_menu_item( "Audio", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    uac_btn->handle_control_event = &_uac_btn;
    uac_btn->frame.y = 34;
    zdj_menu_item_view_state_t * uac_state = (zdj_menu_item_view_state_t*)uac_btn->state;
    uac_state->data.b_val = zdj_usb_state->mode_state.gadget_config.uac2;
    zdj_menu_view_add_item( menu, uac_btn );

    zdj_view_t * midi_btn = zdj_new_menu_item( "Midi", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    midi_btn->handle_control_event = &_midi_btn;
    zdj_menu_item_view_state_t * midi_state = (zdj_menu_item_view_state_t*)midi_btn->state;
    midi_state->data.b_val = zdj_usb_state->mode_state.gadget_config.midi;
    zdj_menu_view_add_item( menu, midi_btn );

    zdj_view_t * hid_btn = zdj_new_menu_item( "HID", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    hid_btn->handle_control_event = &_hid_btn;
    zdj_menu_item_view_state_t * hid_state = (zdj_menu_item_view_state_t*)hid_btn->state;
    hid_state->data.b_val = zdj_usb_state->mode_state.gadget_config.hid;
    zdj_menu_view_add_item( menu, hid_btn );



    zdj_menu_view_add_padding( menu, 5 );
    zdj_view_t * line = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_WHITE ], NULL );
    line->frame.w = 128;
    line->frame.h = 1;
    zdj_menu_view_add_item( menu, line );  
    zdj_menu_view_add_padding( menu, 5 );


    zdj_view_t * drive_btn = zdj_new_menu_item( "Drive Mode", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    drive_btn->handle_control_event = &_drive_btn;
    zdj_menu_view_add_item( menu, drive_btn );

    zdj_view_t * offline_btn = zdj_new_menu_item( "Disable USB", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    offline_btn->handle_control_event = &_offline_btn;
    zdj_menu_view_add_item( menu, offline_btn );
}

static void _host_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_usb_mode_state_t req;
    memset( &req, 0, sizeof( zdj_usb_mode_state_t ) );
    req.mode = ZDJ_USB_MODE_HOST;
    zdj_usb_enable_mode( &req );
    _zdj_usb_panel_state->mode_switch_running = true;
    _zdj_usb_panel_state->needs_layout_update = true;
}

static void _gadget_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_usb_mode_state_t req;
    memset( &req, 0, sizeof( zdj_usb_mode_state_t ) );
    req.mode = ZDJ_USB_MODE_GADGET;
    req.gadget_config.shell = true;
    zdj_usb_enable_mode( &req );
    _zdj_usb_panel_state->mode_switch_running = true;
    _zdj_usb_panel_state->needs_layout_update = true;
}

static void _uac_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_usb_mode_state_t req;
    memset( &req, 0, sizeof( zdj_usb_mode_state_t ) );
    req.mode = ZDJ_USB_MODE_GADGET;
    req.gadget_config.uac2 = !zdj_usb_state->mode_state.gadget_config.uac2;
    req.gadget_config.shell = true;
    zdj_usb_enable_mode( &req );
    _zdj_usb_panel_state->mode_switch_running = true;
    _zdj_usb_panel_state->needs_layout_update = true;
}

static void _midi_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_usb_mode_state_t req;
    memset( &req, 0, sizeof( zdj_usb_mode_state_t ) );
    req.mode = ZDJ_USB_MODE_GADGET;
    req.gadget_config.midi = !zdj_usb_state->mode_state.gadget_config.midi;
    req.gadget_config.shell = true;
    zdj_usb_enable_mode( &req );
    _zdj_usb_panel_state->mode_switch_running = true;
    _zdj_usb_panel_state->needs_layout_update = true;
}

static void _hid_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_usb_mode_state_t req;
    memset( &req, 0, sizeof( zdj_usb_mode_state_t ) );
    req.mode = ZDJ_USB_MODE_GADGET;
    req.gadget_config.hid = !zdj_usb_state->mode_state.gadget_config.hid;
    req.gadget_config.shell = true;
    zdj_usb_enable_mode( &req );
    _zdj_usb_panel_state->mode_switch_running = true;
    _zdj_usb_panel_state->needs_layout_update = true;
}


static void _drive_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_usb_mode_state_t req;
    memset( &req, 0, sizeof( zdj_usb_mode_state_t ) );
    req.mode = ZDJ_USB_MODE_GADGET;
    req.gadget_config.mass_storage = true;
    req.gadget_config.shell = true;
    zdj_usb_enable_mode( &req );
    _zdj_usb_panel_state->mode_switch_running = true;
    _zdj_usb_panel_state->needs_layout_update = true;

    // Deploy Drive mode dialog
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    _zdj_usb_panel_state->usb_drive_mode_dialog = NULL;
    _zdj_usb_panel_state->usb_drive_mode_dialog = zdj_new_usb_drive_view( 
        panel_state->usb_panel, _drive_dialog_cb 
    );
    if( _zdj_usb_panel_state->usb_drive_mode_dialog ) {
        zdj_push_subview( panel_state->usb_panel, _zdj_usb_panel_state->usb_drive_mode_dialog, true );
    }
}

static void _drive_dialog_cb( void ) {
    _zdj_usb_panel_state->usb_drive_mode_dialog = NULL;
}

static void _offline_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_usb_mode_state_t req;
    memset( &req, 0, sizeof( zdj_usb_mode_state_t ) );
    req.mode = ZDJ_USB_MODE_OFFLINE;
    zdj_usb_enable_mode( &req );
    _zdj_usb_panel_state->mode_switch_running = true;
    _zdj_usb_panel_state->needs_layout_update = true;
}
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <SDL2/SDL2_gfxPrimitives.h>


#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/zdj_view_stack.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/modal_view/zdj_modal_view.h>
#include <zerodj/ui/view/progress_bar_view/zdj_progress_bar_view.h>
#include <zerodj/ui/view/usb_drive_view/zdj_usb_drive_view.h>
#include <zerodj/system/usb/zdj_usb.h>

// FIXME:
// USB Drive View seems to fail in apps launched from CMD line.
// Launching from zerod after reboot seems to work fine.

static zdj_usb_drive_view_state_t * _zdj_usb_drive_view_state;

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _handle_control( zdj_view_t * drive_view, zdj_control_event_t * _event );
static void _handle_exit( zdj_view_t * view );
static void _handle_exit_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _deinit_state( zdj_view_t * drive_view );

static void _enable_update_layout( zdj_view_t * view );
static void _disable_update_layout( zdj_view_t * view );
static void _active_update_layout( zdj_view_t * view );
static void _error_update_layout( zdj_view_t * view );

// Swap to USB MSD gadget.
// Persist previous USB gadget state and swap back on exit.
zdj_view_t * zdj_new_usb_drive_view( void ) {
    // // Build view
    // zdj_view_t * view = zdj_new_view( zdj_modal_rect( ) );
    // view->type = ZDJ_VIEW_MODAL;
    // view->draw = &_draw;
    // view->deinit_state = &_deinit_state;
    // view->frame.x = ZDJ_MODAL_X;
    // view->frame.y = ZDJ_SCREEN_H+2;
    // view->in_anim = zdj_new_anim( ZDJ_ANIM_MODAL_SHOW );
    // view->out_anim = zdj_new_anim( ZDJ_ANIM_MODAL_HIDE );

    zdj_view_t * view = zdj_new_view( zdj_dialog_rect( ) );
    view->type = ZDJ_VIEW_DIALOG;
    view->draw = &_draw;
    view->handle_control_event = &_handle_control;
    view->deinit_state = &_deinit_state;

    view->frame.x = ZDJ_DIALOG_X;
    view->frame.y = ZDJ_SCREEN_H+2;
    
    // view->in_anim = zdj_new_anim( ZDJ_ANIM_DIALOG_SHOW );
    // view->out_anim = zdj_new_anim( ZDJ_ANIM_DIALOG_HIDE );
    zdj_set_anim( &view->in_anim, ZDJ_ANIM_DIALOG_SHOW );
    zdj_set_anim( &view->out_anim, ZDJ_ANIM_DIALOG_HIDE );

    // Make menu
    zdj_view_t * _menu = zdj_new_menu_view( ZDJ_HORIZONTAL, zdj_dialog_rect( ) );
    zdj_add_subview( view, _menu );
    _menu->frame.x = 0;
    _menu->frame.y = 0;
    _menu->frame.w = ZDJ_DIALOG_W;
    _menu->frame.h = ZDJ_DIALOG_H;
    
    // Set up header
    zdj_view_t * menu_header = zdj_new_menu_header( 
        " ",
        "Transfer Mode",
        ZDJ_MENU_HEADER_STYLE_NORMAL,
        ZDJ_MENU_HEADER_BACK_STYLE_EXIT
    );
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_header->state;
    header_state->handle_back = &_handle_exit;
    zdj_menu_view_add_header( _menu, menu_header );

    // Make view state
    _zdj_usb_drive_view_state = calloc( 1, sizeof( zdj_usb_drive_view_state_t ) );
    _zdj_usb_drive_view_state->menu_view = _menu;
    _zdj_usb_drive_view_state->mode = ZDJ_USB_DRIVE_VIEW_MODE_ENABLE;
    _zdj_usb_drive_view_state->needs_layout_update = true;
    _zdj_usb_drive_view_state->sync_counter = 0;
    view->state = _zdj_usb_drive_view_state;

    // Store previous USB gadget state for use after transfer mode exits.
    _zdj_usb_drive_view_state->prev_config.uac2 = zdj_usb_status->gadget_config.uac2;
    _zdj_usb_drive_view_state->prev_config.midi = zdj_usb_status->gadget_config.midi;
    _zdj_usb_drive_view_state->prev_config.mass_storage = zdj_usb_status->gadget_config.mass_storage;
    _zdj_usb_drive_view_state->prev_config.hid = zdj_usb_status->gadget_config.hid;
    _zdj_usb_drive_view_state->prev_config.shell = zdj_usb_status->gadget_config.shell;

    return view;
}

static void _handle_control( zdj_view_t * drive_view, zdj_control_event_t * _event ) {
    zdj_control_event_t * e = (zdj_control_event_t *)_event;
    
    // Ignore events which have been blocked by layers above this one.
    if( e->blocked ) { return; }

    // Send events down into the subview stack
    zdj_view_t * top_subview = zdj_view_stack_top_subview_of( drive_view );
    top_subview->handle_control_event( top_subview, _event );
}

static void _handle_exit( zdj_view_t * view ) {
    printf( "_drive view handle_exit\n" );
    zdj_usb_drive_view_state_t * state = (zdj_usb_drive_view_state_t *)view->state;
    // Sync the filesystem before we leave to ensure new files appear
    int fd = open("/sys/kernel/config/usb_gadget/g1/functions/mass_storage.0/lun.0/file", O_RDONLY);
    fsync( fd );
    close( fd );
    zdj_usb_stop_port_partner_poll( );
    _zdj_usb_drive_view_state->mode = ZDJ_USB_DRIVE_VIEW_MODE_DISABLE;
    _zdj_usb_drive_view_state->needs_layout_update = true;
}

static void _handle_exit_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    _handle_exit( view );
}

static void _deinit_state( zdj_view_t * drive_view ) {
    free( drive_view->state );
    drive_view->state = NULL;
}


static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_usb_drive_view_state_t * state = (zdj_usb_drive_view_state_t *)view->state;

    switch ( state->mode ) {
    case ZDJ_USB_DRIVE_VIEW_MODE_ENABLE:
        if( state->needs_layout_update ) {
            _enable_update_layout( view );
        }
        if( zdj_usb_status->has_frontend_update ) {
            zdj_usb_status->has_frontend_update = false;
            if( zdj_usb_status->switch_state == ZDJ_USB_SUBMODE_SWITCH_SUCCESS ) {
                zdj_usb_start_port_partner_poll( );
                state->mode = ZDJ_USB_DRIVE_VIEW_MODE_ACTIVE;
                state->needs_layout_update = true;
            } else if( zdj_usb_status->switch_state > ZDJ_USB_SUBMODE_SWITCH_SUCCESS ) {
                state->mode = ZDJ_USB_DRIVE_VIEW_MODE_ERROR;
                state->needs_layout_update = true;
            }
        }
        break;
    case ZDJ_USB_DRIVE_VIEW_MODE_DISABLE:
        if( zdj_usb_status->has_frontend_update ) {
            zdj_usb_status->has_frontend_update = false;
            if( zdj_usb_status->switch_state == ZDJ_USB_SUBMODE_SWITCH_SUCCESS ) {
                state->mode = ZDJ_USB_DRIVE_VIEW_MODE_EXIT;
            } else if( zdj_usb_status->switch_state > ZDJ_USB_SUBMODE_SWITCH_SUCCESS ) {
                state->mode = ZDJ_USB_DRIVE_VIEW_MODE_ERROR;
                state->needs_layout_update = true;
            }
        }
        if( state->needs_layout_update ) {
            _disable_update_layout( view );
        }
        break;
    case ZDJ_USB_DRIVE_VIEW_MODE_ACTIVE:
        // state->sync_counter++;
        // state->sync_counter %= 100;
        // if( state->sync_counter == 0 ) { sync( ); }
        if( zdj_usb_has_port_partner_update( ) ) { 
            state->needs_layout_update = true;
        }
        if( state->needs_layout_update ) {
            _active_update_layout( view );
        }
        break;
    case ZDJ_USB_DRIVE_VIEW_MODE_ERROR:
        _error_update_layout( view );
        break;
    case ZDJ_USB_DRIVE_VIEW_MODE_EXIT:
        state->mode = ZDJ_USB_DRIVE_VIEW_MODE_DONE;
        zdj_pop_subview_of( zdj_root_view( ), true );
        break;
    default:
        break;
    }
}

static void _enable_update_layout( zdj_view_t * view ) {
    printf( "_enable_update_layout\n" );
    zdj_usb_drive_view_state_t * state = (zdj_usb_drive_view_state_t *)view->state;
    zdj_view_t * menu_view = state->menu_view;

    // Start the USB gadget state transition to MSD-only
    zdj_usb_mode_request_t request;
    request.mode = ZDJ_USB_MODE_GADGET;
    request.gadget_config.uac2 = false;
    request.gadget_config.midi = false;
    request.gadget_config.mass_storage = true;
    request.gadget_config.hid = false;
    request.gadget_config.shell = true;
    request.submode = zdj_usb_submode_for_gadget_config( &request.gadget_config );
    zdj_error_type_t request_err = zdj_usb_request_mode_switch( &request );
    
    // If we're already in drive mode, just jump to the active screen.
    if( request_err == ZDJ_USB_ALREADY_IN_MODE ) {
        zdj_usb_start_port_partner_poll( );
        state->mode = ZDJ_USB_DRIVE_VIEW_MODE_ACTIVE;
        state->needs_layout_update = true;
        return;
    } else if( request_err != ZDJ_ERROR_OKAY ) {
        state->mode = ZDJ_USB_DRIVE_VIEW_MODE_ERROR;
        state->needs_layout_update = true;
    }

    zdj_menu_view_remove_all_subviews( menu_view );
    
    // Add progress view
    zdj_view_t * progress_bar = zdj_new_progress_bar_view( &(zdj_rect_t){ 7,6,86,6 }, ZDJ_PROGRESS_BAR_VIEW_WAIT );
    zdj_menu_view_add_item( menu_view, progress_bar );


    // Add processing labels
    zdj_view_t * processing = zdj_new_label_view( "Switching to Device Mode:", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    processing->frame.x = 5;
    processing->frame.y = 18;
    zdj_menu_view_add_item( menu_view, processing );
    zdj_view_t * processing_2 = zdj_new_label_view( zdj_usb_submode_name[ request.submode ], ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    processing_2->frame.x = 5;
    processing_2->frame.y = 28;
    zdj_menu_view_add_item( menu_view, processing_2 );

    state->needs_layout_update = false;
}

static void _disable_update_layout( zdj_view_t * view ) {
    printf( "_disable_update_layout\n" );
    zdj_usb_drive_view_state_t * state = (zdj_usb_drive_view_state_t *)view->state;
    zdj_view_t * menu_view = state->menu_view;

    // Return to previous gadget state
    zdj_usb_mode_request_t request;
    request.mode = ZDJ_USB_MODE_GADGET;
    request.gadget_config.uac2 = state->prev_config.uac2;
    request.gadget_config.midi = state->prev_config.midi;
    request.gadget_config.mass_storage = state->prev_config.mass_storage;
    request.gadget_config.hid = state->prev_config.hid;
    request.gadget_config.shell = state->prev_config.shell;
    request.submode = zdj_usb_submode_for_gadget_config( &request.gadget_config );
    zdj_usb_request_mode_switch( &request );

    zdj_menu_view_remove_all_subviews( menu_view );

    // Add progress view
    zdj_view_t * progress_bar = zdj_new_progress_bar_view( &(zdj_rect_t){ 7,6,86,6 }, ZDJ_PROGRESS_BAR_VIEW_WAIT );
    zdj_menu_view_add_item( menu_view, progress_bar );

    // Add processing labels
    zdj_view_t * processing = zdj_new_label_view( "Switching to Device Mode:", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    processing->frame.x = 5;
    processing->frame.y = 18;
    zdj_menu_view_add_item( menu_view, processing );
    zdj_view_t * processing_2 = zdj_new_label_view( zdj_usb_submode_name[ request.submode ], ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    processing_2->frame.x = 5;
    processing_2->frame.y = 28;
    zdj_menu_view_add_item( menu_view, processing_2 );

    state->needs_layout_update = false;
}

static void _active_update_layout( zdj_view_t * view ) {
    printf( "_active_update_layout\n" );
    zdj_usb_drive_view_state_t * state = (zdj_usb_drive_view_state_t *)view->state;
    zdj_view_t * menu_view = state->menu_view;

    zdj_menu_view_remove_all_subviews( menu_view );

    zdj_view_t * box;
    zdj_view_t * stat_label;
    if ( zdj_usb_has_port_partner( ) ) {
        // Add status box
        box = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BOX_1 ], NULL );
        box->frame.x = 6;
        box->frame.y = 4;
        zdj_menu_view_add_item( menu_view, box );

        // Add status label
        stat_label = zdj_new_label_view( "...", ZDJ_FONT_12, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
        stat_label->frame.x = 10;
        stat_label->frame.y = 2;
        zdj_menu_view_add_item( menu_view, stat_label );
    } else {
        // Add empty status box
        box = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DOTTED_BOX_1 ], NULL );
        box->frame.x = 6;
        box->frame.y = 3;
        zdj_menu_view_add_item( menu_view, box );
    }

    // Add zero icon
    zdj_view_t * zero = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_ZERO ], NULL );
    zero->frame.x = 78;
    zero->frame.y = 8;
    zdj_menu_view_add_item( menu_view, zero );

    // Add divider
    zdj_view_t * div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_NAR_H_DIV ], NULL );
    div->frame.x = 27;
    div->frame.y = 12;
    div->frame.w = 50;
    zdj_menu_view_add_item( menu_view, div );

    zdj_view_t * exit_btn = zdj_new_menu_item( "Exit", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
    zdj_menu_item_view_state_t * exit_btn_state = (zdj_menu_item_view_state_t*)exit_btn->state;
    exit_btn->handle_control_event = &_handle_exit_btn;
    exit_btn->frame.x = 38;
    exit_btn->frame.y = 24;
    exit_btn->frame.w = 24;
    // nav_up->frame.h = 10;
    zdj_menu_view_add_item( menu_view, exit_btn );

    state->needs_layout_update = false;
}

static void _error_update_layout( zdj_view_t * view ) {
    printf( "_error_update_layout\n" );
    zdj_usb_drive_view_state_t * state = (zdj_usb_drive_view_state_t *)view->state;
    zdj_view_t * menu_view = state->menu_view;

    zdj_menu_view_remove_all_items( menu_view );

    // Add zero icon
    zdj_view_t * zero = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_ZERO ], NULL );
    zero->frame.x = 14;
    zero->frame.y = 8;
    zdj_menu_view_add_item( menu_view, zero );

    // Add usb icon
    zdj_view_t * usb = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_USB ], NULL );
    usb->frame.x = 77;
    usb->frame.y = 8;
    zdj_menu_view_add_item( menu_view, usb );
    
    // Add error label
    zdj_view_t * error = zdj_new_label_view( "Error", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    error->frame.x = 91;
    error->frame.y = 11;
    zdj_menu_view_add_item( menu_view, error );

    // Add error icon
    zdj_view_t * exclaim = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_EXCLAIM_SM ], NULL );
    exclaim->frame.x = 86;
    exclaim->frame.y = 6;
    zdj_menu_view_add_item( menu_view, exclaim );

    // Add divider
    zdj_view_t * div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_NAR_H_DIV ], NULL );
    div->frame.x = 31;
    div->frame.y = 12;
    div->frame.w = 43;
    zdj_menu_view_add_item( menu_view, div );

    state->needs_layout_update = false;
}
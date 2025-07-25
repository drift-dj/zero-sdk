#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/controls/hmi/zdj_hmi.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/zdj_view_stack.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/modal_view/zdj_modal_view.h>
#include <zerodj/ui/view/progress_bar_view/zdj_progress_bar_view.h>
#include <zerodj/ui/view/usb_drive_view/zdj_usb_drive_view.h>
#include <zerodj/usb/zdj_usb.h>

static zdj_usb_drive_view_state_t * _zdj_usb_drive_view_state;

static void _zdj_usb_drive_view_draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_usb_drive_view_handle_exit( zdj_view_t * view );

static void _zdj_usb_drive_view_enable_update_layout( zdj_view_t * view );
static void _zdj_usb_drive_view_disable_update_layout( zdj_view_t * view );
static void _zdj_usb_drive_view_active_update_layout( zdj_view_t * view );
static void _zdj_usb_drive_view_error_update_layout( zdj_view_t * view );

// Swap to USB MSD gadget.
// Persist previous USB gadget state and swap back on exit.
zdj_view_t * zdj_new_usb_drive_view( void ) {
    // Build view
    zdj_view_t * view = zdj_new_modal_view( zdj_modal_rect( ) );
    view->draw = &_zdj_usb_drive_view_draw;
    view->frame->x = ZDJ_MODAL_X;
    view->frame->y = ZDJ_SCREEN_H;

    // Make menu
    zdj_view_t * _menu = zdj_new_menu_view( ZDJ_HORIZONTAL, zdj_modal_rect( ) );
    zdj_add_subview( view, _menu );
    _menu->frame->x = 0;
    _menu->frame->y = 0;
    _menu->frame->w = ZDJ_MODAL_WIDTH;
    _menu->frame->h = ZDJ_MODAL_HEIGHT;
    
    // Set up header
    zdj_view_t * menu_header = zdj_new_menu_header( 
        " ",
        "Transfer Mode",
        ZDJ_MENU_HEADER_STYLE_NORMAL,
        ZDJ_MENU_HEADER_BACK_STYLE_EXIT
    );
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_header->state;
    header_state->handle_back = &_zdj_usb_drive_view_handle_exit;
    zdj_menu_view_add_header( _menu, menu_header );

    // Make view state
    _zdj_usb_drive_view_state = calloc( 1, sizeof( zdj_usb_drive_view_state_t ) );
    _zdj_usb_drive_view_state->menu_view = _menu;
    _zdj_usb_drive_view_state->mode = ZDJ_USB_DRIVE_VIEW_MODE_ENABLE;
    _zdj_usb_drive_view_state->needs_layout_update = true;
    view->state = _zdj_usb_drive_view_state;

    // Store previous USB gadget state for use after transfer mode exits.
    _zdj_usb_drive_view_state->prev_config.uac2 = zdj_usb_status->gadget_config.uac2;
    _zdj_usb_drive_view_state->prev_config.midi = zdj_usb_status->gadget_config.midi;
    _zdj_usb_drive_view_state->prev_config.mass_storage = zdj_usb_status->gadget_config.mass_storage;
    _zdj_usb_drive_view_state->prev_config.hid = zdj_usb_status->gadget_config.hid;
    _zdj_usb_drive_view_state->prev_config.shell = zdj_usb_status->gadget_config.shell;

    return view;
}

void _zdj_usb_drive_view_handle_exit( zdj_view_t * view ) {
    printf( "_zdj_usb_drive_view_handle_exit\n" );
    // zdj_usb_drive_view_state_t * state = (zdj_usb_drive_view_state_t *)view->state;
    // Sync the filesystem before we leave to ensure new files appear
    sync( );
    zdj_usb_stop_port_partner_poll( );
    _zdj_usb_drive_view_state->mode = ZDJ_USB_DRIVE_VIEW_MODE_DISABLE;
    _zdj_usb_drive_view_state->needs_layout_update = true;
}

static void _zdj_usb_drive_view_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_usb_drive_view_state_t * state = (zdj_usb_drive_view_state_t *)view->state;

    switch ( state->mode ) {
    case ZDJ_USB_DRIVE_VIEW_MODE_ENABLE:
        if( state->needs_layout_update ) {
            _zdj_usb_drive_view_enable_update_layout( view );
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
            _zdj_usb_drive_view_disable_update_layout( view );
        }
        break;
    case ZDJ_USB_DRIVE_VIEW_MODE_ACTIVE:
        if( zdj_usb_has_port_partner_update( ) ) { 
            state->needs_layout_update = true;
        }
        if( state->needs_layout_update ) {
            _zdj_usb_drive_view_active_update_layout( view );
        }
        break;
    case ZDJ_USB_DRIVE_VIEW_MODE_ERROR:
        _zdj_usb_drive_view_error_update_layout( view );
        break;
    case ZDJ_USB_DRIVE_VIEW_MODE_EXIT:
        zdj_pop_subview_of( zdj_root_view( ), true );
        state->mode = ZDJ_USB_DRIVE_VIEW_MODE_DONE;
        break;
    default:
        break;
    }

}

static void _zdj_usb_drive_view_enable_update_layout( zdj_view_t * view ) {
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
    if( zdj_usb_request_mode_switch( &request ) != ZDJ_ERROR_OKAY ) {
        state->mode = ZDJ_USB_DRIVE_VIEW_MODE_ERROR;
        state->needs_layout_update = true;
    }

    zdj_menu_view_remove_all_items( menu_view );
    
    // Add progress view
    zdj_view_t * progress_bar = zdj_new_progress_bar_view( &(zdj_rect_t){ 11,8,96,6 }, ZDJ_PROGRESS_BAR_VIEW_WAIT );
    zdj_menu_view_add_item( menu_view, progress_bar );


    // Add processing labels
    zdj_view_t * processing = zdj_new_label_view( "Switching to Device Mode:", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    processing->frame->x = 9;
    processing->frame->y = 18;
    zdj_menu_view_add_item( menu_view, processing );
    zdj_view_t * processing_2 = zdj_new_label_view( zdj_usb_submode_name[ request.submode ], ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    processing_2->frame->x = 9;
    processing_2->frame->y = 28;
    zdj_menu_view_add_item( menu_view, processing_2 );

    state->needs_layout_update = false;
}

static void _zdj_usb_drive_view_disable_update_layout( zdj_view_t * view ) {
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

    zdj_menu_view_remove_all_items( menu_view );

    // Add progress view
    zdj_view_t * progress_bar = zdj_new_progress_bar_view( &(zdj_rect_t){ 11,8,96,6 }, ZDJ_PROGRESS_BAR_VIEW_WAIT );
    zdj_menu_view_add_item( menu_view, progress_bar );

    // Add processing labels
    zdj_view_t * processing = zdj_new_label_view( "Switching to Device Mode:", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    processing->frame->x = 9;
    processing->frame->y = 18;
    zdj_menu_view_add_item( menu_view, processing );
    zdj_view_t * processing_2 = zdj_new_label_view( zdj_usb_submode_name[ request.submode ], ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    processing_2->frame->x = 9;
    processing_2->frame->y = 28;
    zdj_menu_view_add_item( menu_view, processing_2 );

    state->needs_layout_update = false;
}

static void _zdj_usb_drive_view_active_update_layout( zdj_view_t * view ) {
    zdj_usb_drive_view_state_t * state = (zdj_usb_drive_view_state_t *)view->state;
    zdj_view_t * menu_view = state->menu_view;

    zdj_menu_view_remove_all_items( menu_view );

    zdj_view_t * box;
    zdj_view_t * stat_label;
    if ( zdj_usb_has_port_partner( ) ) {
        // Add status box
        box = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BOX_1 ], NULL );
        box->frame->x = 16;
        box->frame->y = 3;
        zdj_menu_view_add_item( menu_view, box );

        // Add status label
        stat_label = zdj_new_label_view( "...", ZDJ_FONT_12, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
        stat_label->frame->x = 20;
        stat_label->frame->y = 2;
        zdj_menu_view_add_item( menu_view, stat_label );
    } else {
        // Add empty status box
        box = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DOTTED_BOX_1 ], NULL );
        box->frame->x = 16;
        box->frame->y = 3;
        zdj_menu_view_add_item( menu_view, box );
    }

    // Add zero icon
    zdj_view_t * zero = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_ZERO ], NULL );
    zero->frame->x = 88;
    zero->frame->y = 8;
    zdj_menu_view_add_item( menu_view, zero );

    // Add divider
    zdj_view_t * div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_NAR_H_DIV ], NULL );
    div->frame->x = 37;
    div->frame->y = 12;
    div->frame->w = 50;
    zdj_menu_view_add_item( menu_view, div );

    state->needs_layout_update = false;
}

static void _zdj_usb_drive_view_error_update_layout( zdj_view_t * view ) {
    zdj_usb_drive_view_state_t * state = (zdj_usb_drive_view_state_t *)view->state;
    zdj_view_t * menu_view = state->menu_view;

    zdj_menu_view_remove_all_items( menu_view );

    // Add zero icon
    zdj_view_t * zero = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_ZERO ], NULL );
    zero->frame->x = 14;
    zero->frame->y = 8;
    zdj_menu_view_add_item( menu_view, zero );

    // Add usb icon
    zdj_view_t * usb = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_USB ], NULL );
    usb->frame->x = 77;
    usb->frame->y = 8;
    zdj_menu_view_add_item( menu_view, usb );
    
    // Add error label
    zdj_view_t * error = zdj_new_label_view( "Error", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    error->frame->x = 91;
    error->frame->y = 11;
    zdj_menu_view_add_item( menu_view, error );

    // Add error icon
    zdj_view_t * exclaim = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_EXCLAIM_SM ], NULL );
    exclaim->frame->x = 86;
    exclaim->frame->y = 6;
    zdj_menu_view_add_item( menu_view, exclaim );

    // Add divider
    zdj_view_t * div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_NAR_H_DIV ], NULL );
    div->frame->x = 31;
    div->frame->y = 12;
    div->frame->w = 43;
    zdj_menu_view_add_item( menu_view, div );

    state->needs_layout_update = false;
}
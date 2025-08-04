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

void _zdj_usb_status_view_reboot_update_layout( zdj_view_t * view );

void zdj_usb_status_view_reboot_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)view->state;

    if( state->needs_layout_update ) { _zdj_usb_status_view_reboot_update_layout( view ); }
}

void _zdj_usb_status_view_reboot_update_layout( zdj_view_t * view ) {
    printf( "zdj_usb_status_view_build_system_reboot_layout\n" );
    zdj_usb_status_view_state_t * state = (zdj_usb_status_view_state_t*)view->state;
    zdj_view_t * menu = state->menu_view;
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu_view->state;

    // Clear the old menu.
    zdj_menu_view_remove_all_items( state->menu_view );

    // Set header title
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_state->header_view->state;
    header_state->name = "USB Mode";
    header_state->title = "Reboot Required";
    header_state->has_valid_display = false;

    // Add mode requested label
    zdj_view_t * mode_label = zdj_new_label_view( "Device Mode Requested.", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    mode_label->frame->x = 16;
    mode_label->frame->y = 5;
    zdj_menu_view_add_item( menu, mode_label );

    // Add zero icon
    zdj_view_t * zero = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_ZERO_REBOOT ], NULL );
    zero->frame->x = 15;
    zero->frame->y = 22;
    zdj_menu_view_add_item( menu, zero );

    // Add divider
    zdj_view_t * div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_NAR_H_DIV ], NULL );
    div->frame->x = 34;
    div->frame->y = 28;
    div->frame->w = 20;
    zdj_menu_view_add_item( menu, div );

    // Add box
    zdj_view_t * box_l = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_TEXT_INPUT_CURSOR ], NULL );
    box_l->frame->x = 56;
    box_l->frame->y = 18;
    box_l->frame->w = 2;
    zdj_menu_view_add_item( menu, box_l );
    zdj_view_t * box_r = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_TEXT_INPUT_CURSOR_R ], NULL );
    box_r->frame->x = 102;
    box_r->frame->y = 18;
    box_r->frame->w = 2;
    zdj_menu_view_add_item( menu, box_r );

    // Add reboot labels
    zdj_view_t * reboot_1_label = zdj_new_label_view( "Reboot Zero", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    reboot_1_label->frame->x = 60;
    reboot_1_label->frame->y = 20;
    zdj_menu_view_add_item( menu, reboot_1_label );
    zdj_view_t * reboot_2_label = zdj_new_label_view( "to Activate.", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    reboot_2_label->frame->x = 60;
    reboot_2_label->frame->y = 28;
    zdj_menu_view_add_item( menu, reboot_2_label );

    state->needs_layout_update = false;
}
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/panel/zdj_ui_panel.h>
#include <zerodj/ui/panel/settings/zdj_settings_panel.h>
#include <zerodj/ui/panel/settings/developer/zdj_settings_developer_panel.h>
#include <zerodj/ui/panel/settings/software/zdj_settings_software_panel.h>
#include <zerodj/ui/panel/settings/ui/zdj_settings_ui_panel.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/menu_section_view/zdj_menu_section_view.h>
#include <zerodj/ui/view/modal_view/zdj_modal_view.h>
#include <zerodj/ui/view/usb_status_view/zdj_usb_status_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

zdj_settings_panel_state_t * _zdj_settings_panel_state;

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_back( zdj_view_t * menu_view );
static void _refresh_menu( zdj_view_t * view );

static void _subview_exit( void * data );

static void _ui_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _software_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _developer_btn( zdj_view_t * view, zdj_control_event_t * event );

zdj_view_t * zdj_new_settings_panel( void ) {
    zdj_view_t * view = zdj_new_modal_view( zdj_modal_rect( ) );
    view->draw = &_draw;
    view->handle_control_event = &_handle_control;
    view->map = ZDJ_CONTROL_MAP_SETTINGS_PANEL;

    _zdj_settings_panel_state = calloc( 1, sizeof( zdj_settings_panel_state_t ) );
    _zdj_settings_panel_state->needs_layout_update = true;
    _zdj_settings_panel_state->event_target = NULL;
    view->state = _zdj_settings_panel_state;

    // Make menu
    zdj_view_t * menu = zdj_new_menu_view( ZDJ_VERTICAL, zdj_modal_rect( ) );
    zdj_add_subview( view, menu );
    menu->frame.x = 0;
    menu->frame.y = 0;
    menu->frame.w = ZDJ_MODAL_WIDTH;
    menu->frame.h = ZDJ_MODAL_HEIGHT;
    _zdj_settings_panel_state->menu = menu;

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

    zdj_view_t * ui_btn = zdj_new_menu_item( "UI", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    ui_btn->handle_control_event = _ui_btn;
    zdj_menu_item_view_state_t * ui_state = (zdj_menu_item_view_state_t*)ui_btn->state;
    ui_state->data.ptr = view;
    zdj_menu_view_add_item( menu, ui_btn );

    zdj_view_t * software_btn = zdj_new_menu_item( "System", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    software_btn->handle_control_event = _software_btn;
    zdj_menu_item_view_state_t * software_state = (zdj_menu_item_view_state_t*)software_btn->state;
    software_state->data.ptr = view;
    zdj_menu_view_add_item( menu, software_btn );

    zdj_view_t * developer_btn = zdj_new_menu_item( "Developer", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    developer_btn->handle_control_event = _developer_btn;
    zdj_menu_item_view_state_t * developer_state = (zdj_menu_item_view_state_t*)developer_btn->state;
    developer_state->data.ptr = view;
    zdj_menu_view_add_item( menu, developer_btn );

    _zdj_settings_panel_state->overlay = zdj_ui_panel_new_overlay( "Settings" );
    zdj_add_subview( view, _zdj_settings_panel_state->overlay );
    
    return view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_settings_panel_state_t * state = (zdj_settings_panel_state_t*)view->state;

    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );

    if( state->overlay_counter > 0 ) { state->overlay->frame.x = 0; state->overlay_counter--; }
    else { state->overlay->frame.x = 129; }

    // if( state->needs_layout_update ) { _refresh_menu( view ); }
}

static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {
    // printf( "settings _handle_control\n" );
    // Ignore events which have been blocked by layers above this one.
    if( _event->blocked ) { return; }

    zdj_settings_panel_state_t * state = (zdj_settings_panel_state_t*) view->state;

    // Send events down into the top subview
    zdj_view_t * subview = zdj_view_stack_top_subview_of( view );
    // Avoid the overlay
    if( subview->type == ZDJ_VIEW_OVERLAY ) { subview = subview->prev; }
    subview->handle_control_event( subview, _event );

    _event->blocked = true;
}

static void _handle_back( zdj_view_t * menu_view ) {
    // printf( "_handle_back\n" );
    zdj_ui_panel_toggle( );
}

static void _subview_exit( void * data ) {
    _zdj_settings_panel_state->event_target = NULL;
}

// static void _refresh_menu( zdj_view_t * view ) {
//     zdj_settings_panel_state_t * state = (zdj_settings_panel_state_t*)view->state;

//     zdj_menu_view_remove_all_subviews( state->menu );

//     zdj_view_t * ui_btn = zdj_new_menu_item( "UI", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
//     ui_btn->handle_control_event = _ui_btn;
//     zdj_menu_item_view_state_t * ui_state = (zdj_menu_item_view_state_t*)ui_btn->state;
//     ui_state->data.ptr = view;
//     zdj_menu_view_add_item( state->menu, ui_btn );

//     zdj_view_t * software_btn = zdj_new_menu_item( "Software", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
//     software_btn->handle_control_event = _software_btn;
//     zdj_menu_item_view_state_t * software_state = (zdj_menu_item_view_state_t*)software_btn->state;
//     software_state->data.ptr = view;
//     zdj_menu_view_add_item( state->menu, software_btn );

//     zdj_view_t * developer_btn = zdj_new_menu_item( "Developer", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
//     developer_btn->handle_control_event = _developer_btn;
//     zdj_menu_item_view_state_t * developer_state = (zdj_menu_item_view_state_t*)developer_btn->state;
//     developer_state->data.ptr = view;
//     zdj_menu_view_add_item( state->menu, developer_btn );
// }

static void _ui_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    printf( "ui_btn\n" );
    zdj_menu_item_view_state_t * btn_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * settings_panel = (zdj_view_t *)btn_state->data.ptr;
    zdj_settings_panel_state_t * settings_panel_state = (zdj_settings_panel_state_t*)settings_panel->state;
    zdj_view_t * ui_panel = zdj_new_settings_ui_panel( &_subview_exit );
    settings_panel_state->event_target = ui_panel;
    zdj_push_subview( settings_panel, ui_panel, true );
}

static void _software_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_menu_item_view_state_t * btn_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * settings_panel = (zdj_view_t *)btn_state->data.ptr;
    zdj_settings_panel_state_t * settings_panel_state = (zdj_settings_panel_state_t*)settings_panel->state;
    zdj_view_t * software_panel = zdj_new_settings_software_panel( &_subview_exit );
    settings_panel_state->event_target = software_panel;
    zdj_push_subview( settings_panel, software_panel, true );
}

static void _developer_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_menu_item_view_state_t * btn_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * settings_panel = (zdj_view_t *)btn_state->data.ptr;
    zdj_settings_panel_state_t * settings_panel_state = (zdj_settings_panel_state_t*)settings_panel->state;
    zdj_view_t * developer_panel = zdj_new_settings_developer_panel( &_subview_exit );
    settings_panel_state->event_target = developer_panel;
    zdj_push_subview( settings_panel, developer_panel, true );
}
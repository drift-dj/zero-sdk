#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
// #include <zerodj/ui/anim/zdj_anim.h>
// #include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/panel/widget_controls/zdj_widget_controls_panel.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/menu_section_view/zdj_menu_section_view.h>
#include <zerodj/ui/view/modal_view/zdj_modal_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_back( zdj_view_t * menu_view );
static void _refresh_menu( zdj_view_t * view );

zdj_view_t * zdj_new_widget_controls_panel( void ) {
    zdj_view_t * view = zdj_new_modal_view( zdj_modal_rect( ) );
    view->draw = &_draw;
    view->handle_control_event = &_handle_control;
    view->map = ZDJ_CONTROL_MAP_WIDGET_PANEL;

    zdj_widget_controls_panel_state_t * state = calloc( 1, sizeof( zdj_widget_controls_panel_state_t ) );
    state->view_needs_refresh = true;
    view->state = state;

    // Make menu
    zdj_view_t * menu = zdj_new_menu_view( ZDJ_VERTICAL, zdj_modal_rect( ) );
    zdj_add_subview( view, menu );
    menu->frame.x = 0;
    menu->frame.y = 0;
    menu->frame.w = ZDJ_MODAL_WIDTH;
    menu->frame.h = ZDJ_MODAL_HEIGHT;
    state->menu = menu;
    
    // Set up header
    zdj_view_t * menu_header = zdj_new_menu_header( 
        "Widget Controls",
        " ",
        ZDJ_MENU_HEADER_STYLE_NORMAL,
        ZDJ_MENU_HEADER_BACK_STYLE_EXIT
    );
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_header->state;
    header_state->handle_back = &_handle_back;
    zdj_menu_view_add_header( menu, menu_header );
    
    return view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_widget_controls_panel_state_t * state = (zdj_widget_controls_panel_state_t*)view->state;

    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );

    if( state->view_needs_refresh ) { _refresh_menu( view ); }
}

static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {

}

static void _handle_back( zdj_view_t * menu_view ) {

}

static void _refresh_menu( zdj_view_t * view ) {
    zdj_widget_controls_panel_state_t * state = (zdj_widget_controls_panel_state_t*)view->state;

    zdj_menu_view_remove_all_subviews( state->menu );

    zdj_menu_view_add_section( state->menu, zdj_new_menu_section( "Volume" ) );
    zdj_menu_view_add_item( state->menu, zdj_new_menu_item( "Slowest", ZDJ_MENU_ITEM_LAYOUT_BASIC_R ) );
    zdj_menu_view_add_section( state->menu, zdj_new_menu_section( "Recording" ) );
    zdj_menu_view_add_item( state->menu, zdj_new_menu_item( "Slowest", ZDJ_MENU_ITEM_LAYOUT_BASIC_R ) );
}
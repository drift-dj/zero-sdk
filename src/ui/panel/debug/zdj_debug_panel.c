#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/perf/zdj_perf.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/panel/debug/zdj_debug_panel.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
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

static void _add_perf_section( zdj_view_t * view );
static void _update_perf_report( zdj_debug_panel_state_t * state );

static void _anim_in_cb( zdj_view_t * superview, zdj_view_t * view );
static void _anim_out_cb( zdj_view_t * superview, zdj_view_t * view );

zdj_view_t * zdj_new_debug_panel( void ) {
    zdj_perf_init( 3000 );

    zdj_view_t * view = zdj_new_modal_view( zdj_modal_rect( ) );
    view->draw = &_draw;
    view->handle_control_event = &_handle_control;
    view->in_anim.cb_fn = &_anim_in_cb;
    view->out_anim.cb_fn = &_anim_out_cb;
    view->map = ZDJ_CONTROL_MAP_DEBUG_PANEL;

    zdj_debug_panel_state_t * state = calloc( 1, sizeof( zdj_debug_panel_state_t ) );
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
        "Debug",
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
    zdj_debug_panel_state_t * state = (zdj_debug_panel_state_t*)view->state;

    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );

    if( state->view_needs_refresh ) { _refresh_menu( view ); }
}

static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {

}

static void _handle_back( zdj_view_t * menu_view ) {

}

static void _refresh_menu( zdj_view_t * view ) {
    zdj_debug_panel_state_t * state = (zdj_debug_panel_state_t*)view->state;

    zdj_menu_view_remove_all_subviews( state->menu );

    // zdj_menu_view_add_section( state->menu, zdj_new_menu_section( "Fast Audio Thread" ) );
    // zdj_menu_view_add_item( state->menu, zdj_new_menu_item( "Slowest", ZDJ_MENU_ITEM_LAYOUT_BASIC_R ) );
    // zdj_menu_view_add_item( state->menu, zdj_new_menu_item( "Normal", ZDJ_MENU_ITEM_LAYOUT_BASIC_R ) );
    // zdj_menu_view_add_item( state->menu, zdj_new_menu_item( "Fastes", ZDJ_MENU_ITEM_LAYOUT_BASIC_R ) );
    
    _add_perf_section( view );
}

static void _add_perf_section( zdj_view_t * view ) {
    zdj_debug_panel_state_t * state = (zdj_debug_panel_state_t*)view->state;
    
    if( state->perf_counter++ > 100 ) {
        state->perf_counter = 0;
        _update_perf_report( state );
    }

    zdj_menu_view_add_section( state->menu, zdj_new_menu_section( "Perf" ) );
    zdj_menu_view_add_item( state->menu, zdj_new_menu_item( state->line_str, ZDJ_MENU_ITEM_LAYOUT_INERT ) );
}

static void _update_perf_report( zdj_debug_panel_state_t * state ) {
    state->report = zdj_perf_make_cycle_timing_report( );
    zdj_reset_perf( );

    double cad_msec;
    double cad_hz;
    double dur_msec;
    char str[ 64 ];
    
    state->line = zdj_perf_report_line_for_name( state->report, ZDJ_PERF_TAG_DECK_MOVE );
    cad_msec = (double)state->line->avg_cadence / 1000000.0;
    cad_hz = 1000.0 / cad_msec;
    dur_msec = (double)state->line->avg_dur / 1000000.0;
    snprintf( state->line_str, sizeof( state->line_str ), "%1.1fmS/%1.0fHz  %1.1fmS", cad_msec, cad_hz, dur_msec );
}

static void _anim_in_cb( zdj_view_t * superview, zdj_view_t * view ) {
    // printf( "debug panel in cb\n" );
    zdj_reset_perf( );
    zdj_enable_perf( );
}

static void _anim_out_cb( zdj_view_t * superview, zdj_view_t * view ) {
    // printf( "debug panel out cb\n" );
    zdj_disable_perf( );
}
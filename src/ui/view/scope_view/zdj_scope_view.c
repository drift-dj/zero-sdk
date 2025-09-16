#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <math.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/modal_view/zdj_modal_view.h>
#include <zerodj/ui/view/scope_view/zdj_scope_view.h>
#include <zerodj/ui/view/text_input_view/zdj_text_input_view.h>
#include <zerodj/ui/view/waveform_view/zdj_waveform_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _zdj_scope_view_draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_scope_view_update_layout( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_souncard_view_handle_control( zdj_view_t * view, zdj_control_event_t * _event );
static void _zdj_scope_view_handle_back( zdj_view_t * scope_view );
static void _zdj_scope_view_deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_scope_view( zdj_soundcard_t * soundcard, zdj_soundcard_node_name_t node_name ) {
    zdj_view_t * scope_view = zdj_new_modal_view( zdj_modal_rect( ) );
    scope_view->draw = &_zdj_scope_view_draw;
    // scope_view->handle_control_event = &_zdj_souncard_view_handle_control;
    scope_view->deinit_state = &_zdj_scope_view_deinit_state;

    // Add a state instance
    zdj_scope_view_state_t * state = calloc( 1, sizeof( zdj_scope_view_state_t ) );
    state->node_name = node_name;
    state->needs_layout_update = true;
    state->soundcard = soundcard;
    scope_view->state = state;

    // Make menu
    zdj_view_t * menu = zdj_new_menu_view( ZDJ_VERTICAL, zdj_modal_rect( ) );
    zdj_add_subview( scope_view, menu );
    menu->frame.x = 0;
    menu->frame.y = 0;
    menu->frame.w = ZDJ_MODAL_WIDTH;
    menu->frame.h = ZDJ_MODAL_HEIGHT;
    state->menu = menu;
    
    // Set up header
    zdj_view_t * menu_header = zdj_new_menu_header( 
        "Signal Scope",
        zdj_soundcard_node_name[ node_name ],
        ZDJ_MENU_HEADER_STYLE_NORMAL,
        ZDJ_MENU_HEADER_BACK_STYLE_EXIT
    );
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_header->state;
    header_state->handle_back = &_zdj_scope_view_handle_back;
    zdj_menu_view_add_header( menu, menu_header );

    return scope_view;
}

void _zdj_souncard_view_handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_control_event_t * e = (zdj_control_event_t *)_event;
    zdj_scope_view_state_t * state = (zdj_scope_view_state_t*)view->state;
    
    // Ignore events which have been blocked by layers above this one.
    if( e->blocked ) { return; }

    // Grab Tone 1,2,3 + Jog push turn to send controls into channels.
    if( e->id == ZDJ_UI_CONTROL_JOG_ADJUST_0 ||
        e->id == ZDJ_UI_CONTROL_TONE_1_ADJUST_0 ||
        e->id == ZDJ_UI_CONTROL_TONE_1_RELEASE_0 ||
        e->id == ZDJ_UI_CONTROL_TONE_2_ADJUST_0
    ) {
        // Get current menu scroll index
        zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu->state;
        zdj_view_t * meter = zdj_menu_view_item_at_current_scroll_index( state->menu );
        
        // Send event into meter
        meter->handle_control_event( meter, _event );
    } else {
        // Send remaining events down into the menu
        state->menu->handle_control_event( state->menu, _event );
    }   
}

void _zdj_scope_view_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_scope_view_state_t * state = (zdj_scope_view_state_t*)view->state;

    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );

    if( state->needs_layout_update ) {
        _zdj_scope_view_update_layout( view, clip );
    }
}


void _zdj_scope_view_update_layout( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_scope_view_state_t * state = (zdj_scope_view_state_t*)view->state;
    zdj_view_t * menu_view = state->menu;
    zdj_soundcard_t * soundcard = state->soundcard;

    state->needs_layout_update = false;

    zdj_menu_view_remove_all_items( menu_view );

    zdj_menu_view_add_padding( menu_view, 2 );
    // Add Node selector.
    zdj_view_t * node_select = zdj_new_menu_item( 
        "Zoom", 
        ZDJ_MENU_ITEM_LAYOUT_DATA_R 
    );
    zdj_menu_view_add_item( menu_view, node_select );

    // Add waveform frame
    zdj_view_t * frame_l = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_TEXT_INPUT_CURSOR ], NULL );
    frame_l->frame.x = 3;
    frame_l->frame.y = 28;
    frame_l->frame.w = 3;
    zdj_add_subview( menu_view, frame_l );
    zdj_view_t * frame_r = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_TEXT_INPUT_CURSOR_R ], NULL );
    frame_r->frame.x = view->frame.w - 4;
    frame_r->frame.y = 28;
    zdj_add_subview( menu_view, frame_r );


    // Add waveform view based on current node.
    state->waveform = zdj_new_live_waveform_view( 
        &(zdj_rect_t){6,15,ZDJ_MODAL_WIDTH-9,30}, 
        zdj_soundcard_get_node_for_name( soundcard, state->node_name )
    );
    zdj_menu_view_add_item( menu_view, state->waveform );
}

void _zdj_scope_view_handle_back( zdj_view_t * scope_view ) {
    zdj_pop_subview_of( zdj_root_view( ), true );
}

void _zdj_scope_view_deinit_state( zdj_view_t * view ) {

}
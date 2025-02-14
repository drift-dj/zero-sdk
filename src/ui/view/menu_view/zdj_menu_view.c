#include <stdio.h>
#include <string.h>

#include <zerodj/hmi/zdj_hmi.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/scroll_view/zdj_scroll_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

void _zdj_menu_handle_hmi( zdj_view_t * view, void * _event );

zdj_view_t * zdj_new_menu_view( zdj_menu_linkage_t * linkage, zdj_ui_orient_t scroll_dir, zdj_rect_t * frame ) {
    zdj_view_t * menu_view = zdj_new_view( frame );
    if( linkage && linkage->draw ) {
        menu_view->draw = linkage->draw;
    }
    if( linkage && linkage->handle_hmi_event ) {
        menu_view->handle_hmi_event = linkage->handle_hmi_event;
    } else {
        menu_view->handle_hmi_event = _zdj_menu_handle_hmi;
    }
    if( linkage && linkage->update_data ) {
        menu_view->update_data = linkage->update_data;
    }

    // Add a scroll_view
    zdj_view_t * menu_scroll_view = zdj_new_scroll_view( frame );
    zdj_add_subview( menu_view, menu_scroll_view );

    // Add a state instance
    zdj_menu_view_state_t * state = calloc( 1, sizeof( zdj_menu_view_state_t ) );
    state->scroll_view = menu_scroll_view;
    menu_view->state = state;

    return menu_view;
}

void zdj_menu_view_add_section( zdj_view_t * menu_view, zdj_view_t * section ) {
    if( !section ) { return; }

    zdj_menu_view_state_t * state = (zdj_menu_view_state_t*)menu_view->state;
    // Align new item to menu's scroll orientation
    zdj_point_t scroll_size;
    zdj_scroll_view_get_size( state->scroll_view, &scroll_size );
    section->frame->y = scroll_size.y;
    section->frame->x = scroll_size.x;
    zdj_scroll_view_add_subview( state->scroll_view, section );
}

void zdj_menu_view_add_item( zdj_view_t * menu_view, zdj_view_t * item ) {
    if( !item ) { return; }

    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)menu_view->state;
    // Align new item to menu's scroll orientation
    zdj_point_t scroll_size;
    zdj_scroll_view_get_size( menu_state->scroll_view, &scroll_size );
    item->frame->y = scroll_size.y;
    item->frame->x = scroll_size.x;
    zdj_scroll_view_add_subview( menu_state->scroll_view, item );

    // Set scroll_index for new item
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;
    item_state->scroll_index = menu_state->item_count++;
}

void _zdj_menu_handle_hmi( zdj_view_t * view, void * _event ) {
    zdj_hmi_event_t * e = (zdj_hmi_event_t *)_event;
    // Prevent view stack from sending events to subviews.
    // Any events will be passed to subviews from this func.
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)view->state;

    // Handle scroll
    zdj_scroll_view_state_t * scroll_state = (zdj_scroll_view_state_t*)menu_state->scroll_view->state;
    if( e->id == ZDJ_HMI_ENCO_2_JOG && e->type == ZDJ_HMI_EVENT_ADJUST ) { 
        menu_state->scroll_index += e->i_val;
        zdj_view_t * menu_item = zdj_menu_item_for_scroll_index( 
            menu_state->scroll_view, 
            menu_state->scroll_index 
        );
        zdj_point_t scroll_point;
        if( menu_state->scroll_dir == ZDJ_VERTICAL ) {
            scroll_point.x = 0;
            scroll_point.y = menu_item->frame->y;
        } else if( menu_state->scroll_dir == ZDJ_HORIZONTAL ) {
            scroll_point.x = menu_item->frame->x;
            scroll_point.y = 0;
        }
        zdj_scroll_view_to_point( menu_state->scroll_view, &scroll_point );
    }

    // Handle mod scroll - pass down to item_view
    // Handle press
    // Handle long-press
}
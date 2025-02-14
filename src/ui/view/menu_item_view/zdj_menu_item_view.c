#include <stdio.h>
#include <string.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

void _zdj_menu_item_static_draw( zdj_view_t * view, zdj_view_clip_t * clip );
void _zdj_menu_item_static_hmi_event( zdj_view_t * view, void * _event );

void _zdj_menu_item_static_hmi_open_view( zdj_view_t * view, void * _event );
void _zdj_menu_item_static_hmi_open_menu( zdj_view_t * view, void * _event );
void _zdj_menu_item_static_hmi_toggle( zdj_view_t * view, void * _event );
void _zdj_menu_item_static_hmi_input( zdj_view_t * view, void * _event );

// Parse a static XML node into a menu_item_view.
zdj_view_t * zdj_new_menu_item( char * title, zdj_menu_linkage_t * linkage ) {
    // Make view
    zdj_view_t * menu_item = zdj_new_view( NULL );
    menu_item->type = ZDJ_VIEW_MENU_ITEM;

    // Build state
    zdj_menu_item_view_state_t * state = calloc( 1, sizeof( zdj_menu_item_view_state_t ) );
    menu_item->state = (void*)state;
    state->data = calloc( 1, sizeof( zdj_ui_data_t ) );

    // Use dynamic linkage if available for draw/hmi_events/data_update.
    if( linkage && linkage->draw ) {
        menu_item->draw = linkage->draw;
    } else {
        menu_item->draw = &_zdj_menu_item_static_draw;
    }
    if( linkage && linkage->handle_hmi_event ) {
        menu_item->handle_hmi_event = linkage->handle_hmi_event;
    } else {
        menu_item->handle_hmi_event = &_zdj_menu_item_static_hmi_event;
    }
    if( linkage && linkage->update_data ) {
        menu_item->update_data = linkage->update_data;
    }

    

    // Build ticker views


    // Setup blink state


    return menu_item;
}

// March through a view's subviews, looking for a menu_item_view
// with a matching scroll_index.
zdj_view_t * zdj_menu_item_for_scroll_index( zdj_view_t * view, int index ) {
    zdj_view_t * subview = view->subviews;
    while( subview ) {
        if( subview->type == ZDJ_VIEW_MENU_ITEM ) { 
            // Only interested in menu_items
            zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)subview->state;
            if( state->scroll_index == index ) {
                // Exit the loop if we find our view
                return subview;
            }
        }
        subview = subview->next;
    }
    return NULL;
}

// Get a matching draw function for a menu_item node's layout attribute
void _zdj_menu_item_static_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    switch ( state->static_layout ) {
        case ZDJ_MENU_ITEM_LAYOUT_BASIC_R:
        case ZDJ_MENU_ITEM_LAYOUT_BASIC_L:
            zdj_menu_item_static_draw_simple_l( view, clip );
            break;
        default:
            break;
    }
}

void _zdj_menu_item_static_hmi_event( zdj_view_t * view, void * _event ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    switch ( state->static_action ) {
        case ZDJ_MENU_ITEM_ACTION_VIEW:
            _zdj_menu_item_static_hmi_open_view( view, _event );
            break;
        case ZDJ_MENU_ITEM_ACTION_MENU:
            _zdj_menu_item_static_hmi_open_menu( view, _event );
            break;
        default:
            break;
    }
}

void _zdj_menu_item_static_hmi_open_view( zdj_view_t * view, void * _event ) {

}

void _zdj_menu_item_static_hmi_open_menu( zdj_view_t * view, void * _event ) {

}

void _zdj_menu_item_static_hmi_toggle( zdj_view_t * view, void * _event ) {

}

void _zdj_menu_item_static_hmi_input( zdj_view_t * view, void * _event ) {

}
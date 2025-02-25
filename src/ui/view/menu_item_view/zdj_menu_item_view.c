#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

void _zdj_menu_item_draw( zdj_view_t * view, zdj_view_clip_t * clip );
void _zdj_menu_item_hmi_event( zdj_view_t * view, void * _event );
void _zdj_menu_item_deinit_state( zdj_view_t * view );

void _zdj_menu_item_set_hilite( zdj_menu_item_view_state_t * state, zdj_view_clip_t * clip, bool hilite );

void _zdj_menu_item_static_hmi_open_view( zdj_view_t * view, void * _event );
void _zdj_menu_item_static_hmi_open_menu( zdj_view_t * view, void * _event );
void _zdj_menu_item_static_hmi_toggle( zdj_view_t * view, void * _event );
void _zdj_menu_item_static_hmi_input( zdj_view_t * view, void * _event );

// Parse a static XML node into a menu_item_view.
zdj_view_t * zdj_new_menu_item( char * title ) {
    // Make view
    zdj_view_t * menu_item = zdj_new_view( NULL );
    menu_item->type = ZDJ_VIEW_MENU_ITEM;
    menu_item->draw = &_zdj_menu_item_draw;
    menu_item->handle_hmi_event = &_zdj_menu_item_hmi_event;
    menu_item->deinit_state = &_zdj_menu_item_deinit_state;

    // Build state
    zdj_menu_item_view_state_t * state = calloc( 1, sizeof( zdj_menu_item_view_state_t ) );
    menu_item->state = (void*)state;
    state->title = strdup( title );
    state->data = calloc( 1, sizeof( zdj_ui_data_t ) );
    state->link = NULL;
    state->update_data = NULL;
    state->update_layout = NULL;
    state->is_blinking = false;
    state->blink_timer = 0;
    state->has_valid_display = false;
    state->normal_view = zdj_new_view( NULL );
    zdj_add_subview( menu_item, state->normal_view );
    state->hilite_view = zdj_new_view( NULL );
    zdj_add_subview( menu_item, state->hilite_view );

    return menu_item;
}

// March through any view's subviews, looking for a menu_item_view
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
void _zdj_menu_item_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // debug bg
    // boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF990000 );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    if( state->update_data ) {
        state->update_data( view );
    }

    if( !state->has_valid_display && state->update_layout ) {
        state->update_layout( view );
    }
    
    if( state->is_blinking ) {
        if( state->blink_timer++ > ZDJ_BLINK_LENGTH ) {
            state->is_blinking = false;
            state->blink_timer = 0;
            state->is_hilite = false;
        } else {
            // blink on a cycle
            if( state->blink_timer % ZDJ_BLINK_PERIOD > ZDJ_BLINK_DUTY ) {
                _zdj_menu_item_set_hilite( state, clip, true );
            } else {
                _zdj_menu_item_set_hilite( state, clip, false );
            }
        }
    } else {
        _zdj_menu_item_set_hilite( state, clip, state->is_hilite );
    }
}

void _zdj_menu_item_set_hilite( zdj_menu_item_view_state_t * state, zdj_view_clip_t * clip, bool hilite ) {
    if( hilite ) {
        state->normal_view->frame->y = -100;
        state->hilite_view->frame->y = 0;
    } else {
        state->normal_view->frame->y = 0;
        state->hilite_view->frame->y = -100;
    }
}

void _zdj_menu_item_hmi_event( zdj_view_t * view, void * _event ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    switch ( state->action ) {
        case ZDJ_MENU_ITEM_ACTION_VIEW:
            // _zdj_menu_item_static_hmi_open_view( view, _event );
            break;
        case ZDJ_MENU_ITEM_ACTION_MENU:
            // _zdj_menu_item_static_hmi_open_menu( view, _event );
            break;
        default:
            break;
    }
}

void _zdj_menu_item_deinit_state( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    if( state->data ) {
        free( state->data->tag );
        free( state->data->c_val );
        free( state->data );
        state->data = NULL;
    }
    if( state->link ) {
        free( state->link );
    }
    free( state );
    view->state = NULL;
}

update_layout_t zdj_menu_item_update_for_layout( zdj_menu_item_view_layout_t layout ) {
    switch ( layout ) {
    case ZDJ_MENU_ITEM_LAYOUT_BASIC_L:
        return zdj_menu_item_basic_l_update_layout;
        break;
    case ZDJ_MENU_ITEM_LAYOUT_BASIC_R:
        return zdj_menu_item_basic_r_update_layout;
        break;
    case ZDJ_MENU_ITEM_LAYOUT_DIR_L:
        return zdj_menu_item_dir_l_update_layout;
        break;
    case ZDJ_MENU_ITEM_LAYOUT_DIR_R:
        return zdj_menu_item_dir_r_update_layout;
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

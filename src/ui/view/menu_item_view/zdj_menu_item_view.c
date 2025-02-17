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
    state->title = title;
    state->data = calloc( 1, sizeof( zdj_ui_data_t ) );
    state->is_blinking = false;
    state->blink_timer = 0;
    state->has_valid_display = false;

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
    
    // Run type-specific draw
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    switch ( state->layout ) {
        case ZDJ_MENU_ITEM_LAYOUT_BASIC_L:
            zdj_menu_item_draw_basic_l( view, clip );
            break;
        case ZDJ_MENU_ITEM_LAYOUT_BASIC_R:
            zdj_menu_item_draw_basic_r( view, clip );
            break;
        case ZDJ_MENU_ITEM_LAYOUT_DATA_L:
            zdj_menu_item_draw_data_l( view, clip );
            break;
        case ZDJ_MENU_ITEM_LAYOUT_DATA_R:
            zdj_menu_item_draw_data_r( view, clip );
            break;
        case ZDJ_MENU_ITEM_LAYOUT_SLIDER_L:
            zdj_menu_item_draw_slider_l( view, clip );
            break;
        case ZDJ_MENU_ITEM_LAYOUT_SLIDER_R:
            zdj_menu_item_draw_slider_r( view, clip );
            break;
            case ZDJ_MENU_ITEM_LAYOUT_TOGGLE_L:
            zdj_menu_item_draw_toggle_l( view, clip );
            break;
        case ZDJ_MENU_ITEM_LAYOUT_TOGGLE_R:
            zdj_menu_item_draw_toggle_r( view, clip );
            break;
        default:
            break;
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
    free( state->data->tag );
    free( state->data->c_val );
    free( state->data );
    free( state->link );
    state->data = NULL;
    free( state );
    view->state = NULL;
}

zdj_menu_item_view_layout_t zdj_menu_item_layout_for_string( char * string ) {
    if( !strcmp( "BAS_L", string ) ) {
        return ZDJ_MENU_ITEM_LAYOUT_BASIC_L;
    } else if( !strcmp( "BAS_R", string ) ) {
        return ZDJ_MENU_ITEM_LAYOUT_BASIC_R;
    } else if( !strcmp( "DAT_L", string ) ) {
        return ZDJ_MENU_ITEM_LAYOUT_DATA_L;
    } else if( !strcmp( "DAT_R", string ) ) {
        return ZDJ_MENU_ITEM_LAYOUT_DATA_R;
    } else if( !strcmp( "TGL_L", string ) ) {
        return ZDJ_MENU_ITEM_LAYOUT_TOGGLE_L;
    } else if( !strcmp( "TGL_R", string ) ) {
        return ZDJ_MENU_ITEM_LAYOUT_TOGGLE_R;
    } else if( !strcmp( "SLD_L", string ) ) {
        return ZDJ_MENU_ITEM_LAYOUT_SLIDER_L;
    } else if( !strcmp( "SLD_R", string ) ) {
        return ZDJ_MENU_ITEM_LAYOUT_SLIDER_R;
    } else {
        return ZDJ_MENU_ITEM_LAYOUT_UNKNOWN;
    }
}

zdj_menu_item_view_action_t zdj_menu_item_action_for_string( char * string ) {
    if( !strcmp( "VEW", string ) ) {
        return ZDJ_MENU_ITEM_ACTION_VIEW;
    } else if( !strcmp( "MNU", string ) ) {
        return ZDJ_MENU_ITEM_ACTION_MENU;
    } else if( !strcmp( "ALRT", string ) ) {
        return ZDJ_MENU_ITEM_ACTION_ALERT;
    } else if( !strcmp( "MODL", string ) ) {
        return ZDJ_MENU_ITEM_ACTION_MODAL;
    } else if( !strcmp( "DD", string ) ) {
        return ZDJ_MENU_ITEM_ACTION_DROPDOWN;
    } else if( !strcmp( "TGL", string ) ) {
        return ZDJ_MENU_ITEM_ACTION_TOGGLE;
    } else if( !strcmp( "SLD", string ) ) {
        return ZDJ_MENU_ITEM_ACTION_SLIDER;
    } else if( !strcmp( "CYC", string ) ) {
        return ZDJ_MENU_ITEM_ACTION_CYCLE;
    } else if( !strcmp( "INP", string ) ) {
        return ZDJ_MENU_ITEM_ACTION_INPUT;
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

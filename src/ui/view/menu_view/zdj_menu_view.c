#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/hmi/zdj_hmi.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/menu_section_view/zdj_menu_section_view.h>
#include <zerodj/ui/view/scroll_view/zdj_scroll_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

#define ZDJ_BACK_INDEX -1

void _zdj_menu_draw( zdj_view_t * view, zdj_view_clip_t * clip );
void _zdj_menu_handle_hmi( zdj_view_t * view, void * _event );
void _zdj_menu_deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_menu_view( zdj_ui_orient_t scroll_dir, zdj_rect_t * frame ) {
    zdj_view_t * menu_view = zdj_new_view( frame );
    menu_view->type = ZDJ_VIEW_MENU;
    menu_view->draw = &_zdj_menu_draw;
    menu_view->handle_hmi_event = _zdj_menu_handle_hmi;
    menu_view->deinit_state = &_zdj_menu_deinit_state;

    // Add a scroll_view
    zdj_view_t * menu_scroll_view = zdj_new_scroll_view( frame );
    zdj_add_subview( menu_view, menu_scroll_view );

    // Add a state instance
    zdj_menu_view_state_t * state = calloc( 1, sizeof( zdj_menu_view_state_t ) );
    state->scroll_view = menu_scroll_view;
    state->scroll_enabled = true;
    menu_view->state = state;

    return menu_view;
}

void _zdj_menu_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_BLACK );
}

void zdj_menu_view_add_header( zdj_view_t * menu_view, zdj_view_t * header ) {
    if( !header ) { return; }

    zdj_add_subview( menu_view, header );
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)header->state;
    zdj_menu_view_state_t * state = (zdj_menu_view_state_t*)menu_view->state;

    state->header_view = header;
    state->has_back = header_state->has_back;

    header->frame->w = menu_view->frame->w;
    header->frame->h = 8;

    // Move the scroll view down to make room for the header
    state->scroll_view->frame->y = 10;
    state->scroll_view->frame->h = menu_view->frame->h - 10;
}

void zdj_menu_view_add_section( zdj_view_t * menu_view, zdj_view_t * section ) {
    if( !section ) { return; }

    zdj_menu_view_state_t * state = (zdj_menu_view_state_t*)menu_view->state;
    // Align new item to menu's scroll orientation
    zdj_point_t scroll_size;
    zdj_scroll_view_get_size( state->scroll_view, &scroll_size );
    section->frame->y = scroll_size.y;
    section->frame->x = scroll_size.x;
    section->frame->w = menu_view->frame->w;
    section->frame->h = 10;
    zdj_scroll_view_add_subview( state->scroll_view, section );
}

void zdj_menu_view_add_item( zdj_view_t * menu_view, zdj_view_t * item ) {
    if( !item ) { return; }

    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)menu_view->state;
    // Align new item to menu's scroll orientation
    zdj_point_t scroll_size;
    zdj_scroll_view_get_size( menu_state->scroll_view, &scroll_size );
    if( item->frame->y == 0 ) {
        item->frame->y += scroll_size.y;
    }
    if( item->frame->x == 0 ) {
        item->frame->x += scroll_size.x;
    }
    if( item->frame->w == 0 ) {
        item->frame->w = menu_view->frame->w;
    }
    if( item->frame->h == 0 ) {
        item->frame->h = 10;
    }
    zdj_scroll_view_add_subview( menu_state->scroll_view, item );

    // Set scroll_index for new item
    if( item->type == ZDJ_VIEW_MENU_ITEM ) {
        zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;
        item_state->scroll_index = menu_state->item_count++;
    }
}

void _zdj_menu_handle_hmi( zdj_view_t * view, void * _event ) {
    zdj_hmi_event_t * e = (zdj_hmi_event_t *)_event;
    
    // Ignore events which have been blocked by layers above this one.
    if( e->blocked ) { return; }

    // Prevent view stack from sending events to subviews.
    // Any events will be passed to subviews from this func.
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)view->state;
    zdj_menu_header_view_state_t * menu_header_state;
    if( menu_state->header_view ){ 
        menu_header_state = (zdj_menu_header_view_state_t*)menu_state->header_view->state; 
    }
    zdj_menu_item_view_state_t * prev_menu_item_state;
    zdj_menu_item_view_state_t * new_menu_item_state;

    // Handle all the jog-wheel stuff (scroll, mod scroll, press/long press, etc.)
    if( e->id == ZDJ_HMI_ENCO_2_JOG ) {
        // Prevent views/menus below this one from getting jog wheel events
        e->blocked = true;

        // Handle scroll - update scroll_index, scroll_view, back btns
        zdj_scroll_view_state_t * scroll_state = (zdj_scroll_view_state_t*)menu_state->scroll_view->state;
        if( e->type == ZDJ_HMI_EVENT_ADJUST ) { 
            // If there are no menu items, set the back btn if available and return
            if( menu_state->item_count == 0 && menu_state->has_back ) {
                menu_state->scroll_index = -1;
                if( menu_state->header_view && menu_header_state ) {
                    menu_header_state->show_back = true;
                    menu_header_state->has_valid_display = false;
                }
                return;
            }

            // Update scroll_index with new jog adjust event value.
            // Don't go above menu's item_count, and don't go below
            // 0, or -1 depending on presence of a back btn in menu.
            int new_scroll = menu_state->scroll_index + e->i_val;
            if( new_scroll >= menu_state->item_count ) {
                new_scroll = menu_state->item_count-1;
            } else if( new_scroll < 0 ) {
                // Stop at -1 if back button is present
                if( menu_state->has_back ) {
                    if( new_scroll < ZDJ_BACK_INDEX ) {
                        new_scroll = ZDJ_BACK_INDEX;
                    }
                } else {
                    new_scroll = 0;
                }
            }

            // Check the new value against the previous value.
            // Only start updating things if the value has changed.
            if( new_scroll != menu_state->scroll_index ) {
                // Clear hilite state of the previous selected item.
                // If it was the header's back item, hide the header's back view.
                // If it was a menu_item, clear menu_item->is_hilite and force
                // it to update its layout to resize the hilite.
                if( menu_state->scroll_index == ZDJ_BACK_INDEX ) {
                    if( menu_state->header_view ) {
                        menu_header_state->hide_back = true;
                        menu_header_state->has_valid_display = false;
                    }
                } else {
                    zdj_view_t * prev_menu_item = zdj_menu_item_for_scroll_index( 
                        menu_state->scroll_view, 
                        menu_state->scroll_index 
                    );
                    prev_menu_item_state = prev_menu_item->state;
                    prev_menu_item_state->is_hilite = false;
                }
                
                // Grab a scroll direction and update the scroll index
                bool scroll_dir = new_scroll > menu_state->scroll_index;
                menu_state->scroll_index = new_scroll;

                // If this is a back btn, show header's back state and return early
                if( menu_state->scroll_index == ZDJ_BACK_INDEX ) {
                    if( menu_state->header_view ) {
                        menu_header_state->show_back = true;
                        menu_header_state->has_valid_display = false;
                    }
                    return;
                }

                // Grab the new menu item + set is_hilite
                zdj_view_t * new_menu_item = zdj_menu_item_for_scroll_index( 
                    menu_state->scroll_view, 
                    menu_state->scroll_index 
                );
                new_menu_item_state = new_menu_item->state;
                new_menu_item_state->is_hilite = true;

                // If scrolling is disabled in menu, we're done.
                if( !menu_state->scroll_enabled ){ return; }
                
                // Update the scroll_view's scroll_offset.
                zdj_point_t scroll_point;
                if( menu_state->scroll_dir == ZDJ_VERTICAL ) {
                    scroll_point.x = 0;
                    scroll_point.y = new_menu_item->frame->y;
                } else if( menu_state->scroll_dir == ZDJ_HORIZONTAL ) {
                    scroll_point.x = new_menu_item->frame->x;
                    scroll_point.y = 0;
                }
                bool is_final_view;
                if( new_menu_item_state->scroll_index == 0 ||
                    new_menu_item_state->scroll_index == menu_state->item_count-1 ) {
                        is_final_view = true;
                    }
                zdj_scroll_view_to_view( menu_state->scroll_view, new_menu_item, scroll_dir, is_final_view );
            }        
        }


        // Release, mod-scroll, long-press, etc. should invoke hmi handler of menu_item @scroll_index
        if( e->type == ZDJ_HMI_EVENT_RELEASE ||
            e->type == ZDJ_HMI_EVENT_MOD_RELEASE ||
            // e->type == ZDJ_HMI_EVENT_MOD_ADJUST ||
            e->type == ZDJ_HMI_EVENT_PRESS_ADJUST ||
            e->type == ZDJ_HMI_EVENT_PRESS_ADJUST_RELEASE ||
            e->type == ZDJ_HMI_EVENT_LONG_PRESS || 
            e->type == ZDJ_HMI_EVENT_LONG_RELEASE 
        ) {
            if( menu_state->scroll_index == ZDJ_BACK_INDEX ) {
                // Blink the back btn
                menu_header_state->is_blinking = true;
                menu_header_state->blink_timer = 0;

                // Call the back btn handler
                menu_header_state->handle_back( view );
            } else {
                // Get the menu_item @ current scroll_index
                zdj_view_t * menu_item = zdj_menu_item_for_scroll_index( 
                    menu_state->scroll_view, 
                    menu_state->scroll_index 
                );
    
                // Blink the selected menu_item if this is a button press
                if( e->type == ZDJ_HMI_EVENT_RELEASE ) {
                    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)menu_item->state;
                    state->is_blinking = true;
                    state->blink_timer = 0;
                }
                
                // Call into handle_hmi_event set by front-end
                if( menu_item->handle_hmi_event ) {
                    menu_item->handle_hmi_event( menu_item, e );
                }
            }
            
        }
    } // Jog-wheel stuff
}

void _zdj_menu_deinit_state( zdj_view_t * view ) {
    zdj_menu_view_state_t * state = (zdj_menu_view_state_t*)view->state;
    free( state );
    view->state = NULL;
}
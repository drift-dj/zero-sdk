#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/controls/hmi/zdj_hmi.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/menu_section_view/zdj_menu_section_view.h>
#include <zerodj/ui/view/scroll_view/zdj_scroll_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

void _zdj_menu_deinit_state( zdj_view_t * view );
void _zdj_menu_update_scroll( zdj_view_t * menu_view, int new_scroll );

zdj_view_t * zdj_new_menu_view( zdj_ui_orient_t scroll_dir, zdj_rect_t * frame ) {
    zdj_view_t * menu_view = zdj_new_view( frame );
    menu_view->type = ZDJ_VIEW_MENU;
    menu_view->draw = &zdj_menu_draw;
    menu_view->handle_hmi_event = zdj_menu_handle_hmi;
    menu_view->deinit_state = &_zdj_menu_deinit_state;
    menu_view->in_anim = zdj_new_anim( ZDJ_ANIM_MENU_SHOW );
    menu_view->out_anim = zdj_new_anim( ZDJ_ANIM_MENU_HIDE );
    
    menu_view->frame->x = 0;
    menu_view->frame->y = ZDJ_MENU_HEIGHT+2;
    menu_view->frame->w = ZDJ_MENU_WIDTH;
    menu_view->frame->h = ZDJ_MENU_HEIGHT;

    // Add a scroll_view
    zdj_view_t * menu_scroll_view = zdj_new_scroll_view( frame );
    zdj_scroll_view_state_t * scroll_view_state = (zdj_scroll_view_state_t*)menu_scroll_view->state;
    scroll_view_state->scroll_dir = scroll_dir;
    zdj_add_subview( menu_view, menu_scroll_view );

    // Add a state instance
    zdj_menu_view_state_t * state = calloc( 1, sizeof( zdj_menu_view_state_t ) );
    state->scroll_view = menu_scroll_view;
    state->scroll_view_frame.x = frame->x;
    state->scroll_view_frame.w = frame->w;
    state->scroll_dir = scroll_dir;
    state->scroll_enabled = true;
    state->scroll_animated = true;
    menu_view->state = state;

    // Add a scroll filter to process jog wheel inputs
    state->scroll_filter = calloc( 1, sizeof( zdj_menu_view_scroll_filter_t ) );
    state->scroll_filter->drag = 0.4f;

    return menu_view;
}

void zdj_menu_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_BLACK );

    // Update the scroll filter physics simulation.
    // Use the output value to set menu scroll index.
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)view->state;
    zdj_menu_view_update_scroll_filter( view );
    if( menu_state->scroll_filter->out_index != menu_state->scroll_index ) {
        _zdj_menu_update_scroll( view, menu_state->scroll_filter->out_index );
    }
}

void zdj_menu_view_set_scrollview_frame( zdj_view_t * menu_view, zdj_rect_t * frame ) {
    zdj_menu_view_state_t * state = (zdj_menu_view_state_t*)menu_view->state;
    state->scroll_view_frame.x = frame->x;
    state->scroll_view_frame.w = frame->w;
}

void zdj_menu_view_add_header( zdj_view_t * menu_view, zdj_view_t * header ) {
    if( !header ) { return; }

    zdj_add_subview( menu_view, header );
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)header->state;
    zdj_menu_view_state_t * state = (zdj_menu_view_state_t*)menu_view->state;

    state->header_view = header;
    state->has_back = header_state->has_back;

    header->frame->w = menu_view->frame->w;
    header->frame->h = 7;

    // Move the scroll view down to make room for the header
    state->scroll_view->frame->y = 8;
    state->scroll_view->frame->h = menu_view->frame->h - 7;
    state->scroll_view->frame->x = state->scroll_view_frame.x;
    state->scroll_view->frame->w = state->scroll_view_frame.w;
}

void zdj_menu_view_add_section( zdj_view_t * menu_view, zdj_view_t * section ) {
    if( !section ) { return; }

    zdj_menu_view_state_t * state = (zdj_menu_view_state_t*)menu_view->state;
    // Align new item to menu's scroll orientation
    zdj_point_t scroll_size;
    zdj_scroll_view_get_size( state->scroll_view, &scroll_size );
   
    if( section->frame->y == 0 ) {
        section->frame->y = scroll_size.y;
    }    
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
        item->frame->w = menu_state->scroll_view_frame.w;
    }
    if( item->frame->h == 0 ) {
        item->frame->h = 10;
    }
    zdj_scroll_view_add_subview( menu_state->scroll_view, item );

    // Set scroll_index for new item
    if( item->type == ZDJ_VIEW_MENU_ITEM ) {
        zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;
        item_state->scroll_index = menu_state->item_count++;
        if( item_state->scroll_index == 0 ){ item_state->is_hilite = true; }
    }
}

void zdj_menu_view_add_chrome_item( zdj_view_t * menu_view, zdj_view_t * item ) {
    if( !item ) { return; }

    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)menu_view->state;
    zdj_add_subview( menu_view, item );

    // Set scroll_index for new item
    if( item->type == ZDJ_VIEW_MENU_ITEM ) {
        zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;
        item_state->scroll_index = menu_state->item_count++;
    }
}

void zdj_menu_view_insert_item( zdj_view_t * menu_view, zdj_view_t * item, int index ) {
    // Get a menu_item at the given scroll_index

    // Insert the view into the scroll_view

    // Loop on all items after index 
    
        // Move x/y by inserted item's size or standard item size if not specified
        // Increment scroll_index
}

// Add n pixels of padding to the scroll_view
void zdj_menu_view_add_padding( zdj_view_t * menu_view, int size ) {
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)menu_view->state;
    zdj_scroll_view_state_t * scroll_state = (zdj_scroll_view_state_t*)menu_state->scroll_view->state;

    // Update the scroll_size based on the newly-added view and the scroll direction
    if( scroll_state->scroll_dir == ZDJ_VERTICAL ) {
        scroll_state->scroll_size.y += size;
    } else if( scroll_state->scroll_dir == ZDJ_HORIZONTAL ) {
        scroll_state->scroll_size.x += size;
    }
}

void zdj_menu_view_remove_all_items( zdj_view_t * menu_view ) {
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t *)menu_view->state;

    // Dump the current scroll_view
    zdj_remove_subview_of( menu_view, menu_state->scroll_view );

    // Add a new scroll_view
    zdj_view_t * menu_scroll_view = zdj_new_scroll_view( menu_view->frame );
    menu_scroll_view->frame->x = menu_state->scroll_view_frame.x;
    menu_scroll_view->frame->w = menu_state->scroll_view_frame.w;

    zdj_scroll_view_state_t * scroll_view_state = (zdj_scroll_view_state_t*)menu_scroll_view->state;
    scroll_view_state->scroll_dir = menu_state->scroll_dir;
    if( menu_state->header_view ) {
        menu_scroll_view->frame->y = 8;
        menu_scroll_view->frame->h = menu_view->frame->h - 7;
    }
    zdj_add_bottom_subview_to( menu_view, menu_scroll_view );
    menu_state->scroll_view = menu_scroll_view;
    menu_state->scroll_index = 0;
    menu_state->section_count = 0;
    menu_state->item_count = 0;
}

void zdj_menu_view_remove_item_at_scroll_index( zdj_view_t * menu_view, int index ) {
    // printf( "zdj_menu_view_remove_item_at_scroll_index: %d\n", index );
    // Reset y, and scroll_index of all menu_items following index.
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t *)menu_view->state;
    if( index >= menu_state->item_count ) { return; }
    zdj_scroll_view_state_t * scroll_state = (zdj_scroll_view_state_t*)menu_state->scroll_view->state;

    // Loop thru all subviews, counting up only menu_item_views toward index.
    // Subview count and item count may not be equal (sections, non-items, etc.)
    zdj_view_t * scroll_view_subview = menu_state->scroll_view->subviews;
    int subview_count = 0;
    int32_t subview_index = INT32_MAX;
    int item_count = 0;
    int removed_view_height = 0;
    while( scroll_view_subview ) {
        // Grab next subview here as we may delete it in a sec.
        zdj_view_t * next_subview = scroll_view_subview->next;

        if( scroll_view_subview->type == ZDJ_VIEW_MENU_ITEM ) {
            // We're at the menu_item at the requested index.
            // Remove it and note the subview index so we can
            // shift everything up/re-index.
            if( item_count == index ) {
                // printf( "removing item %d at index %d\n", subview_count, item_count );
                zdj_remove_subview_of( menu_state->scroll_view, scroll_view_subview ); 
                removed_view_height = scroll_view_subview->frame->h;
                subview_index = subview_count;
            }
            item_count++;
        }

        if( subview_count > subview_index ) {
            if( scroll_view_subview->type == ZDJ_VIEW_MENU_ITEM ) {
                zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)scroll_view_subview->state;
                item_state->scroll_index--;
            }
            scroll_view_subview->frame->y -= removed_view_height;
        }

        subview_count++;
        scroll_view_subview = next_subview;
    }

    menu_state->item_count--;
    scroll_state->scroll_size.y -= removed_view_height;
}

zdj_view_t * zdj_menu_view_item_at_current_scroll_index( zdj_view_t * menu_view ) { 
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t *)menu_view->state;
    return zdj_menu_view_item_at_scroll_index( menu_view, menu_state->scroll_index );
}

zdj_view_t * zdj_menu_view_item_at_scroll_index( zdj_view_t * menu_view, int index ) {
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t *)menu_view->state;
    if( index >= menu_state->item_count ) { return NULL; }

    // Iterate thru scroll_view's subviews, ignoring non-menu_items
    zdj_view_t * scroll_view_subview = menu_state->scroll_view->subviews;
    int item_count = 0;
    while( scroll_view_subview ) {
        if( scroll_view_subview->type == ZDJ_VIEW_MENU_ITEM ) {
            if( item_count == index ) { return scroll_view_subview; }
            item_count++;
        }
        scroll_view_subview = scroll_view_subview->next;
    }
}

zdj_view_t * zdj_menu_view_get_item_for_data_ptr( zdj_view_t * menu_view, void * ptr ) {
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t *)menu_view->state;
    if( !ptr ) { return NULL; }

    // Iterate thru scroll_view's subviews, looking for matching ptr
    zdj_view_t * scroll_view_subview = menu_state->scroll_view->subviews;
    while( scroll_view_subview ) {
        if( scroll_view_subview->type == ZDJ_VIEW_MENU_ITEM ) {
            zdj_menu_item_view_state_t * item_state = ( zdj_menu_item_view_state_t* )scroll_view_subview->state;
            if( item_state->data->ptr &&
                item_state->data->ptr == ptr 
            ) {
                return scroll_view_subview;
            }
            
        }
        scroll_view_subview = scroll_view_subview->next;
    }
    return NULL;
}

zdj_view_t * zdj_menu_view_get_item_for_data_c_val( zdj_view_t * menu_view, char * c_val ) {
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t *)menu_view->state;
    if( !c_val ) { return NULL; }

    // Iterate thru scroll_view's subviews, looking for matching string
    zdj_view_t * scroll_view_subview = menu_state->scroll_view->subviews;
    while( scroll_view_subview ) {
        if( scroll_view_subview->type == ZDJ_VIEW_MENU_ITEM ) {
            zdj_menu_item_view_state_t * item_state = ( zdj_menu_item_view_state_t* )scroll_view_subview->state;
            if( item_state->data->c_val &&
                !strcmp( item_state->data->c_val, c_val )
            ) {
                return scroll_view_subview;
            }
            
        }
        scroll_view_subview = scroll_view_subview->next;
    }
    return NULL;
}

void zdj_menu_handle_hmi( zdj_view_t * view, void * _event ) {
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

    // Handle all the jog-wheel stuff (scroll, mod scroll, press/long press, etc.)
    if( e->id == ZDJ_HMI_ENCO_2_JOG ) {
        // Prevent views/menus below this one from getting jog wheel events
        e->blocked = true;

        if( e->type == ZDJ_HMI_EVENT_ADJUST ) { 
            // Add the event's value to the scroll filter.
            // The scroll filter simulation runs in the draw loop,
            // therefore, menu_update_scroll happens during the draw loop.
            zdj_menu_view_add_scroll_filter_input( view, e->i_val );
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

void zdj_menu_view_set_scroll_index( zdj_view_t * menu_view, int index ) {
    _zdj_menu_update_scroll( menu_view, index );
}

void _zdj_menu_update_scroll( zdj_view_t * menu_view, int new_scroll ) {
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)menu_view->state;
    zdj_scroll_view_state_t * scroll_state = (zdj_scroll_view_state_t*)menu_state->scroll_view->state;
    zdj_menu_item_view_state_t * prev_menu_item_state;
    zdj_menu_item_view_state_t * new_menu_item_state;
    zdj_menu_header_view_state_t * menu_header_state;
    if( menu_state->header_view ){ 
        menu_header_state = (zdj_menu_header_view_state_t*)menu_state->header_view->state; 
    }
    
    // If there are no menu items, set the back btn if available and return
    if( menu_state->item_count == 0 && menu_state->has_back ) {
        menu_state->scroll_index = -1;
        if( menu_state->header_view && menu_header_state ) {
            menu_header_state->show_back = true;
        }
        return;
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
            }
        } else {
            zdj_view_t * prev_menu_item = zdj_menu_item_for_scroll_index( 
                menu_state->scroll_view, 
                menu_state->scroll_index 
            );
            // If item isn't found in scroll_view, check menu view itself 
            // (for non-scrolling chrome items).
            if( !prev_menu_item ) {
                prev_menu_item = zdj_menu_item_for_scroll_index( 
                    menu_view, 
                    menu_state->scroll_index 
                );
            }
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
            }
            return;
        }

        // Grab the new menu item + set is_hilite
        zdj_view_t * new_menu_item = zdj_menu_item_for_scroll_index( 
            menu_state->scroll_view, 
            menu_state->scroll_index 
        );
        // If item isn't found in scroll_view, check menu view itself 
        // (for non-scrolling chrome items).
        bool new_menu_item_is_chrome = false;
        if( !new_menu_item ) {
            new_menu_item = zdj_menu_item_for_scroll_index( 
                menu_view, 
                menu_state->scroll_index 
            );
            new_menu_item_is_chrome = true;
        }
        new_menu_item_state = new_menu_item->state;
        new_menu_item_state->is_hilite = true;

        // If new_menu_item is chrome (not inside scroll view), we're done.
        if( new_menu_item_is_chrome ){ return; }

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
        zdj_scroll_view_to_view( menu_state->scroll_view, new_menu_item, scroll_dir, is_final_view, true );
    }        
}

void _zdj_menu_deinit_state( zdj_view_t * view ) {
    zdj_menu_view_state_t * state = (zdj_menu_view_state_t*)view->state;
    free( state );
    view->state = NULL;
}
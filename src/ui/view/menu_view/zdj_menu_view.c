#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/menu_section_view/zdj_menu_section_view.h>
#include <zerodj/ui/view/scroll_view/zdj_scroll_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _deinit_state( zdj_view_t * view );
static void _update_scroll( zdj_view_t * menu_view, int new_scroll );

static bool _put_prev_section_index( zdj_view_t * menu, int current_index, int * scroll_target );
static bool _put_next_section_index( zdj_view_t * menu, int current_index, int * scroll_target );

zdj_view_t * zdj_new_menu_view( zdj_ui_orient_t scroll_dir, zdj_rect_t * frame ) {
    zdj_view_t * menu_view = zdj_new_view( frame );
    // zdj_view_t * menu_view = zdj_new_view( &(zdj_rect_t){ frame->x-6,frame->y,frame->w+6,frame->h } );
    menu_view->type = ZDJ_VIEW_MENU;
    menu_view->draw = &zdj_menu_draw;
    menu_view->handle_control_event = zdj_menu_handle_control;
    menu_view->deinit_state = &_deinit_state;
    menu_view->map = ZDJ_CONTROL_MAP_MENU_BASE;
    zdj_set_anim( &menu_view->in_anim, ZDJ_ANIM_MENU_SHOW );
    zdj_set_anim( &menu_view->out_anim, ZDJ_ANIM_MENU_HIDE );

    menu_view->frame.y = ZDJ_SCREEN_H + 1;
    menu_view->frame.w = frame->w;
    // menu_view->frame.w = frame->w+6;
    menu_view->frame.h = frame->h;

    // Add a scroll_view
    zdj_view_t * menu_scroll_view = zdj_new_scroll_view( frame );
    // zdj_view_t * menu_scroll_view = zdj_new_scroll_view( &(zdj_rect_t){ frame->x,frame->y,frame->w,frame->h } );
    zdj_scroll_view_state_t * scroll_view_state = (zdj_scroll_view_state_t*)menu_scroll_view->state;
    scroll_view_state->scroll_dir = scroll_dir;
    zdj_add_subview( menu_view, menu_scroll_view );

    // Add a state instance
    zdj_menu_view_state_t * state = calloc( 1, sizeof( zdj_menu_view_state_t ) );
    state->scroll_view = menu_scroll_view;
    state->scroll_view_frame.x = 0;
    // state->scroll_view_frame.x = 6;
    state->scroll_view_frame.w = frame->w;
    state->scroll_dir = scroll_dir;
    state->scroll_enabled = true;
    state->scroll_animated = true;
    state->input_mode = ZDJ_MENU_INPUT_MODE_NORMAL;
    state->edit_enabled = false;
    menu_view->state = state;

    // Add a scroll filter to process jog wheel inputs
    state->scroll_filter = calloc( 1, sizeof( zdj_menu_view_scroll_filter_t ) );
    state->scroll_filter->drag = 0.4f;

    return menu_view;
}

zdj_view_t * zdj_new_lib_menu_view( 
    zdj_ui_orient_t scroll_dir, 
    char * table_name, 
    zdj_rect_t * frame 
) {
    zdj_view_t * menu = zdj_new_menu_view( scroll_dir, frame );
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)menu->state;
    strcpy( menu_state->lib_db_table, table_name );
}

void zdj_menu_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // Let's try dumping the menu BG pixels here
    SDL_Rect s = { 
        zdj_ui_assets[ ZDJ_UI_ASSET_MENU_BG ].x, 
        zdj_ui_assets[ ZDJ_UI_ASSET_MENU_BG ].y, 
        zdj_ui_assets[ ZDJ_UI_ASSET_MENU_BG ].w, 
        zdj_ui_assets[ ZDJ_UI_ASSET_MENU_BG ].h 
    };
    SDL_Rect d = { 0, 0, zdj_ui_assets[ ZDJ_UI_ASSET_MENU_BG ].w, zdj_ui_assets[ ZDJ_UI_ASSET_MENU_BG ].h };
    SDL_RenderCopy( zdj_renderer( ), zdj_asset_atlas( ), &s, &d );

    // Menu BG
    boxColor( zdj_renderer( ), clip->dst.x - 1, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_BLACK );
    // boxColor( zdj_renderer( ), clip->dst.x + 5, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_BLACK );

    // Update the scroll filter physics simulation.
    // Use the output value to set menu scroll index.
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)view->state;
    zdj_menu_view_update_scroll_filter( view );
    if( menu_state->scroll_filter->out_index != menu_state->scroll_index ) {
        _update_scroll( view, menu_state->scroll_filter->out_index );
    }

    if( view->needs_layout_init && view->init_layout ) { view->init_layout( view ); }
    if( view->needs_subview_update && view->update_subviews ) { view->update_subviews( view ); }
}

void zdj_menu_view_set_scrollview_frame( zdj_view_t * menu_view, zdj_rect_t * frame ) {
    zdj_menu_view_state_t * state = (zdj_menu_view_state_t*)menu_view->state;
    state->scroll_view_frame.x = frame->x;
    state->scroll_view_frame.w = frame->w;
    // state->scroll_view_frame.x = frame->x - 3;
    // state->scroll_view_frame.w = frame->w + 3;
}

void zdj_menu_view_add_header( zdj_view_t * menu_view, zdj_view_t * header ) {
    if( !header ) { return; }

    zdj_add_subview( menu_view, header );
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)header->state;
    zdj_menu_view_state_t * state = (zdj_menu_view_state_t*)menu_view->state;

    state->header_view = header;
    state->has_back = header_state->has_back;

    header->frame.w = menu_view->frame.w;
    header->frame.h = 5;

    // Move the scroll view down to make room for the header
    state->scroll_view->frame.y = 6;
    state->scroll_view->frame.h = menu_view->frame.h - 5;
    state->scroll_view->frame.x = state->scroll_view_frame.x;
    state->scroll_view->frame.w = state->scroll_view_frame.w;
}

void zdj_menu_view_add_section( zdj_view_t * menu_view, zdj_view_t * section ) {
    if( !section ) { return; }

    zdj_menu_view_state_t * state = (zdj_menu_view_state_t*)menu_view->state;
    // Align new item to menu's scroll orientation
    zdj_point_t scroll_size;
    zdj_scroll_view_get_size( state->scroll_view, &scroll_size );
   
    if( section->frame.y == 0 ) {
        section->frame.y = scroll_size.y;
    }    
    section->frame.x = scroll_size.x;
    section->frame.w = menu_view->frame.w;
    section->frame.h = 9;
    zdj_scroll_view_add_subview( state->scroll_view, section );
}

void zdj_menu_view_add_item( zdj_view_t * menu_view, zdj_view_t * item ) {
    // printf( "zdj_menu_view_add_item( %p, %p ) h:%1.1f\n", menu_view, item, item->frame.h );
    if( !item ) { return; }

    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)menu_view->state;
    // Align new item to menu's scroll orientation
    zdj_point_t scroll_size;
    zdj_scroll_view_get_size( menu_state->scroll_view, &scroll_size );
    if( item->frame.y == 0 ) {
        item->frame.y += scroll_size.y;
    }
    if( item->frame.x == 0 ) {
        item->frame.x += scroll_size.x;
    }
    if( item->frame.w == 0 ) {
        item->frame.w = menu_state->scroll_view_frame.w;
    }
    if( item->frame.h == 0 ) {
        item->frame.h = 9;
    }
    // item->frame.h+=1;
    zdj_scroll_view_add_subview( menu_state->scroll_view, item );

    // Set scroll_index for new item
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;
    if( item->type == ZDJ_VIEW_MENU_ITEM ) {
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

// Move an item up or down in the menu
void zdj_menu_view_move_item( zdj_view_t * menu_view, zdj_view_t * item, int dir ) {
    // printf( "zdj_menu_view_move_item: %d\n", dir );
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)menu_view->state;
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;

    // Bug out early if we're constrained
    if( item_state->scroll_index >= menu_state->edit_item_move_bottom_index && dir > 0 ) { 
        return;
    } else if ( item_state->scroll_index <= menu_state->edit_item_move_top_index && dir < 0 ) { 
        return; 
    }

    if( dir > 0 ) {
        menu_state->move_item_index += 0.21;
    } else {
        menu_state->move_item_index -= 0.21;
    }

    if( menu_state->move_item_index > menu_state->edit_item_move_bottom_index ) {
        menu_state->move_item_index = (double)menu_state->edit_item_move_bottom_index;
    } else if( menu_state->move_item_index < menu_state->edit_item_move_top_index ) {
        menu_state->move_item_index = (double)menu_state->edit_item_move_top_index;
    }

    int item_index = item_state->scroll_index;
    int target_scroll_index = (int)round( menu_state->move_item_index );
    

    zdj_view_t * target_item = NULL;
    zdj_menu_item_view_state_t * target_item_state;
    if( target_scroll_index != item_index ) {
        target_item = zdj_menu_view_item_at_scroll_index( menu_view, target_scroll_index );
        target_item_state = (zdj_menu_item_view_state_t*)target_item->state;
        // printf( "found target_item @:%1.3f/%d w/%d\n", menu_state->move_item_index, target_scroll_index, target_item_state->scroll_index );
    }

    if( target_item ) {
        target_item_state = (zdj_menu_item_view_state_t*)target_item->state;

        // Swap item/target indexes
        float target_y = target_item->frame.y;
        float item_y = item->frame.y;
        
        item_state->scroll_index = target_scroll_index;
        item->frame.y = target_y;
        
        target_item_state->scroll_index = item_index;
        target_item->frame.y = item_y;
        
        zdj_menu_view_set_scroll_index( menu_view, item_state->scroll_index );
    }
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
    if( !menu_view ) { return; }
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t *)menu_view->state;
    if( menu_state->item_count == 0 ) { return; }

    // Dump the current scroll_view
    zdj_remove_subview_of( menu_view, menu_state->scroll_view );

    // Add a new scroll_view
    zdj_view_t * menu_scroll_view = zdj_new_scroll_view( &menu_view->frame );
    menu_scroll_view->frame.x = menu_state->scroll_view_frame.x;
    menu_scroll_view->frame.w = menu_state->scroll_view_frame.w;

    zdj_scroll_view_state_t * scroll_view_state = (zdj_scroll_view_state_t*)menu_scroll_view->state;
    scroll_view_state->scroll_dir = menu_state->scroll_dir;
    if( menu_state->header_view ) {
        menu_scroll_view->frame.y = 6;
        menu_scroll_view->frame.h = menu_view->frame.h - 5;
    }
    zdj_add_bottom_subview_to( menu_view, menu_scroll_view );
    menu_state->scroll_view = menu_scroll_view;
    menu_state->scroll_index = 0;
    menu_state->section_count = 0;
    menu_state->item_count = 0;
}

void zdj_menu_view_remove_all_subviews( zdj_view_t * menu_view ) {
    if( menu_view->type != ZDJ_VIEW_MENU ) { 
        printf( "Remove Subviews called on non-menu\n" );
        return;
    }
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t *)menu_view->state;
    if( !menu_state) {
        printf( "Remove Subviews called with menu->state = NULL\n" );
        return;
    }
    // Dump the current scroll_view
    if( menu_state->scroll_view) {
        zdj_remove_subview_of( menu_view, menu_state->scroll_view );
    }

    // Add a new scroll_view
    zdj_view_t * menu_scroll_view = zdj_new_scroll_view( &menu_view->frame );
    menu_scroll_view->frame.x = menu_state->scroll_view_frame.x;
    menu_scroll_view->frame.w = menu_state->scroll_view_frame.w;

    zdj_scroll_view_state_t * scroll_view_state = (zdj_scroll_view_state_t*)menu_scroll_view->state;
    scroll_view_state->scroll_dir = menu_state->scroll_dir;
    if( menu_state->header_view ) {
        menu_scroll_view->frame.y = 6;
        menu_scroll_view->frame.h = menu_view->frame.h - 5;
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
                removed_view_height = scroll_view_subview->frame.h;
                subview_index = subview_count;
            }
            item_count++;
        }

        if( subview_count > subview_index ) {
            if( scroll_view_subview->type == ZDJ_VIEW_MENU_ITEM ) {
                zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)scroll_view_subview->state;
                item_state->scroll_index--;
            }
            scroll_view_subview->frame.y -= removed_view_height;
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
    zdj_menu_item_view_state_t * item_state;
    int item_count = 0;
    while( scroll_view_subview ) {
        if( scroll_view_subview->type == ZDJ_VIEW_MENU_ITEM ) {
            item_state = (zdj_menu_item_view_state_t*)scroll_view_subview->state;
            if( item_state->scroll_index == index ) { return scroll_view_subview; }
            item_count++;
        }
        scroll_view_subview = scroll_view_subview->next;
    }
    return NULL;
}

zdj_view_t * zdj_menu_view_get_item_for_data_ptr( zdj_view_t * menu_view, void * ptr ) {
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t *)menu_view->state;
    if( !ptr ) { return NULL; }

    // Iterate thru scroll_view's subviews, looking for matching ptr
    zdj_view_t * scroll_view_subview = menu_state->scroll_view->subviews;
    while( scroll_view_subview ) {
        if( scroll_view_subview->type == ZDJ_VIEW_MENU_ITEM ) {
            zdj_menu_item_view_state_t * item_state = ( zdj_menu_item_view_state_t* )scroll_view_subview->state;
            if( item_state->data.ptr &&
                item_state->data.ptr == ptr 
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
            if( !strcmp( item_state->data.c_val, c_val )
            ) {
                return scroll_view_subview;
            }
            
        }
        scroll_view_subview = scroll_view_subview->next;
    }
    return NULL;
}

void zdj_menu_handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {
    // printf( "zdj_menu_handle_control: %p\n", view );
    zdj_control_event_t * e = (zdj_control_event_t *)_event; 

    // Ignore events which have been blocked by layers above this one.
    if( e->blocked ) { return; }

    // Prevent view stack from sending events to subviews.
    // Any events will be passed to subviews from this func.
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)view->state;
    zdj_menu_header_view_state_t * menu_header_state = NULL;
    if( menu_state->header_view ){ 
        menu_header_state = (zdj_menu_header_view_state_t*)menu_state->header_view->state; 
    }

    // Get the menu_item @ current scroll_index
    zdj_view_t * menu_item = zdj_menu_item_for_scroll_index( 
        menu_state->scroll_view, 
        menu_state->scroll_index 
    );

    zdj_menu_item_view_state_t * menu_item_state = NULL;
    if( menu_item ) {
        menu_item_state = (zdj_menu_item_view_state_t*)menu_item->state;
    }

    if( e->id == ZDJ_UI_CONTROL_JOG_ADJUST_0 ) { 
        if( menu_state->input_mode == ZDJ_MENU_INPUT_MODE_NORMAL ) {
            // Add the event's value to the scroll filter.
            // The scroll filter simulation runs in the draw loop,
            // therefore, menu_update_scroll happens during the draw loop.
            zdj_menu_view_add_scroll_filter_input( view, e->i_val );
            // Prevent views/menus below this one from getting jog wheel events
        } else if( menu_state->input_mode == ZDJ_MENU_INPUT_MODE_EDIT_ITEM_OPTIONS ) {
            // printf( "scroll item options\n" );
            zdj_view_t * item = zdj_menu_item_for_scroll_index( menu_state->scroll_view, menu_state->scroll_index );
            zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;
            zdj_menu_item_scroll_options( item, e->i_val );

            if( item_state->scroll_to_exit_edit_mode ) {
                if( item_state->edit_options_type == ZDJ_MENU_ITEM_OPTIONS_LIB_PLAYLIST ) {
                    if( round(item_state->edit_option_index) == 0 ||
                        round(item_state->edit_option_index) == 10
                    ) {
                        // printf( "exit edit on scroll\n" );  
                        menu_state->input_mode = ZDJ_MENU_INPUT_MODE_NORMAL;
                        if( item_state->exit_edit_mode ) {
                            item_state->exit_edit_mode( item );
                        }
                    }
                } else if( item_state->edit_options_type == ZDJ_MENU_ITEM_OPTIONS_DJ_PLAYLIST ) {
                    if( round(item_state->edit_option_index) == 0 ||
                        round(item_state->edit_option_index) == 11
                    ) {
                        // printf( "exit edit on scroll\n" );  
                        menu_state->input_mode = ZDJ_MENU_INPUT_MODE_NORMAL;
                        if( item_state->exit_edit_mode ) {
                            item_state->exit_edit_mode( item );
                        }
                    }
                } else if( item_state->edit_options_type == ZDJ_MENU_ITEM_OPTIONS_FILE ) {
                    if( round(item_state->edit_option_index) == 0 ||
                        round(item_state->edit_option_index) == 10
                    ) {
                        // printf( "exit edit on scroll\n" );  
                        menu_state->input_mode = ZDJ_MENU_INPUT_MODE_NORMAL;
                        if( item_state->exit_edit_mode ) {
                            item_state->exit_edit_mode( item );
                        }
                    }
                }
            }
        } else if( menu_state->input_mode == ZDJ_MENU_INPUT_MODE_EDIT_ITEM_POSITION ) {
            zdj_menu_view_move_item( view, menu_item, e->i_val );
        }
        
        e->blocked = true;
    
    // Press-scroll should jump to next section
    } else if( e->id == ZDJ_UI_CONTROL_JOG_ADJUST_1 ) { 
        // Find next/prev section in menu
        int target_index;
        bool should_scroll = false;
        if( e->i_val > 0 ) {
            should_scroll = _put_next_section_index( view, menu_state->scroll_index, &target_index );
        } else if( e->i_val < 0 ) { 
            should_scroll = _put_prev_section_index( view, menu_state->scroll_index, &target_index );
        }
        if( should_scroll ) { 
            zdj_menu_view_set_scroll_index( view, target_index );
        }

    // Release, mod-scroll, long-press, etc. should invoke hmi handler of menu_item @scroll_index
    } else if( e->id == ZDJ_UI_CONTROL_JOG_RELEASE_0 ||
        e->id == ZDJ_UI_CONTROL_JOG_RELEASE_1 ||
        e->id == ZDJ_UI_CONTROL_JOG_RELEASE_2 ||
        e->id == ZDJ_UI_CONTROL_JOG_ADJUST_1 ||
        e->id == ZDJ_UI_CONTROL_JOG_ADJUST_2 ||
        e->id == ZDJ_UI_CONTROL_JOG_PRESS_2
    ) {
        
        if( menu_state->scroll_index == ZDJ_BACK_INDEX ) {
            // printf( "handling back jog release\n" );
            // Blink the back btn
            menu_header_state->is_blinking = true;
            menu_header_state->blink_timer = 0;

            // Call the back btn handler
            if( menu_header_state && menu_header_state->handle_back ) {
                menu_header_state->handle_back( view );
            }
       
        } else {

            // Catch long-press-to-edit entry into edit mode
            if( menu_item_state->edit_enabled &&
                menu_state->edit_enabled && 
                menu_state->long_press_to_edit &&
                menu_state->input_mode == ZDJ_MENU_INPUT_MODE_NORMAL &&
                e->id == ZDJ_UI_CONTROL_JOG_PRESS_2 
            ) {
                // Enter edit mode
                menu_state->input_mode = ZDJ_MENU_INPUT_MODE_EDIT_ITEM_OPTIONS;
                menu_state->edit_item = menu_item;
                if( menu_item_state->enter_edit_mode ) {
                    menu_item_state->enter_edit_mode( menu_item );
                }
                

            // Check release-to-edit entry into edit
            } else if( menu_item_state->edit_enabled &&
                       menu_state->edit_enabled && 
                       e->id == ZDJ_UI_CONTROL_JOG_RELEASE_0 
            ) {
                
                if( menu_state->input_mode == ZDJ_MENU_INPUT_MODE_NORMAL &&            
                    !menu_state->long_press_to_edit 
                ) {
                    printf( "menu entering edit mode\n" );
                    // Enter edit mode if not currently editing
                    menu_state->input_mode = ZDJ_MENU_INPUT_MODE_EDIT_ITEM_OPTIONS;
                    menu_state->edit_item = menu_item;
                    // zdj_menu_item_enter_edit_mode( menu_item );
                    if( menu_item_state->enter_edit_mode ) {
                        menu_item_state->enter_edit_mode( menu_item );
                    }
                } else if ( menu_state->input_mode == ZDJ_MENU_INPUT_MODE_EDIT_ITEM_OPTIONS ) {
                    
                    if( menu_item_state->edit_action == ZDJ_MENU_ITEM_ACTION_START_MOVE ) {
                        // Call into handle_control_event set by front-end so it can
                        // enter menu item move mode with appropriate constraints
                        if( menu_item->handle_control_event ) {
                            menu_item->handle_control_event( menu_item, e );
                        }
                    } else {
                        
                        // Select edit option if currently editing
                        // Call into handle_control_event set by front-end
                        if( menu_item->handle_control_event ) {
                            menu_item->handle_control_event( menu_item, e );
                        }
                        menu_state->input_mode = ZDJ_MENU_INPUT_MODE_NORMAL;
                        if( menu_item_state->exit_edit_mode ) {
                            menu_item_state->exit_edit_mode( menu_item );
                        }
                    }
                } else if ( menu_state->input_mode == ZDJ_MENU_INPUT_MODE_EDIT_ITEM_POSITION ) {
                    // If move is selected, put menu into move item mode
                    // zdj_menu_item_exit_move_mode( menu_item );
                    // menu_state->input_mode = ZDJ_MENU_INPUT_MODE_EDIT_ITEM_OPTIONS;
                    menu_state->input_mode = ZDJ_MENU_INPUT_MODE_NORMAL;
                    // zdj_menu_item_exit_edit_mode( menu_item );
                    if( menu_item_state->exit_edit_mode ) {
                        menu_item_state->exit_edit_mode( menu_item );
                    }
                }
                
               
            } else if( e->id == ZDJ_UI_CONTROL_JOG_RELEASE_0 ) {
                // Call the menu item's select handler
                zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)menu_item->state;
                state->is_blinking = true;
                state->blink_timer = 0;
            
                // Call into handle_control_event set by front-end
                if( menu_item->handle_control_event ) {
                    menu_item->handle_control_event( menu_item, e );
                }
            }
        }

        // Prevent views/menus below this one from getting jog wheel events
        e->blocked = true;

    // Handle nav back button
    } else if( e->id == ZDJ_UI_CONTROL_NAV_RELEASE_0 && menu_header_state ) {
        // Blink the back btn
        menu_header_state->is_blinking = true;
        menu_header_state->blink_timer = 0;

        // Exit edit mode if we're in it
        // printf( "nav menu input mode: %d\n", menu_state->input_mode );
        if( menu_state->input_mode != ZDJ_MENU_INPUT_MODE_NORMAL ) {
            printf( "exit edit mode on close\n" );
        }

        // Call the back btn handler
        if( menu_header_state->handle_back ) {
            printf( "direct nav->back\n" );
            menu_header_state->handle_back( view );
        }

        // Prevent views/menus below this one from getting bav release
        e->blocked = true;
    } else {
        // printf( "sending non-standard\n" );
        // If menu accepts more than standard nav controls, send those in as well
        if( menu_state->scroll_index > -1 && menu_item && menu_item_state && menu_item_state->captures_all_events ) {
            if( menu_item->handle_control_event ) {
                menu_item->handle_control_event( menu_item, e );
            }
        }
    }

    // printf( "zdj_menu_handle_control done\n" );
}

void zdj_menu_view_set_scroll_index( zdj_view_t * menu_view, int index ) {
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)menu_view->state;
    menu_state->scroll_filter->position = index;
    _update_scroll( menu_view, index );
}

static void _update_scroll( zdj_view_t * menu_view, int new_scroll ) {
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)menu_view->state;
    zdj_scroll_view_state_t * scroll_state = (zdj_scroll_view_state_t*)menu_state->scroll_view->state;

    // printf( "_update_scroll: %d/%d of %d\n", new_scroll, menu_state->scroll_index, menu_state->item_count );

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
            if( !prev_menu_item ) { return; }
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
            scroll_point.y = new_menu_item->frame.y;
        } else if( menu_state->scroll_dir == ZDJ_HORIZONTAL ) {
            scroll_point.x = new_menu_item->frame.x;
            scroll_point.y = 0;
        }

        bool is_final_view;
        if( new_menu_item_state->scroll_index == 0 ||
            new_menu_item_state->scroll_index == menu_state->item_count-1 
        ) {
            // printf( "final view index: %d/%d\n", 
            //     new_menu_item_state->scroll_index, menu_state->item_count
            // );
            is_final_view = true;
        } else {
            // printf( "not final view: %d/%d\n", 
            //     new_menu_item_state->scroll_index, menu_state->item_count
            // );
            is_final_view = false;
        }

        zdj_scroll_view_to_view( 
            menu_state->scroll_view, 
            new_menu_item, 
            scroll_dir, 
            is_final_view, 
            true 
        );
    }

    // printf( "update scroll done\n" );
}

static void _deinit_state( zdj_view_t * view ) {
    zdj_menu_view_state_t * state = (zdj_menu_view_state_t*)view->state;
    free( state );
    view->state = NULL;
}

static bool _put_prev_section_index( zdj_view_t * menu, int current_index, int * scroll_target ) {
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)menu->state;
    zdj_view_t * menu_item = zdj_menu_view_item_at_scroll_index( menu, current_index )->prev;
    
    while( menu_item ) {
        if( menu_item->type == ZDJ_VIEW_MENU_SECTION ) {
            if( menu_item->prev && menu_item->prev->type == ZDJ_VIEW_MENU_ITEM ) { 
                zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)menu_item->prev->state;
                if( item_state ) {
                    *scroll_target = item_state->scroll_index;
                    return true;
                }
            }
        }
        menu_item = menu_item->prev;
    }

    return false;
}

static bool _put_next_section_index( zdj_view_t * menu, int current_index, int * scroll_target ) {
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)menu->state;
    zdj_view_t * menu_item = zdj_menu_view_item_at_scroll_index( menu, current_index );

    while( menu_item ) {
        if( menu_item->type == ZDJ_VIEW_MENU_SECTION ) {
            if( menu_item->next && menu_item->next->type == ZDJ_VIEW_MENU_ITEM ) { 
                zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)menu_item->next->state;
                if( item_state ) {
                    *scroll_target = item_state->scroll_index;
                    return true;
                }
            }
        }
        menu_item = menu_item->next;
    }

    return false;
}
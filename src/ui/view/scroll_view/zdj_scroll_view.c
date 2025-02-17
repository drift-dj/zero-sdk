#include <stdio.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/scroll_view/zdj_scroll_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

#define ZDJ_SCROLL_VIEW_MARGIN_V 7

void _zdj_scroll_view_debug_draw( zdj_view_t * view, zdj_view_clip_t * clip );
void _zdj_scroll_view_deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_scroll_view( 
    zdj_rect_t * frame
) {
    // Set up scrollview state instance
    zdj_scroll_view_state_t * scroll_view_state = calloc( 1, sizeof( zdj_scroll_view_state_t ) );

    // Build the scroll view
    zdj_view_t * scroll_view = zdj_new_view( frame );
    // scroll_view->draw = &_zdj_scroll_view_debug_draw;
    scroll_view->deinit_state = &_zdj_scroll_view_deinit_state;
    scroll_view->state = (void*)scroll_view_state;

    return scroll_view;
}

void _zdj_scroll_view_deinit_state( zdj_view_t * view ) {
    zdj_scroll_view_state_t * state = (zdj_scroll_view_state_t*)view->state;
    free( state );
    view->state = NULL;
}

void zdj_scroll_view_add_subview( zdj_view_t * scroll_view, zdj_view_t * subview ) {
    // Use normal view method to manage view stack
    zdj_add_subview( scroll_view, subview );
    
    // Do some state management based on new view
    zdj_scroll_view_state_t * state = (zdj_scroll_view_state_t*)scroll_view->state;

    // Update the scroll_size based on the newly-added view and the scroll direction
    if( state->scroll_dir == ZDJ_VERTICAL ) {
        state->scroll_size.y = subview->frame->y + subview->frame->h;
    } else if( state->scroll_dir == ZDJ_HORIZONTAL ) {
        state->scroll_size.x = subview->frame->x + subview->frame->w;
    }
}

void zdj_scroll_view_get_size( zdj_view_t * scroll_view, zdj_point_t * point ) {
    zdj_scroll_view_state_t * state = (zdj_scroll_view_state_t*)scroll_view->state;
    point->x = state->scroll_size.x;
    point->y = state->scroll_size.y;
}

// Note that this is super hacky.  We have to tell it the direction we're scrolling 
// and whether to_view is the final view in that direction.  Find something better.
// Remember the special menu case: scroll to 1st menu_item in list, with menu_section above it.
// Maybe branch menu scroll out to a separate specialized function.
void zdj_scroll_view_to_view( zdj_view_t * scroll_view, zdj_view_t * to_view, bool direction, bool is_final_view ) {
    zdj_scroll_view_state_t * state = (zdj_scroll_view_state_t*)scroll_view->state;

    if( state->scroll_dir == ZDJ_VERTICAL ) {

        // If we're scrolling down 
        if( direction ) {
            int win_bottom = (scroll_view->subview_clip->scroll_offset.y*-1) + scroll_view->frame->h;
            int to_view_bottom = to_view->frame->y + to_view->frame->h;

            // If item bottom falls within [margin] pixels of window bottom
            if( (win_bottom - to_view_bottom) < ZDJ_SCROLL_VIEW_MARGIN_V ) {
                // If view is last in list
                if( is_final_view ) {
                    // Move window so window bottom is 0 pixels from bottom of view
                    scroll_view->subview_clip->scroll_offset.y = to_view_bottom - scroll_view->frame->h;
                    scroll_view->subview_clip->scroll_offset.y *= -1;
                } else {
                    // View is not last in list
                    // Move window so window bottom is [margin] pixels from bottom of view
                    scroll_view->subview_clip->scroll_offset.y = (to_view_bottom + ZDJ_SCROLL_VIEW_MARGIN_V) - scroll_view->frame->h;
                    scroll_view->subview_clip->scroll_offset.y *= -1;
                }
            }

        // If we're scrolling up
        } else {
            int win_top = scroll_view->subview_clip->scroll_offset.y * -1;
            int to_view_top = to_view->frame->y;
            // If item is first in list
            if( is_final_view ) {
                // Move window to y=0
                scroll_view->subview_clip->scroll_offset.y = 0;
            } else if( (to_view_top - win_top) < ZDJ_SCROLL_VIEW_MARGIN_V ) {
                // Item top falls within [margin] pixels of top of window
                // Move window so window top is [margin] pixels from top of view
                scroll_view->subview_clip->scroll_offset.y = to_view_top - ZDJ_SCROLL_VIEW_MARGIN_V;
                scroll_view->subview_clip->scroll_offset.y *= -1;
            }
        }
    }
}

void zdj_scroll_view_to_point( zdj_view_t * scroll_view, zdj_point_t * point ) {
    zdj_scroll_view_state_t * scroll_view_state = (zdj_scroll_view_state_t*)scroll_view->state;
    // scroll_view->subview_clip->scroll_offset.x = point->x * -1;
    // scroll_view->subview_clip->scroll_offset.y = point->y * -1;
}

void zdj_scroll_view_to_ratio( zdj_view_t * scroll_view, zdj_point_t * ratio ) {

}

void zdj_scroll_view_by_point( zdj_view_t * scroll_view, zdj_point_t * point ) {

}

void zdj_scroll_view_by_ratio( zdj_view_t * scroll_view, zdj_point_t * ratio ) {

}

void zdj_scroll_view_by_int( zdj_view_t * scroll_view, int val ) {
    zdj_scroll_view_state_t * state = (zdj_scroll_view_state_t*)scroll_view->state;
    if( state->scroll_dir == ZDJ_VERTICAL ) {
        state->scroll_offset.y += val;
    } else if( state->scroll_dir == ZDJ_HORIZONTAL ) {
        state->scroll_offset.x += val;
    }
}

void _zdj_scroll_view_debug_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF440000 );
}
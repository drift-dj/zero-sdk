#include <stdio.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/scroll_view/zdj_scroll_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

zdj_view_t * zdj_new_scroll_view( 
    zdj_rect_t * frame
) {
    // Set up scrollview state instance
    zdj_scroll_view_state_t * scroll_view_state = calloc( 1, sizeof( zdj_scroll_view_state_t ) );

    // Build the scroll view
    zdj_view_t * scroll_view = zdj_new_view( frame );
    scroll_view->state = (void*)scroll_view_state;

    return scroll_view;
}

void zdj_free_scroll_view( zdj_view_t * scroll_view ) {

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

void zdj_scroll_view_to_point( zdj_view_t * scroll_view, zdj_point_t * point ) {
    zdj_scroll_view_state_t * scroll_view_state = (zdj_scroll_view_state_t*)scroll_view->state;
    scroll_view->subview_clip->scroll_offset.x = point->x;
    scroll_view->subview_clip->scroll_offset.y = point->y;
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
#include <stdio.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/scroll_view/zdj_scroll_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

void zdj_draw_scroll_view( zdj_view_t * scroll_view, zdj_view_clip_t * clip );

zdj_view_t * zdj_new_scroll_view( 
    zdj_rect_t * frame
) {
    // Set up scrollview state instance
    zdj_scroll_view_state_t * scroll_view_state = malloc( sizeof( zdj_scroll_view_state_t ) );

    // Build the scroll view
    zdj_view_t * scroll_view = zdj_view_stack_new_view( );
    scroll_view->draw = &zdj_draw_scroll_view;
    scroll_view->state = (void*)scroll_view_state;
    scroll_view->frame = frame;

    return scroll_view;
}

void zdj_draw_scroll_view( zdj_view_t * scroll_view, zdj_view_clip_t * clip ) { return; }

void zdj_free_scroll_view( zdj_view_t * scroll_view ) {

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
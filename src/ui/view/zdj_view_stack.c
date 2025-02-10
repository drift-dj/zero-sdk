#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/zdj_view_stack.h>
#include <zerodj/display/zdj_display.h>
#include <zerodj/hmi/zdj_hmi.h>
#include <zerodj/health/zdj_health_type.h>

zdj_view_t * zdj_view_stack_bottom;
zdj_view_t * zdj_view_stack_top;
zdj_rect_t view_stack_frame = {0,0,128,64};
static int zdj_view_stack_id = 0;

static zdj_view_t * zdj_view_stack_root_view;
zdj_view_t * _zdj_view_stack_top_subview_of( zdj_view_t * view );
zdj_view_t * _zdj_view_stack_bottom_subview_of( zdj_view_t * view );

zdj_view_t * zdj_view_stack_root( void ) {
    return zdj_view_stack_root_view;
}

void zdj_view_stack_init( void ) {
    zdj_view_stack_root_view = zdj_view_stack_new_view( );

    zdj_view_clip_t * clip = calloc( 1, sizeof( zdj_view_clip_t ) );
    clip->src.w = clip->dst.w = 128;
    clip->src.h = clip->dst.h = 64;
    zdj_view_stack_root_view->subview_clip = clip;
}

void zdj_view_stack_deinit( void ) {
    
}

void zdj_view_stack_update( void ) {
    zdj_view_t * view;
    zdj_hmi_event_t * event;

    // Pass events into root view's subviews (top-down)
    zdj_view_stack_handle_events( _zdj_view_stack_top_subview_of(zdj_view_stack_root( )) );

    // Draw root view's subviews (bottom-up)
    zdj_view_stack_draw( _zdj_view_stack_bottom_subview_of(zdj_view_stack_root( )), zdj_view_stack_root( )->subview_clip );
}

void zdj_view_stack_clear_screen( void ) {
    boxColor( zdj_renderer( ), 0, 0, 128, 64, 0xFF000000 );
}

// Walk backward thru a view's stack of siblings.
// Note that this only walks siblings, it doesn't recurse into subviews.
// This function must be called on the LAST view in the linked list of subviews.
void zdj_view_stack_handle_events( zdj_view_t * view ) {
    if( view->handle_hmi_event ) {
        zdj_hmi_event_t * event = zdj_hmi_event_base;
        while( event ) {
            view->handle_hmi_event( view, event );
            event = event->next;
        }
    }
    if( view->prev ) {
        zdj_view_stack_handle_events( view->prev );
    }
}

// This is a recursive draw, first getting a clip frame from the view's draw func,
// then recursing into the view's subviews, finally moving to the next sibling view.
void zdj_view_stack_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // Update metrics passed to subviews based on that clipping.
    view->update_subview_clip( view, clip );
    // Draw this view's pixels
    if( view->is_visible ) {
        view->draw( view, view->subview_clip );
        // Recurse into the view's subviews, passing clipped metrics.
        if( view->subviews ) {
            zdj_view_stack_draw( view->subviews, view->subview_clip );
        }
    }
    // Step up the view stack and continue drawing
    if( view->next ) {
        zdj_view_stack_draw( view->next, clip );
    }
}

zdj_view_t * zdj_view_stack_new_view( void ) {
    zdj_view_t * view = calloc( 1, sizeof( zdj_view_t ) );
    view->id = zdj_view_stack_id++;
    view->subview_clip = calloc( 1, sizeof( zdj_view_clip_t ) );
    // Default metrics update -- re-define in front-end layer to alter
    view->update_subview_clip = &zdj_view_stack_update_subview_clip;
    return view;
}

void zdj_view_add_subview( zdj_view_t * view, zdj_view_t * subview ) {
    if( view->subviews ) {
        zdj_view_t * top_subview = _zdj_view_stack_top_subview_of( view );
        top_subview->next = subview;
        subview->prev = top_subview;
    } else {
        view->subviews = subview;
    }
}

// Perform some arithmetic on clipping geometry of view's subviews.
// Generate:
//  - an offset from this view's origin to superview's origin
//  - a rect clipped within superview's clipped rect
//  - a rect of screen-space coords to draw view's pixels
void zdj_view_stack_update_subview_clip( zdj_view_t * subview, zdj_view_clip_t * superview_clip ) {
    zdj_view_clip_t * subview_clip = subview->subview_clip;

    // Prep is_visible for view out-of-bounds state
    subview->is_visible = true;
    
    // Build screen x/y for subview's origin
    subview_clip->screen.x = subview->frame->x + superview_clip->screen.x + superview_clip->scroll_offset.x;
    subview_clip->screen.y = subview->frame->y + superview_clip->screen.y + superview_clip->scroll_offset.y;

    // Horizontal arithmetic
    // Clip left draw edge to furthest right pixel of subview/superview frame x.
    subview_clip->dst.x = (int)fmax( subview_clip->screen.x, superview_clip->dst.x );
    subview_clip->src.x = (int)fmax( 0, subview_clip->dst.x-subview_clip->screen.x );
    // Clip right draw edge to narrower pixel val of subview/superview frame widths.
    int sub_dst_lx = subview_clip->screen.x; // screen x of left edge of subview's frame
    int sup_dst_lx = superview_clip->dst.x; // screen x of left edge of superview's frame
    int sub_dst_rx = subview_clip->screen.x+subview->frame->w; // screen x of right edge of subview's frame
    int sup_dst_rx = superview_clip->dst.x+superview_clip->dst.w; // screen x of right edge of superview's frame
    if( (sub_dst_rx <= sup_dst_rx) && (sub_dst_rx >= sup_dst_lx) ) { 
        // Subview right edge falls inside superview screen rect.
        if( sub_dst_lx > sup_dst_lx ) {
            // Entire subview falls within superview screen rect
            subview_clip->dst.w = subview->frame->w;
            subview_clip->src.w = subview_clip->dst.w;
        } else {
            // Subview left edge falls to left of superview screen rect
            // Make width clipped to superview screen rect
            subview_clip->dst.w = subview->frame->w - (subview_clip->dst.x-subview_clip->screen.x);
            subview_clip->src.w = subview_clip->dst.w;
        }
    } else if( (sub_dst_rx > sup_dst_rx) && (sub_dst_lx <= sup_dst_rx) ) { 
        // Subview right edge falls right (outside) of superview screen rect,
        // but subview left edge falls inside or left of superview screen rect.
        // Make width clipped to superview screen rect
        if( sub_dst_lx > sup_dst_lx ) {
            // Subview left edge falls within superview screen rect
            subview_clip->dst.w = (superview_clip->dst.w+superview_clip->dst.x) - subview_clip->screen.x;
            subview_clip->src.w = subview_clip->dst.w;
        } else {
            // Subview left edge falls to left of superview screen rect
            // Make width clipped to superview screen rect
            subview_clip->dst.w = superview_clip->dst.w;
            subview_clip->src.w = subview_clip->dst.w;
        }
    } else {
        // Subview doesn't appear within superview screen rect - Force no draw
        subview_clip->dst.w = 0;
        subview->is_visible = false;
    }

    // Vertical arithmetic
    // Clip top draw edge to lowest pixel of subview/superview frame y.
    subview_clip->dst.y = (int)fmax( subview_clip->screen.y, superview_clip->dst.y );
    subview_clip->src.y = (int)fmax( 0, subview_clip->dst.y-subview_clip->screen.y );
    // Clip bottom draw edge to higher pixel val of subview/superview frame heights.
    int sub_dst_ty = subview_clip->screen.y; // screen y of top edge of subview's frame
    int sup_dst_ty = superview_clip->dst.y; // screen y of top edge of superview's frame
    int sub_dst_by = subview_clip->screen.y+subview->frame->h; // screen y of bottom edge of subview's frame
    int sup_dst_by = superview_clip->dst.y+superview_clip->dst.h; // screen y of bottom edge of superview's frame
    if( (sub_dst_by <= sup_dst_by) && (sub_dst_by >= sup_dst_ty) ) { 
        // Subview to edge falls inside superview screen rect.
        if( sub_dst_ty > sup_dst_ty ) {
            // Entire subview falls within superview screen rect
            subview_clip->dst.h = subview->frame->h;
            subview_clip->src.h = subview_clip->dst.h;
        } else {
            // Subview top edge falls above top of superview screen rect
            // Make width clipped to superview screen rect
            // subview_clip->dst.h = subview->frame->y + subview->frame->h;
            subview_clip->dst.h = subview->frame->h - (subview_clip->dst.y - subview_clip->screen.y);
            subview_clip->src.h = subview_clip->dst.h;
        }
    } else if( (sub_dst_by > sup_dst_by) && (sub_dst_ty <= sup_dst_by) ) { 
        // Subview bottom edge falls below (outside) superview screen rect,
        // but subview top edge falls inside or above top of superview screen rect.
        // Make height clipped to superview screen rect
        if( sub_dst_ty > sup_dst_ty ) {
            // Subview top edge falls within superview screen rect
            // subview_clip->dst.h = (superview_clip->dst.h+superview_clip->src.y) - subview->frame->y;
            subview_clip->dst.h = (superview_clip->dst.h+superview_clip->dst.y) - subview_clip->screen.y;
            subview_clip->src.h = subview_clip->dst.h;
        } else {
            // Subview top edge falls above superview screen rect
            // Make hieght clipped to superview screen rect
            subview_clip->dst.h = superview_clip->dst.h;
            subview_clip->src.h = subview_clip->dst.h;
        }
    } else {
        // Subview doesn't appear within superview screen rect - Force no draw
        subview_clip->dst.h = 0;
        subview->is_visible = false;
    }    
}

// Walk to end of linked subviews list and return the last view.
// Return NULL if there are no subviews.
zdj_view_t * _zdj_view_stack_top_subview_of( zdj_view_t * view ) {
    zdj_view_t * _view = view->subviews;
    while( _view ) {
        if( _view->next ) {
            _view = _view->next;
        } else {
            return _view;
        }
    }
    return NULL;
}

// Return first subview in linked list.
// Return NULL if there are no subviews.
zdj_view_t * _zdj_view_stack_bottom_subview_of( zdj_view_t * view ) {
    return view->subviews;
}

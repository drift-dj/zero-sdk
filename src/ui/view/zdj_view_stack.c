#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/controls/zdj_controls.h>
#include <zerodj/system/debug/zdj_debug.h>
#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/perf/zdj_perf.h>
#include <zerodj/system/screencap/zdj_screencap.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/panel/zdj_ui_panel.h>
#include <zerodj/ui/view/zdj_view_stack.h>
#include <zerodj/ui/widget/zdj_ui_widget.h>
#include <zerodj/system/display/zdj_display.h>
#include <zerodj/health/zdj_health_type.h>

zdj_view_t * zdj_view_stack_bottom;
zdj_view_t * zdj_view_stack_top;
zdj_rect_t view_stack_frame = {0,0,128,64};

zdj_view_t * zdj_view_stack_root_view = NULL;
zdj_view_t * zdj_view_stack_panel_view = NULL;
zdj_view_t * zdj_view_stack_widget_view = NULL;

static void _zdj_view_stack_draw( zdj_view_t * view );

zdj_view_t * zdj_root_view( void ) {
    return zdj_view_stack_root_view;
}

zdj_view_t * zdj_panel_view( void ) {
    return zdj_view_stack_panel_view;
}

zdj_view_t * zdj_widget_view( void ) {
    return zdj_view_stack_widget_view;
}

void zdj_view_stack_init( void ) {
    zdj_view_stack_root_view = zdj_new_view( &view_stack_frame );
    zdj_view_stack_root_view->subview_clip.src.w = zdj_view_stack_root_view->subview_clip.dst.w = 128;
    zdj_view_stack_root_view->subview_clip.src.h = zdj_view_stack_root_view->subview_clip.dst.h = 64;

    zdj_view_stack_panel_view = zdj_new_view( &view_stack_frame );
    zdj_view_stack_panel_view->subview_clip.src.w = zdj_view_stack_panel_view->subview_clip.dst.w = 128;
    zdj_view_stack_panel_view->subview_clip.src.h = zdj_view_stack_panel_view->subview_clip.dst.h = 64;
    zdj_view_stack_panel_view->tag = 33;

    zdj_view_stack_widget_view = zdj_new_view( &view_stack_frame );
    zdj_view_stack_widget_view->subview_clip.src.w = zdj_view_stack_widget_view->subview_clip.dst.w = 128;
    zdj_view_stack_widget_view->subview_clip.src.h = zdj_view_stack_widget_view->subview_clip.dst.h = 64;

    zdj_delete_stack = NULL;
}

void zdj_view_stack_min_init( void ) {
    zdj_view_stack_root_view = zdj_new_view( &view_stack_frame );
    zdj_view_stack_root_view->subview_clip.src.w = zdj_view_stack_root_view->subview_clip.dst.w = 128;
    zdj_view_stack_root_view->subview_clip.src.h = zdj_view_stack_root_view->subview_clip.dst.h = 64;

    zdj_delete_stack = NULL;
}

void zdj_view_stack_deinit( void ) {
    
}

void zdj_view_stack_update( void ) {
    zdj_view_t * view;
    // Open a tag for the UI cycle
    // zdj_perf_tag_t * tag;
    // if( zdj_perf_enabled( ) ) {
    //     tag = zdj_new_perf_tag_for_thread( ZDJ_SYSTEM_THREAD_UI );
    //     tag->name = ZDJ_PERF_TAG_UI_CYCLE;
    //     tag->start = zdj_perf_time( );
    // }

    // If there are unhandled events in the event ring buffer...
    int start_ind = zdj_ui_event_buf_read;
    int end_ind = zdj_ui_event_buf_write;
    if( start_ind != end_ind ) {
        // Invoke any special commands before sending remaining events into stack
        zdj_view_stack_handle_special_events( start_ind, end_ind );
        // ...send events into views for handling.
        zdj_view_stack_handle_events( start_ind, end_ind, zdj_panel_view( ) );
        zdj_view_stack_handle_events( start_ind, end_ind, zdj_widget_view( ) );
        zdj_view_stack_handle_events( 
            start_ind, end_ind, zdj_view_stack_top_subview_of( zdj_root_view( ) ) 
        );
        // Update the event buf's read head so we don't re-process these events
        zdj_ui_event_buf_read = end_ind;
    }

    // Draw views from lowest to highest.
    zdj_new_view_count = 0;
    _zdj_view_stack_draw( zdj_root_view( ) );
    _zdj_view_stack_draw( zdj_panel_view( ) );
    _zdj_view_stack_draw( zdj_widget_view( ) );

    zdj_view_count = zdj_new_view_count;

    // If screen_cap is armed, grab it here after all drawing is complete.
    if( zdj_screen_cap_armed ) { zdj_update_screencap( ); }

    // Delete everything in the delete stack
    zdj_view_t * delete_view = zdj_delete_stack;
    while( delete_view ) {
        delete_view->deinit( delete_view );
        // Deinit shifts the delete stack forward
        delete_view = zdj_delete_stack;
    }
    zdj_delete_stack = NULL;
    
    // if( zdj_perf_enabled( ) ) { tag->end = zdj_perf_time( ); }
}

void _zdj_view_stack_draw( zdj_view_t * view ) {
    // printf( "_zdj_view_stack_draw\n" );
    if( !view ){ return; }
    // Draw view's subviews (bottom-up)
    zdj_view_t * bottom_subview = zdj_view_stack_bottom_subview_of( view );
    if( bottom_subview ) { zdj_view_stack_draw( bottom_subview, &view->subview_clip ); }
    // printf( "_zdj_view_stack_draw done\n" );
}

void zdj_view_stack_clear_screen( void ) {
    boxColor( zdj_renderer( ), 0, 0, 128, 64, 0xFF000000 );
}

// Look for a matching special event and send it to the linked handler.
void zdj_view_stack_handle_special_events( int start_ind, int end_ind ) {
    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_HANDLE_EVENT;
    int i = start_ind;
    while ( i != end_ind ) {
        i++;
        i %= ZDJ_CONTROL_EVENT_BUF_LEN; // Loop i in ring buffer
        // printf( "zdj_view_stack_handle_special_events ui_event_buf[ %d ].id:%d\n", i, zdj_ui_event_buf[ i ].id );
        // Loop thru all 5 special event slots
        for( int e=0; e<5; e++ ) {
            if( zdj_ui_event_buf[ i ].id == zdj_special_control_handlers[ e ].id &&
                zdj_special_control_handlers[ e ].cb 
            ) {
                zdj_special_control_handlers[ e ].cb( &zdj_ui_event_buf[ i ] );
            }
        }
    }
    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_UNCLAIMED;
}

// Walk top-to-bottom thru a view's stack of siblings.
// Note that this only walks siblings, it doesn't recurse into subviews.
// This function must be called on the LAST view in the linked list of subviews.
// Process events from the circular buffer, as specified by start/end_ind.
void zdj_view_stack_handle_events( int start_ind, int end_ind, zdj_view_t * view ) {
    if( !view ) { return; }

    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_HANDLE_EVENT;
    // Get a prev ref before processing view's events in case it gets deleted during processing.
    zdj_view_t * view_prev = view->prev;
    int i = start_ind;
    while ( i != end_ind ) {
        i++;
        i %= ZDJ_CONTROL_EVENT_BUF_LEN; // Loop i in ring buffer
        // printf( "zdj_view_stack_handle_events view: %p ui_event_buf[ %d ].id:%d\n", view, i, zdj_ui_event_buf[ i ].id );
        // Step thru ring buffer, passing each event down into view's subviews.
        if( view->handle_control_event ) {
            view->handle_control_event( view, &zdj_ui_event_buf[ i ] );
        }
    }
    if( view_prev ) {
        zdj_view_stack_handle_events( start_ind, end_ind, view_prev );
    }

    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_UNCLAIMED;
}

// This is a recursive draw, first getting a clip frame from the view's draw func,
// then recursing into the view's subviews, finally moving to the next sibling view.
void zdj_view_stack_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // Build view count by counting every draw invocation.
    zdj_new_view_count++;
    // printf( "zdj_view_stack_draw %p\n", view );
    // Claim any errors for debugging.
    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_VIEW_DRAW;

    // Update anim before updating subview clip
    if( view->anim && view->anim->alive && view->anim->update_fn ){ 
        ((anim_update_t)view->anim->update_fn)( view->anim, view ); 
    }

    // printf( "zdj_view_stack_draw 0 %p\n", view );
    // Update metrics passed to subviews based on that clipping.
    view->update_subview_clip( view, clip );
    // Draw this view's pixels
    if( view->is_visible ) {
        if( !view->is_deleting && view->draw ) {
            // printf( "zdj_view_stack_draw 1 %p\n", view );
            view->draw( view, &view->subview_clip );
        }

        // Recurse into the view's subviews, passing clipped metrics.
        // Note, during draw(), view may have been moved to the delete stack.
        // Be sure we're not drawing the delete stack here.
        if( !view->is_deleting && view->subviews ) {
            // printf( "zdj_view_stack_draw -> subviews %p\n", view );
            zdj_view_stack_draw( view->subviews, &view->subview_clip );
        }
    }

    // printf( "zdj_view_stack_draw 2 %p\n", view );
    // Step up the view stack and continue drawing
    if( view->next ) {
        zdj_view_stack_draw( view->next, clip );
    }

    // printf( "zdj_view_stack_draw 3 %p\n", view );
    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_UNCLAIMED;
}

// Perform some arithmetic on clipping geometry of view's subviews.
// Generate:
//  - an offset from this view's origin to superview's origin
//  - a rect clipped within superview's clipped rect
//  - a rect of screen-space coords to draw view's pixels
void zdj_view_stack_update_subview_clip( zdj_view_t * subview, zdj_view_clip_t * superview_clip ) {
    zdj_view_clip_t * subview_clip = &subview->subview_clip;

    // Prep is_visible for view out-of-bounds state
    subview->is_visible = true;
    
    // Build screen x/y for subview's origin
    subview_clip->screen.x = round(subview->frame.x + superview_clip->screen.x + superview_clip->scroll_offset.cur_x);
    subview_clip->screen.y = round(subview->frame.y + superview_clip->screen.y + superview_clip->scroll_offset.cur_y);

    // Horizontal arithmetic
    // Clip left draw edge to furthest right pixel of subview/superview frame x.
    subview_clip->dst.x = round(fmax( subview_clip->screen.x, superview_clip->dst.x ));
    subview_clip->src.x = round(fmax( 0, subview_clip->dst.x-subview_clip->screen.x ));
    // Clip right draw edge to narrower pixel val of subview/superview frame widths.
    int sub_dst_lx = subview_clip->screen.x; // screen x of left edge of subview's frame
    int sup_dst_lx = superview_clip->dst.x; // screen x of left edge of superview's frame
    int sub_dst_rx = subview_clip->screen.x+subview->frame.w; // screen x of right edge of subview's frame
    int sup_dst_rx = superview_clip->dst.x+superview_clip->dst.w; // screen x of right edge of superview's frame
    if( (sub_dst_rx <= sup_dst_rx) && (sub_dst_rx >= sup_dst_lx) ) { 
        // Subview right edge falls inside superview screen rect.
        if( sub_dst_lx > sup_dst_lx ) {
            // Entire subview falls within superview screen rect
            subview_clip->dst.w = subview->frame.w;
            subview_clip->src.w = subview_clip->dst.w;
        } else {
            // Subview left edge falls to left of superview screen rect
            // Make width clipped to superview screen rect
            subview_clip->dst.w = subview->frame.w - (subview_clip->dst.x-subview_clip->screen.x);
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
    subview_clip->dst.y = round(fmax( subview_clip->screen.y, superview_clip->dst.y ));
    subview_clip->src.y = round(fmax( 0, subview_clip->dst.y-subview_clip->screen.y ));
    // Clip bottom draw edge to higher pixel val of subview/superview frame heights.
    int sub_dst_ty = subview_clip->screen.y; // screen y of top edge of subview's frame
    int sup_dst_ty = superview_clip->dst.y; // screen y of top edge of superview's frame
    int sub_dst_by = subview_clip->screen.y+subview->frame.h; // screen y of bottom edge of subview's frame
    int sup_dst_by = superview_clip->dst.y+superview_clip->dst.h; // screen y of bottom edge of superview's frame
    if( (sub_dst_by <= sup_dst_by) && (sub_dst_by >= sup_dst_ty) ) { 
        // Subview to edge falls inside superview screen rect.
        if( sub_dst_ty > sup_dst_ty ) {
            // Entire subview falls within superview screen rect
            subview_clip->dst.h = ceil(subview->frame.h);
            subview_clip->src.h = subview_clip->dst.h;
        } else {
            // Subview top edge falls above top of superview screen rect
            // Make width clipped to superview screen rect
            // subview_clip.dst.h = subview->frame.y + subview->frame.h;
            subview_clip->dst.h = ceil(subview->frame.h - (subview_clip->dst.y - subview_clip->screen.y));
            subview_clip->src.h = subview_clip->dst.h;
        }
    } else if( (sub_dst_by > sup_dst_by) && (sub_dst_ty <= sup_dst_by) ) { 
        // Subview bottom edge falls below (outside) superview screen rect,
        // but subview top edge falls inside or above top of superview screen rect.
        // Make height clipped to superview screen rect
        if( sub_dst_ty > sup_dst_ty ) {
            // Subview top edge falls within superview screen rect
            // subview_clip.dst.h = (superview_clip->dst.h+superview_clip->src.y) - subview->frame.y;
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
zdj_view_t * zdj_view_stack_top_subview_of( zdj_view_t * view ) {
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
zdj_view_t * zdj_view_stack_bottom_subview_of( zdj_view_t * view ) {
    return view->subviews;
}

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/font/zdj_font.h>
#include <zerodj/ui/view/zdj_view_stack.h>
#include <zerodj/display/zdj_display.h>
#include <zerodj/hmi/zdj_hmi.h>
#include <zerodj/health/zdj_health_type.h>

SDL_Surface* zdj_display_surface;
uint32_t * zdj_ui_pixels = NULL;
static int zdj_view_stack_id = 0;
zdj_view_t * zdj_delete_stack;

void _zdj_view_deinit( struct zdj_view_t * view );
void _zdj_view_update_transit_anim( struct zdj_view_t * view );

void zdj_ui_init( void ) {
    // Grab the display memory
    zdj_display_init( );
 
    // Init the HMI event system
    zdj_hmi_init( );

    // Bringup SDL - exit on fail
    int err = SDL_Init( SDL_INIT_VIDEO );
    if( err != 0 ) {
        printf( "Zero failed to init graphics lib: %s\n", SDL_GetError( ) );
        exit( ZDJ_HEALTH_STATUS_SDL_FAILED );
    }

    zdj_display_surface = SDL_CreateRGBSurface( 0, ZDJ_SCREEN_W, ZDJ_SCREEN_H, 32, 0, 0, 0, 0 );
    zdj_display_renderer = SDL_CreateSoftwareRenderer( zdj_display_surface );
    zdj_ui_pixels = zdj_display_surface->pixels;
    if( !zdj_display_renderer ) {
        printf( "Zero failed to init renderer... exiting\n" );
        exit( ZDJ_HEALTH_STATUS_SDL_FAILED );
    }

    err = zdj_font_init( );
    if( err != 0 ) {
        printf( "Zero failed to init system fonts... exiting\n" );
        exit( ZDJ_HEALTH_STATUS_MISSING_GFX_RESOURCE );
    }

    err = zdj_ui_asset_init( );
    if( err != 0 ) {
        printf( "Zero failed to init system assets... exiting\n" );
        exit( ZDJ_HEALTH_STATUS_MISSING_GFX_RESOURCE );
    }

    // Bringup the display stack
    zdj_view_stack_init( ); 
}

void zdj_ui_deinit( void ) {
    SDL_Quit( );
}

void zdj_ui_update( void ) {
    // Get a fresh set of events from the M7 HMI system
    zdj_hmi_pull_m7_events( true );

    // Clear the screen
    zdj_view_stack_clear_screen( );

    // Update the view stack
    zdj_view_stack_update( );

    // Push the output pixels to the M7 core's memory
    zdj_display_m7_push( );

    // Trigger the HMI event system to post-process the event stack
    zdj_hmi_clear_events( );
}

void zdj_ui_start_events( void ) {
    // Pull and discard any cached M7 events
    zdj_hmi_pull_m7_events( true );
    zdj_hmi_clear_events( );

    // Tell M7 to start scanning HMI for new events
    zdj_hmi_activate( );
}

void zdj_ui_stop_events( void ) {
    zdj_hmi_deactivate( );
}

zdj_view_t * zdj_new_view( zdj_rect_t * frame ) {
    zdj_view_t * view = calloc( 1, sizeof( zdj_view_t ) );
    view->id = zdj_view_stack_id++;
    view->deinit = &_zdj_view_deinit;
    
    // Default metrics update -- re-define in front-end layer to alter
    view->subview_clip = calloc( 1, sizeof( zdj_view_clip_t ) );
    view->update_subview_clip = &zdj_view_stack_update_subview_clip;

    // Use initializer frame if available
    view->frame = calloc( 1, sizeof( zdj_rect_t ) );
    if( frame ) {
        view->frame->x = frame->x;
        view->frame->y = frame->y;
        view->frame->w = frame->w;
        view->frame->h = frame->h;
    }

    // Init transition anim stuff
    view->anim = NULL;
    view->in_anim = NULL;
    view->out_anim = NULL;

    return view;
}

void zdj_add_subview( zdj_view_t * view, zdj_view_t * subview ) {
    if( view->subviews ) {
        zdj_view_t * top_subview = zdj_view_stack_top_subview_of( view );
        top_subview->next = subview;
        subview->prev = top_subview;
    } else {
        view->subviews = subview;
    }
    view->subview_count++;
}

void zdj_add_subview_behind( zdj_view_t * view, zdj_view_t * target_subview, zdj_view_t * new_subview ) {
    if( target_subview->prev ) {
        zdj_view_t * old_prev = target_subview->prev;
        old_prev->next = new_subview;
        target_subview->prev = new_subview;
        new_subview->prev = old_prev;
        new_subview->next = target_subview;
    } else {
        target_subview->prev = new_subview;
        new_subview->next = target_subview;
        view->subviews = new_subview;
    }
    view->subview_count++;
}

void zdj_add_bottom_subview_to( zdj_view_t * view, zdj_view_t * new_subview ) {
    if( view->subviews ) {
        view->subviews->prev = new_subview;
        new_subview->next = view->subviews;
    }
    view->subviews = new_subview;
    view->subview_count++;
}

void zdj_remove_subview_of( zdj_view_t * view, zdj_view_t * subview ) {
    // If this is the base subview, null subviews out
    if( subview->next && subview->prev ) {
        // If we're in the middle of a linked list, splice the subview out
        subview->next->prev = subview->prev;
        subview->prev->next = subview->next;
    } else if( subview->next ) {
        // If we're at the beginning of the linked list,
        // subview.next becomes head of superview's subviews list.
        subview->next->prev = NULL;
        view->subviews = subview->next;
    } else if( subview->prev ) {
        // If we're at the end of the linked list, null out the new end.
        subview->prev->next = NULL;
    }
    view->subview_count--;
    zdj_delete_view( subview );
}

void zdj_remove_all_subviews_of( zdj_view_t * view ) {
    zdj_view_t * subview = view->subviews;
    while( subview ) {
        zdj_view_t * old_subview = subview;
        subview = subview->next;
        zdj_delete_view( old_subview );
    }
    view->subviews = NULL;
    view->subview_count = 0;
}

bool zdj_subview_exists_in( zdj_view_t * view, zdj_view_t * target_subview ) {
    bool target_found = false;
    zdj_view_t * subview = zdj_view_stack_top_subview_of( view );
    while( subview ) {
        if( subview == target_subview ) { 
            target_found = true; 
            break; 
        }   
        subview = subview->prev;
    }
    return target_found;
}

void zdj_push_subview( zdj_view_t * view, zdj_view_t * subview, bool animated ) {
    // Grab a ref to the top subview so we can make it disappear
    zdj_view_t * old_subview = zdj_view_stack_top_subview_of( view );
    // Add the new subview to the top
    zdj_add_subview( view, subview );

    // Show new subview
    if( subview->in_anim ) {
        subview->in_anim->cb_fn = NULL;
        if( subview->out_anim ) { subview->out_anim->cb_fn = NULL; }
        ((anim_init_t)subview->in_anim->init_fn)( subview->in_anim, subview );
        subview->in_anim->view = subview;
        subview->in_anim->superview = view;
        if( !animated ) { subview->in_anim->frame = subview->in_anim->frames; }
        subview->anim = subview->in_anim;
    }

    if( old_subview && old_subview->out_anim ) {
        ((anim_init_t)old_subview->out_anim->init_fn)( old_subview->out_anim, old_subview );
        if( !animated ) { old_subview->out_anim->frame = old_subview->out_anim->frames; }
        old_subview->anim = old_subview->out_anim;
    }
}

void zdj_push_subview_behind( zdj_view_t * view, zdj_view_t * target_subview, zdj_view_t * new_subview, bool animated ) {
    // Add the subview into the correct index
    zdj_add_subview_behind( view, target_subview, new_subview );

    // Pop down to the target subview
    zdj_pop_to_subview_of( view, new_subview, animated );
}

// Remove the top subview of a view
void zdj_pop_subview_of( zdj_view_t * view, bool animated ) {
    // Get top subview to hide
    zdj_view_t * top_subview = zdj_view_stack_top_subview_of( view );
    // Get next highest subview to show
    zdj_view_t * prev_subview = top_subview->prev;
    if( top_subview ) {

        if( top_subview->out_anim ) {
            ((anim_init_t)top_subview->out_anim->init_fn)( top_subview->out_anim, top_subview );
            top_subview->out_anim->view = top_subview;
            top_subview->out_anim->superview = view;
            top_subview->out_anim->cb_fn = &zdj_remove_subview_of; // Delete subview after anim
            if( !animated ) { top_subview->out_anim->frame = top_subview->out_anim->frames; }
            top_subview->anim = top_subview->out_anim;
        } else {
            zdj_remove_subview_of( view, top_subview );
        }
        
        if( prev_subview && prev_subview->in_anim ) {
            ((anim_init_t)prev_subview->in_anim->init_fn)( prev_subview->in_anim, prev_subview );
            if( !animated ) { prev_subview->in_anim->frame = prev_subview->in_anim->frames; }
            prev_subview->anim = prev_subview->in_anim;
        }
    }
}

// Remove subviews to reveal a given subview (if it exists)
void zdj_pop_to_subview_of( zdj_view_t * view, zdj_view_t * target_subview, bool animated ) {
    int pop_count = 0;
    bool target_found = false;
    // Count number of subviews to pop to reveal target subview
    zdj_view_t * subview = zdj_view_stack_top_subview_of( view );
    while( subview ) {
        printf( "subview: %p, targ: %p\n", subview, target_subview );
        if( subview == target_subview ) { 
            printf( "found: %p\n", subview );
            target_found = true; 
            break; 
        }   
        pop_count++;
        subview = subview->prev;
    }
    if( target_found ) { 
        printf( "pop_to_subview: %d views\n", pop_count );
        zdj_pop_n_subviews_of( view, pop_count, animated );
    }

}  

// Remove the top n subviews of a view
void zdj_pop_n_subviews_of( zdj_view_t * view, int count, bool animated ) {
    printf( "zdj_pop_n_subviews_of: %d/%d\n", count, view->subview_count );
    // Confirm there are n subviews to hide + 1 to reveal
    if( view->subview_count < count ) { return; }
    
    // Animate out anything from top view down to n
    zdj_view_t * subview = zdj_view_stack_top_subview_of( view );
    printf( "subview: %p, %p\n", subview, subview->out_anim );
    for( int n=0; n<count; n++ ) {
        if( subview ) {
            if( subview->out_anim ) {
                printf( "animating out: %p\n", subview );
                ((anim_init_t)subview->out_anim->init_fn)( subview->out_anim, subview );
                subview->out_anim->view = subview;
                subview->out_anim->superview = view;
                subview->out_anim->cb_fn = &zdj_remove_subview_of; // Delete subview after anim
                if( !animated ) { subview->out_anim->frame = subview->out_anim->frames; }
                subview->anim = subview->out_anim;
            } else {
                zdj_remove_subview_of( view, subview );
            }
            subview = subview->prev;;
        }
    }

    // Animate in target subview
    // Show next subview down if available
    if( subview && subview->in_anim ) {
        ((anim_init_t)subview->in_anim->init_fn)( subview->in_anim, subview );
        if( !animated ) { subview->in_anim->frame = subview->in_anim->frames; }
        subview->anim = subview->in_anim;
    }
}

// Prepare a view to be deleted at the end of this update cycle.
// View's next/prev links must already be cleaned up before sending it here or
// bad stuff probably happens.
// Note that deinit isn't called here.
// deinit() is called when view_stack_update() traverses the delete stack later. 
void zdj_delete_view( zdj_view_t * view ) {
    view->next = NULL;
    view->prev = NULL;
    if( !zdj_delete_stack ) {
        zdj_delete_stack = view;
    } else {
        zdj_view_t * delete_tip = zdj_delete_stack;
        while( delete_tip ) {
            if( delete_tip->next ) {
                delete_tip = delete_tip->next;
                continue;
            } else {
                break;
            }
        }
        delete_tip->next = view;
    }
}

void _zdj_view_deinit( zdj_view_t * view ) {
    // Deinit subviews
    zdj_view_t * subview = view->subviews;
    while( subview ) {
        zdj_view_t * old_subview = subview;
        subview = subview->next;
        zdj_delete_view( old_subview );
    }
    if( view->deinit_state ) { view->deinit_state( view ); }
    free( view->subview_clip );
    free( view->frame );
    
    if( view->in_anim && view->in_anim->deinit_fn ) { 
        ((anim_deinit_t)view->in_anim->deinit_fn)( view->in_anim ); 
    }
    
    if( view->out_anim && view->out_anim->deinit_fn ) { 
        ((anim_deinit_t)view->out_anim->deinit_fn)( view->out_anim ); 
    }

    free( view->in_anim );
    free( view->out_anim );
    free( view );
}

zdj_view_t * zdj_root_view( void ) {
    return zdj_view_stack_root_view;
}

void zdj_print_subviews_of( zdj_view_t * view ) {
    printf( "Subviews of: %p\n", view );
    zdj_view_t * subview = view->subviews;
    while( subview ) {
        printf( "  %p<%p>%p", subview->prev, subview, subview->next );
        subview = subview->next;
    }
    printf( "\n" );
}
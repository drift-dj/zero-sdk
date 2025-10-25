#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <zerodj/controls/zdj_controls.h>
#include <zerodj/health/zdj_health_type.h>
#include <zerodj/system/display/zdj_display.h>
#include <zerodj/system/error/zdj_error.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/font/zdj_font.h>
#include <zerodj/ui/view/zdj_view_stack.h>

SDL_Surface* zdj_display_surface;
uint32_t * zdj_ui_pixels = NULL;
static int zdj_view_stack_id = 0;
zdj_view_t * zdj_delete_stack;
int zdj_view_count;
int zdj_new_view_count;
bool zdj_screen_cap_armed;

void _zdj_view_deinit( struct zdj_view_t * view );

void zdj_ui_init( void ) {
    // Grab the display memory
    zdj_display_init( );

    // Bringup SDL - exit on fail
    int err = SDL_Init( SDL_INIT_VIDEO );
    if( err != 0 ) {
        printf( "Zero failed to init graphics lib: %s\n", SDL_GetError( ) );
        exit( ZDJ_HEALTH_STATUS_SDL_FAILED );
    }

    zdj_display_surface = SDL_CreateRGBSurface( 0, ZDJ_SCREEN_W, ZDJ_SCREEN_H, 32, 0, 0, 0, 0 );
    zdj_display_renderer = SDL_CreateSoftwareRenderer( zdj_display_surface );
    zdj_ui_pixels = zdj_display_surface->pixels;
    if( !zdj_display_renderer || !zdj_ui_pixels ) {
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

    zdj_view_count = 0;
    zdj_screen_cap_armed = false;

    // Bringup the display stack
    zdj_view_stack_init( ); 
}

void zdj_ui_deinit( void ) {
    SDL_Quit( );
}

void zdj_ui_update( void ) {
    
    // Clear the screen
    zdj_view_stack_clear_screen( );

    // Update the view stack
    zdj_view_stack_update( );

    // Push the output pixels to the M7 core's memory
    zdj_display_m7_push( );
}

zdj_view_t * zdj_new_view( zdj_rect_t * frame ) {
    zdj_view_t * view = calloc( 1, sizeof( zdj_view_t ) );
    view->id = zdj_view_stack_id++;
    view->deinit = &_zdj_view_deinit;
    view->type = ZDJ_VIEW_BASE;

    // Control map inits to 'none.'
    // Change by view/front-end to enable auto-activation of a control mapping.
    view->map = ZDJ_CONTROL_MAP_NONE;

    // Default metrics update -- re-define in front-end layer to alter
    view->update_subview_clip = &zdj_view_stack_update_subview_clip;

    // Use initializer frame if available
    if( frame ) {
        view->frame.x = frame->x;
        view->frame.y = frame->y;
        view->frame.w = frame->w;
        view->frame.h = frame->h;
    }

    // Init transition anim stuff
    view->anim = NULL;

    view->is_deleting = false;

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
    // printf( "zdj_remove_subview_of: %p/%p\n", view, subview );
    if( !subview ){ return; }
    
    if( subview->next && subview->prev ) {
        // printf( "middle case\n" );
        // If we're in the middle of a linked list, splice the subview out
        subview->next->prev = subview->prev;
        subview->prev->next = subview->next;
    } else if( subview->next ) {
        // printf( "beginning case\n" );
        // If we're at the beginning of the linked list,
        // subview.next becomes head of superview's subviews list.
        subview->next->prev = NULL;
        view->subviews = subview->next;
    } else if( subview->prev ) {
        // printf( "end case\n" );
        // If we're at the end of the linked list, null out the new end.
        subview->prev->next = NULL;
    } else {
        // printf( "single case\n" );
        // If this is the base subview, null subviews out
        view->subviews = NULL;
    }
    view->subview_count--;
    zdj_delete_view( subview );

    // printf( "zdj_remove_subview_of done\n" );
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

    // Activate view's control map
    if( subview->map != ZDJ_CONTROL_MAP_NONE ){ zdj_activate_control_map( subview->map ); }

    // printf( "zdj_push_subview: %p old %p new: %p\n", 
    //     view, 
    //     old_subview,
    //     subview 
    // );
    // printf( "zdj_push_subview type: %d, map: %d\n", subview->type, subview->map );

    subview->in_anim.cb_fn = NULL;
    subview->out_anim.cb_fn = NULL;
    if( subview->in_anim.init_fn ) {
        ((anim_init_t)subview->in_anim.init_fn)( &subview->in_anim, subview );
    }
    subview->in_anim.view = subview;
    subview->in_anim.superview = view;
    if( !animated || !subview->in_anim.init_fn ) { 
        subview->in_anim.frame = subview->in_anim.frames; 
    }
    subview->anim = &subview->in_anim;

    if( old_subview && old_subview->out_anim.type != ZDJ_ANIM_NONE ) {
        ((anim_init_t)old_subview->out_anim.init_fn)( &old_subview->out_anim, old_subview );
        if( !animated ) { old_subview->out_anim.frame = old_subview->out_anim.frames; }
        old_subview->anim = &old_subview->out_anim;
    }
    // printf( "zdj_push_subview done\n" );
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

    // printf( "zdj_pop_subview_of: %p top: %p prev: %p\n", view, top_subview, prev_subview );

    // Activate view's control map
    if( prev_subview->map != ZDJ_CONTROL_MAP_NONE ){ zdj_activate_control_map( prev_subview->map ); }

    if( top_subview ) {
        ((anim_init_t)top_subview->out_anim.init_fn)( &top_subview->out_anim, top_subview );
        top_subview->out_anim.view = top_subview;
        top_subview->out_anim.superview = view;
        top_subview->out_anim.cb_fn = &zdj_remove_subview_of; // Delete subview after anim
        if( !animated ) { top_subview->out_anim.frame = top_subview->out_anim.frames; }
        top_subview->anim = &top_subview->out_anim;
        
        if( prev_subview && prev_subview->in_anim.type != ZDJ_ANIM_NONE ) {
            ((anim_init_t)prev_subview->in_anim.update_fn)( &prev_subview->in_anim, prev_subview );
            if( !animated ) { prev_subview->in_anim.frame = prev_subview->in_anim.frames; }
            prev_subview->anim = &prev_subview->in_anim;
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
        // printf( "subview: %p, targ: %p\n", subview, target_subview );
        if( subview == target_subview ) { 
            // printf( "found: %p\n", subview );
            target_found = true; 
            break; 
        }   
        pop_count++;
        subview = subview->prev;
    }
    if( target_found ) { 
        // printf( "pop_to_subview: %d views\n", pop_count );
        zdj_pop_n_subviews_of( view, pop_count, animated );
    }
}  

// Remove the top n subviews of a view
void zdj_pop_n_subviews_of( zdj_view_t * view, int count, bool animated ) {
    // printf( "zdj_pop_n_subviews_of: %d/%d\n", count, view->subview_count );
    // Confirm there are n subviews to hide + 1 to reveal
    if( view->subview_count < count ) { return; }
    
    // Animate out anything from top view down to n
    zdj_view_t * subview = zdj_view_stack_top_subview_of( view );
    for( int n=0; n<count; n++ ) {
        if( subview ) {
            // printf( "starting out anim: %p\n", subview );
            ((anim_init_t)subview->out_anim.init_fn)( &subview->out_anim, subview );
            subview->out_anim.view = subview;
            subview->out_anim.superview = view;
            subview->out_anim.cb_fn = &zdj_remove_subview_of; // Delete subview after anim
            if( !animated ) { subview->out_anim.frame = subview->out_anim.frames; }
            subview->anim = &subview->out_anim;
            
            subview = subview->prev;
        }
    }

    // Activate view's control map
    if( subview->map != ZDJ_CONTROL_MAP_NONE ){ zdj_activate_control_map( subview->map ); }
    
    // Animate in target subview
    if( subview ) {
        ((anim_init_t)subview->in_anim.init_fn)( &subview->in_anim, subview );
        if( !animated ) { subview->in_anim.frame = subview->in_anim.frames; }
        subview->anim = &subview->in_anim;
    }
}

// Prepare a view to be deleted at the end of this update cycle.
// View's next/prev links must already be cleaned up before sending it here or
// bad stuff probably happens.
// Note that deinit isn't called here.
// deinit() is called when view_stack_update() traverses the delete stack later. 
void zdj_delete_view( zdj_view_t * view ) {
    // printf( "zdj_delete_view: %p\n", view );
    view->is_deleting = true;
    view->next = NULL;
    view->prev = NULL;

    // Recurse into subviews and delete
    zdj_view_t * next_subview = view->subviews;
    while( next_subview ) {
        // printf( "next_subview: %p\n", next_subview );
        zdj_view_t * new_next_subview = next_subview->next;
        zdj_delete_view( next_subview );
        next_subview = new_next_subview;
    }

    // Put this view at the head of the delete stack
    if( zdj_delete_stack ) {
        zdj_delete_stack->prev = view;
        view->next = zdj_delete_stack;
    }
    zdj_delete_stack = view;

    // printf( "zdj_delete_view done\n" );
}

void _zdj_view_deinit( zdj_view_t * view ) {
    // printf( "_zdj_view_deinit: %p->%p type: %d id: %d\n", view, view->next, view->type, view->id );
    
    // Deinit the view state
    if( view->deinit_state ) { view->deinit_state( view ); }
    // Shift the view stack forward
    zdj_delete_stack = view->next;
    // Free the view
    free( view );

    // printf( "_zdj_view_deinit done\n" );
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
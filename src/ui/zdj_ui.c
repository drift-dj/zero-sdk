#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/font/zdj_font.h>
#include <zerodj/ui/view/zdj_view_stack.h>
#include <zerodj/display/zdj_display.h>
#include <zerodj/hmi/zdj_hmi.h>
#include <zerodj/health/zdj_health_type.h>

SDL_Surface* zdj_display_surface;
uint32_t * zdj_ui_pixels = NULL;
static int zdj_view_stack_id = 0;

void _zdj_view_deinit( struct zdj_view_t * );

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

    zdj_display_surface = SDL_CreateRGBSurface( 0, ZDJ_SCREEN_WIDTH, ZDJ_SCREEN_HEIGHT, 32, 0, 0, 0, 0 );
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
}

void zdj_remove_subview( zdj_view_t * view, zdj_view_t * subview ) {
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
    subview->deinit( subview );
}

void zdj_add_subview_below( zdj_view_t * view, zdj_view_t * target_subview, zdj_view_t * new_subview ) {
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
}

void zdj_add_bottom_subview_to( zdj_view_t * view, zdj_view_t * new_subview ) {
    if( view->subviews ) {
        view->subviews->prev = new_subview;
        new_subview->next = view->subviews;
    }
    view->subviews = new_subview;
}

// Remove the top subview of a view
void zdj_pop_subview_of( zdj_view_t * view ) {
    if( view->subviews ) {
        zdj_view_t * top_subview = zdj_view_stack_top_subview_of( view );
        zdj_view_t * new_top_subview = top_subview->prev;
        if( new_top_subview ) {
            new_top_subview->next = NULL;
        }
        top_subview->deinit( top_subview );
    }
}

// Remove the top n subviews of a view
void zdj_pop_subviews_of( zdj_view_t * view, int count ) {
    if( view->subviews ) {
        for( int n=0; n<count; n++ ) {
            zdj_view_t * top_subview = zdj_view_stack_top_subview_of( view );
            if( top_subview ) {
                zdj_view_t * new_top_subview = top_subview->prev;
                if( new_top_subview ) {
                    new_top_subview->next = NULL;
                }
                top_subview->deinit( top_subview );
            }
        }
    }
}

void zdj_remove_subviews_of( zdj_view_t * view ) {
    zdj_view_t * subview = view->subviews;
    while( subview ) {
        zdj_view_t * old_subview = subview;
        subview = subview->next;
        old_subview->deinit( old_subview );
    }
    view->subviews = NULL;
}

void _zdj_view_deinit( zdj_view_t * view ) {
    // Deinit subviews
    zdj_view_t * subview = view->subviews;
    while( subview ) {
        zdj_view_t * old_subview = subview;
        subview = subview->next;
        old_subview->deinit( old_subview );
    }
    if( view->deinit_state ) {
        view->deinit_state( view );
    }
    free( view->subview_clip );
    free( view->frame );
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
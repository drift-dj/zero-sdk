#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/font/zdj_font.h>
#include <zerodj/ui/view/zdj_view_stack.h>
#include <zerodj/display/zdj_display.h>
#include <zerodj/hmi/zdj_hmi.h>
#include <zerodj/health/zdj_health_type.h>

SDL_Surface* zdj_display_surface;

uint32_t * zdj_ui_pixels = NULL;

void zdj_ui_init( void ) {
    // Grab the display memory
    zdj_display_init( );
 
    // Init the HMI event system
    zdj_hmi_init( );

    // Bringup SDL - crash on fail
    int err = SDL_Init( SDL_INIT_VIDEO );
    if( err != 0 ) {
        printf( "Zero failed to init graphics lib... exiting\n" );
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
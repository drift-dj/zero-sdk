#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <SDL2/SDL.h>

#include <zerodj/display/zdj_display.h>
#include <zerodj/m7/zdj_m7.h>

uint32_t * zdj_vid_buffer;


SDL_Surface* zdj_display_surface;
SDL_Renderer* zdj_display_renderer;
uint32_t * zdj_display_sdl_pixels;

int zdj_display_init( void ) {
    // Grab references to the shared (M7+A53) memory.
    // See zero kernel 'drift-a106.dtsi' reserved-memory section.
	int mem_fd = open( "/dev/mem", O_RDWR );
	zdj_vid_buffer = (uint32_t *)mmap(0, 0x20000, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, 0x55a11000);

    int stat = SDL_Init( SDL_INIT_VIDEO );
    if( stat != 0 ) {
        printf( "SDL_Init failed:: %d/%s\n", stat, SDL_GetError( ) );
        return stat;
    }
    zdj_display_surface = SDL_CreateRGBSurface( 0, ZDJ_SCREEN_WIDTH, ZDJ_SCREEN_HEIGHT, 32, 0, 0, 0, 0 );
    zdj_display_renderer = SDL_CreateSoftwareRenderer( zdj_display_surface );
    zdj_display_sdl_pixels = zdj_display_surface->pixels;
    printf( "post-SDL_Init( ): %p/%p/%p\n", zdj_display_surface, zdj_display_renderer, zdj_display_surface->pixels );
    return 0;
}

void zdj_display_release( void ) {
    SDL_Quit( );
}

void zdj_display_draw( void ) {
    // Pack SDL's pixels into shared video buffer
    for( int i=0; i<(ZDJ_SCREEN_WIDTH*ZDJ_SCREEN_HEIGHT)/4; i++ ) {
        zdj_vid_buffer[ i ] = ((zdj_display_sdl_pixels[i*4]&0xFF) << 24) +
                          ((zdj_display_sdl_pixels[i*4+1]&0xFF) << 16) +
                          ((zdj_display_sdl_pixels[i*4+2]&0xFF) << 8) +
                          (zdj_display_sdl_pixels[i*4+3]&0xFF);
    }
    // Signal the M7 core to transfer pixel data to the screen (see zero-m7 repo).
    // write( m7( ), "ud", strlen( "ud" )+1 );
    zdj_m7_msg( ZDJ_M7_UPDATE_DISPLAY );
}

SDL_Renderer * zdj_renderer( void ) {
    return zdj_display_renderer;
}
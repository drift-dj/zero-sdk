#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <uuid.h>

// #include <SDL2/SDL_image.h>
#include <SDL2/SDL2_gfxPrimitives.h>


#include <zerodj/system/screen_cap/zdj_screen_cap.h>
#include <zerodj/ui/zdj_ui.h>

void zdj_capture_screen( void ) { zdj_screen_cap_armed = true; }

void zdj_write_screen_cap( void ) {
    char uuid_str[ 64 ];
    char path[ 512 ];

    uuid_t uuid;
    uuid_generate( uuid );
    uuid_unparse_lower( uuid, uuid_str );
    snprintf( path, sizeof( path ), "%s/%s.bmp",
        ZDJ_SCREEN_CAP_DIR,
        uuid_str
    );

    int width, height;
    SDL_GetRendererOutputSize( zdj_renderer( ), &width, &height);
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!surface) { return; }
    if ( SDL_RenderReadPixels( zdj_renderer( ), NULL, SDL_PIXELFORMAT_ARGB8888, surface->pixels, surface->pitch ) != 0) { return; }
    if( SDL_SaveBMP( surface, path ) != 0 ) { printf( "Error saving BMP: %s\n", SDL_GetError( ) ); }

    SDL_FreeSurface( surface );
}
#include <stdlib.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/text/font/zdj_font.h>


TTF_Font * zdj_font_6 = NULL;
TTF_Font * zdj_font_6_caps = NULL;
TTF_Font * zdj_font_6_bold;
TTF_Font * zdj_font_9;
TTF_Font * zdj_font_9_bold;

int zdj_font_init( void ) {
    TTF_Init( );
    zdj_font_6 = TTF_OpenFont( "/root/res/fonts/pixelated.ttf", 8 ); 
    zdj_font_6_caps = TTF_OpenFont( "/root/res/fonts/pixelsix14.ttf", 8 );
    // Font 6 is the only crititcal system font.
    // All others revert back to font 6 in the evetn of a loading error.
    if( !zdj_font_6 ) { return 1; } else { return 0; }
}

// Fonts are lazy-loaded.
// Returns NULL if file isn't found.
TTF_Font * zdj_font( zdj_font_t font_name ) {
    switch (font_name) {
    case ZDJ_FONT_6:
        return zdj_font_6;
        break;
    case ZDJ_FONT_6_CAPS:
        return ( zdj_font_6_caps ) ? zdj_font_6_caps : zdj_font_6;
        break;
    default:
        return zdj_font_6;
        break;
    }
    return NULL;
}
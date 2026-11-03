#include <stdlib.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/font/zdj_font.h>


TTF_Font * zdj_font_6 = NULL;
TTF_Font * zdj_font_6_caps = NULL;
TTF_Font * zdj_font_6_bold;
TTF_Font * zdj_font_9 = NULL;
TTF_Font * zdj_font_9_bold;
TTF_Font * zdj_font_12;
TTF_Font * zdj_font_12_caps;
TTF_Font * zdj_font_18;
TTF_Font * zdj_font_18_caps;

TTF_Font * zdj_font_tiny5;
float zdj_font_tiny5_lineheight = 7;
TTF_Font * zdj_font_bytesized;
float zdj_font_bytesized_lineheight = 6;
TTF_Font * zdj_font_pixelsix;
float zdj_font_pixelsix_lineheight = 7;

TTF_Font * zdj_font_mona_12;
TTF_Font * zdj_font_mona_12_narrow;
float zdj_font_mona_12_lineheight = 14;


int zdj_font_init( void ) {
    TTF_Init( );
    zdj_font_6 = TTF_OpenFont( "/root/res/fonts/pixelated.ttf", 8 );
    zdj_font_6_caps = TTF_OpenFont( "/root/res/fonts/pixelsix14.ttf", 8 );
    zdj_font_9 = TTF_OpenFont( "/root/res/fonts/lo-res09-nar.ttf", 9 );
    zdj_font_12 = TTF_OpenFont( "/root/res/fonts/pixelated.ttf", 15 );
    zdj_font_12_caps = TTF_OpenFont( "/root/res/fonts/pixelsix14.ttf", 15 );
    zdj_font_18 = TTF_OpenFont( "/root/res/fonts/pixelated.ttf", 24 );
    zdj_font_18_caps = TTF_OpenFont( "/root/res/fonts/pixelsix14.ttf", 24 );

    zdj_font_tiny5 = TTF_OpenFont( "/root/res/fonts/Tiny5-Regular.ttf", 8 );
    zdj_font_bytesized = TTF_OpenFont( "/root/res/fonts/Bytesized-Regular.ttf", 8 );
    zdj_font_pixelsix = TTF_OpenFont( "/root/res/fonts/pixelsix14.ttf", 8 );

    zdj_font_mona_12 = TTF_OpenFont( "/root/res/fonts/Mona12.ttf", 12 );
    zdj_font_mona_12_narrow = TTF_OpenFont( "/root/res/fonts/Mona10x12.ttf", 12 );

    // Font 6 is the only crititcal system font.
    // All others revert back to font 6 in the event of a loading error.
    if( !zdj_font_6 ) { return 1; } else { return 0; }
}

// Returns NULL if file isn't found.
TTF_Font * zdj_font( zdj_font_t font_name ) {
    switch (font_name) {
    case ZDJ_FONT_6:
        return zdj_font_6;
        break;
    case ZDJ_FONT_6_CAPS:
        return ( zdj_font_6_caps ) ? zdj_font_6_caps : zdj_font_6;
        break;
    case ZDJ_FONT_9:
        return zdj_font_9;
        break;
    case ZDJ_FONT_12:
        return zdj_font_12;
        break;
    case ZDJ_FONT_12_CAPS:
        return ( zdj_font_12_caps ) ? zdj_font_12_caps : zdj_font_12;
        break;
    case ZDJ_FONT_18:
        return zdj_font_18;
        break;
    case ZDJ_FONT_18_CAPS:
        return ( zdj_font_18_caps ) ? zdj_font_18_caps : zdj_font_18;
        break;
    case ZDJ_FONT_TINY5:
        return ( zdj_font_tiny5 ) ? zdj_font_tiny5 : zdj_font_6;
        break;
    case ZDJ_FONT_BYTESIZED:
        return ( zdj_font_bytesized ) ? zdj_font_bytesized : zdj_font_6;
        break;
    case ZDJ_FONT_PIXELSIX:
        return ( zdj_font_pixelsix ) ? zdj_font_pixelsix : zdj_font_6;
        break;
    case ZDJ_FONT_MONA_12:
        return ( zdj_font_mona_12 ) ? zdj_font_mona_12 : zdj_font_6;
        break;
    case ZDJ_FONT_MONA_12_NAR:
        return ( zdj_font_mona_12_narrow ) ? zdj_font_mona_12_narrow : zdj_font_6;
        break;
    default:
        return zdj_font_6;
        break;
    }
    return NULL;
}
#include <stdlib.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/text/font/zdj_font.h>
#include <zerodj/ui/text/ticker_str/zdj_ticker_str.h>

zdj_ticker_str_t * new_ticker_str( 
    char * str,
    zdj_font_t font,
    zdj_justify_t justify,
    SDL_Color color
) {
    zdj_ticker_str_t * ticker_str = malloc( sizeof( zdj_ticker_str_t ) );
    ticker_str->str = strdup( str );
    ticker_str->justify = justify;

    // Build type texture/metrics
    TTF_Font * ttf_font = zdj_font( font );
    if( ttf_font ) {
        // Build the texture/text size from rendered string
        SDL_Surface *surface;
        surface = TTF_RenderText_Solid( ttf_font, str, color );
        ticker_str->tex = SDL_CreateTextureFromSurface( zdj_renderer( ), surface );
        ticker_str->str_width = surface->w;
        ticker_str->str_height = surface->h;
        SDL_FreeSurface( surface );
    } else {
        // Load the error texture
        
    }
    return ticker_str;
}

void free_ticker_str( zdj_ticker_str_t * ticker_str ) {
    free( ticker_str->str );
    SDL_DestroyTexture( ticker_str->tex );
    free( ticker_str );
}

void draw_ticker_str( zdj_ticker_str_t * ticker_str, zdj_rect_t clip_rect ) {
    SDL_Rect s = { 0, 0, ticker_str->str_width, ticker_str->str_height };
    SDL_RenderCopy( zdj_renderer( ), ticker_str->tex, &s, &s );
}
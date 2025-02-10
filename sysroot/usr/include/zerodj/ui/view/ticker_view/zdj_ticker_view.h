#ifndef ZDJ_TICKER_STR_H
#define ZDJ_TICKER_STR_H

#include <SDL2/SDL.h>

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    char * str;
    zdj_font_t font;
    zdj_justify_t justify;
    SDL_Texture * tex;
    int tex_w;
    int tex_h;
    int string_w;
    bool is_scrolling;
    float scroll_rate;
    float scroll_offset;
} zdj_ticker_state_t;

zdj_view_t * new_ticker_view( 
    char * str,
    zdj_font_t font,
    zdj_justify_t justify,
    SDL_Color color,
    zdj_rect_t * frame
);
void free_ticker_view( zdj_view_t * ticker_view );

#endif
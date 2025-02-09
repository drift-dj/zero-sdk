#ifndef ZDJ_TICKER_STR_H
#define ZDJ_TICKER_STR_H

#include <SDL2/SDL.h>

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    char * str;
    zdj_font_t font;
    zdj_justify_t justify;
    int str_width;
    int str_height;
    float scroll_rate;
    int8_t scroll_dir;
    float scroll_offset;
    SDL_Texture * tex;
    SDL_Texture * tex_hilite;
    uint8_t is_blinking;
    int8_t blink_frame;
} zdj_ticker_str_t;

zdj_ticker_str_t * new_ticker_str( 
    char * str,
    zdj_font_t font,
    zdj_justify_t justify,
    SDL_Color color
);

void free_ticker_str( zdj_ticker_str_t * ticker_str );

void draw_ticker_str( zdj_ticker_str_t * ticker_str, zdj_rect_t clip_rect );

#endif
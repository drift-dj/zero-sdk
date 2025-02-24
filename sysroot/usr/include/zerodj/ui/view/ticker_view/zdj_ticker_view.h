#ifndef ZDJ_TICKER_VIEW_H
#define ZDJ_TICKER_VIEW_H

#include <SDL2/SDL.h>

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    char * str;
    zdj_font_t font;
    zdj_justify_t justify;
    zdj_view_t * texture_view;
    int text_h;
    int single_text_w;
    int double_text_w;
    int text_w; // For superviews which need to know final width of ticker
    bool is_scrolling;
    float scroll_rate;
    float scroll_offset;
} zdj_ticker_state_t;

zdj_view_t * zdj_new_ticker_view( 
    char * str,
    zdj_font_t font,
    zdj_justify_t justify,
    SDL_Color color
);

int zdj_ticker_view_get_text_w( zdj_view_t * view );

#endif
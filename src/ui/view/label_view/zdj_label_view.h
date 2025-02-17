#ifndef ZDJ_LABEL_VIEW_H
#define ZDJ_LABEL_VIEW_H

#include <SDL2/SDL.h>

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    char * str;
    zdj_justify_t justify;
    SDL_Texture * tex;
    int tex_w;
    int tex_h;
} zdj_label_state_t;

zdj_view_t * zdj_new_label_view( 
    char * str,
    zdj_font_t font,
    zdj_justify_t justify,
    SDL_Color color
);

#endif
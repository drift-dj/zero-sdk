#ifndef ZDJ_TEXTURE_VIEW_H
#define ZDJ_TEXTURE_VIEW_H

#include <SDL2/SDL.h>

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    SDL_Texture * tex;
    int tex_w;
    int tex_h;
} zdj_texture_view_state_t;

zdj_view_t * zdj_new_texture_view( SDL_Texture * tex, int tex_w, int tex_h );

#endif
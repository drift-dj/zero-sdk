#ifndef ZDJ_ASSET_VIEW_H
#define ZDJ_ASSET_VIEW_H

#include <SDL2/SDL.h>

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    SDL_Texture * tex;
    zdj_rect_t asset_rect;
} zdj_asset_view_state_t;

zdj_view_t * zdj_new_asset_view( SDL_Rect * rect, SDL_Texture * tex );

#endif
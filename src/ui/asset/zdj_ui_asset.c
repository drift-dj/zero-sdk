#include <stdio.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/asset/zdj_ui_asset.h>

SDL_Texture * _zdj_asset_atlas;

SDL_Rect zdj_ui_assets[ ZDL_UI_ASSET_COUNT ] = {
    { 0,80,4,4 }, // ZDJ_UI_ASSET_ERROR_TEX
    { 0,0,128,64 }, // ZDJ_UI_ASSET_DOT_BG
    { 0,74,128,1 }, // ZDJ_UI_ASSET_NAR_H_DIV
    { 0,76,128,1 }, // ZDJ_UI_ASSET_MID_H_DIV
    { 0,78,128,1 } // ZDJ_UI_ASSET_WIDE_H_DIV
};

int zdj_ui_asset_init( void ) {
    _zdj_asset_atlas = zdj_ui_texture_from_bmp( "/root/res/zero_atlas-32bit" );

    if( !_zdj_asset_atlas ) { return 1; } else { return 0; }
}

void zdj_ui_asset_draw( zdj_ui_asset_t asset, zdj_rect_t * dest ) {
    SDL_RenderCopy( zdj_renderer( ), _zdj_asset_atlas, &zdj_ui_assets[asset], &(SDL_Rect){dest->x,dest->y,dest->w,dest->h} );
}
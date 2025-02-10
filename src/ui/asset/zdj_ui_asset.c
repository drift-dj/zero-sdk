#include <stdio.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/asset/zdj_ui_asset.h>

SDL_Texture * _zdj_asset_atlas;
SDL_Texture * _zdj_asset_error;

SDL_Rect zdj_ui_assets[ ZDL_UI_ASSET_COUNT ] = {
    { 0,80,4,4 }, // ZDJ_UI_ASSET_ERROR_TEX
    { 129,0,128,64 } // ZDJ_UI_ASSET_DOT_BG
};

void zdj_ui_asset_init( void ) {
    _zdj_asset_atlas = zdj_ui_texture_from_bmp( "/root/res/zero_atlas-32bit" );
    _zdj_asset_error = zdj_ui_texture_from_bmp( "/root/res/zero_error_tex" );
}

SDL_Texture * zdj_ui_asset_error_tex( void ) {
    return _zdj_asset_error;
}

void zdj_ui_asset_draw( zdj_ui_asset_t asset, zdj_rect_t * dest ) {
    SDL_RenderCopy( zdj_renderer( ), _zdj_asset_atlas, &zdj_ui_assets[asset], dest );
}
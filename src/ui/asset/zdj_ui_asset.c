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
    { 0,78,128,1 }, // ZDJ_UI_ASSET_WIDE_H_DIV
    { 0,64,128,8 }, // ZDJ_UI_ASSET_ALERT_STRIP
    { 6,82,5,1 }, // ZDJ_UI_ASSET_DOT_DASH
    { 6,81,5,1 }, // ZDJ_UI_ASSET_DOT_DASH_HI
    { 13,81,1,5 }, // ZDJ_UI_ASSET_DOT_PIPE
    { 14,81,5,1 }, // ZDJ_UI_ASSET_DOT_PIPE_HI
    { 17,80,8,7 }, // ZDJ_UI_ASSET_FOLDER
    { 26,80,8,7 }, // ZDJ_UI_ASSET_FOLDER_HI
    { 40,82,4,3 }, // ZDJ_UI_ASSET_CHECKMARK
    { 35,82,4,3 }, // ZDJ_UI_ASSET_CHECKMARK_HI
    { 49,82,3,3 }, // ZDJ_UI_ASSET_PLUS
    { 45,82,3,3 }, // ZDJ_UI_ASSET_PLUS_HI
    { 52,81,10,6 }, // ZDJ_UI_ASSET_ADD_MUSIC
    { 60,81,10,6 }, // ZDJ_UI_ASSET_ADD_MUSIC_HI
    { 68,82,7,5 }, // ZDJ_UI_ASSET_INSTALL
    { 76,82,7,5 }, // ZDJ_UI_ASSET_INSTALL_HI
    { 13,89,12,7 }, // ZDJ_UI_ASSET_DIR_UP
    { 27,89,12,7 }, // ZDJ_UI_ASSET_DIR_UP_HI
    { 41,89,12,7 }, // ZDJ_UI_ASSET_DIR_ADD
    { 55,89,12,7 }, // ZDJ_UI_ASSET_DIR_ADD_HI
    { 85,81,15,9 }, // ZDJ_UI_ASSET_ZERO
    { 101,81,15,9 }, // ZDJ_UI_ASSET_ZERO_HI
    { 85,91,12,10 }, // ZDJ_UI_ASSET_DRIVE
    { 98,91,12,10 }, // ZDJ_UI_ASSET_DRIVE_HI
    { 129,0,128,64 } // ZDJ_UI_ASSET_WHITE
};

int zdj_ui_asset_init( void ) {
    _zdj_asset_atlas = zdj_ui_texture_from_bmp( "/root/res/zero_atlas-32bit" );

    if( !_zdj_asset_atlas ) { return 1; } else { return 0; }
}

SDL_Texture * zdj_asset_atlas( void ) {
    if( _zdj_asset_atlas ) { return _zdj_asset_atlas; } else {
        // It's probably bad if this case is called, but it's here for safety.
        return zdj_ui_texture_from_bmp( "/root/res/zero_atlas-32bit" );
    }
}

void zdj_ui_asset_draw( zdj_ui_asset_t asset, zdj_rect_t * dest ) {
    SDL_RenderCopy( zdj_renderer( ), _zdj_asset_atlas, &zdj_ui_assets[asset], &(SDL_Rect){dest->x,dest->y,dest->w,dest->h} );
}
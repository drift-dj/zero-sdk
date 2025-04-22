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
    { 40,97,13,7 }, // ZDJ_UI_ASSET_DIR_SELECT,
    { 54,97,13,7 }, // ZDJ_UI_ASSET_DIR_SELECT_HI,
    { 85,81,15,9 }, // ZDJ_UI_ASSET_ZERO
    { 101,81,15,9 }, // ZDJ_UI_ASSET_ZERO_HI
    { 85,91,12,10 }, // ZDJ_UI_ASSET_DRIVE
    { 98,91,12,10 }, // ZDJ_UI_ASSET_DRIVE_HI
    { 0,113,127,8 }, // ZDJ_UI_ASSET_HILITE_LEFT,
    { 1,113,120,8 }, // ZDJ_UI_ASSET_HILITE_CENTER,
    { 128,113,1,8 }, // ZDJ_UI_ASSET_HILITE_RIGHT,
    { 69,90,7,6 }, // ZDJ_UI_ASSET_ALERT,
    { 77,90,7,6 }, // ZDJ_UI_ASSET_ALERT_HI,
    { 118,92,10,10 }, // ZDJ_UI_ASSET_BIG_PLUS,
    { 118,81,10,10 }, // ZDJ_UI_ASSET_BIG_PLUS_HI,
    { 101,103,13,9 }, // ZDJ_UI_ASSET_LIBRARY,
    { 115,103,13,9 }, // ZDJ_UI_ASSET_LIBRARY_HI,
    { 78,97,5,6 }, // ZDJ_UI_ASSET_SHIFT_KEY,
    { 72,97,5,6 }, // ZDJ_UI_ASSET_SHIFT_KEY_HI,
    { 44,105,9,7 }, // ZDJ_UI_ASSET_INSERT_KEY,
    { 54,105,9,7 }, // ZDJ_UI_ASSET_INSERT_KEY_HI,
    { 72,106,8,5 }, // ZDJ_UI_ASSET_SPACE_KEY,
    { 64,106,8,5 }, // ZDJ_UI_ASSET_SPACE_KEY_HI,
    { 90,104,9,7 }, // ZDJ_UI_ASSET_BACKSPACE_KEY,
    { 81,104,9,7 }, // ZDJ_UI_ASSET_BACKSPACE_KEY_HI,
    { 0,122,128,2 }, // ZDJ_UI_ASSET_UNDERLINE,
    { 0,125,21,21 }, // ZDJ_UI_ASSET_TEXT_INPUT_CURSOR,
    { 0,147,119,20 }, // ZDJ_UI_ASSET_BIG_ACTION_L,
    { 119,147,9,20 }, // ZDJ_UI_ASSET_BIG_ACTION_R,
    { 129,0,128,64 }, // ZDJ_UI_ASSET_WHITE
    { 257,0,128,64 } // ZDJ_UI_ASSET_BLACK
};

int zdj_ui_asset_init( void ) {
    _zdj_asset_atlas = zdj_ui_texture_from_bmp( "/root/res/zero_atlas-32bit.bmp" );

    if( !_zdj_asset_atlas ) { return 1; } else { return 0; }
}

SDL_Texture * zdj_asset_atlas( void ) {
    if( _zdj_asset_atlas ) { return _zdj_asset_atlas; } else {
        // It's probably bad if this case is called, but it's here for safety.
        return zdj_ui_texture_from_bmp( "/root/res/zero_atlas-32bit.bmp" );
    }
}

void zdj_ui_asset_draw( zdj_ui_asset_t asset, zdj_rect_t * dest ) {
    SDL_RenderCopy( zdj_renderer( ), _zdj_asset_atlas, &zdj_ui_assets[asset], &(SDL_Rect){dest->x,dest->y,dest->w,dest->h} );
}
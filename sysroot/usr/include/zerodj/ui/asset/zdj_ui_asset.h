#ifndef ZDJ_UI_ASSET_H
#define ZDJ_UI_ASSET_H

#include <zerodj/ui/zdj_ui.h>

typedef enum {
    ZDJ_UI_ASSET_ERROR_TEX,
    ZDJ_UI_ASSET_DOT_BG,
    ZDJ_UI_ASSET_NAR_H_DIV,
    ZDJ_UI_ASSET_MID_H_DIV,
    ZDJ_UI_ASSET_WIDE_H_DIV,
    ZDJ_UI_ASSET_ALERT_STRIP,
    ZDJ_UI_ASSET_DOT_DASH,
    ZDJ_UI_ASSET_DOT_DASH_HI,
    ZDJ_UI_ASSET_DOT_PIPE,
    ZDJ_UI_ASSET_DOT_PIPE_HI,
    ZDJ_UI_ASSET_FOLDER,
    ZDJ_UI_ASSET_FOLDER_HI,
    ZDJ_UI_ASSET_CHECKMARK,
    ZDJ_UI_ASSET_CHECKMARK_HI,
    ZDJ_UI_ASSET_PLUS,
    ZDJ_UI_ASSET_PLUS_HI,
    ZDJ_UI_ASSET_ADD_MUSIC,
    ZDJ_UI_ASSET_ADD_MUSIC_HI,
    ZDJ_UI_ASSET_INSTALL,
    ZDJ_UI_ASSET_INSTALL_HI,
    ZDJ_UI_ASSET_DIR_UP,
    ZDJ_UI_ASSET_DIR_UP_HI,
    ZDJ_UI_ASSET_DIR_ADD,
    ZDJ_UI_ASSET_DIR_ADD_HI,
    ZDJ_UI_ASSET_ZERO,
    ZDJ_UI_ASSET_ZERO_HI,
    ZDJ_UI_ASSET_DRIVE,
    ZDJ_UI_ASSET_DRIVE_HI,
    ZDJ_UI_ASSET_WHITE,
    ZDL_UI_ASSET_COUNT
} zdj_ui_asset_t;

extern SDL_Rect zdj_ui_assets[ ZDL_UI_ASSET_COUNT ];

int zdj_ui_asset_init( void );
SDL_Texture * zdj_ui_asset_error_tex( void );

SDL_Texture * zdj_asset_atlas( void );
void zdj_ui_asset_draw( zdj_ui_asset_t asset, zdj_rect_t * dest );

#endif
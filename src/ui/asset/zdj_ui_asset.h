#ifndef ZDJ_UI_ASSET_H
#define ZDJ_UI_ASSET_H

#include <zerodj/ui/zdj_ui.h>

typedef enum {
    ZDJ_UI_ASSET_ERROR_TEX,
    ZDJ_UI_ASSET_DOT_BG,
    ZDL_UI_ASSET_COUNT
} zdj_ui_asset_t;

extern SDL_Rect zdj_ui_assets[ ZDL_UI_ASSET_COUNT ];

void zdj_ui_asset_init( void );
SDL_Texture * zdj_ui_asset_error_tex( void );

void zdj_ui_asset_draw( zdj_ui_asset_t asset, zdj_rect_t * dest );

#endif
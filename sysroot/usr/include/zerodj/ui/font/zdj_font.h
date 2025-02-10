#ifndef ZDJ_FONT_H
#define ZDJ_FONT_H

#include <SDL2/SDL_ttf.h>

#include <zerodj/ui/zdj_ui.h>

int zdj_font_init( void );
TTF_Font * zdj_font( zdj_font_t font_name );

#endif
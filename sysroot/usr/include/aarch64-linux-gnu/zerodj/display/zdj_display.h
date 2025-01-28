#ifndef ZDJ_DISPLAY_H
#define ZDJ_DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#define ZDJ_SCREEN_WIDTH 128
#define ZDJ_SCREEN_HEIGHT 64

void zdj_display_init( void );
void zdj_display_draw( void );
void zdj_display_release( void );

SDL_Renderer * zdj_renderer( void );

#endif
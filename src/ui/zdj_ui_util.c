#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

#include <zerodj/display/zdj_display.h>
#include <zerodj/ui/zdj_ui.h>

#define ZDJ_MODAL_X 4
#define ZDJ_MODAL_Y 3
#define ZDJ_MODAL_WIDTH 100
#define ZDJ_MODAL_HEIGHT 60

#define ZDJ_MENU_X 37
#define ZDJ_MENU_Y 7
#define ZDJ_MENU_WIDTH 90
#define ZDJ_MENU_HEIGHT 57

SDL_Renderer* zdj_display_renderer;
static zdj_rect_t * _zdj_screen_rect_priv;
static zdj_rect_t * _zdj_modal_rect_priv;
static zdj_rect_t * _zdj_menu_rect_priv;

SDL_Renderer * zdj_renderer( void ) {
    return zdj_display_renderer;
}

zdj_rect_t * zdj_screen_rect( void ) {
    if( !_zdj_screen_rect_priv ) {
        _zdj_screen_rect_priv = calloc( 1, sizeof( zdj_rect_t ) );
        _zdj_screen_rect_priv->w = ZDJ_SCREEN_WIDTH;
        _zdj_screen_rect_priv->h = ZDJ_SCREEN_HEIGHT;
    }
    return _zdj_screen_rect_priv;
}

zdj_rect_t * zdj_modal_rect( void ) {
    if( !_zdj_modal_rect_priv ) {
        _zdj_modal_rect_priv = calloc( 1, sizeof( zdj_rect_t ) );
        _zdj_modal_rect_priv->x = ZDJ_MODAL_X;
        _zdj_modal_rect_priv->y = ZDJ_MODAL_Y;
        _zdj_modal_rect_priv->w = ZDJ_MODAL_WIDTH;
        _zdj_modal_rect_priv->h = ZDJ_MODAL_HEIGHT;
    }
    return _zdj_modal_rect_priv;
}

zdj_rect_t * zdj_menu_rect( void ) {
    if( !_zdj_menu_rect_priv ) {
        _zdj_menu_rect_priv = calloc( 1, sizeof( zdj_rect_t ) );
        _zdj_menu_rect_priv->x = ZDJ_MENU_X;
        _zdj_menu_rect_priv->y = ZDJ_MENU_Y;
        _zdj_menu_rect_priv->w = ZDJ_MENU_WIDTH;
        _zdj_menu_rect_priv->h = ZDJ_MENU_HEIGHT;
    }
    return _zdj_menu_rect_priv;
}

bool zdj_ui_intersect( zdj_rect_t * rect1, zdj_rect_t * rect2 ) {
    if ( rect1->x > rect2->x + rect2->w || 
         rect1->x + rect1->w < rect2->x || 
         rect1->y > rect2->y + rect2->h || 
         rect1->y + rect1->h < rect2->y ) {
         return false;
    }
    return true;
}

SDL_Texture * zdj_ui_texture_from_bmp( char * filepath ) {
    // printf( "texture_from_bmp(%s)\n", filepath );
    SDL_Surface * tmp_surf = SDL_LoadBMP( strdup( filepath ) );
    if( tmp_surf ) {
        SDL_Texture * result = SDL_CreateTextureFromSurface( zdj_renderer( ), tmp_surf );
        SDL_FreeSurface( tmp_surf );
        return result;
    } else {
        return NULL;
    }
}
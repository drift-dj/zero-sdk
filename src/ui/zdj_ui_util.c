#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

#include <zerodj/ui/zdj_ui.h>

SDL_Renderer* zdj_display_renderer;

SDL_Renderer * zdj_renderer( void ) {
    return zdj_display_renderer;
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
    SDL_Texture * result = SDL_CreateTextureFromSurface( zdj_renderer( ), tmp_surf );
    SDL_FreeSurface( tmp_surf );
    return result;
}
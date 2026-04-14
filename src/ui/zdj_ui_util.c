#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include <SDL2/SDL.h>

#include <zerodj/system/display/zdj_display.h>
#include <zerodj/ui/zdj_ui.h>

SDL_Renderer* zdj_display_renderer;
static zdj_rect_t * _zdj_screen_rect_priv;
static zdj_rect_t * _zdj_modal_rect_priv;
static zdj_rect_t * _zdj_dialog_rect_priv;
static zdj_rect_t * _zdj_menu_rect_priv;
static zdj_rect_t * _zdj_menu_rect_sm_priv;
static zdj_rect_t * _zdj_menu_rect_med_priv;
static zdj_rect_t * _zdj_dj_deck_page_rect_priv;
static zdj_rect_t * _zdj_debug_panel_rect_priv;
static zdj_rect_t * _zdj_perf_panel_rect_priv;

SDL_Renderer * zdj_renderer( void ) {
    return zdj_display_renderer;
}

void zdj_ui_set_refresh_hz( int hz ) {
    zdj_ui_refresh_hz = hz;
}

int zdj_ui_get_frame_nanos( void ) {
    // Build the nanoseconds from the current display hz
    return 1000000000 / zdj_ui_refresh_hz;
}

int zdj_ui_msec_to_frames( int msec ) {
    double nanos_per_frame = 1000000000.0 / (double)zdj_ui_refresh_hz;
    double nanos = msec * 1000000.0;
    return round( nanos / nanos_per_frame );
}

zdj_rect_t * zdj_screen_rect( void ) {
    if( !_zdj_screen_rect_priv ) {
        _zdj_screen_rect_priv = calloc( 1, sizeof( zdj_rect_t ) );
        _zdj_screen_rect_priv->w = ZDJ_SCREEN_W;
        _zdj_screen_rect_priv->h = ZDJ_SCREEN_H;
    }
    return _zdj_screen_rect_priv;
}

zdj_rect_t * zdj_modal_rect( void ) {
    if( !_zdj_modal_rect_priv ) {
        _zdj_modal_rect_priv = calloc( 1, sizeof( zdj_rect_t ) );
        _zdj_modal_rect_priv->x = 0;
        _zdj_modal_rect_priv->y = 0;
        _zdj_modal_rect_priv->w = ZDJ_MODAL_WIDTH;
        _zdj_modal_rect_priv->h = ZDJ_MODAL_HEIGHT;
    }
    return _zdj_modal_rect_priv;
}

zdj_rect_t * zdj_dialog_rect( void ) {
    if( !_zdj_dialog_rect_priv ) {
        _zdj_dialog_rect_priv = calloc( 1, sizeof( zdj_rect_t ) );
        _zdj_dialog_rect_priv->x = 0;
        _zdj_dialog_rect_priv->y = 0;
        _zdj_dialog_rect_priv->w = ZDJ_DIALOG_W;
        _zdj_dialog_rect_priv->h = ZDJ_DIALOG_H;
    }
    return _zdj_dialog_rect_priv;
}

zdj_rect_t * zdj_menu_rect( void ) {
    if( !_zdj_menu_rect_priv ) {
        _zdj_menu_rect_priv = calloc( 1, sizeof( zdj_rect_t ) );
        _zdj_menu_rect_priv->x = 16;
        _zdj_menu_rect_priv->y = 0;
        _zdj_menu_rect_priv->w = ZDJ_MENU_WIDTH;
        _zdj_menu_rect_priv->h = ZDJ_MENU_HEIGHT;
    }
    return _zdj_menu_rect_priv;
}

zdj_rect_t * zdj_menu_rect_sm( void ) {
    if( !_zdj_menu_rect_sm_priv ) {
        _zdj_menu_rect_sm_priv = calloc( 1, sizeof( zdj_rect_t ) );
        _zdj_menu_rect_sm_priv->x = 63;
        _zdj_menu_rect_sm_priv->y = 14;
        _zdj_menu_rect_sm_priv->w = 65;
        _zdj_menu_rect_sm_priv->h = 49;
    }
    return _zdj_menu_rect_sm_priv;
}

zdj_rect_t * zdj_menu_rect_med( void ) {
    if( !_zdj_menu_rect_med_priv ) {
        _zdj_menu_rect_med_priv = calloc( 1, sizeof( zdj_rect_t ) );
        _zdj_menu_rect_med_priv->x = 40;
        _zdj_menu_rect_med_priv->y = 14;
        _zdj_menu_rect_med_priv->w = 88;
        _zdj_menu_rect_med_priv->h = 60;
    }
    return _zdj_menu_rect_med_priv;
}

zdj_rect_t * zdj_dj_deck_page_rect( void ) {
    if( !_zdj_dj_deck_page_rect_priv ) {
        _zdj_dj_deck_page_rect_priv = calloc( 1, sizeof( zdj_rect_t ) );
        _zdj_dj_deck_page_rect_priv->x = 0;
        _zdj_dj_deck_page_rect_priv->y = 0;
        _zdj_dj_deck_page_rect_priv->w = 120;
        _zdj_dj_deck_page_rect_priv->h = 9;
    }
    return _zdj_dj_deck_page_rect_priv;
}

zdj_rect_t * zdj_debug_panel_rect( void ) {
    if( !_zdj_debug_panel_rect_priv ) {
        _zdj_debug_panel_rect_priv = calloc( 1, sizeof( zdj_rect_t ) );
        _zdj_debug_panel_rect_priv->x = 0;
        _zdj_debug_panel_rect_priv->y = 0;
        _zdj_debug_panel_rect_priv->w = ZDJ_DEBUG_PANEL_WIDTH;
        _zdj_debug_panel_rect_priv->h = ZDJ_DEBUG_PANEL_HEIGHT;
    }
    return _zdj_debug_panel_rect_priv;
}

zdj_rect_t * zdj_perf_panel_rect( void ) {
    if( !_zdj_perf_panel_rect_priv ) {
        _zdj_perf_panel_rect_priv = calloc( 1, sizeof( zdj_rect_t ) );
        _zdj_perf_panel_rect_priv->x = 0;
        _zdj_perf_panel_rect_priv->y = 0;
        _zdj_perf_panel_rect_priv->w = ZDJ_PERF_PANEL_WIDTH;
        _zdj_perf_panel_rect_priv->h = ZDJ_PERF_PANEL_HEIGHT;
    }
    return _zdj_perf_panel_rect_priv;
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

    SDL_Surface * tmp_surf = SDL_LoadBMP( filepath );
    if( tmp_surf ) {
        SDL_Texture * result = SDL_CreateTextureFromSurface( zdj_renderer( ), tmp_surf );
        SDL_FreeSurface( tmp_surf );
        return result;
    } else {
        return NULL;
    }
}
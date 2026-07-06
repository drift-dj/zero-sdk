// zero-emu: the fake-M7 display. See zero_emu_video.h.

#include "zero_emu_video.h"

#include <stdint.h>
#include <stdio.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <zerodj/system/display/zdj_display.h>
#include <zerodj/system/m7/zdj_platform.h>

#define EMU_W ZDJ_DISPLAY_WIDTH   // 128
#define EMU_H ZDJ_DISPLAY_HEIGHT  // 64

static SDL_Window *   g_win;
static SDL_Renderer * g_ren;
static SDL_Texture *  g_tex;

// Same backing buffer the library packs pixels into (emulator zdj_platform
// backend returns the same pointer for a given address to every caller).
static const uint32_t * g_vid;
static uint32_t         g_argb[ EMU_W * EMU_H ];

// Unpack the device's packed video buffer (4 pixels per uint32, each lane a
// 4-bit grayscale level produced by zdj_display_m7_push) into ARGB8888.
static void _unpack_video( const uint32_t * vid, uint32_t * argb ) {
    const int words = ( EMU_W * EMU_H ) / 4;  // 2048
    for( int i = 0; i < words; i++ ) {
        uint32_t w = vid[ i ];
        uint8_t g[ 4 ] = {
            (uint8_t)( ( w >> 24 ) & 0xFF ),
            (uint8_t)( ( w >> 16 ) & 0xFF ),
            (uint8_t)( ( w >>  8 ) & 0xFF ),
            (uint8_t)(   w         & 0xFF ),
        };
        for( int k = 0; k < 4; k++ ) {
            uint8_t v = g[ k ];
            argb[ i * 4 + k ] = 0xFF000000u | ( v << 16 ) | ( v << 8 ) | v;
        }
    }
}

bool zero_emu_video_init( int scale ) {
    g_win = SDL_CreateWindow(
        "Zero Emulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        EMU_W * scale, EMU_H * scale, SDL_WINDOW_SHOWN );
    if( !g_win ) {
        printf( "zero-emu: SDL_CreateWindow failed: %s\n", SDL_GetError( ) );
        return false;
    }
    g_ren = SDL_CreateRenderer( g_win, -1, SDL_RENDERER_ACCELERATED );
    if( !g_ren ) { g_ren = SDL_CreateRenderer( g_win, -1, 0 ); }
    g_tex = SDL_CreateTexture(
        g_ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, EMU_W, EMU_H );

    g_vid = (const uint32_t *)zdj_platform_map_shared( ZDJ_SHARED_VIDEO_BUF_ADDR, 0x2000 );
    return true;
}

void zero_emu_video_present( void ) {
    _unpack_video( g_vid, g_argb );
    SDL_UpdateTexture( g_tex, NULL, g_argb, EMU_W * (int)sizeof( uint32_t ) );
    SDL_RenderClear( g_ren );
    SDL_RenderCopy( g_ren, g_tex, NULL, NULL );
    SDL_RenderPresent( g_ren );
}

bool zero_emu_video_dump( const char * path ) {
    SDL_Surface * s = SDL_CreateRGBSurfaceWithFormatFrom(
        g_argb, EMU_W, EMU_H, 32, EMU_W * 4, SDL_PIXELFORMAT_ARGB8888 );
    if( !s ) { return false; }
    bool ok = IMG_SavePNG( s, path ) == 0;
    SDL_FreeSurface( s );
    return ok;
}

void zero_emu_video_deinit( void ) {
    SDL_DestroyTexture( g_tex );
    SDL_DestroyRenderer( g_ren );
    SDL_DestroyWindow( g_win );
}

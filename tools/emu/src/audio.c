// zero-emu: the fake-M7 audio bridge. Design notes in audio.h.

#include "audio.h"

#include <stdint.h>
#include <stdio.h>
#include <time.h>

#include <SDL2/SDL.h>

#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/system/m7/zdj_m7.h>
#include <zerodj/system/m7/zdj_platform.h>

static volatile zdj_shared_audio_state_t * g_audio_state;
static volatile int32_t *                  g_dac;
static SDL_AudioDeviceID                    g_audio_dev;

// The ring is single-producer/single-consumer: g_ring_w is written only by the
// producer thread, g_ring_r only by the SDL audio callback. Atomic cursors
// (SDL_AtomicSet is a full barrier, so samples are visible before the cursor
// that publishes them) make it lock-free -- the audio callback never blocks.
#define EMU_RING_FRAMES 16384
#define EMU_RING_TARGET ( ZDJ_SOUNDCARD_BUF_LEN * 4 )  // ~36ms of slack
static int32_t      g_ring[ EMU_RING_FRAMES * 2 ];
static SDL_atomic_t g_ring_r, g_ring_w;  // frame read/write cursors
static volatile int g_audio_run;

static SDL_atomic_t g_underrun_fr;  // frames the callback had to zero-fill (diagnostic)

static int _ring_fill( void ) {
    return ( SDL_AtomicGet( &g_ring_w ) - SDL_AtomicGet( &g_ring_r ) + EMU_RING_FRAMES )
        % EMU_RING_FRAMES;
}

// Run one M7 audio cycle: request a DAC fill (cycle_ready), wait for the io
// thread's fill to complete (emu_dac_fill_count advances -- cycle_ready alone
// only says the request was *taken*, and reading then races the DAC write),
// then return the freshly-mixed main LR output via `out` (ZDJ_SOUNDCARD_BUF_LEN
// stereo frames). The ~20ms cap means a stalled pipeline degrades to stale
// samples instead of hanging the producer.
static void _pull_one_cycle( int32_t * out ) {
    uint32_t fill0 = g_audio_state->emu_dac_fill_count;
    g_audio_state->cycle_ready = 1;
    for( int spin = 0; g_audio_state->emu_dac_fill_count == fill0 && spin < 20000; spin++ ) {
        struct timespec t = { 0, 1000 }; nanosleep( &t, NULL );  // 1us, ~20ms cap
    }
    for( int i = 0; i < ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
        out[ i*2+0 ] = g_dac[ i*4+0 ];   // analog out 0 L
        out[ i*2+1 ] = g_dac[ i*4+1 ];   // analog out 0 R
    }
}

static int _audio_producer( void * arg ) {
    (void)arg;
    int32_t cycle[ ZDJ_SOUNDCARD_BUF_LEN * 2 ];
    while( g_audio_run ) {
        if( _ring_fill( ) >= EMU_RING_TARGET ) {
            struct timespec t = { 0, 1000000 }; nanosleep( &t, NULL );  // 1ms
            continue;
        }
        _pull_one_cycle( cycle );
        int w = SDL_AtomicGet( &g_ring_w );
        int r = SDL_AtomicGet( &g_ring_r );
        int room = ( r - w - 1 + EMU_RING_FRAMES ) % EMU_RING_FRAMES;
        int n = ZDJ_SOUNDCARD_BUF_LEN < room ? ZDJ_SOUNDCARD_BUF_LEN : room;  // won't clip below target
        for( int i = 0; i < n; i++ ) {
            g_ring[ w*2+0 ] = cycle[ i*2+0 ];
            g_ring[ w*2+1 ] = cycle[ i*2+1 ];
            w = ( w + 1 ) % EMU_RING_FRAMES;
        }
        SDL_AtomicSet( &g_ring_w, w );  // publish only after the samples are written
    }
    return 0;
}

static void _audio_cb( void * ud, Uint8 * stream, int len ) {
    (void)ud;
    int32_t * dst = (int32_t *)stream;
    int need = len / (int)( 2 * sizeof( int32_t ) );
    int r = SDL_AtomicGet( &g_ring_r );
    int w = SDL_AtomicGet( &g_ring_w );
    while( need > 0 && r != w ) {
        dst[ 0 ] = g_ring[ r*2+0 ];
        dst[ 1 ] = g_ring[ r*2+1 ];
        r = ( r + 1 ) % EMU_RING_FRAMES;
        dst += 2; need--;
    }
    SDL_AtomicSet( &g_ring_r, r );
    if( need > 0 ) { SDL_AtomicAdd( &g_underrun_fr, need ); }
    for( int i = 0; i < need * 2; i++ ) { dst[ i ] = 0; }  // underrun -> silence
}

void zero_emu_audio_start( void ) {
    // Map the shared regions the soundcard's analog-io node fills BEFORE the
    // device is unpaused (the callback touches them immediately).
    g_audio_state = zdj_platform_map_shared( ZDJ_SHARED_AUDIO_STATE_ADDR, 0x1000 );
    g_dac = zdj_platform_map_shared( ZDJ_SHARED_DAC_BUF, 0x8000 );

    SDL_InitSubSystem( SDL_INIT_AUDIO );
    SDL_AudioSpec want; SDL_zero( want );
    want.freq = 44100; want.format = AUDIO_S32SYS; want.channels = 2;
    want.samples = 1024; want.callback = _audio_cb;
    g_audio_dev = SDL_OpenAudioDevice( NULL, 0, &want, NULL, 0 );
    if( !g_audio_dev ) { printf( "zero-emu: SDL audio open failed: %s\n", SDL_GetError( ) ); }

    // Start the producer and prebuffer to the target depth before unpausing.
    g_audio_run = 1;
    SDL_CreateThread( _audio_producer, "zero-emu-audio", NULL );
    for( int i = 0; i < 500 && _ring_fill( ) < EMU_RING_TARGET; i++ ) { SDL_Delay( 1 ); }
    SDL_PauseAudioDevice( g_audio_dev, 0 );
}

bool zero_emu_audio_active( void ) {
    return g_audio_dev != 0;
}

int zero_emu_audio_underruns( void ) {
    return SDL_AtomicGet( &g_underrun_fr );
}

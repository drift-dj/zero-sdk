// zero-emu: a desktop harness that drives libzerodj on a dev machine.
//
// It plays the role drift-os plays on the device: it brings up the library,
// then runs a frame loop. It also plays the role of the M7 co-processor -- it
// shares the same emulated memory regions as the library (via the emulator
// backend of zdj_platform_map_shared), reading the packed video buffer to draw
// the 128x64 panel into a scaled SDL window, and translating PC keyboard input
// into HMI state that the library's control thread consumes.
//
// Build: ./scripts/build_emu.sh   (CMake -DZDJ_EMU=ON, native host toolchain)

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>

#include <zerodj/controls/zdj_controls.h>
#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/zdj_deck_manager.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/system/display/zdj_display.h>
#include <zerodj/system/emu/zdj_emu_input.h>
#include <zerodj/system/usb/zdj_usb.h>
#include <zerodj/system/m7/zdj_m7.h>
#include <zerodj/system/m7/zdj_platform.h>
#include <zerodj/system/settings/zdj_settings.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/panel/zdj_ui_panel.h>
#include <zerodj/ui/view/zdj_view_stack.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>

#define EMU_W ZDJ_DISPLAY_WIDTH   // 128
#define EMU_H ZDJ_DISPLAY_HEIGHT  // 64

// Counts injected per encoder keypress. On hardware one physical detent of a
// quadrature encoder emits a burst of edges (the quad decode in the HMI scan
// yields ~4 per click), and the consumers are tuned for that -- notably the
// menu scroll filter is a momentum sim whose snap threshold a single count
// can't cross, so a +/-1 keypress decays without advancing an item (you'd have
// to hold the key to accumulate force). One detent's worth makes a discrete tap
// step exactly once. See zdj_menu_view_scroll_filter.c.
#define EMU_ENC_DETENT 4

static int   _env_int( const char * name, int fallback );
static void  _print_keymap( void );
static void  _handle_key( SDL_Keysym key, bool down );
static void  _unpack_video( const uint32_t * vid, uint32_t * argb );

// --- Track load + fake-M7 audio bridge --------------------------------------
// libzerodj's audio fast-cycle thread fills the DAC buffer once each time
// shared_audio_state->cycle_ready is asserted, advancing the deck transport by
// exactly ZDJ_SOUNDCARD_BUF_LEN frames per assertion. So the assert rate IS the
// playback clock and must match the audio device's consumption clock, or the
// stream drifts and underruns.
//
// To keep the blocking handshake out of the SDL callback (any jitter there is
// an instant glitch), a producer thread runs the M7 cycles into a ring buffer
// and the callback just drains it. The producer is paced by the ring fill
// level: it only produces while the ring is below a target depth. Because the
// callback drains at the exact device rate, that backpressure pins the
// producer's average rate -- and thus the deck's playback speed -- to 1x, while
// the target depth gives ~tens of ms of slack to absorb a slow cycle.

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

// Probe the file with ffmpeg to fill the metadata the decode node needs, build
// a song graph, and load it onto DJ deck station 1.
static zdj_deck_t * _load_track( const char * path ) {
    AVFormatContext * fmt = NULL;
    if( avformat_open_input( &fmt, path, NULL, NULL ) != 0 ) {
        printf( "zero-emu: could not open track: %s\n", path );
        return NULL;
    }
    avformat_find_stream_info( fmt, NULL );
    int sidx = -1;
    for( unsigned i = 0; i < fmt->nb_streams; i++ ) {
        if( fmt->streams[ i ]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO ) { sidx = (int)i; break; }
    }
    if( sidx < 0 ) { printf( "zero-emu: no audio stream in %s\n", path ); avformat_close_input( &fmt ); return NULL; }

    AVCodecParameters * cp = fmt->streams[ sidx ]->codecpar;
    double dur_sec = fmt->duration > 0 ? (double)fmt->duration / AV_TIME_BASE : 0.0;

    zdj_library_song_t * song = zdj_library_create_file_import_song_graph( (char *)path, zdj_library_db );
    song->audio->av_codec_id     = cp->codec_id;
    song->audio->av_stream_index = sidx;
    song->audio->av_sample_rate  = cp->sample_rate;
    song->audio->av_channel_count= cp->channels;
    song->audio->av_sample_format= cp->format;
    song->audio->duration_sec    = dur_sec;
    song->audio->duration_pcm    = (int64_t)( dur_sec * 44100.0 );
    printf( "zero-emu: track '%s' codec=0x%x ch=%d sr=%d dur=%.1fs\n",
            path, cp->codec_id, cp->channels, cp->sample_rate, dur_sec );
    avformat_close_input( &fmt );

    return zdj_deck_manager_add_deck( ZDJ_DECK_TYPE_DJ, ZDJ_DECK_STATION_1, (void *)song, 16 );
}

// Enqueue a play/pause toggle for deck 1; the control thread dispatches it.
static void _deck_play_pause( void ) {
    int w = zdj_get_next_deck_event_ind( );
    zdj_deck_event_buf[ w ].id = ZDJ_DECK_1_CONTROL_PLAY_PAUSE;
}

int main( int argc, char ** argv ) {
    (void)argc; (void)argv;
    setvbuf( stdout, NULL, _IONBF, 0 );  // unbuffered so logs survive a kill

    int   scale      = _env_int( "ZERO_EMU_SCALE", 6 );
    int   refresh_hz = _env_int( "ZERO_EMU_HZ", 30 );
    bool  full_ui    = _env_int( "ZERO_EMU_FULL_UI", 1 ) != 0;  // set 0 for the bare min-UI
    if( scale < 1 )      { scale = 1; }
    if( refresh_hz < 1 ) { refresh_hz = 1; }

    printf( "zero-emu: bringing up libzerodj (%s UI, %dx scale, %d Hz)\n",
            full_ui ? "full" : "min", scale, refresh_hz );

    // --- Library bring-up (mirrors drift-os's init order) ---------------------
    // settings first: opens/creates the settings DB and sets the UI refresh Hz.
    zdj_settings_init( );

    // UI: brings up SDL video, the software renderer + offscreen surface, fonts,
    // the texture atlas and the view stack. min-init skips the panel/widget
    // subsystems (which expect soundcard/library state); full-init builds them.
    if( full_ui ) {
        // The panels (soundcard is the default) read these subsystems during
        // draw, so they must exist before zdj_ui_init builds + draws the panels.
        // library DB (self-creates db/dirs/default data sources) for the browser.
        zdj_library_open_db( );
        // usb subsystem: allocates zdj_usb_state, which the browser panel draws.
        zdj_usb_init( );
        // deck manager first: zdj_soundcard_init adds a clock deck to it.
        zdj_deck_manager_init( );
        zdj_soundcard_init( NULL );  // NULL -> __temp__ record; self-creates the DB
        zdj_ui_init( );
        // Panels are retracted by default; deploy the current one (soundcard)
        // so there's something on screen. Tab toggles it at runtime.
        zdj_ui_panel_toggle( );
    } else {
        zdj_ui_min_init( );
    }

    // In min-UI mode there are no panels, so the screen is otherwise blank.
    // Push a couple of label views onto the root so the window visibly proves
    // the font + atlas render path end-to-end.
    if( !full_ui ) {
        zdj_view_t * title = zdj_new_label_view( "ZERO EMU", ZDJ_FONT_6, ZDJ_JUSTIFY_CENTER, ZDJ_SDL_WHITE );
        title->frame.x = 64 - title->frame.w / 2;
        title->frame.y = 20;
        zdj_add_subview( zdj_root_view( ), title );

        zdj_view_t * hint = zdj_new_label_view( "arrows + enter", ZDJ_FONT_6, ZDJ_JUSTIFY_CENTER, ZDJ_SDL_WHITE );
        hint->frame.x = 64 - hint->frame.w / 2;
        hint->frame.y = 36;
        zdj_add_subview( zdj_root_view( ), hint );
    }

    // controls: maps the (emulated) shared HMI model and launches the ~1 kHz
    // control cycle thread, whose scan stub pulls from our injection buffer.
    zdj_controls_init( );

    // --- Our window + the shared regions we share with the library -----------
    SDL_Window * win = SDL_CreateWindow(
        "Zero Emulator",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        EMU_W * scale, EMU_H * scale, SDL_WINDOW_SHOWN );
    if( !win ) {
        printf( "zero-emu: SDL_CreateWindow failed: %s\n", SDL_GetError( ) );
        return 1;
    }
    SDL_Renderer * ren = SDL_CreateRenderer( win, -1, SDL_RENDERER_ACCELERATED );
    if( !ren ) { ren = SDL_CreateRenderer( win, -1, 0 ); }
    SDL_Texture * tex = SDL_CreateTexture(
        ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, EMU_W, EMU_H );

    // Same backing buffers the library writes to (emulator zdj_platform backend
    // returns the same pointer for a given address to every caller).
    const uint32_t * vid = (const uint32_t *)zdj_platform_map_shared( ZDJ_SHARED_VIDEO_BUF_ADDR, 0x2000 );
    volatile zdj_shared_msg_buffer_t * msg = zdj_m7_shared_msg_buffer( );

    static uint32_t argb[ EMU_W * EMU_H ];

    // Optional: load a track onto a deck and bridge audio to the host.
    zdj_deck_t * deck = NULL;
    bool played = false;
    const char * track = getenv( "ZERO_EMU_TRACK" );
    if( full_ui && track && *track ) {
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

        deck = _load_track( track );

        // Start the producer and prebuffer to the target depth before unpausing.
        g_audio_run = 1;
        SDL_CreateThread( _audio_producer, "zero-emu-audio", NULL );
        for( int i = 0; i < 500 && _ring_fill( ) < EMU_RING_TARGET; i++ ) { SDL_Delay( 1 ); }
        SDL_PauseAudioDevice( g_audio_dev, 0 );
    }

    _print_keymap( );

    // --- Frame loop ----------------------------------------------------------
    bool running = true;
    long  frame_n = 0;
    bool  dumped = false;
    const char * dump_path  = getenv( "ZERO_EMU_DUMP" );
    long  dump_frame = _env_int( "ZERO_EMU_DUMP_FRAME", 90 );  // settle anims first
    bool  autokey   = _env_int( "ZERO_EMU_AUTOKEY", 0 ) != 0; // scripted next-panel tap
    Uint32 frame_ms = (Uint32)( 1000 / refresh_hz );
    while( running ) {
        // Scripted-input self-test: tap FN_3 (NEXT_PANEL) every ~40 frames so a
        // headless run can cycle through every panel and verify the keyboard ->
        // control thread -> UI loop drives the UI without crashing.
        if( autokey ) {
            long ph = frame_n % 40;
            if( ph == 30 ) { zdj_emu_input_button( ZDJ_EMU_BTN_FN_3, true ); }
            if( ph == 33 ) { zdj_emu_input_button( ZDJ_EMU_BTN_FN_3, false ); }
        }
        Uint32 t0 = SDL_GetTicks( );

        SDL_Event e;
        while( SDL_PollEvent( &e ) ) {
            switch( e.type ) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_KEYDOWN:
                    if( e.key.keysym.sym == SDLK_ESCAPE && ( e.key.keysym.mod & KMOD_SHIFT ) ) {
                        running = false;  // Shift+Esc quits; plain Esc is the NAV/back button.
                        break;
                    }
                    _handle_key( e.key.keysym, true );
                    break;
                case SDL_KEYUP:
                    _handle_key( e.key.keysym, false );
                    break;
                default:
                    break;
            }
        }

        // Once the loaded deck finishes spinning up its pipeline, start playback.
        if( deck && !played && deck->status == ZDJ_DECK_STATUS_RUNNING ) {
            printf( "zero-emu: deck RUNNING -> play\n" );
            _deck_play_pause( );
            played = true;
        }
        // Report audio-bridge underruns (~1/s) so we can see if the ring starves.
        if( g_audio_dev && frame_n > 0 && ( frame_n % refresh_hz ) == 0 ) {
            static int last_ur = 0;
            int ur = SDL_AtomicGet( &g_underrun_fr );
            if( ur != last_ur ) { printf( "zero-emu: underrun frames +%d (total %d)\n", ur - last_ur, ur ); last_ur = ur; }
        }

        // Drive the library: renders the view stack to its surface, packs the
        // pixels into the shared video buffer and raises update_display_req.
        // This also consumes the UI control events the control thread produced.
        zdj_ui_update( );

        if( msg->update_display_req ) {
            _unpack_video( vid, argb );
            SDL_UpdateTexture( tex, NULL, argb, EMU_W * (int)sizeof( uint32_t ) );
            SDL_RenderClear( ren );
            SDL_RenderCopy( ren, tex, NULL, NULL );
            SDL_RenderPresent( ren );
            msg->update_display_req = 0;  // ack, like the M7 would
            if( frame_n == 0 ) { printf( "zero-emu: first frame presented\n" ); }
            frame_n++;

            if( dump_path && *dump_path && !dumped && frame_n >= dump_frame ) {
                SDL_Surface * s = SDL_CreateRGBSurfaceWithFormatFrom(
                    argb, EMU_W, EMU_H, 32, EMU_W * 4, SDL_PIXELFORMAT_ARGB8888 );
                if( s ) { IMG_SavePNG( s, dump_path ); SDL_FreeSurface( s ); }
                printf( "zero-emu: dumped frame %ld to %s\n", frame_n, dump_path );
                dumped = true;
            }
        }

        Uint32 dt = SDL_GetTicks( ) - t0;
        if( dt < frame_ms ) { SDL_Delay( frame_ms - dt ); }
    }

    zdj_ui_deinit( );
    SDL_DestroyTexture( tex );
    SDL_DestroyRenderer( ren );
    SDL_DestroyWindow( win );
    return 0;
}

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

// PC keyboard -> HMI. Each encoder keypress injects one detent's worth of
// counts (so a discrete tap steps once; key repeat keeps stepping); buttons
// track held state. See _print_keymap for the layout.
static void _handle_key( SDL_Keysym key, bool down ) {
    switch( key.sym ) {
        // Jog / nav wheel: scroll + select.
        case SDLK_UP:    if( down ) { zdj_emu_input_encoder( ZDJ_EMU_ENC_JOG, -EMU_ENC_DETENT ); } break;
        case SDLK_DOWN:  if( down ) { zdj_emu_input_encoder( ZDJ_EMU_ENC_JOG, +EMU_ENC_DETENT ); } break;
        case SDLK_RETURN: zdj_emu_input_button( ZDJ_EMU_BTN_JOG, down ); break;

        // Output volume encoder (often drives panel scroll too).
        case SDLK_LEFT:  if( down ) { zdj_emu_input_encoder( ZDJ_EMU_ENC_OUT, -EMU_ENC_DETENT ); } break;
        case SDLK_RIGHT: if( down ) { zdj_emu_input_encoder( ZDJ_EMU_ENC_OUT, +EMU_ENC_DETENT ); } break;
        case SDLK_o:     zdj_emu_input_button( ZDJ_EMU_BTN_OUT, down ); break;

        // Transport / nav / hotcue.
        case SDLK_ESCAPE: zdj_emu_input_button( ZDJ_EMU_BTN_NAV, down ); break;
        case SDLK_TAB:    if( down ) { zdj_ui_panel_toggle( ); } break;
        case SDLK_p:      if( down ) { _deck_play_pause( ); } break;  // deck 1 play/pause
        case SDLK_SPACE:  zdj_emu_input_button( ZDJ_EMU_BTN_PLAY, down ); break;
        case SDLK_h:      zdj_emu_input_button( ZDJ_EMU_BTN_HOTCUE, down ); break;

        // Function buttons.
        case SDLK_1: zdj_emu_input_button( ZDJ_EMU_BTN_FN_1, down ); break;
        case SDLK_2: zdj_emu_input_button( ZDJ_EMU_BTN_FN_2, down ); break;
        case SDLK_3: zdj_emu_input_button( ZDJ_EMU_BTN_FN_3, down ); break;

        // Tone encoders (q/a, w/s, e/d = turn down/up) and their push switches.
        case SDLK_q: if( down ) { zdj_emu_input_encoder( ZDJ_EMU_ENC_TONE_1, -EMU_ENC_DETENT ); } break;
        case SDLK_a: if( down ) { zdj_emu_input_encoder( ZDJ_EMU_ENC_TONE_1, +EMU_ENC_DETENT ); } break;
        case SDLK_w: if( down ) { zdj_emu_input_encoder( ZDJ_EMU_ENC_TONE_2, -EMU_ENC_DETENT ); } break;
        case SDLK_s: if( down ) { zdj_emu_input_encoder( ZDJ_EMU_ENC_TONE_2, +EMU_ENC_DETENT ); } break;
        case SDLK_e: if( down ) { zdj_emu_input_encoder( ZDJ_EMU_ENC_TONE_3, -EMU_ENC_DETENT ); } break;
        case SDLK_d: if( down ) { zdj_emu_input_encoder( ZDJ_EMU_ENC_TONE_3, +EMU_ENC_DETENT ); } break;

        default: break;
    }
}

static void _print_keymap( void ) {
    printf(
        "\nzero-emu keymap:\n"
        "  Up/Down ....... jog wheel scroll        Enter ... jog press (select)\n"
        "  Left/Right .... output encoder          o ....... output encoder press\n"
        "  Esc ........... NAV (back)              Space ... PLAY        h ... HOTCUE\n"
        "  Tab ........... deploy/retract panel\n"
        "  1 / 2 / 3 ..... FN1 / FN2 / FN3\n"
        "  q/a w/s e/d ... tone 1/2/3 encoder (turn down/up)\n"
        "  p ............. deck 1 play/pause (with ZERO_EMU_TRACK)\n"
        "  Shift+Esc ..... quit\n\n" );
}

static int _env_int( const char * name, int fallback ) {
    const char * v = getenv( name );
    if( !v || !*v ) { return fallback; }
    return atoi( v );
}

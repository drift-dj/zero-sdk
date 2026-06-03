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

#include <zerodj/controls/zdj_controls.h>
#include <zerodj/library/zdj_library.h>
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

static int   _env_int( const char * name, int fallback );
static void  _print_keymap( void );
static void  _handle_key( SDL_Keysym key, bool down );
static void  _unpack_video( const uint32_t * vid, uint32_t * argb );

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

// PC keyboard -> HMI. Encoders inject a per-press delta (key repeat scrolls);
// buttons track held state. See _print_keymap for the layout.
static void _handle_key( SDL_Keysym key, bool down ) {
    switch( key.sym ) {
        // Jog / nav wheel: scroll + select.
        case SDLK_UP:    if( down ) { zdj_emu_input_encoder( ZDJ_EMU_ENC_JOG, +1 ); } break;
        case SDLK_DOWN:  if( down ) { zdj_emu_input_encoder( ZDJ_EMU_ENC_JOG, -1 ); } break;
        case SDLK_RETURN: zdj_emu_input_button( ZDJ_EMU_BTN_JOG, down ); break;

        // Output volume encoder (often drives panel scroll too).
        case SDLK_LEFT:  if( down ) { zdj_emu_input_encoder( ZDJ_EMU_ENC_OUT, -1 ); } break;
        case SDLK_RIGHT: if( down ) { zdj_emu_input_encoder( ZDJ_EMU_ENC_OUT, +1 ); } break;
        case SDLK_o:     zdj_emu_input_button( ZDJ_EMU_BTN_OUT, down ); break;

        // Transport / nav / hotcue.
        case SDLK_ESCAPE: zdj_emu_input_button( ZDJ_EMU_BTN_NAV, down ); break;
        case SDLK_TAB:    if( down ) { zdj_ui_panel_toggle( ); } break;
        case SDLK_SPACE:  zdj_emu_input_button( ZDJ_EMU_BTN_PLAY, down ); break;
        case SDLK_h:      zdj_emu_input_button( ZDJ_EMU_BTN_HOTCUE, down ); break;

        // Function buttons.
        case SDLK_1: zdj_emu_input_button( ZDJ_EMU_BTN_FN_1, down ); break;
        case SDLK_2: zdj_emu_input_button( ZDJ_EMU_BTN_FN_2, down ); break;
        case SDLK_3: zdj_emu_input_button( ZDJ_EMU_BTN_FN_3, down ); break;

        // Tone encoders (q/a, w/s, e/d = turn down/up) and their push switches.
        case SDLK_q: if( down ) { zdj_emu_input_encoder( ZDJ_EMU_ENC_TONE_1, -1 ); } break;
        case SDLK_a: if( down ) { zdj_emu_input_encoder( ZDJ_EMU_ENC_TONE_1, +1 ); } break;
        case SDLK_w: if( down ) { zdj_emu_input_encoder( ZDJ_EMU_ENC_TONE_2, -1 ); } break;
        case SDLK_s: if( down ) { zdj_emu_input_encoder( ZDJ_EMU_ENC_TONE_2, +1 ); } break;
        case SDLK_e: if( down ) { zdj_emu_input_encoder( ZDJ_EMU_ENC_TONE_3, -1 ); } break;
        case SDLK_d: if( down ) { zdj_emu_input_encoder( ZDJ_EMU_ENC_TONE_3, +1 ); } break;

        default: break;
    }
}

static void _print_keymap( void ) {
    printf(
        "\nzero-emu keymap:\n"
        "  Up/Down ....... jog wheel scroll        Enter ... jog press (select)\n"
        "  Left/Right .... output encoder          o ....... output encoder press\n"
        "  Esc ........... NAV (back)              Space ... PLAY        h ... HOTCUE\n"
        "  Tab ........... deploy/retract panel    Enter-hold ... same (jog long-press)\n"
        "  1 / 3 ......... prev / next panel        (FN1/FN3; 2 = FN2)\n"
        "  q/a w/s e/d ... tone 1/2/3 encoder (turn down/up)\n"
        "  Shift+Esc ..... quit\n\n" );
}

static int _env_int( const char * name, int fallback ) {
    const char * v = getenv( name );
    if( !v || !*v ) { return fallback; }
    return atoi( v );
}

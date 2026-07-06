// zero-emu: a desktop harness that drives libzerodj on a dev machine.
//
// It plays the role drift-os plays on the device: it brings up the library,
// then runs a frame loop. It also plays the role of the M7 co-processor -- it
// shares the same emulated memory regions as the library (via the emulator
// backend of zdj_platform_map_shared), presenting the packed video buffer in
// a scaled SDL window (zero_emu_video), bridging the DAC to the host audio
// device (zero_emu_audio), and translating PC keyboard input into HMI state
// that the library's control thread consumes.
//
// Build: ./scripts/build_emu.sh   (CMake -DZDJ_EMU=ON, native host toolchain)

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h>

#include <libavformat/avformat.h>
#include <libavutil/avutil.h>

#include <zerodj/controls/zdj_controls.h>
#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/zdj_deck_manager.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/system/emu/zdj_emu_input.h>
#include <zerodj/system/usb/zdj_usb.h>
#include <zerodj/system/m7/zdj_m7.h>
#include <zerodj/system/settings/zdj_settings.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/panel/zdj_ui_panel.h>
#include <zerodj/ui/view/zdj_view_stack.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>

#include "zero_emu_audio.h"
#include "zero_emu_video.h"

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

    // --- Our window + the M7 message handshake --------------------------------
    if( !zero_emu_video_init( scale ) ) {
        return 1;
    }
    volatile zdj_shared_msg_buffer_t * msg = zdj_m7_shared_msg_buffer( );

    // Optional: load a track onto a deck and bridge audio to the host.
    zdj_deck_t * deck = NULL;
    bool played = false;
    const char * track = getenv( "ZERO_EMU_TRACK" );
    if( full_ui && track && *track ) {
        zero_emu_audio_start( );
        deck = _load_track( track );
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
        if( zero_emu_audio_active( ) && frame_n > 0 && ( frame_n % refresh_hz ) == 0 ) {
            static int last_ur = 0;
            int ur = zero_emu_audio_underruns( );
            if( ur != last_ur ) { printf( "zero-emu: underrun frames +%d (total %d)\n", ur - last_ur, ur ); last_ur = ur; }
        }

        // Drive the library: renders the view stack to its surface, packs the
        // pixels into the shared video buffer and raises update_display_req.
        // This also consumes the UI control events the control thread produced.
        zdj_ui_update( );

        if( msg->update_display_req ) {
            zero_emu_video_present( );
            msg->update_display_req = 0;  // ack, like the M7 would
            if( frame_n == 0 ) { printf( "zero-emu: first frame presented\n" ); }
            frame_n++;

            if( dump_path && *dump_path && !dumped && frame_n >= dump_frame ) {
                zero_emu_video_dump( dump_path );
                printf( "zero-emu: dumped frame %ld to %s\n", frame_n, dump_path );
                dumped = true;
            }
        }

        Uint32 dt = SDL_GetTicks( ) - t0;
        if( dt < frame_ms ) { SDL_Delay( frame_ms - dt ); }
    }

    zdj_ui_deinit( );
    zero_emu_video_deinit( );
    return 0;
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

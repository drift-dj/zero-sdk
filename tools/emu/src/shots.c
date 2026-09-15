// zero-shots: headless, deterministic screenshots of the libzerodj UI.
//
// Brings the library up the way zero-emu does, but with no window, no audio
// and no wall clock in the input path. The control cycle runs on this thread:
// before each UI frame it steps a fixed number of control cycles, so injected
// input reaches the UI on the same frame every run. After the last frame it
// writes the shared video buffer -- the 4-bit frame the M7 sends to the panel,
// not the SDL surface -- as an 8-bit greyscale PNG, and prints a hash of it.
//
// --threaded runs the stock control cycle thread in real time instead, with a
// sleep per frame like zero-emu, as a baseline to compare against.
//
// Run inside the fake root (scripts/run_emu.sh with ZERO_EMU_BIN set) so the
// library finds /root/res and /media/internal.

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <png.h>

#include <zerodj/controls/zdj_controls.h>
#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/deck/zdj_deck_manager.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/system/display/zdj_display.h>
#include <zerodj/system/emu/zdj_emu_input.h>
#include <zerodj/system/log/zdj_log.h>
#include <zerodj/system/m7/zdj_m7.h>
#include <zerodj/system/m7/zdj_platform.h>
#include <zerodj/system/settings/zdj_settings.h>
#include <zerodj/system/usb/zdj_usb.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/panel/zdj_ui_panel.h>

#define SHOT_W ZDJ_DISPLAY_WIDTH
#define SHOT_H ZDJ_DISPLAY_HEIGHT

// The device's control thread sleeps 800us per cycle, so it runs at roughly
// 1 kHz. Stepping this many cycles per second of UI frames keeps the HMI state
// machines' cycle-counted windows (debounce, long press) close to their
// on-device durations.
#define SHOT_CONTROL_HZ 1000

typedef enum {
    SHOT_PRESS,
    SHOT_RELEASE,
} shot_action_t;

typedef struct {
    int           frame;
    shot_action_t action;
    zdj_emu_btn_t btn;
} shot_input_t;

// Prototype fixture: tap NEXT_PANEL (FN3, which fires on release) twice.
static const shot_input_t _next_panel_x2[ ] = {
    {  5, SHOT_PRESS,   ZDJ_EMU_BTN_FN_3 },
    {  7, SHOT_RELEASE, ZDJ_EMU_BTN_FN_3 },
    { 25, SHOT_PRESS,   ZDJ_EMU_BTN_FN_3 },
    { 27, SHOT_RELEASE, ZDJ_EMU_BTN_FN_3 },
};
#define SHOT_INPUT_COUNT ( sizeof( _next_panel_x2 ) / sizeof( _next_panel_x2[ 0 ] ) )

static void _usage( void ) {
    fprintf( stderr,
        "usage: zero-shots [--frames N] [--out PATH.png] [--threaded]\n"
        "  --frames N    UI frames to run before capturing (default 60)\n"
        "  --out PATH    write the captured frame as an 8-bit grey PNG\n"
        "  --threaded    use the real-time control thread (non-deterministic baseline)\n" );
}

// Unpack the shared video buffer (four 4-bit pixels per word, packed by
// zdj_display_m7_push as (level << 4) | 0xF) into one 8-bit grey byte per
// pixel, scaling the 16 levels onto 0..255.
static void _unpack_grey( const uint32_t * vid, uint8_t * grey ) {
    for( int i = 0; i < ( SHOT_W * SHOT_H ) / 4; i++ ) {
        for( int k = 0; k < 4; k++ ) {
            uint8_t level = ( vid[ i ] >> ( 24 - 8 * k + 4 ) ) & 0xF;
            grey[ i * 4 + k ] = level * 0x11;
        }
    }
}

// FNV-1a over the decoded pixels, so runs can be compared without the PNG
// encoding getting in the way.
static uint64_t _hash( const uint8_t * grey ) {
    uint64_t h = 0xcbf29ce484222325ull;
    for( int i = 0; i < SHOT_W * SHOT_H; i++ ) {
        h ^= grey[ i ];
        h *= 0x100000001b3ull;
    }
    return h;
}

static bool _write_png( const char * path, const uint8_t * grey ) {
    FILE * fp = fopen( path, "wb" );
    if( !fp ) { perror( path ); return false; }
    png_structp png = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
    png_infop info = png ? png_create_info_struct( png ) : NULL;
    if( !info || setjmp( png_jmpbuf( png ) ) ) {
        png_destroy_write_struct( &png, &info );
        fclose( fp );
        return false;
    }
    png_init_io( png, fp );
    png_set_IHDR( png, info, SHOT_W, SHOT_H, 8, PNG_COLOR_TYPE_GRAY,
        PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT );
    png_write_info( png, info );
    for( int y = 0; y < SHOT_H; y++ ) {
        png_write_row( png, (png_const_bytep)&grey[ y * SHOT_W ] );
    }
    png_write_end( png, NULL );
    png_destroy_write_struct( &png, &info );
    fclose( fp );
    return true;
}

int main( int argc, char ** argv ) {
    int          frames   = 60;
    const char * out      = NULL;
    bool         threaded = false;
    for( int i = 1; i < argc; i++ ) {
        if( !strcmp( argv[ i ], "--frames" ) && i + 1 < argc ) {
            frames = atoi( argv[ ++i ] );
        } else if( !strcmp( argv[ i ], "--out" ) && i + 1 < argc ) {
            out = argv[ ++i ];
        } else if( !strcmp( argv[ i ], "--threaded" ) ) {
            threaded = true;
        } else {
            _usage( );
            return 2;
        }
    }

    setvbuf( stdout, NULL, _IONBF, 0 );  // unbuffered so logs survive a crash

    // No window: zdj_ui_init only needs SDL video for its software renderer.
    setenv( "SDL_VIDEODRIVER", "dummy", 0 );

    // Library bring-up, in zero-emu's (and drift-os's) order.
    zdj_settings_init( );
    zdj_log_init( );
    zdj_library_open_db( );
    zdj_usb_init( );
    zdj_deck_manager_init( );
    zdj_soundcard_init( NULL );
    zdj_ui_init( );
    zdj_ui_panel_toggle( );
    if( threaded ) {
        zdj_controls_init( );
    } else {
        zdj_controls_init_stepped( );
    }

    int steps_per_frame = SHOT_CONTROL_HZ / zdj_ui_refresh_hz;
    struct timespec frame_sleep = { 0, zdj_ui_get_frame_nanos( ) };
    volatile zdj_shared_msg_buffer_t * msg = zdj_m7_shared_msg_buffer( );

    size_t next_input = 0;
    for( int frame = 0; frame < frames; frame++ ) {
        while( next_input < SHOT_INPUT_COUNT && _next_panel_x2[ next_input ].frame == frame ) {
            const shot_input_t * in = &_next_panel_x2[ next_input++ ];
            zdj_emu_input_button( in->btn, in->action == SHOT_PRESS );
        }
        if( threaded ) {
            nanosleep( &frame_sleep, NULL );
        } else {
            for( int s = 0; s < steps_per_frame; s++ ) { zdj_control_cycle_step( ); }
        }
        zdj_ui_update( );
        msg->update_display_req = 0;  // ack, like the M7 would
    }

    const uint32_t * vid = zdj_platform_map_shared( ZDJ_SHARED_VIDEO_BUF_ADDR, 0x2000 );
    uint8_t grey[ SHOT_W * SHOT_H ];
    _unpack_grey( vid, grey );
    printf( "zero-shots: hash %016llx\n", (unsigned long long)_hash( grey ) );
    if( out && !_write_png( out, grey ) ) {
        fprintf( stderr, "zero-shots: failed to write %s\n", out );
        return 1;
    }
    // Library threads (usb, deck manager) are still running; don't unwind.
    fflush( stdout );
    _exit( 0 );
}

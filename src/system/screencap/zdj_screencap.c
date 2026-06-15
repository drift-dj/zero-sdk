#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <uuid.h>

// #include <SDL2/SDL_image.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/system/screencap/zdj_screencap.h>
#include <zerodj/system/settings/zdj_settings.h>
#include <zerodj/library/zdj_library.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/widget/notify/zdj_notify_widget.h>

// NEW API FOR IMAGE/VIDEO CAPTURE
void zdj_request_screencap( zdj_screencap_type_t type ) {
    zdj_screen_cap_armed = true;

}
void zdj_end_screencap( ) {
    zdj_screen_cap_armed = false;
}


void zdj_capture_screen( void ) { zdj_screen_cap_armed = true; }

// void zdj_write_screen_cap( void ) {
void zdj_update_screencap( void ) {
    // printf( "zdj_write_screen_cap\n" );

    zdj_screen_cap_armed = false;
    
    char uuid_str[ 64 ];
    char path[ 512 ];

    uuid_t uuid;
    uuid_generate( uuid );
    uuid_unparse_lower( uuid, uuid_str );

    int screencap_num = zdj_setting_increment_int( ZDJ_SETTING_SCREENSHOT_COUNTER );

    snprintf( path, sizeof( path ), "%s/zero_screencap_%03d.bmp",
        ZDJ_SCREEN_CAP_DIR,
        screencap_num
    );

    printf( "saving screenshot: %s\n", path );


    int width, height;
    SDL_GetRendererOutputSize( zdj_renderer( ), &width, &height);
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!surface) { printf( "failed to create screenshot surface\n" ); return; }
    if ( SDL_RenderReadPixels( zdj_renderer( ), NULL, SDL_PIXELFORMAT_ARGB8888, surface->pixels, surface->pitch ) != 0) { printf( "failed to read screenshot pixels\n" ); return; }
    SDL_SetHint(SDL_HINT_BMP_SAVE_LEGACY_FORMAT, "1");
    if( SDL_SaveBMP( surface, path ) != 0 ) { printf( "Error saving BMP: %s\n", SDL_GetError( ) ); }

    SDL_FreeSurface( surface );

    sync( );

    char note[ 32 ];
    snprintf( note, sizeof( note ), "zero_screencap_%03d.bmp", screencap_num );
    zdj_show_notify_widget( note, NULL, NULL );
}

void zdj_reset_screencaps( void ) {
    zdj_fs_remove_dir( ZDJ_SCREEN_CAP_DIR );
    zdj_fs_mkdir_p( ZDJ_SCREEN_CAP_DIR );
    sync( );
    zdj_setting_set_int( ZDJ_SETTING_SCREENSHOT_COUNTER, 0 );
}
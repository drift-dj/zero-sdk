#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <SDL2/SDL.h>

#include <zerodj/display/zdj_display.h>
#include <zerodj/m7/zdj_m7.h>
#include <zerodj/ui/zdj_ui.h>

uint32_t * zdj_vid_buffer;

void zdj_display_init( void ) {
    // Grab references to the shared (M7+A53) memory.
    // See zero kernel 'drift-a106.dtsi' reserved-memory section.
	int mem_fd = open( "/dev/mem", O_RDWR );
	zdj_vid_buffer = (uint32_t *)mmap(0, 0x20000, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, 0x55a11000);
}

void zdj_display_m7_push( void ) {
    // Bug out if we haven't brought up SDL yet
    if( !zdj_ui_pixels ) { return; }

    // Pack SDL's pixels into shared video buffer
    for( int i=0; i<(ZDJ_SCREEN_WIDTH*ZDJ_SCREEN_HEIGHT)/4; i++ ) {
        zdj_vid_buffer[ i ] = ((zdj_ui_pixels[i*4]&0xFF) << 24) +
                          ((zdj_ui_pixels[i*4+1]&0xFF) << 16) +
                          ((zdj_ui_pixels[i*4+2]&0xFF) << 8) +
                          (zdj_ui_pixels[i*4+3]&0xFF);
    }
    // Signal the M7 core to transfer pixel data to the screen (see zero-m7 repo).
    // write( m7( ), "ud", strlen( "ud" )+1 );
    zdj_m7_msg( ZDJ_M7_UPDATE_DISPLAY );
}


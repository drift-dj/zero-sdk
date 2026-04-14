#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <SDL2/SDL.h>

#include <zerodj/system/display/zdj_display.h>
#include <zerodj/system/m7/zdj_m7.h>
#include <zerodj/system/settings/zdj_settings.h>
#include <zerodj/ui/zdj_ui.h>

uint32_t * zdj_vid_buffer;
volatile bool zdj_display_flip;

void zdj_display_init( void ) {
    // printf( "zdj_display_init\n" );
    // Grab references to the shared (M7+A53) memory.
    // See zero kernel 'drift-a106.dtsi' reserved-memory section.
	int mem_fd = open( "/dev/mem", O_RDWR );
    zdj_vid_buffer = (uint32_t *)mmap(NULL, 0x2000, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, ZDJ_SHARED_VIDEO_BUF_ADDR);

    zdj_display_flip = false;
}

void zdj_display_m7_push( void ) {
    // Bug out if we haven't brought up SDL yet
    if( !zdj_ui_pixels ) { return; }

    uint32_t quad = 0;
    uint32_t p1 = 0;
    uint32_t p2 = 0;
    uint32_t p3 = 0;
    uint32_t p4 = 0;
    // Pack SDL's pixels into shared video buffer

    if( zdj_display_flip ){ 
        int display_buf_len = ((ZDJ_DISPLAY_WIDTH*ZDJ_DISPLAY_HEIGHT)-1)/4;
        for( int i=display_buf_len; i>0; i-- ) {
            p1 = ((zdj_ui_pixels[i*4+3]&0xF0)+0xF) << 24;
            p2 = ((zdj_ui_pixels[i*4+2]&0xF0)+0xF) << 16;
            p3 = ((zdj_ui_pixels[i*4+1]&0xF0)+0xF) << 8;
            p4 = ((zdj_ui_pixels[i*4]&0xF0)+0xF);
            zdj_vid_buffer[ display_buf_len - i ] = p1+p2+p3+p4;
        }
    } else {
        for( int i=0; i<(ZDJ_DISPLAY_WIDTH*ZDJ_DISPLAY_HEIGHT)/4; i++ ) {
            p1 = ((zdj_ui_pixels[i*4]&0xF0)+0xF) << 24;
            p2 = ((zdj_ui_pixels[i*4+1]&0xF0)+0xF) << 16;
            p3 = ((zdj_ui_pixels[i*4+2]&0xF0)+0xF) << 8;
            p4 = ((zdj_ui_pixels[i*4+3]&0xF0)+0xF);
            zdj_vid_buffer[ i ] = p1+p2+p3+p4;
        }
    }

    

    // Signal the M7 core to transfer pixel data to the screen (see zero-m7 repo).
    zdj_m7_shared_msg_buffer( )->update_display_req = true;
}


#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>


#include <zerodj/boot/zdj_boot.h>
#include <zerodj/m7/zdj_m7.h>

// int zdj_m7_fd;
static volatile zdj_shared_msg_buffer_t * _zdj_shared_msg_buffer;
// static volatile zdj_shared_audio_state_t * _zdj_shared_audio_state;
// static bool _zdj_m7_shared_audio_init = false;

volatile zdj_shared_msg_buffer_t * zdj_m7_shared_msg_buffer( void ) {
    if( !_zdj_shared_msg_buffer ) {
        int mem_fd = open( "/dev/mem", O_RDWR );
        _zdj_shared_msg_buffer = mmap(0, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, ZDJ_SHARED_MSG_BUF_ADDR);
    }
    return _zdj_shared_msg_buffer;
}

// volatile zdj_shared_audio_state_t * zdj_m7_shared_audio_state( void ){
//     if( !_zdj_m7_shared_audio_init ) {
//         _zdj_m7_shared_audio_init = true;
//         int mem_fd = open( "/dev/mem", O_RDWR );
        
//         _zdj_shared_audio_state = (zdj_shared_audio_state_t*)mmap(0, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, ZDJ_SHARED_AUDIO_STATE_ADDR);
//     }
//     return _zdj_shared_audio_state;
// }
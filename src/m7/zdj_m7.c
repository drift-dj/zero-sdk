#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "zdj_m7.h"

int zdj_m7_fd;

// See zero-m7 repo for corresponding message defs.
char * _zdj_m7_msg_str[] = {
    "fh",
    "ud",
    "ah",
    "dh",
    "bs",
    "de",
    "tm",
    "dr",
    "er"
};

// Return a reference to the M7 core's rpmsg endpoint.
int zdj_m7( void ) {
    // Lazy-load the endpoint.
    if( !zdj_m7_fd ) {
        zdj_m7_fd = open( "/dev/rpmsg0", O_RDWR );
    }
    return zdj_m7_fd;
}

void zdj_m7_msg( zdj_m7_message_t msg ) {
    write( zdj_m7( ), _zdj_m7_msg_str[ msg ], 3 );
}
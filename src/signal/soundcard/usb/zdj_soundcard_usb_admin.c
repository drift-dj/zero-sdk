#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/signal/soundcard/usb/zdj_soundcard_usb.h>
#include <zerodj/system/usb/zdj_usb.h>

pthread_t _zdj_soundcard_usb_admin_thread;
static void * _admin_thread_main( void * arg );

void zdj_soundcard_launch_usb_admin_thread( void ) {
    pthread_create( 
        &_zdj_soundcard_usb_admin_thread, 
        NULL, 
        _admin_thread_main, 
        (void*)zdj_usb_state 
    );
}

static void * _admin_thread_main( void * arg ) {
    // Set core affinity to Core #1;
    cpu_set_t cpuset;
	CPU_ZERO( &cpuset );
	CPU_SET( 0,&cpuset );
	int err = sched_setaffinity( syscall(SYS_gettid), sizeof(cpu_set_t), &cpuset );
    if( err != 0 ) {
        perror( "set affinity failed" );
    }
    
    // while( 1 ) {
    //     if( zdj_usb_state->host_state.has_soundcard_update ) { 
    //         // Look for all attached devices which don't have nodes in soundcard
    //         // If devices isn't in block list
    //             // Read sound card data
    //     } else if( zdj_usb_state->gadget_state.has_update ) { 

    //     } 

    //     sleep( 1 );
    // }
}
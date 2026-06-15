#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <sys/syscall.h>

#include <zerodj/signal/pipeline/node/audio/io/zdj_io_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/signal/soundcard/usb/zdj_soundcard_usb.h>
#include <zerodj/system/usb/zdj_usb.h>

pthread_t _zdj_soundcard_usb_admin_thread;
static void * _admin_thread_main( void * arg );

void zdj_soundcard_launch_usb_admin_thread( zdj_soundcard_t * soundcard ) {
    pthread_create( 
        &_zdj_soundcard_usb_admin_thread, 
        NULL, 
        _admin_thread_main, 
        (void*)soundcard
    );
}

static void * _admin_thread_main( void * arg ) {
    zdj_soundcard_t * admin_soundcard = (zdj_soundcard_t*)arg;
    // Set core affinity to Core #1;
    cpu_set_t cpuset;
	CPU_ZERO( &cpuset );
	CPU_SET( 0,&cpuset );
	int err = sched_setaffinity( syscall(SYS_gettid), sizeof(cpu_set_t), &cpuset );
    if( err != 0 ) {
        perror( "set affinity failed" );
    }
    
    // ARCH DEV NOTE:
    // This is designed to do thge ALSA device pre-processing off the Fast Soundcard thread.
    // ALSA may not like this much.  So it may change as the system firms up.
    while( 1 ) {
        if( (zdj_usb_state != NULL) && 
            zdj_usb_state->mode_state.mode == ZDJ_USB_MODE_HOST &&
            zdj_usb_state->host_state.has_soundcard_update &&
            zdj_usb_state->host_state.attached.count > 0
        ) { 
            
            // Call into the USB I/O node and hand it a new device to prepare.
            // Note that this should only read/test the device HW params to find
            // the right combination of sample rate, data format, etc. for the
            // attached UAC2 device.
            // Actually calling snd_pcm_start( ) must happen on the Soundcard Fast thread
            // since that's where the buffer data will be read/written.
            // Open questions as to whether we can cal prepare / writei to charge on this thread
            zdj_io_usb_node_state_t * io_node_state = (zdj_io_usb_node_state_t*)admin_soundcard->usb_io_node->state;
            // printf( "usb_io_node_state 1: %p %p %d\n", 
            //     admin_soundcard->usb_io_node, io_node_state, io_node_state->phase
            // );
            printf( "atempting USB device hwparam discovery: %d\n", io_node_state->phase );
            if( io_node_state->phase == ZDJ_IO_USB_PHASE_INIT ) {
                zdj_io_usb_discover_hwparams(
                    admin_soundcard->usb_io_node,
                    zdj_usb_state->host_state.attached.devices
                );
                zdj_usb_state->host_state.has_soundcard_update = false;
            }
            if( io_node_state->phase == ZDJ_IO_USB_PHASE_TEARDOWN ) {
                zdj_io_usb_alsa_teardown( admin_soundcard->usb_io_node );
            }
        }

        sleep( 1 );
    }
}
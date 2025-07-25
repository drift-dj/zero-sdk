#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/inotify.h>

#include <zerodj/error/zdj_error.h>
#include <zerodj/usb/zdj_usb.h>

#define NAME_MAX 255

zdj_usb_status_t * zdj_usb_status;
static pthread_t _zdj_usb_status_thread;
void * _zdj_usb_status_thread_loop( void * arg );

zdj_error_type_t zdj_usb_init_frontend( void ) {
    // Bringup inital status
    zdj_usb_status = calloc( 1, sizeof( zdj_usb_status_t ) );
    FILE * fd = fopen( ZDJ_USB_STATUS_PATH, "r" );
    if( !fd ) { 
        exit( 1 ); 
    } else {
        fseek( fd, 0, SEEK_SET );
        fread( zdj_usb_status, sizeof( zdj_usb_status_t ), 1, fd );
        fclose( fd );
    }

    printf( "usb front-end at boot: %s, %s, %d\n", 
            zdj_usb_mode_name[ zdj_usb_status->mode ],
            zdj_usb_submode_name[ zdj_usb_status->submode ],
            zdj_usb_status->requires_reboot
        );
        printf( "gadget_config: au:%d mid:%d msd:%d hid:%d shl:%d\n",
            zdj_usb_status->gadget_config.uac2,
            zdj_usb_status->gadget_config.midi,
            zdj_usb_status->gadget_config.mass_storage,
            zdj_usb_status->gadget_config.hid,
            zdj_usb_status->gadget_config.shell
        );

    // Bringup inotify thread to detect status change from zero-usb backend
    pthread_create( &_zdj_usb_status_thread, NULL, _zdj_usb_status_thread_loop, zdj_usb_status );

    return ZDJ_ERROR_OKAY;
}

// Thread to trigger front-end update when inotify detects a change in the usb status file 
// stored in /etc/zero_data. (change is made by zero-usb backend.)
void * _zdj_usb_status_thread_loop( void * arg ) {
    zdj_usb_status_t * status = (zdj_usb_status_t *)arg;

    struct inotify_event * event;
    int buf_len = 1 * ( sizeof(struct inotify_event ) + NAME_MAX + 1 );
    char buf[ buf_len ] __attribute__ ((aligned(8)));
    int inotifyFd = inotify_init( );
    int wd = inotify_add_watch( inotifyFd, ZDJ_USB_STATUS_PATH, IN_CLOSE_WRITE );

    while ( 1 ) {
        // Sleep thread until inotify has an event.
        read( inotifyFd, buf, buf_len );

        printf( "usb_status inotify\n" );
        // Overwrite current status with data in file and alert frontend.
        FILE * fd = fopen( ZDJ_USB_STATUS_PATH, "r" );
        if( !fd ) { 
            exit( 1 ); 
        } else {
            fseek( fd, 0, SEEK_SET );
            fread( status, sizeof( zdj_usb_status_t ), 1, fd );
            // Update the config based on functionFS entries.
            zdj_usb_update_gadget_config_from_functionfs( &status->gadget_config );
            fclose( fd );

        }

        printf( "usb front-end got update: %s, %s, %d\n", 
            zdj_usb_mode_name[ status->mode ],
            zdj_usb_submode_name[ status->submode ],
            status->requires_reboot
        );
        printf( "gadget_config: au:%d mid:%d msd:%d hid:%d shl:%d\n",
            status->gadget_config.uac2,
            status->gadget_config.midi,
            status->gadget_config.mass_storage,
            status->gadget_config.hid,
            status->gadget_config.shell
        );

        status->has_frontend_update = true;
    }
}

// Write a request struct to the command file.
// zero-usb will receive an inotify update when this happens.
// zero-usb manages the state transition and reports status
// in the status file. (Status file is read in inotify thread)
zdj_error_type_t zdj_usb_request_mode_switch( zdj_usb_mode_request_t * request ) {
    printf( "zdj_usb_request_mode: %s, %s\n", 
        zdj_usb_mode_name[ request->mode ],
        zdj_usb_submode_name[ request->submode ]
    );
    // Return error for request to enter current mode
    if( request->mode == zdj_usb_status->mode && request->submode == zdj_usb_status->submode ) {
        printf( "request to enter current mode\n" );
        return ZDJ_USB_ALREADY_IN_MODE;
    }

    // Build a submode from the gadget_config if we need to.
    if( request->submode == ZDJ_USB_SUBMODE_UNKNOWN ) {
        request->submode = zdj_usb_submode_for_gadget_config( &request->gadget_config );
    }
    FILE * fd = fopen( ZDJ_USB_REQUEST_PATH, "w" );
    if( !fd ) { return ZDJ_ERROR_BAD_COMMAND; }

    if( fd ) {
        fseek( fd, 0, SEEK_SET );
        fwrite( request, sizeof( zdj_usb_mode_request_t ), 1, fd );
        fclose( fd );
    }
    return ZDJ_ERROR_OKAY;
}

zdj_usb_submode_t zdj_usb_submode_for_gadget_config( zdj_usb_gadget_config_t * config ) {
    if( 
        config->uac2 && 
        !config->midi && 
        !config->mass_storage && 
        !config->hid &&
        !config->shell
    ) {
        return ZDJ_USB_SUBMODE_GADGET_UAC2;
    } else if( 
        config->uac2 && 
        !config->midi && 
        !config->mass_storage && 
        !config->hid &&
        config->shell
    ) {
        return ZDJ_USB_SUBMODE_GADGET_UAC2_SHELL;
    } else if( 
        config->uac2 && 
        config->midi && 
        !config->mass_storage && 
        !config->hid &&
        !config->shell
    ) {
        return ZDJ_USB_SUBMODE_GADGET_UAC2_MIDI;
    } else if( 
        config->uac2 && 
        config->midi && 
        !config->mass_storage && 
        !config->hid &&
        config->shell
    ) {
        return ZDJ_USB_SUBMODE_GADGET_UAC2_MIDI_SHELL;
    } else if( 
        !config->uac2 && 
        config->midi && 
        !config->mass_storage && 
        !config->hid &&
        !config->shell
    ) {
        return ZDJ_USB_SUBMODE_GADGET_MIDI;
    } else if( 
        !config->uac2 && 
        config->midi && 
        !config->mass_storage && 
        !config->hid &&
        config->shell
    ) {
        return ZDJ_USB_SUBMODE_GADGET_MIDI_SHELL;
    } else if( 
        !config->uac2 && 
        !config->midi && 
        !config->mass_storage && 
        config->hid &&
        !config->shell
    ) {
        return ZDJ_USB_SUBMODE_GADGET_HID;
    } else if( 
        !config->uac2 && 
        !config->midi && 
        !config->mass_storage && 
        config->hid &&
        config->shell
    ) {
        return ZDJ_USB_SUBMODE_GADGET_HID_SHELL;
    } else if( 
        !config->uac2 && 
        !config->midi && 
        !config->mass_storage && 
        !config->hid &&
        config->shell
    ) {
        return ZDJ_USB_SUBMODE_GADGET_SHELL;
    } else if( 
        !config->uac2 && 
        !config->midi && 
        config->mass_storage && 
        !config->hid &&
        config->shell
    ) {
        return ZDJ_USB_SUBMODE_GADGET_SHELL_DRIVE;
    } else if( 
        !config->uac2 && 
        !config->midi && 
        config->mass_storage && 
        !config->hid &&
        !config->shell
    ) {
        return ZDJ_USB_SUBMODE_GADGET_DRIVE;
    } else {
        return ZDJ_USB_SUBMODE_GADGET_SHELL;
    }
}

zdj_error_type_t zdj_usb_update_gadget_config_from_functionfs( zdj_usb_gadget_config_t * config ) {
    config->uac2 = access( "/sys/kernel/config/usb_gadget/g1/functions/uac2.usb0", F_OK ) == 0;
    config->midi = access( "/sys/kernel/config/usb_gadget/g1/functions/midi.usb0", F_OK ) == 0;
    config->mass_storage = access( "/sys/kernel/config/usb_gadget/g1/functions/mass_storage.0", F_OK ) == 0;
    config->hid = access( "/sys/kernel/config/usb_gadget/g1/functions/hid.usb0", F_OK ) == 0;
    config->shell = access( "/sys/kernel/config/usb_gadget/g1/functions/acm.ttyGS0", F_OK ) == 0;
}
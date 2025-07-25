#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <uuid.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/usb/zdj_usb.h>

typedef struct {
    bool run;
    bool has_update;
    int line_count;
} zdj_sysfs_devices_thread_state_t;
static zdj_sysfs_devices_thread_state_t * _zdj_sysfs_devices_thread_state = NULL;
static pthread_t _zdj_usb_sysfs_devices_thread;
static void * _zdj_sysfs_devices_thread_main( void * arg );

typedef struct {
    bool run;
    bool has_update;
    bool has_partner;
} zdj_port_partner_thread_state_t;
static zdj_port_partner_thread_state_t * _zdj_port_partner_thread_state = NULL;
static pthread_t _zdj_usb_port_partner_thread;
static void * _zdj_port_partner_thread_main( void * arg );

bool zdj_usb_has_port_partner_update( void ) {
    if( _zdj_port_partner_thread_state &&
        _zdj_port_partner_thread_state->has_update
     ) {
        _zdj_port_partner_thread_state->has_update = false;
        return true;
    }
    return false;
}

// Check for type-c port partner indicating a host is physically attached.
bool zdj_usb_has_port_partner( void ) {
    if( _zdj_port_partner_thread_state ) {
        return _zdj_port_partner_thread_state->has_partner;
    }
    return false;
}

zdj_error_type_t zdj_usb_start_port_partner_poll( void ) {
    if ( !_zdj_port_partner_thread_state ) {
        _zdj_port_partner_thread_state = calloc( 1, sizeof( _zdj_port_partner_thread_state ) );
    }

    _zdj_port_partner_thread_state->run = true;
    _zdj_port_partner_thread_state->has_update = false;
    _zdj_port_partner_thread_state->has_partner = false;

    pthread_create( 
        &_zdj_usb_port_partner_thread, 
        NULL, 
        _zdj_port_partner_thread_main, 
        (void*)_zdj_port_partner_thread_state 
    );

    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t zdj_usb_stop_port_partner_poll( void ) {
    if ( _zdj_port_partner_thread_state ) {
        _zdj_port_partner_thread_state->run = false;
    }

    return ZDJ_ERROR_OKAY;
}

static void * _zdj_port_partner_thread_main( void * arg ) {
    zdj_port_partner_thread_state_t * state = (zdj_port_partner_thread_state_t*)arg;

    while( 1 ) {
        if( state->run ) {
            FILE * fp = popen( "ls /sys/class/typec | grep -i partner", "r" );
            if ( fp != NULL ) {
                char line[ 256 ];
                fgets( line, sizeof( line ), fp );
                printf( "/typec:%s\n", line );
                if( strstr( line, "partner" ) ) { 
                    // Partner exists
                    if( state->has_partner == false ) {
                        state->has_update = true;
                    }
                    state->has_partner = true;
                } else {
                    // Partner does not exist
                    if( state->has_partner == true ) {
                        state->has_update = true;
                    }
                    state->has_partner = false;
                }
            }
            pclose( fp );
            sleep( 1 );
        } else {
            return NULL;
        }
    }
}

bool zdj_usb_has_sysfs_devices_update( void ) {
    if ( _zdj_sysfs_devices_thread_state &&
         _zdj_sysfs_devices_thread_state->has_update 
    ) {
        _zdj_sysfs_devices_thread_state->has_update = false;
        return true;
    } else {
        return false;
    }
}

zdj_error_type_t zdj_usb_start_sysfs_devices_poll( void ) {
    if ( !_zdj_sysfs_devices_thread_state ) {
        _zdj_sysfs_devices_thread_state = calloc( 1, sizeof( _zdj_sysfs_devices_thread_state ) );
    }

    _zdj_sysfs_devices_thread_state->run = true;
    _zdj_sysfs_devices_thread_state->has_update = false;
    _zdj_sysfs_devices_thread_state->line_count = 0;

    pthread_create( 
        &_zdj_usb_sysfs_devices_thread, 
        NULL, 
        _zdj_sysfs_devices_thread_main, 
        (void*)_zdj_sysfs_devices_thread_state 
    );

    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t zdj_usb_stop_sysfs_devices_poll( void ) {
    if ( _zdj_sysfs_devices_thread_state ) {
        _zdj_sysfs_devices_thread_state->run = false;
    }

    return ZDJ_ERROR_OKAY;
}

void * _zdj_sysfs_devices_thread_main( void * arg ) {
    zdj_sysfs_devices_thread_state_t * state = (zdj_sysfs_devices_thread_state_t*)arg;

    int line_count;
    while( 1 ) {
        // Check if we should exit.
        if( state->run ) {
            // Count lines in devices output.
            line_count = 0;
            FILE * fp = popen( "cat /sys/kernel/debug/usb/devices", "r" );
            if ( fp == NULL ) {
                printf( "couldn't open usb devices\n" );
            } else {
                char line[ 256 ];
                while( fgets( line, sizeof( line ), fp ) ) {
                    line_count++;
                }
            }
            pclose( fp );

            // If line_count doesn't match last time, flag.
            if( line_count != state->line_count ) {
                sleep( 3 ); // give udev a sec to mount any msd devices
                state->has_update = true;
                state->line_count = line_count;
            }
        } else {
            return NULL;
        }

        sleep( 1 );
    }
}
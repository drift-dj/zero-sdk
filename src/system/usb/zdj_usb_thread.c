#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <uuid.h>
#include <sys/syscall.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/system/usb/zdj_usb.h>

pthread_t _zdj_usb_thread;
static void * _zdj_usb_thread_main( void * arg );

static void _update_port_partner( zdj_usb_state_t * state );
static void _update_msd_gadget_state( zdj_usb_state_t * state );
static void _update_hosted_devices( zdj_usb_state_t * state );
static void _switch_state( zdj_usb_state_t * state );

void zdj_usb_launch_state_thread( void ) {
    zdj_usb_state->run_state_thread = true;
    zdj_usb_state->has_port_partner = false;

    zdj_usb_state->host_state.has_control_update = false;
    zdj_usb_state->host_state.has_file_browser_update = false;
    zdj_usb_state->host_state.has_soundcard_update = false;
    zdj_usb_state->host_state.has_usb_panel_update = false;
    zdj_usb_state->host_state.devices_line_count = 0;

    zdj_usb_state->gadget_state.has_update = false;
    zdj_usb_state->gadget_state.msd_has_been_mounted = false;
    zdj_usb_state->gadget_state.msd_has_been_hot_unplugged = false;
    zdj_usb_state->gadget_state.msd_has_been_unmounted_by_host = false;

    pthread_create( 
        &_zdj_usb_thread, 
        NULL, 
        _zdj_usb_thread_main, 
        (void*)zdj_usb_state 
    );
}

////////////////
// USB Thread //
////////////////
// Periodically check and update status of the entire USB system.
// Note whether there is a port partner attached to the USB system.
// While in HOST mode: Maintain the database/DTOs of attached devices.
// While in GADGET mode: Track the state of connection to host.
// Post updates to the notification widget.
static void * _zdj_usb_thread_main( void * arg ) {
    zdj_usb_state_t * state = (zdj_usb_state_t*)arg;

    // Set core affinity to Core #1;
    cpu_set_t cpuset;
	CPU_ZERO( &cpuset );
	CPU_SET( 0,&cpuset );
	int err = sched_setaffinity( syscall(SYS_gettid), sizeof(cpu_set_t), &cpuset );
    if( err != 0 ) {
        perror( "set affinity failed" );
    }

    while( state->run_state_thread ) {
        ///////////////////////////
        // Mode Switch Requested //
        ///////////////////////////
        if( state->switch_data.switch_req ) {
            state->switch_data.switch_req = false;
            _switch_state( state );
        }


        ///////////////
        // HOST MODE //
        ///////////////
        if( state->mode_state.mode == ZDJ_USB_MODE_HOST ) {
            // Check change to attached devices
            _update_hosted_devices( state );
        




        /////////////////
        // GADGET MODE //
        /////////////////
        } else if( state->mode_state.mode == ZDJ_USB_MODE_GADGET ) {
            _update_port_partner( state );
            _update_msd_gadget_state( state );
        }

        // sleep for a bit between checks
        sleep( 1 );
    }
}


static void _update_port_partner( zdj_usb_state_t * state ) {

    char port_partner[ 128 ];
    char msd_file[ 64 ];
            
    memset( port_partner, 0, 128 );
    zdj_fs_get_popen( 
        "ls /sys/class/typec | grep -i partner", 
        port_partner 
    );
    // printf( "port partner: %s\n", port_partner );

    if( strstr( port_partner, "partner" ) ) { 
        // Partner exists
        if( state->has_port_partner == false ) {
            state->has_port_partner_update = true;
        }
        // printf( "port has partner\n" );
        state->has_port_partner = true;
    } else {
        // Partner does not exist
        if( state->has_port_partner == true ) {
            state->has_port_partner_update = true;
        }
        // printf( "port doesn't have partner\n" );
        state->has_port_partner = false;
    }
}

static void _update_msd_gadget_state( zdj_usb_state_t * state ) {
    char msd_file[ 256 ];
    if( !state->gadget_state.msd_has_been_mounted ) {
        if( access( "/sys/kernel/config/usb_gadget/g1/functions/mass_storage.0/lun.0/file", F_OK ) == 0 ) {
            zdj_fs_get_popen( 
                "cat /sys/kernel/config/usb_gadget/g1/functions/mass_storage.0/lun.0/file", 
                msd_file 
            );
            if( !strncmp( msd_file, "/dev/mmcblk2p4", 14 ) ) {
                printf( "host has mounted\n" );
                state->gadget_state.msd_has_been_mounted = true;
            }
        }

    } else if( state->gadget_state.msd_has_been_mounted ) {
        if( access( "/sys/kernel/config/usb_gadget/g1/functions/mass_storage.0/lun.0/file", F_OK ) == 0 ) {
            zdj_fs_get_popen( 
                "cat /sys/kernel/config/usb_gadget/g1/functions/mass_storage.0/lun.0/file", 
                msd_file 
            );
            if( strncmp( msd_file, "/dev/mmcblk2p4", 14 ) ) {
                // printf( "host has ejected\n" );
                state->gadget_state.msd_has_been_unmounted_by_host = true;
            }
        }
    }
}

static void _update_hosted_devices( zdj_usb_state_t * state ) {
    // Count lines in devices output.
    int line_count = 0;
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

    // If line_count doesn't match last time, refresh the devices and flag.
    if( line_count != state->host_state.devices_line_count ) {
        printf( "found updated line count\n" );
        zdj_usb_update_attached_devices( );
        // Loop thru attached devices and find any matching ALSA cards
        // IMPORTANT - Currently this isn't fully implemented.
        // Only 1 device will be recognized at a time
        if( zdj_usb_state->host_state.attached.count > 0 ) {
            zdj_usb_device_t * device = zdj_usb_state->host_state.attached.devices;
            zdj_usb_update_alsa_profiles( state, device );
        }

        state->host_state.devices_line_count = line_count;

        // Naively set all update flags
        // TODO: only set updates based on new device type
        state->host_state.has_control_update = true;
        state->host_state.has_file_browser_update = true;
        state->host_state.has_browser_panel_update = true;
        state->host_state.has_soundcard_update = true;
        state->host_state.has_usb_panel_update = true;

        printf( "attached devices: %d\n", state->host_state.attached.count );
    }
}




// Perform a blocking switch sequence to the requested state.
// This is intended to be called from the USB state thread.
static void _switch_state( zdj_usb_state_t * state ) {
    zdj_usb_log_begin( );
    sprintf( zdj_usb_state->log_str, "USB Switch State\n" );
    zdj_usb_log( zdj_usb_state->log_str );

    // Make sure we have the latest state
    zdj_usb_update_mode_from_sysfs( zdj_usb_state );
    
    // Read the request in from disk
    zdj_usb_mode_state_t request;
    FILE * fd = fopen( ZDJ_USB_STATUS_PATH, "r" );
    if( fd ) { 
        fread( &request, sizeof( zdj_usb_mode_state_t ), 1, fd );
        fclose( fd );
    } else {
        printf( "failed to read USB switch request from disk\n" );
        return;
    }

    printf( "zdj_usb_switch_state: %d/%d %d\n", request.mode, state->mode_state.mode, request.submode );
    sprintf( zdj_usb_state->log_str, "From:%s To:%s\n", 
        zdj_usb_mode_name[ state->mode_state.mode ], 
        zdj_usb_mode_name[ request.mode ] 
    );
    zdj_usb_log( zdj_usb_state->log_str );

    struct timespec settle_sleep = { 0, 100000000 };

    char cmd[ 256 ];
    strcpy( state->switch_data.switch_str_1, " " );
    strcpy( state->switch_data.switch_str_2, " " );
    zdj_usb_state->switch_data.should_show_lib_rescan = false;

    // Check if we need to teardown any gadgets
    if( zdj_usb_has_active_gadget( zdj_usb_state ) ) {
        // If we're exiting drive mode, make a note for later.
        // We'll pop a dialog asking if user wants to enter import.
        if( zdj_usb_state->mode_state.submode == ZDJ_USB_SUBMODE_GADGET_SHELL_DRIVE ) {
            zdj_usb_state->switch_data.should_show_lib_rescan = true;
        }

        printf( "Tearing down gadget\n" );
        sprintf( zdj_usb_state->log_str, "Tearing down gadget state\n" );
        zdj_usb_log( zdj_usb_state->log_str );
        strcpy( state->switch_data.switch_str_1, "Removing Gadget(s)" );
        state->switch_data.has_update = true;
        strcpy( cmd, "/root/boot/teardown_gadget.sh" );
        system( cmd );
        nanosleep( &settle_sleep, NULL );
        // settle_sleep.tv_sec = 1;
        settle_sleep.tv_nsec = 100000000;
    }


    // Check if we need to teardown the entire USB stack before enabling new mode
    if( request.mode != zdj_usb_state->mode_state.mode &&
        zdj_usb_state->mode_state.mode > ZDJ_USB_MODE_OFFLINE ) {
        printf( "Switching USB to offline\n" );
        sprintf( zdj_usb_state->log_str, "modprobe reset seq:\n" );
        zdj_usb_log( zdj_usb_state->log_str );

        strcpy( state->switch_data.switch_str_1, "Resetting USB Stack" );
        // strcpy( cmd, "/root/boot/switch_to_usb_offline.sh" );
        // system( cmd );

        strcpy( state->switch_data.switch_str_2, "Shell" );
        state->switch_data.has_update = true;
        printf( "removing shell\n" );
        sprintf( zdj_usb_state->log_str, "modprobe -r usb_f_acm\n" );
        zdj_usb_log( zdj_usb_state->log_str );
        strcpy( cmd, "modprobe -r usb_f_acm" );
        system( cmd );
        nanosleep( &settle_sleep, NULL );
        // settle_sleep.tv_sec = 1;
        settle_sleep.tv_nsec = 100000000;

        strcpy( state->switch_data.switch_str_2, "libcomposite" );
        state->switch_data.has_update = true;
        printf( "removing libcomposite\n" );
        sprintf( zdj_usb_state->log_str, "modprobe -r libcomposite\n" );
        zdj_usb_log( zdj_usb_state->log_str );
        strcpy( cmd, "modprobe -r libcomposite" );
        system( cmd );
        nanosleep( &settle_sleep, NULL );
        // settle_sleep.tv_sec = 1;
        settle_sleep.tv_nsec = 100000000;

        strcpy( state->switch_data.switch_str_2, "tcpci" );
        state->switch_data.has_update = true;
        printf( "removing tcpci\n" );
        sprintf( zdj_usb_state->log_str, "modprobe -r tcpci\n" );
        zdj_usb_log( zdj_usb_state->log_str );
        strcpy( cmd, "modprobe -r tcpci" );
        system( cmd );
        nanosleep( &settle_sleep, NULL );
        // settle_sleep.tv_sec = 1;
        settle_sleep.tv_nsec = 100000000;

        strcpy( state->switch_data.switch_str_2, "ci_hdrc_imx" );
        state->switch_data.has_update = true;
        printf( "removing ci_hdrc_imx\n" );
        sprintf( zdj_usb_state->log_str, "modprobe -r ci_hdrc_imx\n" );
        zdj_usb_log( zdj_usb_state->log_str );
        strcpy( cmd, "modprobe -r ci_hdrc_imx" );
        system( cmd );
        printf( "3\n" );
        nanosleep( &settle_sleep, NULL );
        // settle_sleep.tv_sec = 1;
        settle_sleep.tv_nsec = 100000000;

        strcpy( state->switch_data.switch_str_2, "usbmisc_imx" );
        state->switch_data.has_update = true;
        printf( "adding usbmisc_imx\n" );
        sprintf( zdj_usb_state->log_str, "modprobe usbmisc_imx\n" );
        zdj_usb_log( zdj_usb_state->log_str );
        strcpy( cmd, "modprobe usbmisc_imx" );
        system( cmd );
        nanosleep( &settle_sleep, NULL );
        // settle_sleep.tv_sec = 1;
        settle_sleep.tv_nsec = 100000000;
        
    }
    
    printf( "Bringing up USB stack:\n" );
    sprintf( zdj_usb_state->log_str, "Bringing up USB stack script seq:\n" );
    zdj_usb_log( zdj_usb_state->log_str );
    // printf( "req:%p\n", request );
    // printf( "req:%d\n", request.mode );
    // printf( "cur:%d\n",  zdj_usb_state->mode_state.mode );
    // Invoke the mode switch script and wait.
    switch ( request.mode ) {
        case ZDJ_USB_MODE_HOST:
            strcpy( state->switch_data.switch_str_1, "Host Bringup" );
            strcpy( state->switch_data.switch_str_2, " " );
            state->switch_data.has_update = true;
            printf( "Bringing up USB host\n" );
            sprintf( zdj_usb_state->log_str, "switch_to_usb_host.sh\n" );
            zdj_usb_log( zdj_usb_state->log_str );
            strcpy( cmd, "/root/boot/switch_to_usb_host.sh" );
            system( cmd );
            break;
        case ZDJ_USB_MODE_GADGET:
            // Always bring up the shell
            request.gadget_config.shell = true;
            
            if( zdj_usb_state->mode_state.mode != ZDJ_USB_MODE_GADGET ) {
                strcpy( state->switch_data.switch_str_1, "Gadget Bringup" );
                strcpy( state->switch_data.switch_str_2, " " );
                state->switch_data.has_update = true;
                printf( "Switching to USB Gadget\n" );
                sprintf( zdj_usb_state->log_str, "switch_to_usb_gadget.sh\n" );
                zdj_usb_log( zdj_usb_state->log_str );
                strcpy( cmd, "/root/boot/switch_to_usb_gadget.sh" );
                system( cmd );
                nanosleep( &settle_sleep, NULL );
                settle_sleep.tv_nsec = 100000000;
            }
            // If we're switching to gadget, invoke the appropriate gadget bringup script.
            if( request.gadget_config.shell && request.gadget_config.mass_storage ) {
                strcpy( state->switch_data.switch_str_2, "Func:  Shell + Drive" );
                state->switch_data.has_update = true;
                printf( "Bringing up Drive + Shell gadget\n" );
                sprintf( zdj_usb_state->log_str, "bringup_shell_drive_gadget.sh\n" );
                zdj_usb_log( zdj_usb_state->log_str );
                strcpy( cmd, "/root/boot/bringup_shell_drive_gadget.sh" );
                system( cmd );
                nanosleep( &settle_sleep, NULL );
            } else if( request.gadget_config.shell ) {
                strcpy( state->switch_data.switch_str_2, "Func:  Shell" );
                state->switch_data.has_update = true;
                printf( "Bringing up Shell gadget\n" );
                sprintf( zdj_usb_state->log_str, "bringup_shell_gadget.sh\n" );
                zdj_usb_log( zdj_usb_state->log_str );
                strcpy( cmd, "/root/boot/bringup_shell_gadget.sh" );
                system( cmd );
                nanosleep( &settle_sleep, NULL );
            }
            break;
    }

    printf( "USB switch done\n" );
    sprintf( zdj_usb_state->log_str, "USB Switch State Done\n" );
    zdj_usb_log( zdj_usb_state->log_str );
    state->switch_data.state = ZDJ_USB_SUBMODE_SWITCH_SUCCESS;
    state->switch_data.has_update = true;

    zdj_usb_log_end( );
}
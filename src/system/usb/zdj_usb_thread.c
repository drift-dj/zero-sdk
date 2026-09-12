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
#include <zerodj/system/log/zdj_log.h>
#include <zerodj/system/settings/zdj_settings.h>
#include <zerodj/system/usb/zdj_usb.h>

pthread_t _zdj_usb_thread;
static void * _zdj_usb_thread_main( void * arg );

static void _update_gadget_port_partner( zdj_usb_state_t * state );
static void _update_msd_gadget_state( zdj_usb_state_t * state );
static void _update_hosted_devices( zdj_usb_state_t * state );

static void _init_state( zdj_usb_state_t * state );
static void _switch_state( zdj_usb_state_t * state );

void zdj_usb_launch_state_thread( void ) {
    zdj_usb_state->run_state_thread = true;
    
    zdj_usb_state->host_status.has_update = false;
    zdj_usb_state->host_status.has_port_partner = false;
    zdj_usb_state->host_status.has_control_update = false;
    zdj_usb_state->host_status.has_file_browser_update = false;
    zdj_usb_state->host_status.has_soundcard_update = false;
    zdj_usb_state->host_status.has_usb_panel_update = false;
    zdj_usb_state->host_status.devices_line_count = 0;

    zdj_usb_state->gadget_status.has_update = false;
    zdj_usb_state->gadget_status.has_port_partner = false;
    zdj_usb_state->gadget_status.msd_has_been_mounted = false;
    zdj_usb_state->gadget_status.msd_has_been_hot_unplugged = false;
    zdj_usb_state->gadget_status.msd_has_been_unmounted_by_host = false;

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
        // printf( "usb_thread\n" );
        // zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "thread start mode: %s", zdj_usb_mode_name[ state->mode_state.mode ] );

        ///////////////////////////
        // Standup //
        ///////////////////////////
        if( state->mode_state.mode == ZDJ_USB_MODE_INIT ) {
            _init_state( state );
        }
        
        ///////////////////////////
        // Mode Switch Requested //
        ///////////////////////////
        if( state->switch_ctx.has_request ) {
            state->switch_ctx.busy = true;
            state->switch_ctx.has_request = false;
            _switch_state( state );
            state->switch_ctx.busy = false;
            state->switch_ctx.has_update = true;
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
            _update_gadget_port_partner( state );
            _update_msd_gadget_state( state );
        }

        // printf( "usb_thread done\n" );
        // sleep for a bit between checks
        sleep( 1 );
    }

    return NULL;
}


static void _update_gadget_port_partner( zdj_usb_state_t * state ) {
    // printf( "_update_gadget_port_partner\n" );
    char port_partner[ 128 ];
    char msd_file[ 64 ];

    // Error out if usb driver state isn't valid
    if( access( "/sys/class/typec", F_OK ) != 0 ) {
        zdj_usb_put_error_mode( &state->mode_state, ZDJ_USB_MODE_CONFIG_ERROR );
        return;
    }
            
    memset( port_partner, 0, 128 );
    zdj_fs_get_popen( 
        "ls /sys/class/typec | grep -i partner", 
        port_partner 
    );
    // printf( "port partner: %s\n", port_partner );

    if( strstr( port_partner, "partner" ) ) { 
        // Partner exists
        if( state->gadget_status.has_port_partner == false ) {
            zdj_log( ZDJ_LOG_USB, ZDJ_LOG_MSG, "Port Connect" );
            state->gadget_status.has_port_partner_update = true;
        }
        // printf( "port has partner\n" );
        state->gadget_status.has_port_partner = true;
    } else {
        // Partner does not exist
        if( state->gadget_status.has_port_partner == true ) {
            zdj_log( ZDJ_LOG_USB, ZDJ_LOG_MSG, "Port Disconnect" );
            state->gadget_status.has_port_partner_update = true;
        }
        // printf( "port doesn't have partner\n" );
        state->gadget_status.has_port_partner = false;
    }
    // printf( "_update_gadget_port_partner done\n" );
}

static void _update_msd_gadget_state( zdj_usb_state_t * state ) {
    // printf( "_update_msd_gadget_state\n" );
    char msd_file[ 256 ];
    if( !state->gadget_status.msd_has_been_mounted ) {
        if( access( "/sys/kernel/config/usb_gadget/g1/functions/mass_storage.0/lun.0/file", F_OK ) == 0 ) {
            zdj_fs_get_popen( 
                "cat /sys/kernel/config/usb_gadget/g1/functions/mass_storage.0/lun.0/file", 
                msd_file 
            );
            if( !strncmp( msd_file, "/dev/mmcblk2p4", 14 ) ) {
                // printf( "host has mounted\n" );
                zdj_log( ZDJ_LOG_USB, ZDJ_LOG_MSG, "Host mounted MSD" );
                state->gadget_status.msd_has_been_mounted = true;
            }
        }

    } else if( state->gadget_status.msd_has_been_mounted ) {
        if( access( "/sys/kernel/config/usb_gadget/g1/functions/mass_storage.0/lun.0/file", F_OK ) == 0 ) {
            zdj_fs_get_popen( 
                "cat /sys/kernel/config/usb_gadget/g1/functions/mass_storage.0/lun.0/file", 
                msd_file 
            );
            if( strncmp( msd_file, "/dev/mmcblk2p4", 14 ) ) {
                // printf( "host has ejected: [%s]\n", msd_file );
                zdj_log( ZDJ_LOG_USB, ZDJ_LOG_MSG, "Host ejected MSD" );
                state->gadget_status.msd_has_been_unmounted_by_host = true;
            }
        }
    }
    // printf( "_update_msd_gadget_state done\n" );
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
    if( line_count != state->host_status.devices_line_count ) {
        zdj_log( ZDJ_LOG_USB, ZDJ_LOG_MSG, "Attached device update" );
        // printf( "found updated line count\n" );
        zdj_usb_update_attached_devices( );
        // Loop thru attached devices and find any matching ALSA cards
        // IMPORTANT - Currently this isn't fully implemented.
        // Only 1 device will be recognized at a time
        if( zdj_usb_state->host_status.attached.count > 0 ) {
            zdj_usb_device_t * device = zdj_usb_state->host_status.attached.devices;
            zdj_usb_update_alsa_profiles( state, device );
        }

        state->host_status.devices_line_count = line_count;

        // Naively set all update flags
        // TODO: only set updates based on new device type
        state->host_status.has_control_update = true;
        state->host_status.has_file_browser_update = true;
        state->host_status.has_browser_panel_update = true;
        state->host_status.has_soundcard_update = true;
        state->host_status.has_usb_panel_update = true;

        // printf( "attached devices: %d\n", state->host_status.attached.count );
    }
}


// Bring up the USB system.  Depending on Settings, we can keep any valid
// state we find or force the system into a pre-selected state.
static void _init_state( zdj_usb_state_t * state ) {
    // Bring up the user-specified USB mode by reading settings and submitting requests.
    // This may happen at device boot, or after a crash/relaunch, or after an app transition
    zdj_usb_setting_init_option_t init_option = zdj_setting_get( ZDJ_SETTING_USB_INIT_OPTION )->i_val;
    zdj_usb_mode_t prev_mode;
    switch( init_option ) {
        case ZDJ_SETTING_USB_INIT_OFFLINE:
            zdj_log( ZDJ_LOG_USB, ZDJ_LOG_MSG, "Init prefs: [Offline]" );
            zdj_usb_put_offline_mode( &state->switch_ctx.request );
            state->switch_ctx.has_request = true;
            return;
        case ZDJ_SETTING_USB_INIT_HOST:
            zdj_log( ZDJ_LOG_USB, ZDJ_LOG_MSG, "Init prefs: [Host]" );
            zdj_usb_put_host_mode( &state->switch_ctx.request );
            state->switch_ctx.has_request = true;
            return;
        case ZDJ_SETTING_USB_INIT_GADGET:
            zdj_log( ZDJ_LOG_USB, ZDJ_LOG_MSG, "Init prefs: [Gadget]" );
            zdj_usb_put_empty_gadget_mode( &state->switch_ctx.request );
            state->switch_ctx.request.gadget_config.shell = true;
            state->switch_ctx.has_request = true;
            return;
        case ZDJ_SETTING_USB_INIT_PREVIOUS:
            // Case 1: We are relaunching after a crash or app transition.
            
            // Check if we are in Gadget state at init.
            zdj_usb_update_gadget_driver_health( state );
            if( state->gadget_status._flags == 0x0 ) { 
                zdj_usb_put_current_gadget_mode( &state->mode_state );
                zdj_log( ZDJ_LOG_USB, ZDJ_LOG_MSG, "Init prefs: Previous - Relaunch > [Gadget]" );
                return;
            }
            // Check if we are in Host state at init.
            zdj_usb_update_host_driver_health( state );
            if( state->host_status._flags == 0x0 ) { 
                zdj_usb_put_host_mode( &state->mode_state );
                zdj_log( ZDJ_LOG_USB, ZDJ_LOG_MSG, "Init prefs: Previous - Relaunch > [Host]" );
                return; 
            }

            // Case 2: We are launching after reboot
            // For now, reboot only boots into prev mode if it was Gadget
            prev_mode = zdj_setting_get( ZDJ_SETTING_USB_PREV_MODE )->i_val;
            if( prev_mode == ZDJ_USB_MODE_GADGET ) {
                zdj_log( ZDJ_LOG_USB, ZDJ_LOG_MSG, "Init prefs: Previous - Reboot > [Gadget]" );
                zdj_usb_put_empty_gadget_mode( &state->switch_ctx.request );
                state->switch_ctx.request.gadget_config.shell = true;
                state->switch_ctx.has_request = true;
            } else {
                zdj_log( ZDJ_LOG_USB, ZDJ_LOG_MSG, "Init prefs: Previous - Reboot > [Offline]" );
                zdj_usb_put_offline_mode( &state->switch_ctx.request );
                state->switch_ctx.has_request = true;
            }
            return;
    }

    // If we get here, something went wrong. Go into error mode
    zdj_log( ZDJ_LOG_USB, ZDJ_LOG_ERROR, "Init State Error" );
    zdj_usb_put_error_mode( &state->mode_state, ZDJ_USB_MODE_INIT_ERROR );
}


// Perform a blocking switch sequence to the requested state.
// This is intended to be called from the USB state thread.
static void _switch_state( zdj_usb_state_t * state ) {
    zdj_log( ZDJ_LOG_USB, ZDJ_LOG_MSG, "Switch: [%s] -> [%s]", zdj_usb_mode_name[ state->mode_state.mode ], zdj_usb_mode_name[ state->switch_ctx.request.mode ] );

    // Make sure we have the latest state
    zdj_usb_update_mode_from_sysfs( &state->mode_state );
    
    // System must be healthy to begin a mode switch to Gadget/Host.
    // Allow an unhealthy system if we're switching to Offline
    // If there's a mode error, bug out and let the user
    // attempt to resolve it via the USB debug UI.
    if( state->switch_ctx.request.mode != ZDJ_USB_MODE_OFFLINE ) {
        if( state->mode_state.mode == ZDJ_USB_MODE_INIT_ERROR ||
            state->mode_state.mode == ZDJ_USB_MODE_CONFIG_ERROR || 
            state->mode_state.mode == ZDJ_USB_MODE_SWITCH_ERROR
        ) {
            zdj_log( ZDJ_LOG_USB, ZDJ_LOG_ERROR, "[%s] has error. Can't switch to [%s]", zdj_usb_mode_name[ state->mode_state.mode ], zdj_usb_mode_name[ state->switch_ctx.request.mode ] );
            state->switch_ctx.busy = false;
            state->switch_ctx.has_update = true;
            return;
        }
    }

    // Begin processing the switch request
    zdj_usb_mode_state_t * request = &state->switch_ctx.request;

    

    struct timespec settle_sleep = { 0, 100000000 };

    char cmd[ 1024 ];

    // Check if we need to teardown any gadgets.
    // This is needs to work in the case of a partially stood-up driver stack
    // so it needs to be more complex than just checking for current valid state.
    if( zdj_usb_detect_gadgetfs( state ) ) {
        zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Teardown Gadget" );

        // If we're exiting drive mode, make a note for later.
        // We'll pop a dialog asking if user wants to enter import.
        if( zdj_usb_state->mode_state.gadget_config.mass_storage ) {
            // zdj_usb_state->gadget_status.should_show_lib_rescan = true;
        }

        if( !zdj_usb_teardown_gadget( state ) ) {
            zdj_usb_put_error_mode( &state->mode_state, ZDJ_USB_MODE_SWITCH_ERROR );
            zdj_log( ZDJ_LOG_USB, ZDJ_LOG_ERROR, "USB Error at: [%s]", zdj_usb_mode_switch_err_name[ state->switch_ctx.error ] );
            state->switch_ctx.busy = false;
            state->switch_ctx.has_update = true;
            return;
        }

        nanosleep( &settle_sleep, NULL );
        settle_sleep.tv_nsec = 100000000;
    }


    // Check if we need to teardown the entire USB stack before enabling new mode
    if( request->mode != state->mode_state.mode &&
        state->mode_state.mode > ZDJ_USB_MODE_OFFLINE 
    ) {
        // printf( "Switching USB to offline\n" );
        zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Teardown Drivers" );
        if( !zdj_usb_teardown_driver_mods( state ) ) {
            zdj_usb_put_error_mode( &state->mode_state, ZDJ_USB_MODE_SWITCH_ERROR );
            zdj_log( ZDJ_LOG_USB, ZDJ_LOG_ERROR, "USB Error at: [%s]", zdj_usb_mode_switch_err_name[ state->switch_ctx.error ] );
            state->switch_ctx.busy = false;
            state->switch_ctx.has_update = true;
            return;
        }
    }

    
    switch ( request->mode ) {
        case ZDJ_USB_MODE_OFFLINE:
            zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Offline Mode" );
            zdj_usb_put_offline_mode( &state->mode_state );
            break;
        case ZDJ_USB_MODE_HOST:
            zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Standup Host" );
            if( !zdj_usb_standup_host_mode( state ) ) {
                zdj_usb_put_error_mode( &state->mode_state, ZDJ_USB_MODE_SWITCH_ERROR );
                zdj_log( ZDJ_LOG_USB, ZDJ_LOG_ERROR, "USB Error at: [%s]", zdj_usb_mode_switch_err_name[ state->switch_ctx.error ] );
                state->switch_ctx.busy = false;
                state->switch_ctx.has_update = true;
                return;
            }
            break;
        case ZDJ_USB_MODE_GADGET:
            zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Standup Gadget" );
            // Always bring up the shell
            request->gadget_config.shell = true;
            // If we aren't currently in gadget mode, we need to switch the USB driver's role
            if( state->mode_state.mode != ZDJ_USB_MODE_GADGET ) {
                if( !zdj_usb_standup_gadget_mode( state ) ) {
                    zdj_usb_put_error_mode( &state->mode_state, ZDJ_USB_MODE_SWITCH_ERROR );
                    zdj_log( ZDJ_LOG_USB, ZDJ_LOG_ERROR, "USB Error at: [%s]", zdj_usb_mode_switch_err_name[ state->switch_ctx.error ] );
                    state->switch_ctx.busy = false;
                    state->switch_ctx.has_update = true;
                    return;
                }
            }

            // Bring up the selected set of USB Gadget functions
            zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Standup Gadget Functions" );
            if( !zdj_usb_standup_gadget_functions( state ) ) {
                zdj_usb_put_error_mode( &state->mode_state, ZDJ_USB_MODE_SWITCH_ERROR );
                zdj_log( ZDJ_LOG_USB, ZDJ_LOG_ERROR, "USB Error at: [%s]", zdj_usb_mode_switch_err_name[ state->switch_ctx.error ] );
                state->switch_ctx.busy = false;
                state->switch_ctx.has_update = true;
                return;
            }
            
            break;
    }

    // Update from latest state
    zdj_usb_update_mode_from_sysfs( &state->mode_state );
    zdj_log( ZDJ_LOG_USB, ZDJ_LOG_MSG, "Mode: [%s]", zdj_usb_mode_name[ state->mode_state.mode ] );

    // Store previous USB mode in prefs
    zdj_setting_set_int( ZDJ_SETTING_USB_PREV_MODE, state->mode_state.mode );
}
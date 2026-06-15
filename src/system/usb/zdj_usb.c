#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <sys/syscall.h>

#include <libfdt/libfdt.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/system/usb/zdj_usb.h>

#define NAME_MAX 255

zdj_usb_state_t * zdj_usb_state;

zdj_error_type_t zdj_usb_init( void ) {
    // printf( "zdj_usb_init\n" );
    // Create DB if it doesn't exits
    if( access( ZDJ_USB_DEVICE_DB_PATH, F_OK) != 0 || zdj_usb_devices_db_needs_init( ) ) {
        zdj_usb_reset_devices_db( );
    }

    // Bringup inital status
    zdj_usb_state = calloc( 1, sizeof( zdj_usb_state_t ) );

    // Get current state
    zdj_usb_update_mode_from_sysfs( zdj_usb_state );

    // Get the background USB status thread running.
    zdj_usb_launch_state_thread( );

    // printf( "zdj_usb_init done\n" );

    return ZDJ_ERROR_OKAY;
}

void zdj_usb_bringup_last_requested_state( void ) {
    // Attempt to switch into persisted mode.
    // Note that the USB stack may already be in this mode from 
    // a previous usb_init( ) call.

    zdj_usb_log_begin( );
    sprintf( zdj_usb_state->log_str, "## zdj_usb_bringup_last_requested_state ##\n" );
    zdj_usb_log( zdj_usb_state->log_str );

    zdj_usb_mode_state_t * previous_mode = calloc( 1, sizeof( zdj_usb_mode_state_t ) );
    FILE * fd = fopen( ZDJ_USB_STATUS_PATH, "r" );
    if( fd ) { 
        fseek( fd, 0, SEEK_SET );
        fread( previous_mode, sizeof( zdj_usb_mode_state_t ), 1, fd );
        fclose( fd );
        printf( "FOUND USB MODE AT BRINGUP: %s\n", zdj_usb_mode_name[ previous_mode->mode ] );
        sprintf( zdj_usb_state->log_str, "Found %s mode at bringup\n", zdj_usb_mode_name[ previous_mode->mode ] );
        zdj_usb_log( zdj_usb_state->log_str );
        
        // Never boot with drive mode on
        previous_mode->gadget_config.mass_storage = false;
        
        // If we're not currently in the last requested state, bring it up.
        // This step is important for launching zero apps over the USB shell.
        // Resetting the USB stack will disconnect the shell and freeze the device.
        // TODO: is there a way to fix this?
        zdj_usb_update_mode_from_sysfs( zdj_usb_state );
        
        if( zdj_usb_state->mode_state.mode == previous_mode->mode &&
            zdj_usb_state->mode_state.gadget_config.hid == previous_mode->gadget_config.hid &&
            zdj_usb_state->mode_state.gadget_config.midi == previous_mode->gadget_config.midi && 
            zdj_usb_state->mode_state.gadget_config.uac2 == previous_mode->gadget_config.uac2
        ) {
            // ignore request for current USB state
            printf( "USB ignoring request for current state\n" );
            sprintf( zdj_usb_state->log_str, "Ignoring request for current state\n" );
            zdj_usb_log( zdj_usb_state->log_str );
            return;
        } else {
            zdj_usb_enable_mode( previous_mode );
        }
    } else {
        sprintf( zdj_usb_state->log_str, "Missing USB status file\n" );
        zdj_usb_log( zdj_usb_state->log_str );
    }

    // fclose( zdj_usb_state->log_fp );
    // zdj_usb_state->log_fp = NULL;
    zdj_usb_log_end( );
}


zdj_error_type_t zdj_usb_disable( ) {

}

zdj_error_type_t zdj_usb_enable_mode( zdj_usb_mode_state_t * request ) {
    // Write request to disk -- we'll read this from the switch thread AND to bring up
    // the USB state at next boot.

    sprintf( zdj_usb_state->log_str, "## zdj_usb_enable_mode ##\n" );
    zdj_usb_log( zdj_usb_state->log_str );

    FILE * fd = fopen( ZDJ_USB_STATUS_PATH, "w" );
    if( fd ) { 
        printf( "PERSISTING USB MODE: %s\n", zdj_usb_mode_name[ request->mode ] );
        sprintf( zdj_usb_state->log_str, "Persisting requested USB mode: %s\n", zdj_usb_mode_name[ request->mode ] );
        zdj_usb_log( zdj_usb_state->log_str );

        fseek( fd, 0, SEEK_SET );
        fwrite( request, sizeof( zdj_usb_mode_state_t ), 1, fd );
        fclose( fd );
    }

    if( !zdj_usb_state->switch_data.switch_req ) {    
        printf( "enabling mode: %s\n", zdj_usb_mode_name[ request->mode ] );
        sprintf( zdj_usb_state->log_str, "Enabling USB mode: %s\n", zdj_usb_mode_name[ request->mode ] );
        zdj_usb_log( zdj_usb_state->log_str );

        // Command the USB state thread to begin switching.
        zdj_usb_state->switch_data.state = ZDJ_USB_SUBMODE_SWITCH_RUNNING;
        zdj_usb_state->switch_data.switch_req = true;
    }
    
    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t zdj_usb_update_mode_from_sysfs( zdj_usb_state_t * state ) {
    sprintf( zdj_usb_state->log_str, "## zdj_usb_update_mode_from_sysfs ##\n" );
    zdj_usb_log( zdj_usb_state->log_str );
    // If USB has not been initialized, go with offline
    struct stat s;
    int err = stat( "/sys/kernel/debug/usb/", &s );
    if( err == -1 ) {
        // printf( "usb offline\n" );
        state->mode_state.mode = ZDJ_USB_MODE_OFFLINE;
        sprintf( zdj_usb_state->log_str, "USB Offline Mode\n" );
        zdj_usb_log( zdj_usb_state->log_str );
        return ZDJ_ERROR_OKAY;
    }

    // If USB has been initialized, check the role value
    char role[ 256 ];
    char cmd[ 256 ];
    strcpy( cmd, "cat /sys/kernel/debug/usb/ci_hdrc.0/role" );
    // Open a pipe to execute the command
    zdj_fs_get_popen( cmd, role );

    if( !strncmp( role, "host", 4 ) ) {
        // printf( "found host\n" );
        sprintf( zdj_usb_state->log_str, "USB Host Mode\n" );
        zdj_usb_log( zdj_usb_state->log_str );
        state->mode_state.mode = ZDJ_USB_MODE_HOST;
    } else if( !strncmp( role, "gadget", 6) ) {
        // printf( "found gadget\n" );
        sprintf( zdj_usb_state->log_str, "USB Gadget Mode\n" );
        zdj_usb_log( zdj_usb_state->log_str );
        state->mode_state.mode = ZDJ_USB_MODE_GADGET;
        zdj_usb_update_gadget_config_from_functionfs( &state->mode_state.gadget_config );

    } else {
        // printf( "found none\n" );
        state->mode_state.mode = ZDJ_USB_MODE_ERROR;
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

    sprintf( zdj_usb_state->log_str, "USB Gadget Submode:\n UAC2:%d MIDI:%d MSD:%d HID:%d Shell:%d\n",
          config->uac2, config->midi, config->mass_storage, config->hid, config->shell
    );
    zdj_usb_log( zdj_usb_state->log_str );

    return ZDJ_ERROR_OKAY;
}

bool zdj_usb_has_active_gadget( zdj_usb_state_t * state ) {
    bool res = false;
    if( state->mode_state.mode != ZDJ_USB_MODE_GADGET ) { return false; }
    if( state->mode_state.gadget_config.hid ||
        state->mode_state.gadget_config.mass_storage ||
        state->mode_state.gadget_config.midi ||
        state->mode_state.gadget_config.shell ||
        state->mode_state.gadget_config.uac2 
    ) {
        return true;
    } else {
        return false;
    }
}

void zdj_usb_reset_status( void ) {
    remove( ZDJ_USB_STATUS_PATH );
}

void zdj_usb_log_begin( void ) {
     // If enabled, bringup log
    char log_path[ 256 ];
    sprintf( log_path, "%s/usb_log_%03d.txt", ZDJ_LOG_DIR, zdj_new_error_log_num( ) );
    zdj_usb_state->log_fp = fopen( log_path, "w" );
    if( !zdj_usb_state->log_fp ) { printf( "USB Log failed to open\n" ); }
}
void zdj_usb_log( char * str ) { 
    if( zdj_usb_state->log_fp ) { 
        printf( "USB LOG: %s", str );
        fprintf( zdj_usb_state->log_fp, "%s", str ); 
    } 
}
void zdj_usb_log_end( void ) { 
    fclose( zdj_usb_state->log_fp );
    zdj_usb_state->log_fp = NULL; 
    sync( );
}
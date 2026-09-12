#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/inotify.h>
#include <sys/syscall.h>

#include <libfdt/libfdt.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/system/log/zdj_log.h>
#include <zerodj/system/settings/zdj_settings.h>
#include <zerodj/system/usb/zdj_usb.h>

#define NAME_MAX 255

zdj_usb_state_t * zdj_usb_state;

static bool _zdj_usb_stored_state_is_error( void );
static void _zdj_usb_clear_stored_state( void );

zdj_error_type_t zdj_usb_init( void ) {
    // printf( "zdj_usb_init\n" );
    zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "zdj_usb_init" );
    // Clear previous USB state if it was an error
    if( access( ZDJ_USB_STATUS_PATH, F_OK) != 0 || _zdj_usb_stored_state_is_error( ) ) {
        _zdj_usb_clear_stored_state( );
    }

    // Create DB if it doesn't exist
    if( access( ZDJ_USB_DEVICE_DB_PATH, F_OK) != 0 || zdj_usb_devices_db_needs_init( ) ) {
        zdj_usb_reset_devices_db( );
    }

    // Bringup inital status
    zdj_usb_state = calloc( 1, sizeof( zdj_usb_state_t ) );
    zdj_usb_state->mode_state.mode = ZDJ_USB_MODE_INIT;

    // Get the background USB status thread running.
    zdj_usb_launch_state_thread( );

    // printf( "zdj_usb_init done\n" );

    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t zdj_usb_disable( ) {

}

zdj_error_type_t zdj_usb_update_mode_from_sysfs( zdj_usb_mode_state_t * state ) {    
    if( access( "/sys/kernel/debug/usb/ci_hdrc.0", F_OK ) != 0 ) {
        // printf( "no ci_hdrc found - putting offline\n" );
        zdj_usb_put_offline_mode( state );
        return ZDJ_ERROR_OKAY;
    }

    // If USB has been initialized, check the role value
    char role[ 256 ];
    char cmd[ 256 ];
    strcpy( cmd, "cat /sys/kernel/debug/usb/ci_hdrc.0/role" );
    // Open a pipe to execute the command
    zdj_fs_get_popen( cmd, role );
    
    if( !strncmp( role, "host", 4 ) ) {
        zdj_usb_put_host_mode( state );
    
    } else if( !strncmp( role, "gadget", 6) ) {
        zdj_usb_put_current_gadget_mode( state );

    } else {
        zdj_usb_put_error_mode( state, ZDJ_USB_MODE_CONFIG_ERROR );
    }

    zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Get mode from sysFS: [%s]", zdj_usb_mode_name[ state->mode ] );
    // printf( "zdj_usb_update_mode_from_sysfs done\n" );
    return ZDJ_ERROR_OKAY;
}

// Expects gadgetfs to be in valid state, clobbers state to error if not.
zdj_error_type_t zdj_usb_update_gadget_config_from_functionfs( zdj_usb_mode_state_t * mode ) {
    if( access( "/sys/kernel/config/usb_gadget/g1/functions", F_OK ) != 0 ) {
        zdj_usb_put_error_mode( mode, ZDJ_USB_MODE_CONFIG_ERROR );
        return ZDJ_ERROR_OKAY;
    }

    mode->gadget_config.uac2 = access( "/sys/kernel/config/usb_gadget/g1/functions/uac2.usb0", F_OK ) == 0;
    mode->gadget_config.midi = access( "/sys/kernel/config/usb_gadget/g1/functions/midi.usb0", F_OK ) == 0;
    mode->gadget_config.mass_storage = access( "/sys/kernel/config/usb_gadget/g1/functions/mass_storage.0", F_OK ) == 0;
    mode->gadget_config.hid = access( "/sys/kernel/config/usb_gadget/g1/functions/hid.usb0", F_OK ) == 0;
    mode->gadget_config.shell = access( "/sys/kernel/config/usb_gadget/g1/functions/acm.ttyGS0", F_OK ) == 0;

    return ZDJ_ERROR_OKAY;
}

bool zdj_usb_detect_gadgetfs( zdj_usb_state_t * state ) {
    if( access( "/sys/kernel/config/usb_gadget/g1", F_OK ) == 0 ) {
        return true;
    } else {
        return false;
    }
}

void zdj_usb_reset_status( void ) {
    _zdj_usb_clear_stored_state( );
}

static bool _zdj_usb_stored_state_is_error( void ) {
    zdj_usb_mode_state_t state;
    FILE * fd = fopen( ZDJ_USB_STATUS_PATH, "r" );
    if( fd ) { 
        fread( &state, sizeof( zdj_usb_mode_state_t ), 1, fd );
        fclose( fd );
        if( state.valid == ZDJ_USB_VALID_FLAG ) { return false; }
    }
    return true;
}

static void _zdj_usb_clear_stored_state( void ) {
    zdj_log( ZDJ_LOG_USB, ZDJ_LOG_MSG, "Clearing Stored State" );
    zdj_setting_set_int( ZDJ_SETTING_USB_PREV_MODE, ZDJ_USB_MODE_OFFLINE );
}

void zdj_usb_put_offline_mode( zdj_usb_mode_state_t * state ) {
    state->valid = 0;
    state->mode = ZDJ_USB_MODE_OFFLINE;
    state->gadget_config.uac2 = false;
    state->gadget_config.midi = false;
    state->gadget_config.mass_storage = false;
    state->gadget_config.hid = false;
    state->gadget_config.shell = false;
}

void zdj_usb_put_host_mode( zdj_usb_mode_state_t * state ) {
    state->valid = 0;
    state->mode = ZDJ_USB_MODE_HOST;
    state->gadget_config.uac2 = false;
    state->gadget_config.midi = false;
    state->gadget_config.mass_storage = false;
    state->gadget_config.hid = false;
    state->gadget_config.shell = false;
}

void zdj_usb_put_empty_gadget_mode( zdj_usb_mode_state_t * state ) {
    state->valid = 0;
    state->mode = ZDJ_USB_MODE_GADGET;
    state->gadget_config.uac2 = false;
    state->gadget_config.midi = false;
    state->gadget_config.mass_storage = false;
    state->gadget_config.hid = false;
    state->gadget_config.shell = false;
}

void zdj_usb_put_current_gadget_mode( zdj_usb_mode_state_t * state ) {
    state->valid = 0;
    state->mode = ZDJ_USB_MODE_GADGET;
    zdj_usb_update_gadget_config_from_functionfs( state );
}

void zdj_usb_put_error_mode( zdj_usb_mode_state_t * state, zdj_usb_mode_t error_mode ) {
    state->valid = 0;
    state->mode = error_mode;
    state->gadget_config.uac2 = false;
    state->gadget_config.midi = false;
    state->gadget_config.mass_storage = false;
    state->gadget_config.hid = false;
    state->gadget_config.shell = false;
}
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/stat.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/system/log/zdj_log.h>
#include <zerodj/system/usb/zdj_usb.h>

// Assumes the state has been set - not suitable during init
bool zdj_usb_gather_driver_health( zdj_usb_state_t * state ) {
    // reject check if USB is currently switching.
    if( state->mode_state.mode == ZDJ_USB_MODE_SWITCH ||
        state->mode_state.mode == ZDJ_USB_MODE_CHECK 
    ) { return false; }
    switch ( state->mode_state.mode ) {
        case ZDJ_USB_MODE_OFFLINE:
            zdj_usb_update_offline_driver_health( state );
            break;
        case ZDJ_USB_MODE_GADGET:
            zdj_usb_update_gadget_driver_health( state );
            break;
        case ZDJ_USB_MODE_HOST:
            zdj_usb_update_host_driver_health( state );
            break;
        default: return false;
    }

    return true;
}


bool zdj_usb_update_offline_driver_health( zdj_usb_state_t * state ) {
    char str[ 512 ];
    state->offline_flags = 0x0;

    // Ensure USB debugFS isn't running 
    if( access( "/sys/kernel/debug/usb", F_OK ) == 0 ) {
        state->offline_flags |= ZDJ_USB_DRIVER_FLAG_OFFLINE_HAS_DEBUGFS;
        return false;
    }

    // Ensure USB configFS isn't running 
    if( access( "/sys/kernel/config/usb_gadget", F_OK ) == 0 ) {
        state->offline_flags |= ZDJ_USB_DRIVER_FLAG_OFFLINE_HAS_CONFIGFS;
        return false;
    }

    return true;
}

// Check for indications of a functioning USB Gadget driver stack
bool zdj_usb_update_gadget_driver_health( zdj_usb_state_t * state ) {
    char str[ 512 ];
    state->gadget_status._flags = 0x0;
    
    zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Gadget Driver Health Check" );
    // Insure overlay dir exists
    if( access( "/sys/kernel/config/device-tree/overlays/usb", F_OK ) != 0 ) {
        zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Bad Overlays Dir" );
        state->gadget_status._flags |= ZDJ_USB_DRIVER_FLAG_GADGET_NO_OVERLAY_DIR;
        return false;
    }

    // Check for USB role
    zdj_fs_get_popen( "cat /sys/kernel/debug/usb/ci_hdrc.0/role", str );
    zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Gadget check found role: %s", str );
    if( strncmp( str, "gadget", 6 ) != 0 ) {
        zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Didn't Find Gadget Role" );
        state->gadget_status._flags |= ZDJ_USB_DRIVER_FLAG_BAD_GADGET_ROLE;
        return false;
    } else {
        zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Found Role: %s", str );
    }

    // Check for a good gadgetfs
    if( access( "/sys/kernel/config/usb_gadget/g1", F_OK ) != 0 ) {
        zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "G1 Dir Missing" );
        state->gadget_status._flags |= ZDJ_USB_DRIVER_FLAG_GADGETFS_MISSING;
        return false;
    }

    // Check for functionfs
    if( access( "/sys/kernel/config/usb_gadget/g1/functions", F_OK ) != 0 ) {
        zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "FuncFS Missing" );
        state->gadget_status._flags |= ZDJ_USB_DRIVER_FLAG_FUNCTIONFS_MISSING;
        return false;
    }

    // Confirm at least one function is running
    bool found_gadget = false;
    if( access( "/sys/kernel/config/usb_gadget/g1/functions/uac2.usb0", F_OK ) == 0 ) { found_gadget = true; }
    if( access( "/sys/kernel/config/usb_gadget/g1/functions/midi.usb0", F_OK ) == 0 ) { found_gadget = true; }
    if( access( "/sys/kernel/config/usb_gadget/g1/functions/mass_storage.0", F_OK ) == 0 ) { found_gadget = true; }
    if( access( "/sys/kernel/config/usb_gadget/g1/functions/hid.usb0", F_OK ) == 0 ) { found_gadget = true; }
    if( access( "/sys/kernel/config/usb_gadget/g1/functions/acm.ttyGS0", F_OK ) == 0 ) { found_gadget = true; }
    if( !found_gadget ) {
        zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "No Active Functions" );
        state->gadget_status._flags |= ZDJ_USB_DRIVER_FLAG_NO_GADGET_FUNCTION;
        return false;
    }

    return true;
}

// Check for indications of a functioning USB Host driver stack
bool zdj_usb_update_host_driver_health( zdj_usb_state_t * state ) {
    char str[ 512 ];
    state->host_status._flags = 0x0;
    
    zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Host Driver Health Check" );
    // Insure overlay dir exists
    if( access( "/sys/kernel/config/device-tree/overlays/usb", F_OK ) != 0 ) {
        zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Bad Overlays Dir" );
        state->host_status._flags |= ZDJ_USB_DRIVER_FLAG_HOST_NO_OVERLAY_DIR;
        return false;
    }

    // Check for USB role
    zdj_fs_get_popen( "cat /sys/kernel/debug/usb/ci_hdrc.0/role", str );
    zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Host check found role: %s", str );
    if( strncmp( str, "host", 4 ) != 0 ) {
        zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Didn't Find Host Role" );
        state->host_status._flags |= ZDJ_USB_DRIVER_FLAG_BAD_HOST_ROLE;
        return false;
    } else {
        zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Found Role: %s", str );
    }

    return true;
}


bool zdj_usb_teardown_driver_mods( zdj_usb_state_t * state ) {
    // printf( "removing shell\n" );
    struct timespec settle_sleep = { 0, 500000000 };

    if( !zdj_usb_modprobe_remove( "usb_f_acm" ) ) {
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_REMOVE_ACM_FAILED;
        return false;
    }
    nanosleep( &settle_sleep, NULL );
    settle_sleep.tv_nsec = 100000000;

    if( !zdj_usb_modprobe_remove( "libcomposite" ) ) {
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_REMOVE_LIBCOMPOSITE_FAILED;
        return false;
    }
    nanosleep( &settle_sleep, NULL );
    settle_sleep.tv_nsec = 100000000;

    if( !zdj_usb_modprobe_remove( "tcpci" ) ) {
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_REMOVE_TCPCI_FAILED;
        return false;
    }
    nanosleep( &settle_sleep, NULL );
    settle_sleep.tv_nsec = 100000000;

    if( !zdj_usb_modprobe_remove( "ci_hdrc_imx" ) ) {
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_REMOVE_CI_HDRC_IMX_FAILED;
        return false;
    }
    nanosleep( &settle_sleep, NULL );
    settle_sleep.tv_nsec = 100000000;

    if( !zdj_usb_modprobe_remove( "usbmisc_imx" ) ) {
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_REMOVE_USBMISC_IMX_FAILED;
        return false;
    }
    nanosleep( &settle_sleep, NULL );
    settle_sleep.tv_nsec = 100000000;

    return true;
}

bool zdj_usb_teardown_gadget( zdj_usb_state_t * state ) {
    char str[ 512 ];
    char res[ 512 ];
    
    // If getty is active, kill it
    strcpy( str, "pidof -s getty" );
    zdj_fs_get_popen( str, res );
    if( strlen( res ) ) {
        zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Killing geTTY" );
        zdj_fs_get_popen( "killall getty 2>&1", res );
        // Error mode: if we know getty is active, and killall fails 
        if( strlen( res ) > 0 ) { 
            zdj_log( ZDJ_LOG_USB, ZDJ_LOG_ERROR, "Failed to kill geTTY", res );
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_KILL_GETTY_FAILED;
            return false;
        }
    } else {
        zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "geTTY not running", res );
    }

    // Unbind and confirm UDC
    if( access( "/sys/kernel/config/usb_gadget/g1/UDC", F_OK ) == 0 ) {
        strcpy( res, "" );
        zdj_fs_get_popen( "cat /sys/kernel/config/usb_gadget/g1/UDC", res );
        if( strlen( res ) > 0 ) {
            zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Unbinding UDC", res );
            zdj_fs_write_buffer( "/sys/kernel/config/usb_gadget/g1/UDC", "\n" );
            
            // Settle for a moment to allow driver to unbind
            struct timespec settle_sleep = { 0, 500000000 };
            nanosleep( &settle_sleep, NULL );

            // Check to see if it worked
            strcpy( res, "" );
            zdj_fs_get_popen( "cat /sys/kernel/config/usb_gadget/g1/UDC", res );
            if( strlen( res ) > 1 ) { 
                // UDC is still bound - error out
                zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "UDC failed to unbind" );
                state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_UDC_UNBIND_FAILED;
                return false;
            }
        }
    }

    // Remove any configured symlinks
    if( state->mode_state.gadget_config.shell ) { 
        if ( unlink( "/sys/kernel/config/usb_gadget/g1/configs/c.1/acm.ttyGS0" ) < 0 ) {
            zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Unlink ttyGS0 failed", res );
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_UNLINK_TTYGS0_FAILED;
            return false;
        }
        if ( unlink( "/sys/kernel/config/usb_gadget/g1/configs/c.1/acm.ttyGS1" ) < 0 ) {
            zdj_log( ZDJ_LOG_USB, ZDJ_LOG_ERROR, "Unlink ttyGS1 failed", res );
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_UNLINK_TTYGS1_FAILED;
            return false;
        }
    }
    if( state->mode_state.gadget_config.uac2 ) { 
        if ( unlink( "/sys/kernel/config/usb_gadget/g1/configs/c.1/uac2.usb0" ) < 0 ) {
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_UNLINK_UAC2_FAILED;
            return false;
        }
    }
    if( state->mode_state.gadget_config.mass_storage ) { 
        if ( unlink( "/sys/kernel/config/usb_gadget/g1/configs/c.1/mass_storage.0" ) < 0 ) {
            zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Unlink msd failed", res );
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_UNLINK_MSD_FAILED;
            return false;
        }
    }
    if( state->mode_state.gadget_config.hid ) { 
        if ( unlink( "/sys/kernel/config/usb_gadget/g1/configs/c.1/hid.usb0" ) < 0 ) {
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_UNLINK_HID_FAILED;
            return false;
        }
    }
    if( state->mode_state.gadget_config.midi ) { 
        if ( unlink( "/sys/kernel/config/usb_gadget/g1/configs/c.1/midi.usb0" ) < 0 ) {
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_UNLINK_MIDI_FAILED;
            return false;
        }
    }

    // Remove configuration strings directory
    if ( ( access( "/sys/kernel/config/usb_gadget/g1/configs/c.1/strings/0x409", F_OK ) == 0 ) &&
         ( rmdir( "/sys/kernel/config/usb_gadget/g1/configs/c.1/strings/0x409" ) < 0 ) 
    ) {
        zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Failed to remove: /sys/kernel/config/usb_gadget/g1/configs/c.1/strings/0x409: %s", strerror( errno ) );
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_RMDIR_CONFIGS_FAILED;
        return false;
    }

    // Remove configuration directory
    if ( ( access( "/sys/kernel/config/usb_gadget/g1/configs/c.1", F_OK ) == 0 ) &&
         ( rmdir( "/sys/kernel/config/usb_gadget/g1/configs/c.1" ) < 0 ) 
    ) {
        zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Failed to remove: /sys/kernel/config/usb_gadget/g1/configs/c.1: %s", strerror( errno ) );
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_RMDIR_CONFIGS_FAILED;
        return false;
    }

    // Remove function directories
    if( state->mode_state.gadget_config.shell ) { 
        if ( rmdir( "/sys/kernel/config/usb_gadget/g1/functions/acm.ttyGS0" ) < 0 ) {
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_RMDIR_TTYGS0_FAILED;
            return false;
        }
        if ( rmdir( "/sys/kernel/config/usb_gadget/g1/functions/acm.ttyGS1" ) < 0 ) {
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_RMDIR_TTYGS1_FAILED;
            return false;
        }
    }
    if( state->mode_state.gadget_config.uac2 ) { 
        if ( rmdir( "/sys/kernel/config/usb_gadget/g1/functions/uac2.usb0" ) < 0 ) {
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_RMDIR_UAC2_FAILED;
            return false;
        }
    }
    if( state->mode_state.gadget_config.mass_storage ) { 
        if ( rmdir( "/sys/kernel/config/usb_gadget/g1/functions/mass_storage.0" ) < 0 ) {
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_RMDIR_MSD_FAILED;
            return false;
        }
    }
    if( state->mode_state.gadget_config.hid ) { 
        if ( rmdir( "/sys/kernel/config/usb_gadget/g1/functions/hid.usb0" ) < 0 ) {
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_RMDIR_HID_FAILED;
            return false;
        }
    }
    if( state->mode_state.gadget_config.midi ) { 
        if ( rmdir( "/sys/kernel/config/usb_gadget/g1/functions/midi.usb0" ) < 0 ) {
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_RMDIR_MIDI_FAILED;
            return false;
        }
    }

    // Remove gadget strings directory
    if ( ( access( "/sys/kernel/config/usb_gadget/g1/strings/0x409", F_OK ) == 0 ) &&
         ( rmdir( "/sys/kernel/config/usb_gadget/g1/strings/0x409" ) < 0 ) 
    ) {
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_RMDIR_GADGET_STRINGS_FAILED;
        return false;
    }

    // Remove main gadget root directory
    if ( ( access( "/sys/kernel/config/usb_gadget/g1", F_OK ) == 0 ) &&
         ( rmdir( "/sys/kernel/config/usb_gadget/g1" ) < 0 ) 
    ) {
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_RMDIR_G1_FAILED;
        return false;
    }

    return true;
}

bool zdj_usb_standup_gadget_mode( zdj_usb_state_t * state ) {
    if( access( "/sys/kernel/config/device-tree/overlays/usb", F_OK ) == 0 ) {
        if ( rmdir( "/sys/kernel/config/device-tree/overlays/usb" ) < 0 ) {
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_GADGET_OVERLAY_DIR_FAILED;
            return false;
        }
    }
    
    if ( mkdir( "/sys/kernel/config/device-tree/overlays/usb", 0744 ) < 0 ) {
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_GADGET_OVERLAY_DIR_FAILED;
        return false;
    }
    
    FILE * src_fd = fopen( "/root/dt/zero-usb-gadget-overlay.dtbo", "r" );
    if( !src_fd ) { 
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_GADGET_WRITE_OVERLAY_FAILED;
        return false;
    }
    FILE * dst_fd = fopen( "/sys/kernel/config/device-tree/overlays/usb/dtbo", "w" );
    if( !dst_fd ) { 
        if( src_fd ) { fclose( src_fd ); }
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_GADGET_WRITE_OVERLAY_FAILED;
        return false;
    }
    int a;
    while ( ( a = fgetc( src_fd ) ) != EOF ) { 
        fputc( a, dst_fd ); 
    }
    if( src_fd ) { fclose( src_fd ); }
    if( dst_fd ) { fclose( dst_fd ); }

    struct timespec settle_sleep = { 0, 500000000 };
    nanosleep( &settle_sleep, NULL );

    // Confirm the ci_hdrc.0 controller is available:
    char udc[ 128 ];
    zdj_fs_get_popen( "ls /sys/class/udc | grep ci_hdrc.0", udc );
    if( strncmp( udc, "ci_hdrc.0", 9 ) != 0 ) {
        zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "UDC not avail" );
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_GADGET_WRITE_OVERLAY_FAILED;
        return false;
    }

    zdj_fs_write_buffer( "/sys/kernel/debug/usb/ci_hdrc.0/role", "gadget" );

    settle_sleep.tv_nsec = 100000000;
    nanosleep( &settle_sleep, NULL );

    zdj_usb_modprobe_install( "libcomposite" );
    settle_sleep.tv_nsec = 300000000;
    nanosleep( &settle_sleep, NULL );

    if( access( "/sys/kernel/config/usb_gadget", F_OK ) != 0 ) {
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_SET_GADGET_ROLE_FAILED;
        return false;
    }

    return true;
}

bool zdj_usb_standup_gadget_functions( zdj_usb_state_t * state ) {

    if( access( "/sys/kernel/config/usb_gadget", F_OK ) != 0 ) {
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_CONFIGFS_MISSING;
        return false;
    }

    if( zdj_fs_mkdir_p( "/sys/kernel/config/usb_gadget/g1" ) < 0 ) {
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_MKDIR_G1_FAILED;
        return false;
    }
    
    bool configfs_err = false;
    if ( zdj_fs_write_buffer( "/sys/kernel/config/usb_gadget/g1/idVendor", "0x0787" ) < 1 ) {
        configfs_err = true;
    }
    
    if ( zdj_fs_write_buffer( "/sys/kernel/config/usb_gadget/g1/idProduct", "0x0009" ) < 1 ) {
        configfs_err = true;
    }
    if ( zdj_fs_write_buffer( "/sys/kernel/config/usb_gadget/g1/bcdDevice", "0x0600" ) < 1 ) {
        configfs_err = true;
    }
    if ( zdj_fs_write_buffer( "/sys/kernel/config/usb_gadget/g1/bcdUSB", "0x0200" ) < 1 ) {
        configfs_err = true;
    }
    if( zdj_fs_mkdir_p( "/sys/kernel/config/usb_gadget/g1/strings/0x409" ) < 0 ) {

        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_STRINGS_FAILED;
        return false;
    }
    if ( zdj_fs_write_buffer( 
        "/sys/kernel/config/usb_gadget/g1/strings/0x409/serialnumber", 
        "d005" 
    ) < 1 ) {
        configfs_err = true;
    }
    if ( zdj_fs_write_buffer( 
        "/sys/kernel/config/usb_gadget/g1/strings/0x409/manufacturer", 
        "Drift DJ Industries, Inc." 
    ) < 1 ) {
        configfs_err = true;
    }
    if ( zdj_fs_write_buffer( 
        "/sys/kernel/config/usb_gadget/g1/strings/0x409/product", 
        "Drift Zero Console" 
    ) < 1 ) {
        configfs_err = true;
    }

    if( configfs_err ) {  
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_STRINGS_FAILED;
        return false;
    }

    if( zdj_fs_mkdir_p( "/sys/kernel/config/usb_gadget/g1/functions" ) < 0 ) {
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_MKDIR_FUNCTIONS_FAILED;
        return false;
    }
    if( state->switch_ctx.request.gadget_config.shell ) { 
        if( zdj_fs_mkdir_p( "/sys/kernel/config/usb_gadget/g1/functions/acm.ttyGS0" ) < 0 ) {
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_MKDIR_TTYGS0_FAILED;
            return false;
        }
        if( zdj_fs_mkdir_p( "/sys/kernel/config/usb_gadget/g1/functions/acm.ttyGS1" ) < 0 ) {
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_MKDIR_TTYGS1_FAILED;
            return false;
        }
    }
    if( state->switch_ctx.request.gadget_config.uac2 ) { 
        if( zdj_fs_mkdir_p( "/sys/kernel/config/usb_gadget/g1/functions/uac2.usb0" ) < 0 ) {
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_MKDIR_UAC2_FAILED;
            return false;
        }
    }
    if( state->switch_ctx.request.gadget_config.mass_storage ) { 
        // Reset mount state for front-end
        state->gadget_status.msd_has_been_mounted = false;
        state->gadget_status.msd_has_been_unmounted_by_host = false;
        
        if( zdj_fs_mkdir_p( "/sys/kernel/config/usb_gadget/g1/functions/mass_storage.0" ) < 0 ) {
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_MKDIR_MSD_FAILED;
            return false;
        }
        // Wait for driver to populate mass_storage dir
        struct timespec settle_sleep = { 0, 100000000 };
        nanosleep( &settle_sleep, NULL );

        if ( zdj_fs_write_buffer( 
            "/sys/kernel/config/usb_gadget/g1/functions/mass_storage.0/lun.0/file", 
            "/dev/mmcblk2p4" 
        ) < 1 ) {
            zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Failed to set MSD volume: %s", strerror( errno ) );
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_MOUNT_MSD_FAILED;
            return false;
        }
    }
    if( state->switch_ctx.request.gadget_config.hid ) { 
        if( zdj_fs_mkdir_p( "/sys/kernel/config/usb_gadget/g1/functions/hid.usb0" ) < 0 ) {
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_MKDIR_HID_FAILED;
            return false;
        }
    }
    if( state->switch_ctx.request.gadget_config.midi ) { 
        if( zdj_fs_mkdir_p( "/sys/kernel/config/usb_gadget/g1/functions/midi.usb0" ) < 0 ) {
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_MKDIR_MIDI_FAILED;
            return false;
        }
    }
    if( zdj_fs_mkdir_p( "/sys/kernel/config/usb_gadget/g1/configs/c.1/strings/0x409" ) < 0 ) {
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_CONFIGFS_FAILED;
        return false;
    }
    if( zdj_fs_write_buffer( 
        "/sys/kernel/config/usb_gadget/g1/configs/c.1/strings/0x409/configuration", 
        "Config 1: Debug Mode" 
    ) < 1 ) {
        configfs_err = true;
    }
    
    if( zdj_fs_write_buffer( 
        "/sys/kernel/config/usb_gadget/g1/configs/c.1/MaxPower", 
        "500" 
    ) < 1 ) {
        configfs_err = true;
    }

    if( state->switch_ctx.request.gadget_config.shell ) { 
        if ( symlink( "/sys/kernel/config/usb_gadget/g1/functions/acm.ttyGS0", "/sys/kernel/config/usb_gadget/g1/configs/c.1/acm.ttyGS0" ) != 0 ) {
            zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "Failed to link ttyGS0: %s", strerror( errno ) );
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_LINK_TTYGS0_FAILED;
            return false;
        }
        if ( symlink( "/sys/kernel/config/usb_gadget/g1/functions/acm.ttyGS1", "/sys/kernel/config/usb_gadget/g1/configs/c.1/acm.ttyGS1" ) != 0 ) {
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_LINK_TTYGS1_FAILED;
            return false;
        }
    }
    if( state->switch_ctx.request.gadget_config.uac2 ) { 
        if ( symlink( "/sys/kernel/config/usb_gadget/g1/functions/uac2.usb0", "/sys/kernel/config/usb_gadget/g1/configs/c.1/uac2.usb0" ) != 0 ) {
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_LINK_UAC2_FAILED;
            return false;
        }
    }
    if( state->switch_ctx.request.gadget_config.mass_storage ) { 
        if ( symlink( "/sys/kernel/config/usb_gadget/g1/functions/mass_storage.0", "/sys/kernel/config/usb_gadget/g1/configs/c.1/mass_storage.0" ) != 0 ) {
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_LINK_MSD_FAILED;
            return false;
        }
    }
    if( state->switch_ctx.request.gadget_config.hid ) { 
        if ( symlink( "/sys/kernel/config/usb_gadget/g1/functions/hid.usb0", "/sys/kernel/config/usb_gadget/g1/configs/c.1/hid.usb0" ) != 0 ) {
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_LINK_HID_FAILED;
            return false;
        }
    }
    if( state->switch_ctx.request.gadget_config.midi ) { 
        if ( symlink( "/sys/kernel/config/usb_gadget/g1/functions/midi.usb0", "/sys/kernel/config/usb_gadget/g1/configs/c.1/midi.usb0" ) != 0 ) {
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_LINK_MIDI_FAILED;
            return false;
        }
    }

    if( access( "/sys/kernel/config/usb_gadget/g1/UDC", F_OK ) == 0 ) {
        char udc[ 128 ];
        zdj_fs_get_popen( "cat /sys/kernel/config/usb_gadget/g1/UDC", udc );
    } else {
        zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "can't access UDC" );
    }

    int bw = zdj_fs_write_buffer( 
        "/sys/kernel/config/usb_gadget/g1/UDC", 
        "ci_hdrc.0" 
    );
    
    // Wait for UDC bind to finish bringing up the Drivers
    struct timespec settle_sleep = { 0, 500000000 };
    nanosleep( &settle_sleep, NULL );

    if( access( "/sys/kernel/config/usb_gadget/g1/UDC", F_OK ) == 0 ) {
        char udc[ 128 ];
        zdj_fs_get_popen( "cat /sys/kernel/config/usb_gadget/g1/UDC", udc );
    } else {
        zdj_log( ZDJ_LOG_USB, ZDJ_LOG_DEBUG, "can't access UDC"  );
    }

    // Check for debugfs dir to confirm driver standup
    if( access( "/sys/kernel/debug/usb", F_OK ) != 0 ) {
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_BIND_UDC_FAILED;
        return false;
    }

    // Fork geTTY and wait for it to start running
    zdj_usb_shell_launch( );

    settle_sleep.tv_nsec = 500000000;
    nanosleep( &settle_sleep, NULL );

    // Check for getty running
    // If getty is not active, fail the switch
    if( !zdj_usb_shell_is_running( ) ) {
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_GETTY_FAILED;
        return false;
    }

    // Everything worked, let it cook...
    return true;
}

bool zdj_usb_standup_host_mode( zdj_usb_state_t * state ) {
    if( access( "/sys/kernel/config/device-tree/overlays/usb", F_OK ) == 0 ) {
        if ( rmdir( "/sys/kernel/config/device-tree/overlays/usb" ) < 0 ) {
            state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_HOST_OVERLAY_DIR_FAILED;
            return false;
        }
    }
    
    if ( mkdir( "/sys/kernel/config/device-tree/overlays/usb", 0744 ) < 0 ) {
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_HOST_OVERLAY_DIR_FAILED;
        return false;
    }
    
    FILE * src_fd = fopen( "/root/dt/zero-usb-host-overlay.dtbo", "r" );
    if( !src_fd ) { 
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_HOST_WRITE_OVERLAY_FAILED;
        return false;
    }
    FILE * dst_fd = fopen( "/sys/kernel/config/device-tree/overlays/usb/dtbo", "w" );
    if( !dst_fd ) { 
        state->switch_ctx.error = ZDJ_USB_SWITCH_ERR_HOST_WRITE_OVERLAY_FAILED;
        return false;
    }
    int a;
    while ( ( a = fgetc( src_fd ) ) != EOF ) { fputc( a, dst_fd ); }
    if( src_fd ) { fclose( src_fd ); }
    if( dst_fd ) { fclose( dst_fd ); }

    struct timespec settle_sleep = { 0, 500000000 };
    nanosleep( &settle_sleep, NULL );

    zdj_fs_write_buffer( "/sys/kernel/debug/usb/ci_hdrc.0/role", "host" );

    settle_sleep.tv_nsec = 500000000;
    nanosleep( &settle_sleep, NULL );

    return true;
}


bool zdj_usb_modprobe_install( char * module_name ) {
    char cmd[ 256 ];
    char res[ 256 ];
    snprintf( cmd, sizeof(cmd), "modprobe %s", module_name );
    zdj_fs_get_popen( cmd, res );
    if( strlen( res ) ) { 
        return false;
    } else {
        return true;
    }
}

bool zdj_usb_modprobe_remove( char * module_name ) {
    char cmd[ 256 ];
    char res[ 256 ];
    snprintf( cmd, sizeof(cmd), "modprobe -r %s", module_name );
    zdj_fs_get_popen( cmd, res );
    if( strlen( res ) ) { 
        return false;
    } else {
        return true;
    }
}

bool zdj_usb_modprobe_check( char * module_name ) {

}
// Copyright (c) 2025 Drift DJ Industries

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ZDJ_USB_H
#define ZDJ_USB_H
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <sqlite3.h>

#include <zerodj/system/error/zdj_error.h>

#define ZDJ_USB_STATUS_PATH "/media/internal/.system/usb_status"
#define ZDJ_USB_DEVICE_DB_PATH "/media/internal/.system/device.db"

#define ZDJ_USB_VALID_FLAG 0xAA

typedef enum {
    ZDJ_USB_MODE_INIT,
    ZDJ_USB_MODE_SWITCH,
    ZDJ_USB_MODE_CHECK,
    ZDJ_USB_MODE_INIT_ERROR,
    ZDJ_USB_MODE_CONFIG_ERROR,
    ZDJ_USB_MODE_SWITCH_ERROR,
    ZDJ_USB_MODE_OFFLINE,
    ZDJ_USB_MODE_HOST,
    ZDJ_USB_MODE_GADGET,
    ZDJ_USB_MODE_COUNT
} zdj_usb_mode_t;

static char * zdj_usb_mode_name[ ZDJ_USB_MODE_COUNT ] = {
    "Initializing",// ZDJ_USB_MODE_INIT,
    "Switching",// ZDJ_USB_MODE_SWITCH,
    "System Check",// ZDJ_USB_MODE_CHECK,
    "Init Error",// ZDJ_USB_MODE_INIT_ERROR,
    "Config Error",// ZDJ_USB_MODE_CONFIG_ERROR,
    "Swtich Error",// ZDJ_USB_MODE_SWITCH_ERROR,
    "Offline",// ZDJ_USB_MODE_OFFLINE,
    "Host Mode",// ZDJ_USB_MODE_HOST,
    "Gadget Mode"// ZDJ_USB_MODE_GADGET,
};

typedef struct {
    bool uac2;
    bool midi;
    bool mass_storage;
    bool hid;
    bool shell;
} zdj_usb_gadget_config_t;

typedef struct {
    char valid;
    zdj_usb_mode_t mode;
    zdj_usb_gadget_config_t gadget_config;
} zdj_usb_mode_state_t;

typedef enum {
    ZDJ_USB_SWITCH_ERR_NONE,

    //////////////////////////
    // Teardown Gadget Mode //
    //////////////////////////
    ZDJ_USB_SWITCH_ERR_SYSFS_FAILED,
    ZDJ_USB_SWITCH_ERR_KILL_GETTY_FAILED,
    ZDJ_USB_SWITCH_ERR_UDC_UNBIND_FAILED,
    ZDJ_USB_SWITCH_ERR_UNLINK_TTYGS0_FAILED,
    ZDJ_USB_SWITCH_ERR_UNLINK_TTYGS1_FAILED,
    ZDJ_USB_SWITCH_ERR_UNLINK_UAC2_FAILED,
    ZDJ_USB_SWITCH_ERR_UNLINK_MSD_FAILED,
    ZDJ_USB_SWITCH_ERR_UNLINK_HID_FAILED,
    ZDJ_USB_SWITCH_ERR_UNLINK_MIDI_FAILED,
    ZDJ_USB_SWITCH_ERR_RMDIR_CONFIGS_FAILED, // removing the strings/etc. from config
    ZDJ_USB_SWITCH_ERR_RMDIR_TTYGS0_FAILED,
    ZDJ_USB_SWITCH_ERR_RMDIR_TTYGS1_FAILED,
    ZDJ_USB_SWITCH_ERR_RMDIR_UAC2_FAILED,
    ZDJ_USB_SWITCH_ERR_RMDIR_MSD_FAILED,
    ZDJ_USB_SWITCH_ERR_RMDIR_HID_FAILED,
    ZDJ_USB_SWITCH_ERR_RMDIR_MIDI_FAILED,
    ZDJ_USB_SWITCH_ERR_RMDIR_GADGET_STRINGS_FAILED,
    ZDJ_USB_SWITCH_ERR_RMDIR_G1_FAILED,

    /////////////////////////
    // Standup Gadget Mode //
    /////////////////////////
    ZDJ_USB_SWITCH_ERR_GADGET_OVERLAY_DIR_FAILED,
    ZDJ_USB_SWITCH_ERR_GADGET_WRITE_OVERLAY_FAILED,
    ZDJ_USB_SWITCH_ERR_SET_GADGET_ROLE_FAILED,

    //////////////////////////////
    // Standup Gadget Functions //
    //////////////////////////////
    ZDJ_USB_SWITCH_ERR_CONFIGFS_MISSING,
    ZDJ_USB_SWITCH_ERR_MKDIR_G1_FAILED,
    ZDJ_USB_SWITCH_ERR_STRINGS_FAILED,
    ZDJ_USB_SWITCH_ERR_MKDIR_FUNCTIONS_FAILED,
    ZDJ_USB_SWITCH_ERR_MKDIR_TTYGS0_FAILED,
    ZDJ_USB_SWITCH_ERR_MKDIR_TTYGS1_FAILED,
    ZDJ_USB_SWITCH_ERR_MKDIR_UAC2_FAILED,
    ZDJ_USB_SWITCH_ERR_MKDIR_MSD_FAILED,
    ZDJ_USB_SWITCH_ERR_MOUNT_MSD_FAILED,
    ZDJ_USB_SWITCH_ERR_MKDIR_HID_FAILED,
    ZDJ_USB_SWITCH_ERR_MKDIR_MIDI_FAILED,
    ZDJ_USB_SWITCH_ERR_CONFIGFS_FAILED,
    ZDJ_USB_SWITCH_ERR_LINK_TTYGS0_FAILED,
    ZDJ_USB_SWITCH_ERR_LINK_TTYGS1_FAILED,
    ZDJ_USB_SWITCH_ERR_LINK_UAC2_FAILED,
    ZDJ_USB_SWITCH_ERR_LINK_MSD_FAILED,
    ZDJ_USB_SWITCH_ERR_LINK_HID_FAILED,
    ZDJ_USB_SWITCH_ERR_LINK_MIDI_FAILED,

    ZDJ_USB_SWITCH_ERR_BIND_UDC_FAILED,
    ZDJ_USB_SWITCH_ERR_GETTY_FAILED,

    ///////////////////////
    // Standup Host Mode //
    ///////////////////////
    ZDJ_USB_SWITCH_ERR_HOST_OVERLAY_DIR_FAILED,
    ZDJ_USB_SWITCH_ERR_HOST_WRITE_OVERLAY_FAILED,
    ZDJ_USB_SWITCH_ERR_SET_HOST_ROLE_FAILED,

    /////////////////////////////
    // Teardown Kernel Modules //
    /////////////////////////////
    ZDJ_USB_SWITCH_ERR_REMOVE_ACM_FAILED,
    ZDJ_USB_SWITCH_ERR_REMOVE_LIBCOMPOSITE_FAILED,
    ZDJ_USB_SWITCH_ERR_REMOVE_TCPCI_FAILED,
    ZDJ_USB_SWITCH_ERR_REMOVE_CI_HDRC_IMX_FAILED,
    ZDJ_USB_SWITCH_ERR_REMOVE_USBMISC_IMX_FAILED,

    ZDJ_USB_SWITCH_ERR_COUNT
} zdj_usb_mode_switch_error_t;


static char * zdj_usb_mode_switch_err_name[ ZDJ_USB_SWITCH_ERR_COUNT ] = {
    "Unknown", // ZDJ_USB_SWITCH_ERR_NONE,
    // //////////////////////////
    // // Teardown Gadget Mode //
    // //////////////////////////
    "SysFS Check", // ZDJ_USB_SWITCH_ERR_SYSFS_FAILED,
    "geTTY Kill", // ZDJ_USB_SWITCH_ERR_KILL_GETTY_FAILED,
    "UDC Unbind", // ZDJ_USB_SWITCH_ERR_UDC_UNBIND_FAILED,
    "Unlink TTYGS0", // ZDJ_USB_SWITCH_ERR_UNLINK_TTYGS0_FAILED,
    "Unlink TTYGS1", // ZDJ_USB_SWITCH_ERR_UNLINK_TTYGS1_FAILED,
    "Unlink UAC2", // ZDJ_USB_SWITCH_ERR_UNLINK_UAC2_FAILED,
    "Unlink MSD", // ZDJ_USB_SWITCH_ERR_UNLINK_MSD_FAILED,
    "Unlink HID", // ZDJ_USB_SWITCH_ERR_UNLINK_HID_FAILED,
    "Unlink MIDI", // ZDJ_USB_SWITCH_ERR_UNLINK_MIDI_FAILED,
    "Remove ConfigFS", // ZDJ_USB_SWITCH_ERR_RMDIR_CONFIGS_FAILED, // removing the strings/etc. from config
    "Remove TTYGS0", // ZDJ_USB_SWITCH_ERR_RMDIR_TTYGS0_FAILED,
    "Remove TTYGS1", // ZDJ_USB_SWITCH_ERR_RMDIR_TTYGS1_FAILED,
    "Remove UAC2", // ZDJ_USB_SWITCH_ERR_RMDIR_UAC2_FAILED,
    "Remove MSD", // ZDJ_USB_SWITCH_ERR_RMDIR_MSD_FAILED,
    "Remove HID", // ZDJ_USB_SWITCH_ERR_RMDIR_HID_FAILED,
    "Remove MIDI", // ZDJ_USB_SWITCH_ERR_RMDIR_MIDI_FAILED,
    "Remove Gadget Strings", // ZDJ_USB_SWITCH_ERR_RMDIR_GADGET_STRINGS_FAILED,
    "Remove G1 Gadget", // ZDJ_USB_SWITCH_ERR_RMDIR_G1_FAILED,

    // /////////////////////////
    // // Standup Gadget Mode //
    // /////////////////////////
    "Check Overlay", // ZDJ_USB_SWITCH_ERR_GADGET_OVERLAY_DIR_FAILED,
    "Replace Overlay", // ZDJ_USB_SWITCH_ERR_GADGET_WRITE_OVERLAY_FAILED,
    "Set Gadget Role", // ZDJ_USB_SWITCH_ERR_SET_GADGET_ROLE_FAILED,

    // //////////////////////////////
    // // Standup Gadget Functions //
    // //////////////////////////////
    "ConfigFS Check", // ZDJ_USB_SWITCH_ERR_CONFIGFS_MISSING,
    "Add G1 Gadget", // ZDJ_USB_SWITCH_ERR_MKDIR_G1_FAILED,
    "Add Gadget Strings", // ZDJ_USB_SWITCH_ERR_STRINGS_FAILED,
    "Add Function Dir", // ZDJ_USB_SWITCH_ERR_MKDIR_FUNCTIONS_FAILED,
    "Add TTYGS0", // ZDJ_USB_SWITCH_ERR_MKDIR_TTYGS0_FAILED,
    "Add TTYGS1", // ZDJ_USB_SWITCH_ERR_MKDIR_TTYGS1_FAILED,
    "Add UAC2", // ZDJ_USB_SWITCH_ERR_MKDIR_UAC2_FAILED,
    "Add MSD", // ZDJ_USB_SWITCH_ERR_MKDIR_MSD_FAILED,
    "Mount MSD Vol", // ZDJ_USB_SWITCH_ERR_MOUNT_MSD_FAILED,
    "Add HID", // ZDJ_USB_SWITCH_ERR_MKDIR_HID_FAILED,
    "Add MIDI", // ZDJ_USB_SWITCH_ERR_MKDIR_MIDI_FAILED,
    "Add ConfigFS", // ZDJ_USB_SWITCH_ERR_CONFIGFS_FAILED,
    "Link TTYGS0", // ZDJ_USB_SWITCH_ERR_LINK_TTYGS0_FAILED,
    "Link TTYGS1", // ZDJ_USB_SWITCH_ERR_LINK_TTYGS1_FAILED,
    "Link UAC2", // ZDJ_USB_SWITCH_ERR_LINK_UAC2_FAILED,
    "Link MSD", // ZDJ_USB_SWITCH_ERR_LINK_MSD_FAILED,
    "Link HID", // ZDJ_USB_SWITCH_ERR_LINK_HID_FAILED,
    "Link MIDI", // ZDJ_USB_SWITCH_ERR_LINK_MIDI_FAILED,

    "Bind UDC", // ZDJ_USB_SWITCH_ERR_BIND_UDC_FAILED,
    "geTTY Check", // ZDJ_USB_SWITCH_ERR_GETTY_FAILED,

    // ///////////////////////
    // // Standup Host Mode //
    // ///////////////////////
    "Check Overlay", // ZDJ_USB_SWITCH_ERR_HOST_OVERLAY_DIR_FAILED,
    "Replace Overlay", // ZDJ_USB_SWITCH_ERR_HOST_WRITE_OVERLAY_FAILED,
    "Set Host Role", // ZDJ_USB_SWITCH_ERR_SET_HOST_ROLE_FAILED,

    // /////////////////////////////
    // // Teardown Kernel Modules //
    // /////////////////////////////
    "Remove ACM Module", // ZDJ_USB_SWITCH_ERR_REMOVE_ACM_FAILED,
    "Remove Libcomposite Module", // ZDJ_USB_SWITCH_ERR_REMOVE_LIBCOMPOSITE_FAILED,
    "Remove TCPCI Module", // ZDJ_USB_SWITCH_ERR_REMOVE_TCPCI_FAILED,
    "Remove CI_HDRC Module", // ZDJ_USB_SWITCH_ERR_REMOVE_CI_HDRC_IMX_FAILED,
    "Remove USBMISC Module" // ZDJ_USB_SWITCH_ERR_REMOVE_USBMISC_IMX_FAILED
};


typedef struct {
    zdj_usb_mode_state_t request;
    bool has_request;
    bool busy;
    bool has_update;
    zdj_usb_mode_switch_error_t error;
    char err_desc[ 256 ];
    void (*success_cb) ( void* );
    void (*error_cb) ( void* );
    void (*update_cb) ( void* );
} zdj_usb_mode_switch_context_t;

typedef enum {
    ZDJ_USB_DRIVER_FLAG_GADGET_NO_OVERLAY_DIR = 0x1,
    ZDJ_USB_DRIVER_FLAG_BAD_GADGET_ROLE = 0x2,
    ZDJ_USB_DRIVER_FLAG_GADGETFS_MISSING = 0x4,
    ZDJ_USB_DRIVER_FLAG_FUNCTIONFS_MISSING = 0x8,
    ZDJ_USB_DRIVER_FLAG_NO_GADGET_FUNCTION = 0x16,
    ZDJ_USB_DRIVER_FLAG_GADGET_DEBUGFS_MISSING = 0x32,
    ZDJ_USB_DRIVER_FLAG_GADGET_CONFIGFS_MISSING = 0x64
} zdj_usb_gadget_driver_flag_t;

typedef struct {
    bool has_update;
    bool has_port_partner;
    bool has_port_partner_update;
    bool msd_has_been_mounted;
    bool msd_has_been_hot_unplugged;
    bool msd_has_been_unmounted_by_host;
    bool should_show_lib_rescan;
    zdj_usb_gadget_driver_flag_t _flags;
} zdj_usb_gadget_status_t;

typedef struct zdj_usb_device_t {
    char entity_id[ 64 ];
    char hash[ 64 ];
    char usb_vendor_id[ 64 ];
    char usb_product_id[ 64 ];
    char manufacturer[ 64 ];
    char product[ 64 ];
    char serial[ 64 ];
    char name_user[ 128 ];
    char mount_path[ 256 ];
    bool mount_path_valid;
    bool attached;
    bool has_audio;
    bool has_audio_in;
    char snd_card_capture_name[ 32 ];
    bool has_audio_out;
    char snd_card_playback_name[ 32 ];
    bool has_msd;
    bool has_hid;
    bool has_midi;
    bool has_midi_in;
    char snd_card_rawmidi_in_name[ 32 ];
    bool has_midi_out;
    char snd_card_rawmidi_out_name[ 32 ];
    struct zdj_usb_device_t * next;
} zdj_usb_device_t;

typedef enum {
    ZDJ_USB_TYPE_ANY,
    ZDJ_USB_TYPE_AUDIO,
    ZDJ_USB_TYPE_HID,
    ZDJ_USB_TYPE_MIDI,
    ZDJ_USB_TYPE_MSD
} zdj_usb_device_filter_t;

typedef struct {
    int count;
    zdj_usb_device_t * devices;
} zdj_usb_attached_devices_t;

typedef enum {
    ZDJ_USB_DRIVER_FLAG_HOST_NO_OVERLAY_DIR = 0x1,
    ZDJ_USB_DRIVER_FLAG_BAD_HOST_ROLE = 0x2,
    ZDJ_USB_DRIVER_FLAG_HOST_DEBUGFS_MISSING = 0x4,
    ZDJ_USB_DRIVER_FLAG_HOST_CONFIGFS_MISSING = 0x8
} zdj_usb_host_driver_flag_t;

typedef struct {
    bool has_update;
    bool has_port_partner;
    bool has_port_partner_update;
    bool has_browser_panel_update;
    bool has_file_browser_update;
    bool has_usb_panel_update;
    bool has_soundcard_update;
    bool has_control_update;
    int devices_line_count;
    zdj_usb_attached_devices_t attached;
    zdj_usb_host_driver_flag_t _flags;
} zdj_usb_host_status_t;

typedef enum {
    ZDJ_USB_DRIVER_FLAG_OFFLINE_HAS_DEBUGFS = 0x1,
    ZDJ_USB_DRIVER_FLAG_OFFLINE_HAS_CONFIGFS = 0x2
} zdj_usb_offline_driver_flag_t;

typedef struct {
    zdj_usb_mode_state_t mode_state;
    zdj_usb_mode_switch_context_t switch_ctx;
    bool run_state_thread;
    zdj_usb_host_status_t host_status;
    zdj_usb_gadget_status_t gadget_status;
    zdj_usb_offline_driver_flag_t offline_flags;
} zdj_usb_state_t;

typedef enum {
    ZDJ_SETTING_USB_INIT_OFFLINE,
    ZDJ_SETTING_USB_INIT_HOST,
    ZDJ_SETTING_USB_INIT_GADGET,
    ZDJ_SETTING_USB_INIT_PREVIOUS,
} zdj_usb_setting_init_option_t;

extern zdj_usb_state_t * zdj_usb_state;

// USB Lifecycle
zdj_error_type_t zdj_usb_init( void );
zdj_error_type_t zdj_usb_init_shell( void ); // Init and force mode to gadget w/shell func
zdj_error_type_t zdj_usb_disable( );

// USB Mode Control API
zdj_error_type_t zdj_usb_update_mode_from_sysfs( zdj_usb_mode_state_t * state );
void zdj_usb_put_offline_mode( zdj_usb_mode_state_t * state );
void zdj_usb_put_host_mode( zdj_usb_mode_state_t * state );
void zdj_usb_put_empty_gadget_mode( zdj_usb_mode_state_t * state );
void zdj_usb_put_current_gadget_mode( zdj_usb_mode_state_t * state );
void zdj_usb_put_error_mode( zdj_usb_mode_state_t * state, zdj_usb_mode_t error_mode );

// USB Gadget Mode API
bool zdj_usb_detect_gadgetfs( zdj_usb_state_t * state );
zdj_error_type_t zdj_usb_update_gadget_config_from_functionfs( zdj_usb_mode_state_t * state );

// USB Host Mode API
zdj_usb_attached_devices_t * zdj_usb_update_attached_devices( void );
void zdj_usb_update_alsa_profiles( zdj_usb_state_t * state, zdj_usb_device_t * device );

// USB UI Status API
void zdj_usb_put_status_headline( char * str );
void zdj_usb_put_status_line_1( char * str );
void zdj_usb_put_status_line_2( char * str );

void zdj_usb_launch_state_thread( void );

// geTTY Shell Control API
void zdj_usb_shell_launch( void );
bool zdj_usb_shell_is_running( void );

// USB Devices API (Private)
sqlite3 * _zdj_usb_get_device_db( void );
zdj_usb_device_t * zdj_usb_device_create_dto( 
    char * crc,
    char * usb_vendor,
    char * usb_product_id,
    char * manufacturer,
    char * product,
    char * serial,
    char * name_user
);
zdj_error_type_t zdj_usb_device_free_dto( zdj_usb_device_t * device );
zdj_error_type_t zdj_usb_device_store_dto( zdj_usb_device_t * device, sqlite3 * db );
zdj_usb_device_t * zdj_usb_device_fetch_dto_for_entity_id( char * entity_id, sqlite3 * db );
zdj_error_type_t zdj_usb_device_fetch_all_entity_ids( char ** arr, int count, sqlite3 * db );
zdj_usb_device_t * zdj_usb_device_fetch_dto_for_hash( char * hash, sqlite3 * db );
int zdj_usb_device_count_in_db( sqlite3 * db );
// char * zdj_usb_device_get_uuid( void );
zdj_error_type_t zdj_usb_device_cleanup_str( char * buf, size_t buf_len );

bool zdj_usb_devices_db_needs_init( void );
sqlite3 * zdj_usb_create_devices_db( void );
void zdj_usb_reset_devices_db( void );
void zdj_usb_reset_status( void );

// Driver Controls
bool zdj_usb_gather_driver_health( zdj_usb_state_t * state );
bool zdj_usb_update_offline_driver_health( zdj_usb_state_t * state );
bool zdj_usb_update_gadget_driver_health( zdj_usb_state_t * state );
bool zdj_usb_update_host_driver_health( zdj_usb_state_t * state );

bool zdj_usb_teardown_driver_mods( zdj_usb_state_t * state );
bool zdj_usb_teardown_gadget( zdj_usb_state_t * state );

bool zdj_usb_standup_gadget_mode( zdj_usb_state_t * state );
bool zdj_usb_standup_gadget_functions( zdj_usb_state_t * state );

bool zdj_usb_standup_host_mode( zdj_usb_state_t * state );

bool zdj_usb_modprobe_install( char * module_name );
bool zdj_usb_modprobe_remove( char * module_name );
bool zdj_usb_modprobe_check( char * module_name );

// void zdj_usb_log_begin( void );
// void zdj_usb_log( char * str );
// void zdj_usb_log_end( void );

#endif
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

#include <sqlite3.h>

#include <zerodj/system/error/zdj_error.h>

#define ZDJ_USB_REQUEST_PATH "/etc/zero_data/usb_request"
#define ZDJ_USB_STATUS_PATH "/etc/zero_data/usb_status"
#define ZDJ_USB_DEVICE_DB_PATH "/etc/zero_data/device.db"

typedef enum {
    ZDJ_USB_MODE_UNKNOWN,
    ZDJ_USB_MODE_OFFLINE,
    ZDJ_USB_MODE_ERROR,
    ZDJ_USB_MODE_HOST,
    ZDJ_USB_MODE_GADGET,
    ZDJ_USB_MODE_COUNT
} zdj_usb_mode_t;

static char * zdj_usb_mode_name[ ZDJ_USB_MODE_COUNT ] = {
    "Unknown Mode",// ZDJ_USB_MODE_UNKNOWN,
    "Offline",// ZDJ_USB_MODE_OFFLINE,
    "System Error",// ZDJ_USB_MODE_FATAL_ERROR,
    "Host Mode",// ZDJ_USB_MODE_HOST,
    "Gadget Mode"// ZDJ_USB_MODE_HOST_ERROR
};

typedef enum {
    ZDJ_USB_SUBMODE_UNKNOWN,
    ZDJ_USB_SUBMODE_OFFLINE,
    ZDJ_USB_SUBMODE_SWITCH,
    ZDJ_USB_SUBMODE_HOST_ERROR,
    ZDJ_USB_SUBMODE_GADGET_UAC2,
    ZDJ_USB_SUBMODE_GADGET_UAC2_SHELL,
    ZDJ_USB_SUBMODE_GADGET_UAC2_MIDI,
    ZDJ_USB_SUBMODE_GADGET_UAC2_MIDI_SHELL,
    ZDJ_USB_SUBMODE_GADGET_MIDI,
    ZDJ_USB_SUBMODE_GADGET_MIDI_SHELL,
    ZDJ_USB_SUBMODE_GADGET_HID,
    ZDJ_USB_SUBMODE_GADGET_HID_SHELL,
    ZDJ_USB_SUBMODE_GADGET_SHELL,
    ZDJ_USB_SUBMODE_GADGET_SHELL_DRIVE,
    ZDJ_USB_SUBMODE_GADGET_DRIVE,
    ZDJ_USB_SUBMODE_GADGET_RECOVERY,
    ZDJ_USB_SUBMODE_GADGET_ERROR,
    ZDJ_USB_SUBMODE_COUNT
} zdj_usb_submode_t;

static char * zdj_usb_submode_name[ ZDJ_USB_SUBMODE_COUNT ] = {
    "Unknown Mode",// ZDJ_USB_MODE_UNKNOWN,
    "Offline",// ZDJ_USB_MODE_OFFLINE,
    "Submode Switch",// ZDJ_USB_MODE_CHANGE,
    "Host Error",// ZDJ_USB_MODE_HOST_ERROR,
    "Audio (UAC2)",// ZDJ_USB_MODE_GADGET_UAC2,
    "Audio (UAC2) + Shell",// ZDJ_USB_MODE_GADGET_UAC2_SHELL,
    "Audio (UAC2) + MIDI",// ZDJ_USB_MODE_GADGET_UAC2_MIDI,
    "Audio (UAC2) + MIDI + Shell",// ZDJ_USB_MODE_GADGET_UAC2_MIDI_SHELL,
    "MIDI",// ZDJ_USB_MODE_GADGET_MIDI,
    "MIDI + Shell",// ZDJ_USB_MODE_GADGET_MIDI_SHELL,
    "Controller (HID)",// ZDJ_USB_MODE_GADGET_HID,
    "Controller (HID) + Shell",// ZDJ_USB_MODE_GADGET_HID_SHELL,
    "Shell",// ZDJ_USB_MODE_GADGET_SHELL,
    "Shell + Drive (Mass Storage)",// ZDJ_USB_MODE_GADGET_SHELL_DRIVE,
    "Drive (Mass Storage)",// ZDJ_USB_MODE_GADGET_DRIVE,
    "Recovery",// ZDJ_USB_MODE_GADGET_RECOVERY,
    "USB Gadget Error"// ZDJ_USB_MODE_GADGET_ERROR,
};

typedef enum {
    ZDJ_USB_SUBMODE_SWITCH_INIT,
    ZDJ_USB_SUBMODE_SWITCH_RUNNING,
    ZDJ_USB_SUBMODE_SWITCH_SUCCESS,
    ZDJ_USB_SUBMODE_SWITCH_ERROR,
    ZDJ_USB_SUBMODE_SWITCH_ERROR_GADGET_BRINGUP,
    ZDJ_USB_SUBMODE_SWITCH_ERROR_GADGET_TEARDOWN,
    ZDJ_USB_SUBMODE_SWITCH_ERROR_HOST_BRINGUP,
    ZDJ_USB_SUBMODE_SWITCH_COUNT
} zdj_usb_submode_switch_state_t;

static char * zdj_usb_submode_switch_name[ ZDJ_USB_SUBMODE_SWITCH_COUNT ] = {
    "Init",// ZDJ_USB_MODE_CHANGE_INIT,
    "Processing",// ZDJ_USB_MODE_CHANGE_RUNNING,
    "Success",// ZDJ_USB_MODE_CHANGE_SUCCESS,
    "Unknown Error",// ZDJ_USB_MODE_CHANGE_ERROR
    "Failed to Launch USB Gadget",// ZDJ_USB_MODE_CHANGE_ERROR_GADGET_BRINGUP
    "Failed to Shutdown USB Gadget",// ZDJ_USB_MODE_CHANGE_ERROR_GADGET_TEARDOWN
    "Failed to Enable USB Host",// ZDJ_USB_MODE_CHANGE_ERROR_HOST_BRINGUP
};

typedef struct {
    bool uac2;
    bool midi;
    bool mass_storage;
    bool hid;
    bool shell;
} zdj_usb_gadget_config_t;

typedef struct {
    zdj_usb_mode_t mode;
    zdj_usb_submode_t submode;
    zdj_usb_gadget_config_t gadget_config;
} zdj_usb_mode_request_t;

typedef struct {
    zdj_usb_mode_t mode;
    zdj_usb_submode_t submode;
    zdj_usb_gadget_config_t gadget_config;
    zdj_usb_submode_switch_state_t switch_state;
    bool has_frontend_update;
    bool requires_reboot;
} zdj_usb_status_t;

typedef struct zdj_usb_device_t {
    char * entity_id;
    char * hash;
    char * usb_vendor_id;
    char * usb_product_id;
    char * manufacturer;
    char * product;
    char * serial;
    char * name_user;
    char * mount_path;
    bool attached;
    bool has_audio;
    bool has_msd;
    bool has_hid;
    struct zdj_usb_device_t * next;
} zdj_usb_device_t;

typedef struct {
    int count;
    zdj_usb_device_t * devices;
} zdj_usb_attached_devices_t;

extern zdj_usb_status_t * zdj_usb_status;

// USB Lifecycle
zdj_error_type_t zdj_usb_init_frontend( void );


// USB Mode Control API
zdj_error_type_t zdj_usb_request_mode_switch( zdj_usb_mode_request_t * request );
zdj_usb_submode_t zdj_usb_submode_for_gadget_config( zdj_usb_gadget_config_t * config );
zdj_error_type_t zdj_usb_update_gadget_config_from_functionfs( zdj_usb_gadget_config_t * config );

// USB Connection Polling API
bool zdj_usb_has_port_partner( void );
bool zdj_usb_has_port_partner_update( void );
zdj_error_type_t zdj_usb_start_port_partner_poll( void );
zdj_error_type_t zdj_usb_stop_port_partner_poll( void );
bool zdj_usb_has_sysfs_devices_update( void );
zdj_error_type_t zdj_usb_start_sysfs_devices_poll( void );
zdj_error_type_t zdj_usb_stop_sysfs_devices_poll( void );

// USB Device API
zdj_usb_attached_devices_t * zdj_usb_get_attached_devices( void );
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
char * zdj_usb_device_get_uuid( void );
zdj_error_type_t zdj_usb_device_cleanup_str( char * buf, size_t buf_len );

// USB Mass Storage Device API
zdj_error_type_t zdj_usb_msd_device_set_mount_path( zdj_usb_device_t * device );


#endif
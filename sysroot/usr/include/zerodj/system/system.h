#pragma once
#include <stdint.h>
#include <stdbool.h>

#define ZERO_CU_DEVICE "/dev/cu.usbmodemd0053"

bool detect_mfgtools( void );
bool detect_uuu( void );

bool detect_upload_device( void );
bool detect_usb_serial_load_device( void );
char * usb_serial_load_device_info( void );
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <uuid.h>

#include <mutils/mhash.h>
#include <sqlite3.h>

#include <zerodj/error/zdj_error.h>
#include <zerodj/sql/zdj_sql.h>
#include <zerodj/usb/zdj_usb.h>

zdj_error_type_t zdj_usb_msd_device_set_mount_path( zdj_usb_device_t * device ) {
    printf( "zdj_usb_msd_device_set_mount_path\n" );
    // Open findmnt for line scanning
    // EX:
    // /dev/mmcblk2p3 /media/internal
    // none /sys/kernel/config
    // /dev/sda1 /media/usb0
    FILE * findmnt_fp = popen( "findmnt -rn --output=SOURCE,TARGET", "r" );
    if ( findmnt_fp == NULL ) {
        printf( "couldn't list mounted drives\n" );
        return ZDJ_ERROR_SYS_ERROR;
    } else {
        // usbmount adds msd devices at /dev/sda entries
        char findmnt_line[ 256 ];
        char sda_prefix[] = "/dev/sda";
        while( fgets( findmnt_line, sizeof( findmnt_line ), findmnt_fp ) ) {
            // printf( "findmnt line: %p/%s\n", findmnt_line, findmnt_line );
            if( !strncmp( findmnt_line, sda_prefix, strlen( sda_prefix ) ) ) {
                // We will split the findmnt output on space char
                char * space_ptr = strchr( findmnt_line, ' ' );
                int space_index = 0;
                if( space_ptr ) {
                    space_index = space_ptr - findmnt_line;
                }

                // Grab the dev path
                char dev_path[ 64 ];
                strncpy( dev_path, findmnt_line, space_index );
                zdj_usb_device_cleanup_str( &dev_path, sizeof( dev_path ) );
                printf( "dev_path: %s\n", dev_path );
                
                // Look at udevadm for a matching serial number
                // EX:
                // E: ID_REVISION=1.00
                // E: ID_SERIAL=USB_SanDisk_3.2Gen1_03038827102724045428-0:0
                // E: ID_SERIAL_SHORT=03038827102724045428
                char udev_cmd[ 128 ];
                // snprintf( udev_cmd, sizeof( udev_cmd ), "udevadm info -n %s", dev_path );
                snprintf( udev_cmd, sizeof( udev_cmd ), "udevadm info -n %s", "/dev/sda1" );
                printf( "udev_cmd: %s\n", udev_cmd );
                char udev_line[ 256 ];
                FILE * udev_fp = popen( strdup(udev_cmd), "r" );
                printf( "udev: %p\n", udev_fp );
                if ( udev_fp == NULL ) {
                    printf( "couldn't list mounted drives\n" );
                    return ZDJ_ERROR_SYS_ERROR;
                } else {
                    printf( "udev lines:\n" );
                    while( fgets( udev_line, sizeof( udev_line ), udev_fp ) ) {
                        printf( "udev_line: %s\n", udev_line );

                        char udev_prefix[] = "E: ID_SERIAL_SHORT";
                        if( !strncmp( udev_line, udev_prefix, strlen( udev_prefix ) ) ) {
                            // Get the serial number from the line
                            char serial[ 128 ];
                            strcpy( serial, udev_line+19 );
                            zdj_usb_device_cleanup_str( &serial, sizeof( serial ) );
                            printf( "looking for serial %s/%s\n", serial, device->serial );


                            // Check serial against the device DTO
                            if( !strcmp( serial, device->serial ) ) {
                                // If there's a serial number match, read the second field 
                                // of the findmnt output to get the mount path.
                                char mount_path[ 128 ];
                                strcpy( mount_path, space_ptr+1 );
                                printf( "mount_path: %s\n", mount_path );
                                device->mount_path = strdup( mount_path );
                            }
                        }
                    }
                    printf( "udev lines done\n" );
                }

            }
        }
    }
    pclose( findmnt_fp );
    
    // Look at the udevadm info for this path and match the serial number
    // against the device DTO

    // exit early w/success if we find a match
}
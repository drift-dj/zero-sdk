#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <uuid.h>

#include <mutils/mhash.h>
#include <sqlite3.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/sql/zdj_sql.h>
#include <zerodj/system/usb/zdj_usb.h>
#include <zerodj/system/uuid/zdj_uuid.h>

// DB must be accessed from USB thread only.
static sqlite3 * _zdj_usb_device_db;
// static zdj_usb_attached_devices_t * _zdj_usb_attached_devices = NULL;

// static sqlite3 * _zdj_usb_get_device_db( void );
// static zdj_error_type_t _zdj_usb_device_cleanup_str( char * buf, size_t buf_len );

static zdj_error_type_t _add_attached_device( 
    char * usb_vendor,
    char * usb_product_id,
    char * manufacturer,
    char * product,
    char * serial_number,
    bool has_audio,
    bool has_msd,
    bool has_hid,
    bool has_midi
);
static zdj_error_type_t _clear_attached_devices( void ); 

static void _msd_device_put_mount_path( zdj_usb_device_t * device );

// In host mode, consume /usb/devices and build DTOs
// for everything discovered.
// Match discovered devices against existing records
// in the device.db, or create new records for devices
// we haven't seen before.

// Call from USB thread only!
zdj_usb_attached_devices_t * zdj_usb_update_attached_devices( void ) {
    // if( !_zdj_usb_attached_devices ) {
    //     _zdj_usb_attached_devices = calloc( 1, sizeof( zdj_usb_attached_devices_t ) );
    //     _zdj_usb_attached_devices->count = 0;
    //     _zdj_usb_attached_devices->devices = NULL;
    // }

    // printf( "zdj_usb_get_attached_devices\n" );
    _clear_attached_devices( );

    FILE * fp = popen( "cat /sys/kernel/debug/usb/devices", "r" );
    if ( fp == NULL ) {
        printf( "couldn't open usb devices\n" );
    } else {
        char line[ 256 ];

        char usb_vendor[ 16 ];
        char usb_product_id[ 16 ];
        char manufacturer[ 256 ];
        char product[ 256 ];
        char serial_number[ 256 ];
        bool has_audio;
        bool has_msd;
        bool has_hid;
        bool has_midi;

        bool first_t_line = false;

        // Read line by line, managing state until all lines are read.
        while( fgets( line, sizeof( line ), fp ) ) {
            // printf( "%s\n", line );

            char * p;
            int ind;

            // Capture current devices @ T: line
            if( !strncmp( line, "T:", 2 ) ) {
                // Don't write at first t line.
                if( first_t_line ) {
                    // Capture current state to a device
                    _add_attached_device( 
                        usb_vendor,
                        usb_product_id,
                        manufacturer,
                        product,
                        serial_number,
                        has_audio,
                        has_msd,
                        has_hid,
                        has_midi
                    );
                    // Clear current state for next read
                    memset( &usb_vendor, 0, 16 );
                    memset( &usb_product_id, 0, 16 );
                    memset( &manufacturer, 0, 256 );
                    memset( &product, 0, 256 );
                    memset( &serial_number, 0, 256 );
                    has_audio = false;
                    has_msd = false;
                    has_hid = false;
                    has_midi = false;
                }
                first_t_line = true;
            } else if ( (p = strstr( line, "Vendor=" )) ) {
                ind = p - line;
                memcpy( &usb_vendor, &line[ ind+7 ], sizeof(char)*4 );
                zdj_usb_device_cleanup_str( &usb_vendor, sizeof( usb_vendor ) );

                p = strstr( line, "ProdID=" );
                ind = p - line;
                memcpy( &usb_product_id, &line[ ind+7 ], sizeof(char)*4 );
                zdj_usb_device_cleanup_str( &usb_product_id, sizeof( usb_product_id ) );
            } else if ( (p = strstr( line, "Manufacturer=" )) ) {
                ind = p - line;
                memcpy( &manufacturer, &line[ ind+13 ], sizeof( line )-ind-1 );
                zdj_usb_device_cleanup_str( &manufacturer, sizeof( manufacturer ) );
            } else if ( (p = strstr( line, "Product=" )) ) {
                ind = p - line;
                memcpy( &product, &line[ ind+8 ], sizeof( line )-ind-1 );
                zdj_usb_device_cleanup_str( &product, sizeof( product ) );
            } else if ( (p = strstr( line, "SerialNumber=" )) ) {
                ind = p - line;
                memcpy( &serial_number, &line[ ind+13 ], sizeof( line )-ind-1 );
                zdj_usb_device_cleanup_str( &serial_number, sizeof( serial_number ) );
            } else if ( (p = strstr( line, "Cls=01(audio)" )) ) {
                has_audio = true;
                // has_midi = true; // This can only be discovered by ALSA
            } else if ( (p = strstr( line, "Cls=03(HID  )" )) ) {
                has_hid = true;
            } else if ( (p = strstr( line, "Cls=08(stor.)" )) ) {
                has_msd = true;
            }
        }

        _add_attached_device( 
            usb_vendor,
            usb_product_id,
            manufacturer,
            product,
            serial_number,
            has_audio,
            has_msd,
            has_hid,
            has_midi
        );
    }
    pclose( fp );
}

zdj_usb_device_t * zdj_usb_device_create_dto( 
    char * crc,
    char * usb_vendor,
    char * usb_product_id,
    char * manufacturer,
    char * product,
    char * serial,
    char * name_user
) {
    zdj_usb_device_t * device = calloc( 1, sizeof( zdj_usb_device_t ) );
    // strcpy( device->entity_id, zdj_usb_device_get_uuid( ) );
    zdj_put_uuid_no_dash( device->entity_id );
    strcpy( device->hash, crc );
    strcpy( device->usb_vendor_id, usb_vendor );
    strcpy( device->usb_product_id, usb_product_id );
    strcpy( device->manufacturer, manufacturer );
    strcpy( device->product, product );
    strcpy( device->serial, serial );
    strcpy( device->name_user, name_user );
    return device;  
}

zdj_error_type_t zdj_usb_device_free_dto( zdj_usb_device_t * device ) {

}

zdj_error_type_t zdj_usb_device_store_dto( zdj_usb_device_t * device, sqlite3 * db ) {

}


// Call from USB thread only!
zdj_usb_device_t * zdj_usb_device_fetch_dto_for_entity_id( char * entity_id, sqlite3 * db ) {
    int res;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "select * from Device_Entity where entity_id like \'%s\'", 
        entity_id
    );

    int _eid_col = 0;
    int _hsh_col = 1;
    int _uid_col = 2;
    int _up_col = 3;
    int _nu_col = 4;
    int _man_col = 5;
    int _prd_col = 6;
    int _ser_col = 7;
    zdj_usb_device_t * device = NULL;

    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
    if( stmt ) {
        while ( ( res = sqlite3_step( stmt ) ) == SQLITE_ROW ) { 

            device = calloc( 1, sizeof( zdj_usb_device_t ) );
            strcpy( device->entity_id, (char*)sqlite3_column_text ( stmt, _eid_col ) );
            strcpy( device->hash, strdup( (char*)sqlite3_column_text ( stmt, _hsh_col ) ) );
            strcpy( device->usb_vendor_id, (char*)sqlite3_column_text ( stmt, _uid_col ) );
            strcpy( device->usb_product_id, (char*)sqlite3_column_text ( stmt, _up_col ) );
            strcpy( device->name_user, (char*)sqlite3_column_text ( stmt, _nu_col ) );
            strcpy( device->manufacturer, (char*)sqlite3_column_text ( stmt, _man_col ) );
            strcpy( device->product, (char*)sqlite3_column_text ( stmt, _prd_col ) );
            strcpy( device->serial, (char*)sqlite3_column_text ( stmt, _ser_col ) );
            
            device->attached = false;
            device->has_audio = false;
            device->has_hid = false;
            device->has_msd = false;
            device->has_midi = false;
        }
        sqlite3_finalize( stmt );
    }

    return device;
}

zdj_error_type_t zdj_usb_device_fetch_all_entity_ids( char ** arr, int count, sqlite3 * db ) {

}

// Call from USB thread only!
zdj_usb_device_t * zdj_usb_device_fetch_dto_for_hash( char * hash, sqlite3 * db ) {
    int res;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "select * from Device_Entity where hash like \'%s\'", 
        hash
    );

    int _eid_col = 0;
    int _hsh_col = 1;
    int _uid_col = 2;
    int _up_col = 3;
    int _nu_col = 4;
    int _man_col = 5;
    int _prd_col = 6;
    int _ser_col = 7;
    zdj_usb_device_t * device = NULL;

    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
    if( stmt ) {
        while ( ( res = sqlite3_step( stmt ) ) == SQLITE_ROW ) { 

            device = calloc( 1, sizeof( zdj_usb_device_t ) );
            strcpy( device->entity_id, (char*)sqlite3_column_text ( stmt, _eid_col ) );
            strcpy( device->hash, strdup( (char*)sqlite3_column_text ( stmt, _hsh_col ) ) );
            strcpy( device->usb_vendor_id, (char*)sqlite3_column_text ( stmt, _uid_col ) );
            strcpy( device->usb_product_id, (char*)sqlite3_column_text ( stmt, _up_col ) );
            strcpy( device->name_user, (char*)sqlite3_column_text ( stmt, _nu_col ) );
            strcpy( device->manufacturer, (char*)sqlite3_column_text ( stmt, _man_col ) );
            strcpy( device->product, (char*)sqlite3_column_text ( stmt, _prd_col ) );
            strcpy( device->serial, (char*)sqlite3_column_text ( stmt, _ser_col ) );
            device->attached = false;
            device->has_audio = false;
            device->has_hid = false;
            device->has_msd = false;
            device->has_midi = false;
        }
        sqlite3_finalize( stmt );
    }

    return device;
}

int zdj_usb_device_count_in_db( sqlite3 * db ) {

}

// Call from USB thread only!
static zdj_error_type_t _add_attached_device( 
    char * usb_vendor,
    char * usb_product_id,
    char * manufacturer,
    char * product,
    char * serial_number,
    bool has_audio,
    bool has_msd,
    bool has_hid,
    bool has_midi
) {
    printf( "_add_attached_device: %s, %s, %s, %s, %s\n", usb_vendor, usb_product_id, manufacturer, product, serial_number );

    // Ignore the ECHI host controller
    if( !strcmp( serial_number, "ci_hdrc.0" ) ) { return ZDJ_ERROR_OKAY; }

    // Make a CRC from SerialNumber, Vender + ProdID
    char * device_crc;
    int i;
    int hash_block_size = 256;
    unsigned char buffer[ 256 ];
    MHASH td = mhash_init( MHASH_CRC32B );
    if ( td != MHASH_FAILED ){
        mhash( td, &buffer, hash_block_size );

        unsigned char * hash_out;
        char result[ mhash_get_block_size( MHASH_CRC32B ) * 2 + 1 ];
        hash_out = mhash_end( td );
        for (i = 0; i < mhash_get_block_size( MHASH_CRC32B ); i++) {
            sprintf( &result[ i*2 ], "%.2x", hash_out[ i ] );
        }
        device_crc = strdup( result );
    } else {
        return ZDJ_ERROR_OKAY;
    }

    // Check for record w/matching hash in db.
    zdj_usb_device_t * device = zdj_usb_device_fetch_dto_for_hash( 
        device_crc, 
        _zdj_usb_get_device_db( ) 
    );

    // Make/insert new DTO if no matching hash.
    char name_user[ 128 ];
    sprintf( name_user, "%s %s", manufacturer, product );
    if( !device ) { 
        device = zdj_usb_device_create_dto(
            device_crc,
            usb_vendor,
            usb_product_id,
            manufacturer,
            product,
            serial_number,
            name_user
        );
        if( device ) {
            zdj_usb_device_store_dto( device, _zdj_usb_get_device_db( ) );
        } else {
            return ZDJ_ERROR_USB_DEVICE_DB_ERROR;
        }
    }

    // Set gadget capabilitites
    device->has_audio = has_audio;
    device->has_hid = has_hid;
    device->has_msd = has_msd;
    device->has_midi = has_midi;

    // Set mount path of msd device
    if( device->has_msd ) {
        // sleep for a sec to let udev populate findmnt (used in put_mount_path)
        sleep( 1 );
        _msd_device_put_mount_path( device );
    }
    
    // Set device attached state and add to attached devs
    device->attached = true;

    // Insert new device at head of linked list
    device->next = zdj_usb_state->host_state.attached.devices;
    zdj_usb_state->host_state.attached.devices = device;
    zdj_usb_state->host_state.attached.count++;

    // if( !_zdj_usb_attached_devices ) {
    //     _zdj_usb_attached_devices = calloc( 1, sizeof( zdj_usb_attached_devices_t ) );
    //     _zdj_usb_attached_devices->count = 0;
    //     _zdj_usb_attached_devices->devices = NULL;
    // }

    // Insert new device at head of linked list
    // if( _zdj_usb_attached_devices->devices ) {
    //     device->next = _zdj_usb_attached_devices->devices;
    // }
    // _zdj_usb_attached_devices->devices = device;

    // Increment device count
    // _zdj_usb_attached_devices->count++;

    

    return ZDJ_ERROR_OKAY;
}

static zdj_error_type_t _clear_attached_devices( void ) {
    // zdj_usb_device_t * device = zdj_usb_state->host_state.attached.devices;
    // while( device ) {
    //     zdj_usb_device_t * next_device = device->next;
    //     // free( device );
    //     device = next_device;
    // } 
    zdj_usb_state->host_state.attached.devices = NULL;
    zdj_usb_state->host_state.attached.count = 0;

    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t zdj_usb_device_cleanup_str( char * buf, size_t buf_len ) {
    for( int i=0; i<buf_len; i++ ) {
        if( buf[ i ] == '\n' ||
            buf[ i ] == 0x0a ) {
            buf[ i ] = '\0';
        }
        if( buf[ i ] == '\0' ) {
            return ZDJ_ERROR_OKAY;
        }
    }
    return ZDJ_ERROR_OKAY;
}

static void _msd_device_put_mount_path( zdj_usb_device_t * device ) {
    printf( "_msd_device_put_mount_path\n" );
    // Open findmnt for line scanning
    // EX:
    // /dev/mmcblk2p3 /media/internal
    // none /sys/kernel/config
    // /dev/sda1 /media/usb0
    strcpy( device->mount_path, "/" );
    device->mount_path_valid = false;
    FILE * findmnt_fp = popen( "findmnt -rn --output=SOURCE,TARGET", "r" );
    if ( findmnt_fp == NULL ) {
        printf( "couldn't list mounted drives\n" );
        return;
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
                zdj_usb_device_cleanup_str( dev_path, sizeof( dev_path ) );
                printf( "dev_path: %s\n", dev_path );
                
                // Look at udevadm for a matching serial number
                // EX:
                // E: ID_REVISION=1.00
                // E: ID_SERIAL=USB_SanDisk_3.2Gen1_03038827102724045428-0:0
                // E: ID_SERIAL_SHORT=03038827102724045428
                char udev_cmd[ 128 ];
                // snprintf( udev_cmd, sizeof( udev_cmd ), "udevadm info -n %s", dev_path );
                snprintf( udev_cmd, sizeof( udev_cmd ), "udevadm info -n %s", "/dev/sda" );
                // printf( "udev_cmd: %s\n", udev_cmd );
                char udev_line[ 256 ];
                FILE * udev_fp = popen( strdup(udev_cmd), "r" );
                // printf( "udev: %p\n", udev_fp );
                if ( udev_fp == NULL ) {
                    printf( "couldn't list mounted drives\n" );
                    return;
                } else {
                    // printf( "udev lines:\n" );
                    while( fgets( udev_line, sizeof( udev_line ), udev_fp ) ) {
                        // printf( "udev_line: %s\n", udev_line );

                        char udev_prefix[] = "E: ID_SERIAL_SHORT";
                        if( !strncmp( udev_line, udev_prefix, strlen( udev_prefix ) ) ) {
                            // Get the serial number from the line
                            char serial[ 128 ];
                            strcpy( serial, udev_line+19 );
                            zdj_usb_device_cleanup_str( serial, sizeof( serial ) );
                            // printf( "looking for serial %s/%s\n", serial, device->serial );

                            // Check serial against the device DTO
                            if( !strcmp( serial, device->serial ) ) {
                                // If there's a serial number match, read the second field 
                                // of the findmnt output to get the mount path.
                                char mount_path[ 128 ];
                                strcpy( device->mount_path, space_ptr+1 );
                                zdj_usb_device_cleanup_str( device->mount_path, sizeof( device->mount_path ) );
                                device->mount_path_valid = true;
                                printf( "mount_path: %s\n", device->mount_path );
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





// DB get/create/reset API

// DB can only be read and written from USB thread.
// Data from DB is accessible from outside USB thread on the zdj_usb_state object.
sqlite3 * _zdj_usb_get_device_db( void ) {
    if( !_zdj_usb_device_db ) {
        _zdj_usb_device_db = zdj_sql_open( ZDJ_USB_DEVICE_DB_PATH );
    }
    
    return _zdj_usb_device_db;
}

bool zdj_usb_devices_db_needs_init( void ) {
    // Check for DB w/o Device_Entity tables
    sqlite3 * db = zdj_sql_open( ZDJ_USB_DEVICE_DB_PATH );

    bool device_table_exists = false;

    if( db ) {
        char sql[ 2048 ];
        sprintf( sql, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='Device_Entity'" );
        int res;
        sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( sql, db );
        if( stmt ) {
            while ( (( res = sqlite3_step( stmt ) ) == SQLITE_ROW) ) {
                int count = sqlite3_column_int(stmt, 0);
                if ( count > 0 ) {
                    device_table_exists = true;
                }
            }
            sqlite3_finalize( stmt );
        }
        zdj_sql_close( db );
    }

    return device_table_exists;
}

sqlite3 * zdj_usb_create_devices_db( void ) {
    // if( !_zdj_usb_device_db ) {
    //     _zdj_usb_device_db = zdj_sql_open( ZDJ_USB_DEVICE_DB_PATH );
    // }
    
    // return _zdj_usb_device_db;
}


void zdj_usb_reset_devices_db( void ) {
    printf( "getting db ref\n" );
    sqlite3 * db = _zdj_usb_get_device_db( );
    if( !db ) { 
        printf( "Devices reset failed: couldn't open DB path" );
        return; 
    }
    printf( "closing db ref\n" );
    zdj_sql_close( db );

    printf( "deleting file\n" );
    remove( ZDJ_USB_DEVICE_DB_PATH );

    printf( "creating new db\n" );
    db = zdj_sql_open( ZDJ_USB_DEVICE_DB_PATH );
    if( !db ) {
        printf( "Devices reset failed: couldn't open new DB path" );
        return;
    }

    char sql[ 1024 ];
    strcpy( sql, "CREATE TABLE IF NOT EXISTS 'Device_Entity' ( 'entity_id' TEXT NOT NULL, 'hash' TEXT, 'usb_vendor_id' TEXT, 'usb_product_id' TEXT, 'name_user' TEXT, 'manufacturer' TEXT, 'product' TEXT, 'serial' TEXT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( sql, db );

    zdj_sql_close( db );
}
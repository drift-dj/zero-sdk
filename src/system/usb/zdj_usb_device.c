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

static sqlite3 * _zdj_usb_device_db;
static zdj_usb_attached_devices_t * _zdj_usb_attached_devices = NULL;

static sqlite3 * _zdj_usb_get_device_db( void );
// static zdj_error_type_t _zdj_usb_device_cleanup_str( char * buf, size_t buf_len );

zdj_error_type_t _zdj_usb_add_attached_device( 
    char * usb_vendor,
    char * usb_product_id,
    char * manufacturer,
    char * product,
    char * serial_number,
    bool has_audio,
    bool has_msd,
    bool has_hid
);
zdj_error_type_t _zdj_usb_clear_attached_devices( void ); 

// In host mode, consume /usb/devices and build DTOs
// for everything discovered.
// Match discovered devices against existing records
// in the device.db, or create new records for devices
// we haven't seen before.
zdj_usb_attached_devices_t * zdj_usb_get_attached_devices( void ) {
    if( !_zdj_usb_attached_devices ) {
        _zdj_usb_attached_devices = calloc( 1, sizeof( zdj_usb_attached_devices_t ) );
        _zdj_usb_attached_devices->count = 0;
        _zdj_usb_attached_devices->devices = NULL;
    }

    // printf( "zdj_usb_get_attached_devices\n" );
    _zdj_usb_clear_attached_devices( );

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
                    _zdj_usb_add_attached_device( 
                        usb_vendor,
                        usb_product_id,
                        manufacturer,
                        product,
                        serial_number,
                        has_audio,
                        has_msd,
                        has_hid
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
            } else if ( (p = strstr( line, "Cls=03(HID  )" )) ) {
                has_hid = true;
            } else if ( (p = strstr( line, "Cls=08(stor.)" )) ) {
                has_msd = true;
            }
        }
        // Capture remaining state to a device
        _zdj_usb_add_attached_device( 
            usb_vendor,
            usb_product_id,
            manufacturer,
            product,
            serial_number,
            has_audio,
            has_msd,
            has_hid
        );
    }
    pclose( fp );

    return _zdj_usb_attached_devices;
}

char * zdj_usb_device_get_uuid( void ) {
    uuid_t uuid;
    uuid_generate( uuid );
    char uuid_str[ 37 ];
    char uuid_str_no_dash[ 37 ];
    uuid_unparse_lower( uuid, uuid_str );
    int n = 0;
    for( int i=0; i<37; i++ ) {
        if( uuid_str[ i ] != '-' ) {
            uuid_str_no_dash[ n ] = uuid_str[ i ];
            n++;
        }
    }
    return strdup( uuid_str_no_dash );
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
    device->entity_id = zdj_usb_device_get_uuid( );
    device->hash = strdup( crc );
    device->usb_vendor_id = strdup( usb_vendor );
    device->usb_product_id = strdup( usb_product_id );
    device->manufacturer = strdup( manufacturer );
    device->product = strdup( product );
    device->serial = strdup( serial );
    device->name_user = strdup( name_user );
    return device;  
}

zdj_error_type_t zdj_usb_device_free_dto( zdj_usb_device_t * device ) {

}

zdj_error_type_t zdj_usb_device_store_dto( zdj_usb_device_t * device, sqlite3 * db ) {

}


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
            device->entity_id = strdup( (char*)sqlite3_column_text ( stmt, _eid_col ) );
            device->hash = strdup( (char*)sqlite3_column_text ( stmt, _hsh_col ) );
            device->usb_vendor_id = strdup( (char*)sqlite3_column_text ( stmt, _uid_col ) );
            device->usb_product_id = strdup( (char*)sqlite3_column_text ( stmt, _up_col ) );
            device->name_user = strdup( (char*)sqlite3_column_text ( stmt, _nu_col ) );
            device->manufacturer = strdup( (char*)sqlite3_column_text ( stmt, _man_col ) );
            device->product = strdup( (char*)sqlite3_column_text ( stmt, _prd_col ) );
            device->serial = strdup( (char*)sqlite3_column_text ( stmt, _ser_col ) );
            device->attached = false;
            device->has_audio = false;
            device->has_hid = false;
            device->has_msd = false;
        }
        sqlite3_finalize( stmt );
    }

    return device;
}

zdj_error_type_t zdj_usb_device_fetch_all_entity_ids( char ** arr, int count, sqlite3 * db ) {

}

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
            device->entity_id = strdup( (char*)sqlite3_column_text ( stmt, _eid_col ) );
            device->hash = strdup( (char*)sqlite3_column_text ( stmt, _hsh_col ) );
            device->usb_vendor_id = strdup( (char*)sqlite3_column_text ( stmt, _uid_col ) );
            device->usb_product_id = strdup( (char*)sqlite3_column_text ( stmt, _up_col ) );
            device->name_user = strdup( (char*)sqlite3_column_text ( stmt, _nu_col ) );
            device->manufacturer = strdup( (char*)sqlite3_column_text ( stmt, _man_col ) );
            device->product = strdup( (char*)sqlite3_column_text ( stmt, _prd_col ) );
            device->serial = strdup( (char*)sqlite3_column_text ( stmt, _ser_col ) );
            device->attached = false;
            device->has_audio = false;
            device->has_hid = false;
            device->has_msd = false;
        }
        sqlite3_finalize( stmt );
    }

    return device;
}

int zdj_usb_device_count_in_db( sqlite3 * db ) {

}

// Main thread only!
sqlite3 * _zdj_usb_get_device_db( void ) {
    if( !_zdj_usb_device_db ) {
        _zdj_usb_device_db = zdj_sql_open( ZDJ_USB_DEVICE_DB_PATH );
    }
    
    return _zdj_usb_device_db;
}

zdj_error_type_t _zdj_usb_add_attached_device( 
    char * usb_vendor,
    char * usb_product_id,
    char * manufacturer,
    char * product,
    char * serial_number,
    bool has_audio,
    bool has_msd,
    bool has_hid
) {
    printf( "_zdj_usb_add_attached_device: %s, %s, %s, %s, %s\n", usb_vendor, usb_product_id, manufacturer, product, serial_number );

    // Reject the ECHI host controller
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
    if( !device ) { 
        device = zdj_usb_device_create_dto(
            device_crc,
            usb_vendor,
            usb_product_id,
            manufacturer,
            product,
            serial_number,
            product // Use product name as user name 
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

    // Set mount path of msd device
    if( device->has_msd ) {
        zdj_usb_msd_device_set_mount_path( device );
    }
    
    // Set device attached state and add to attached devs
    device->attached = true;

    if( !_zdj_usb_attached_devices ) {
        _zdj_usb_attached_devices = calloc( 1, sizeof( zdj_usb_attached_devices_t ) );
        _zdj_usb_attached_devices->count = 0;
        _zdj_usb_attached_devices->devices = NULL;
    }

    // Insert new device at head of linked list
    if( _zdj_usb_attached_devices->devices ) {
        device->next = _zdj_usb_attached_devices->devices;
    }
    _zdj_usb_attached_devices->devices = device;

    // Increment device count
    _zdj_usb_attached_devices->count++;

    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t _zdj_usb_clear_attached_devices( void ) {
    if( !_zdj_usb_attached_devices ) { return ZDJ_ERROR_OKAY; }

    zdj_usb_device_t * device = _zdj_usb_attached_devices->devices;
    while( device ) {
        zdj_usb_device_t * next_device = device->next;
        free( device->entity_id );
        free( device->hash );
        free( device->usb_product_id );
        free( device->usb_vendor_id );
        free( device->manufacturer );
        free( device->product );
        free( device->serial );
        free( device->name_user );
        // free( device );
        device = next_device;
    } 
    _zdj_usb_attached_devices->devices = NULL;
    _zdj_usb_attached_devices->count = 0;

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
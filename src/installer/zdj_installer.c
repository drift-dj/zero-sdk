#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sqlite3.h>

#include <zerodj/installer/zdj_installer.h>
#include <zerodj/sql/zdj_sql.h>

zdj_installer_t * zdj_installer_for_filepath( char * filepath ) {
    // Attempt to open filepath
    FILE * installer_fd = fopen( filepath, "r" );
    if( installer_fd ) {
        printf( "Reading installer binary at: %s\n", filepath );
    } else {
        printf( "Failed to open installer binary.\n" );
        return NULL;
    }

    // Attempt to read installer header
    fseek( installer_fd, ZDJ_INSTALLER_HEADER_START, SEEK_SET );
    zdj_installer_t * installer = calloc( 1, sizeof( zdj_installer_t ) );
    int br = fread( installer, sizeof( zdj_installer_t ), 1, installer_fd );
    if( !br ) {
        return NULL;
    }

    installer->fd = installer_fd;
    printf( "installer_fd: %p installer->fd: %p\n", installer_fd, installer->fd );
    installer->manifest_db = NULL;
    installer->next = NULL;
    installer->prev = NULL;
    return installer;
}

int zdj_installer_file_count( zdj_installer_t * installer ) {
    // If manifest isn't extracted yet, do it.
    if( !installer->manifest_db ) { zdj_installer_extract_manifest( installer ); }
    if( !installer->manifest_db ) { return 0; }
    
    return zdj_sql_rows_in_table( "Manifest", NULL, installer->manifest_db );
}

bool zdj_installer_extract_manifest( zdj_installer_t * installer ) {
    // Seek to start of manifest db
    if( !installer->fd ) { return false; }
    fseek( installer->fd, installer->data_offsets.manifest_offset, SEEK_SET );

    // Write manifest bytes to installer tmp dir
    FILE * manifest_fd = fopen( ZDJ_INSTALLER_MANIFEST_PATH, "w" );
    if( !manifest_fd ) { return false; }
    int i=0;
    char a;
    while ( i++ < installer->data_offsets.manifest_length ) {
        a = fgetc( installer->fd );
        fputc( a, manifest_fd );
    }
    fwrite( installer->fd, installer->data_offsets.manifest_length, 1, manifest_fd );
    fclose( manifest_fd );

    // Open db + return
    installer->manifest_db = zdj_sql_open( ZDJ_INSTALLER_MANIFEST_PATH );
    return installer->manifest_db;
}

void zdj_installer_iterate_manifest( zdj_installer_t * installer, zdj_installer_manifest_item_cb cb, void * data ) {
    // Iterate over rows in manifest table
    // Execute row stepper
    int res;
    sqlite3_stmt * c_stmt = zdj_sql_prep_row_stepper( "select * from Manifest", (sqlite3*)installer->manifest_db );
    if( c_stmt ) {
        while ( ( res = sqlite3_step( c_stmt ) ) == SQLITE_ROW ) {
            // Invoke file_cb for each row
            zdj_installer_manifest_item_t item;
            strcpy( &item.filepath, (char*)sqlite3_column_text( c_stmt, 0 ) );
            item.blob_start = sqlite3_column_int( c_stmt, 1 );
            item.blob_length = sqlite3_column_int( c_stmt, 2 );
            cb( &item, data );
        }
        sqlite3_finalize( c_stmt );
    }
}

bool zdj_installer_extract_files( zdj_installer_t * installer ) {
    // Read manifest records and extract files from bin blob to tmp
}

bool zdj_installer_validate_files( zdj_installer_t * installer ) {
    // Test all copy operations for source + dest
    // Return success
}

bool zdj_installer_commit( zdj_installer_t * installer ) {
    // Run copy operations
    // Add/update registry install record
}

bool zdj_installer_cleanup( zdj_installer_t * installer ) {
    // Close manifest db
    // Remove files from tmp
    // Close installer fd
}
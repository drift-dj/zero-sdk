#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sqlite3.h>

#include <zerodj/fs/zdj_fs.h>
#include <zerodj/health/zdj_health_type.h>
#include <zerodj/installer/zdj_installer.h>
#include <zerodj/sql/zdj_sql.h>

bool _zdj_installer_size_check( zdj_installer_t * installer );
void _zdj_installer_extract_file_cb( zdj_installer_manifest_item_t * item, void * data );
void _zdj_installer_validate_file_cb( zdj_installer_manifest_item_t * item, void * data );
void _zdj_installer_commit_file_cb( zdj_installer_manifest_item_t * item, void * data );

zdj_installer_t * zdj_installer_for_filepath( char * filepath ) {
    // Attempt to open filepath
    FILE * installer_fd = fopen( filepath, "r" );
    if( !installer_fd ) {
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

    strcpy( installer->path, filepath );
    installer->valid = true;
    installer->status = ZDJ_HEALTH_STATUS_UNKNOWN;
    
    // Check available disk space
    _zdj_installer_size_check( installer );

    installer->fd = installer_fd;
    installer->manifest_db = NULL;
    installer->next = NULL;
    installer->prev = NULL;
    return installer;
}

int zdj_installer_file_count( zdj_installer_t * installer ) {
    // If manifest isn't extracted yet, do it.
    if( !installer->manifest_db ) { zdj_installer_extract_manifest( installer ); }
    if( !installer->manifest_db ) { 
        installer->valid = false;
        installer->status = ZDJ_HEALTH_STATUS_SYS_ERROR;
        return 0;
    }
    
    return zdj_sql_rows_in_table( "Manifest", NULL, installer->manifest_db );
}

bool zdj_installer_extract_manifest( zdj_installer_t * installer ) {
    // Seek to start of manifest db
    if( !installer->fd ) { 
        installer->valid = false;
        installer->status = ZDJ_HEALTH_STATUS_SYS_ERROR;
        return false;
    }

    // Write manifest bytes to installer tmp dir
    zdj_health_status_t extract_health = zdj_fs_extract_file_from_binary(
        installer->path, 
        ZDJ_INSTALLER_MANIFEST_PATH, 
        installer->data_offsets.manifest_offset, 
        installer->data_offsets.manifest_length, 
        true
    );
    if( extract_health > ZDJ_HEALTH_STATUS_OKAY ) { 
        installer->valid = false;
        installer->status = extract_health;
        return false;
    }

    // Open db + return
    installer->manifest_db = zdj_sql_open( ZDJ_INSTALLER_MANIFEST_PATH );
    return installer->manifest_db;
}

// Loop over every file in the manifest_db.
// Create a manifest_item for each row, and send it to the callback.
void zdj_installer_iterate_manifest( 
    zdj_installer_t * installer, 
    zdj_installer_manifest_item_cb cb, 
    void * data 
) {
    // Iterate over rows in manifest table
    // Execute row stepper
    int res;
    sqlite3_stmt * c_stmt = zdj_sql_prep_row_stepper( "select * from Manifest", (sqlite3*)installer->manifest_db );
    if( c_stmt ) {
        while ( ( res = sqlite3_step( c_stmt ) ) == SQLITE_ROW ) {
            zdj_installer_manifest_item_t item;
            // Build dest/extract/rollback paths
            strcpy( &item.dest_path, (char*)sqlite3_column_text( c_stmt, 0 ) );
            snprintf( item.extract_path, sizeof( item.extract_path ), "%s/%s", 
                ZDJ_INSTALLER_EXTRACT_DIR, 
                basename( item.dest_path )
            );
            snprintf( item.rollback_path, sizeof( item.rollback_path ), "%s/%s", 
                ZDJ_INSTALLER_ROLLBACK_DIR, 
                basename( item.dest_path )
            );
            // Build file size data
            item.blob_start = sqlite3_column_int( c_stmt, 1 );
            item.blob_length = sqlite3_column_int( c_stmt, 2 );
            // Invoke file_cb for each row
            cb( &item, data );
        }
        sqlite3_finalize( c_stmt );
    }
}

// Per-manifest item callback
// Pull file from installer's binary blob to extract dir
void _zdj_installer_extract_file_cb( zdj_installer_manifest_item_t * item, void * data ) {
    zdj_installer_t * installer = (zdj_installer_t*)data;
    if( !installer->fd ) { return; }

    // If there isn't enough drive space to extract files + copy into place, invalidate.
    if( !_zdj_installer_size_check( installer ) ) { return; }

    // Write item bytes to installer tmp dir
    zdj_health_status_t extract_health = zdj_fs_extract_file_from_binary(
        installer->path, 
        item->extract_path, 
        item->blob_start, 
        item->blob_length, 
        false
    );

    if( extract_health > ZDJ_HEALTH_STATUS_OKAY ) { 
        installer->valid = false;
        installer->status = extract_health;
    }
}

// Write out all files in manifest_db to extract dir
bool zdj_installer_extract_files( zdj_installer_t * installer ) {
    // Read manifest records and extract files from bin blob to tmp
    zdj_installer_iterate_manifest( installer, _zdj_installer_extract_file_cb, installer );
    return installer->valid;
}

// Per-manifest_item callback.
// Validate each manifest_item for proper pathing, permissions, size, etc.
void _zdj_installer_validate_file_cb( zdj_installer_manifest_item_t * item, void * data ) {
    zdj_installer_t * installer = (zdj_installer_t*)data;
 
    // If doesn't exist or doesn't match blob length, invalidate.
    int size = zdj_fs_get_size( item->extract_path );
    if( size != item->blob_length ) {
        installer->valid = false;
        installer->status = ZDJ_HEALTH_STATUS_BAD_FILESIZE;
    }

    // TODO - Check for protected dest path + installer permissions
}

// Validate all files in the manifest against a set of criteria
// before the commit phase to catch any potential errors early.
bool zdj_installer_validate_files( zdj_installer_t * installer ) {
    installer->valid = true;
    // Test all copy operations for source + dest
    zdj_installer_iterate_manifest( installer, _zdj_installer_validate_file_cb, installer );
    // Return success
    return installer->valid;
}

void _zdj_installer_commit_file_cb( zdj_installer_manifest_item_t * item, void * data ) {
    zdj_installer_t * installer = (zdj_installer_t*)data;
    zdj_health_status_t rollback_health = ZDJ_HEALTH_STATUS_UNKNOWN;

    // Copy dest file to rollback path if it exists
    if( access( item->dest_path, F_OK ) == 0 ) {
        rollback_health = zdj_fs_copy_file( item->dest_path, item->rollback_path, false );
        if( rollback_health > ZDJ_HEALTH_STATUS_OKAY ) { 
            installer->valid = false;
            installer->status = rollback_health;
            return; 
        }
    }

    // Overwrite dest path with extracted file
    zdj_health_status_t dest_health = zdj_fs_copy_file( item->extract_path, item->dest_path, true );
    // If copy to dest fails, force a rollback
    if( rollback_health > ZDJ_HEALTH_STATUS_OKAY ) { 
        installer->valid = false;
        installer->status = ZDJ_HEALTH_STATUS_ROLLBACK;
        return; 
    }
}

void _zdj_installer_rollback_file_cb( zdj_installer_manifest_item_t * item, void * data ) {
    zdj_installer_t * installer = (zdj_installer_t*)data;

    // Overwrite the dest file with the rollback file
    zdj_health_status_t rollback_health = zdj_fs_copy_file( item->rollback_path, item->dest_path, true );
    
    // If rollback fails, corrupt the install
    if( rollback_health > ZDJ_HEALTH_STATUS_OKAY ) { 
        installer->valid = false;
        installer->status = ZDJ_HEALTH_STATUS_CORRUPT;
        return; 
    }
}

bool zdj_installer_commit( zdj_installer_t * installer ) {
    // Run copy operations
    zdj_installer_iterate_manifest( installer, _zdj_installer_commit_file_cb, installer );
    
    // If something bad happened during copy, roll back to previous state
    if( installer->status == ZDJ_HEALTH_STATUS_ROLLBACK ) {
        zdj_installer_iterate_manifest( installer, _zdj_installer_rollback_file_cb, installer );
        // If rollback fails, corrupt the install record
        if( installer->status > ZDJ_HEALTH_STATUS_OKAY ) {
            printf( "rollback failed, uh oh.\n" );
            installer->install.health = ZDJ_HEALTH_STATUS_CORRUPT;
        }
    }

    // Add/update registry install record
    zdj_registry_commit_install( &installer->install );

    return installer->valid;
}

bool zdj_installer_cleanup( zdj_installer_t * installer ) {
    // Close manifest db
    sqlite3_close( installer->manifest_db );

    // Remove installer dir
    zdj_fs_remove_dir( ZDJ_INSTALLER_EXTRACT_DIR );
    // Remove rollback dir
    zdj_fs_remove_dir( ZDJ_INSTALLER_ROLLBACK_DIR );

    // Close fds
    fclose( installer->fd );
}

// Check disk to see if there's enough space for this installer
bool _zdj_installer_size_check( zdj_installer_t * installer ) {
    // Need space for manifest db.
    // Need space for 3 copies of each file in manifest:
    // 1 for extracted file in validation dir
    // 1 for rollback copy
    // 1 for destination file
    int installer_disk_size = (installer->data_offsets.binary_length*3) + installer->data_offsets.manifest_length;
    if( installer_disk_size > zdj_fs_sys_space( ) ) {
        installer->valid = false;
        installer->status = ZDJ_HEALTH_STATUS_NO_SPACE;
        return false;
    }
    return true;
}
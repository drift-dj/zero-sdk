#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sqlite3.h>

#include <zerodj/fs/zdj_fs.h>
#include <zerodj/health/zdj_health_type.h>
#include <zerodj/library/zdj_library.h>
#include <zerodj/sql/zdj_sql.h>

zdj_library_import_type_t zdj_library_get_import_type_for_path( char * path ) {
    if( zdj_fs_path_is_audio_filename( path ) ) {
        return ZDJ_LIBRARY_IMPORT_TYPE_AUDIO_FILE;
    } else if( zdj_fs_path_is_external_database_filename( path ) ) {
        return ZDJ_LIBRARY_IMPORT_TYPE_LIBRARY_FILE;
    } else if( zdj_fs_path_is_external_database_dir( path ) ) {
        return ZDJ_LIBRARY_IMPORT_TYPE_LIBRARY_DIR;
    } else if( zdj_fs_path_is_audio_dir( path ) ) {
        return ZDJ_LIBRARY_IMPORT_TYPE_AUDIO_DIR;
    } else {
        return ZDJ_LIBRARY_IMPORT_TYPE_UNKNOWN;
    }
    
}

zdj_health_status_t zdj_library_new_import_db( void ) { 
    // Remove existing import db
    if( access(ZDJ_LIBRARY_IMPORT_DB_PATH, F_OK) == 0 ) {
        remove( ZDJ_LIBRARY_IMPORT_DB_PATH );
    }

    // Create new import db
    zdj_library_import_db = zdj_sql_open( ZDJ_LIBRARY_IMPORT_DB_PATH );
    if( !zdj_library_import_db ) { 
        printf( "failed to open zero import db\n" );
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR; 
    }
    return ZDJ_HEALTH_STATUS_OKAY;
}

zdj_health_status_t zdj_library_open_import_db( void ) { 
    // Open import db
    zdj_library_import_db = zdj_sql_open( ZDJ_LIBRARY_IMPORT_DB_PATH );
    if( !zdj_library_import_db ) { 
        printf( "failed to open zero import db\n" );
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR; 
    }
    return ZDJ_HEALTH_STATUS_OKAY;
}

zdj_health_status_t zdj_library_close_import_db( void ) {
    int res = zdj_sql_close( zdj_library_import_db );
    if( res ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    } else {
        return ZDJ_HEALTH_STATUS_OKAY;
    }
}
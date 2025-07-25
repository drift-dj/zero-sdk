#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <uuid.h>

#include <zerodj/fs/zdj_fs.h>
#include <zerodj/health/zdj_health_type.h>
#include <zerodj/library/zdj_library.h>
#include <zerodj/sql/zdj_sql.h>

sqlite3 * zdj_library_db;
sqlite3 * zdj_library_import_db;
zdj_library_config_t * library_config;
static char _sql[ 1024 ];

char * zdj_library_get_uuid( ) {
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

// zdj_health_status_t zdj_library_init( void ) {
    // Confirm lib db exits.
    // if( access( ZDJ_LIBRARY_DB_PATH, F_OK ) != 0 ) { return ZDJ_HEALTH_STATUS_MISSING_LIBRARY_DB; }

    // // Confirm lib db opens.
    // if( zdj_library_db_init( ) > ZDJ_HEALTH_STATUS_OKAY ) {
    //     return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    // }

    // return ZDJ_HEALTH_STATUS_OKAY;
// }

// zdj_health_status_t zdj_library_db_init( void ) {
    // zdj_library_db = zdj_sql_open( ZDJ_LIBRARY_DB_PATH );
    // if( !zdj_library_db ) { 
    //     printf( "failed to open zero db\n" );
    //     return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR; 
    // }
    // return ZDJ_HEALTH_STATUS_OKAY;

    
// }

zdj_health_status_t zdj_library_db_flush( void ) {
    sqlite3_db_cacheflush( zdj_library_db );
    return ZDJ_HEALTH_STATUS_OKAY;
}

zdj_health_status_t zdj_library_health( void ) {
    // Confirm lib db exits.
    if( access( ZDJ_LIBRARY_DB_PATH, F_OK ) != 0 ) { return ZDJ_HEALTH_STATUS_MISSING_LIBRARY_DB; }
}

zdj_health_status_t zdj_library_new_db( void ) {
    // Remove existing db
    if( access(ZDJ_LIBRARY_DB_PATH, F_OK) == 0 ) {
        printf( "removing lib db\n" );
        remove( ZDJ_LIBRARY_DB_PATH );
    }

    // Create new db
    zdj_library_db = zdj_sql_open( ZDJ_LIBRARY_DB_PATH );
    if( !zdj_library_db ) { 
        printf( "failed to open zero db\n" );
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR; 
    }

    // Create tables
    char sql[ 2048 ];
    // Audio
    strcpy( sql, "CREATE TABLE 'Audio_Data_Entity' ( 'entity_id' TEXT NOT NULL, 'data_source_entity_id' TEXT, 'filepath' TEXT, 'file_checksum' TEXT, 'has_procedural_edit' INT, 'procedural_edit_filepath' TEXT, 'av_codec_id' INT, 'av_stream_index' INT, 'av_sample_rate' INT, 'av_sample_format' INT, 'av_channel_count' INT, 'duration' REAL, 'error' INT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, zdj_library_db );
    // Catalog
    strcpy( sql, "CREATE TABLE 'Catalog_Data_Entity' ('entity_id' TEXT NOT NULL, 'data_source_entity_id' TEXT, 'title' TEXT, 'artist' TEXT, 'album' TEXT, 'label' TEXT, 'genre' TEXT, 'year' INT, 'error' INT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, zdj_library_db );
    // Curation
    strcpy( sql, "CREATE TABLE 'Curation_Data_Entity' ('entity_id' TEXT NOT NULL, 'data_source_entity_id' TEXT, 'tag_links' TEXT, 'playlist_links' TEXT, 'error' INT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, zdj_library_db );
    // Performance
    strcpy( sql, "CREATE TABLE 'Performance_Data_Entity' ('entity_id' TEXT NOT NULL, 'data_source_entity_id' TEXT, 'length_in_samples' INT, 'key' INT, 'bpm' REAL, 'has_beat_grid' INT, 'beat_grid_start_sample' INT, 'cuepoint_links' TEXT, 'error' INT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, zdj_library_db );
    // Song
    strcpy( sql, "CREATE TABLE 'Song_Entity' ('entity_id' TEXT NOT NULL, 'catalog_data_links' TEXT, 'catalog_data_entity_id' TEXT, 'performance_data_links' TEXT, 'performance_data_entity_id' TEXT, 'curation_data_links' TEXT, 'curation_data_entity_id' TEXT, 'audio_data_links' TEXT, 'audio_data_entity_id' TEXT, 'analysis_state' INT, 'analysis_progress' REAL, 'has_error' INT, 'error_flags' INT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, zdj_library_db );
    // Library
    strcpy( sql, "CREATE TABLE 'Library_Entity' ('entity_id' TEXT NOT NULL, 'name' TEXT, 'song_links' TEXT, 'playlist_links' TEXT, 'curation_data_links' TEXT, 'setting_links' TEXT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, zdj_library_db );
    // Library Config
    strcpy( sql, "CREATE TABLE 'Library_Config_Entity' ('entity_id' TEXT NOT NULL, 'entity_counter' INT, 'current_lib_entity_id' INT, PRIMARY KEY('entity_id'))" );
    // Setting
    strcpy( sql, "CREATE TABLE 'Setting_Entity' ('entity_id' TEXT NOT NULL, 'library_entity_id' TEXT, 'type' INT, 'i_val' INT, 'b_val' INT, 'f_val' REAL, 'c_val' TEXT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, zdj_library_db );

    return ZDJ_HEALTH_STATUS_OKAY;
}

zdj_health_status_t zdj_library_open_db( void ) {
    // Open import db
    zdj_library_db = zdj_sql_open( ZDJ_LIBRARY_DB_PATH );
    if( !zdj_library_db ) { 
        printf( "failed to open zero db\n" );
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR; 
    }
    return ZDJ_HEALTH_STATUS_OKAY;
}

zdj_health_status_t zdj_library_close_db( void ) {
    int res = zdj_sql_close( zdj_library_db );
    zdj_library_db = NULL;
    if( res ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    } else {
        return ZDJ_HEALTH_STATUS_OKAY;
    }
}

// Import db stuff
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
        printf( "removing import db\n" );
        remove( ZDJ_LIBRARY_IMPORT_DB_PATH );
    }

    // Create new import db
    zdj_library_import_db = zdj_sql_open( ZDJ_LIBRARY_IMPORT_DB_PATH );
    if( !zdj_library_import_db ) { 
        printf( "failed to open zero import db\n" );
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR; 
    }

    // Create tables
    char sql[ 2048 ];
    // Audio
    strcpy( sql, "CREATE TABLE 'Audio_Data_Entity' ( 'entity_id' TEXT NOT NULL, 'data_source_entity_id' TEXT, 'filepath' TEXT, 'file_checksum' TEXT, 'has_procedural_edit' INT, 'procedural_edit_filepath' TEXT, 'av_codec_id' INT, 'av_stream_index' INT, 'av_sample_rate' INT, 'av_sample_format' INT, 'av_channel_count' INT, 'duration' REAL, 'error' INT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, zdj_library_import_db );
    // Catalog
    strcpy( sql, "CREATE TABLE 'Catalog_Data_Entity' ('entity_id' TEXT NOT NULL, 'data_source_entity_id' TEXT, 'title' TEXT, 'artist' TEXT, 'album' TEXT, 'label' TEXT, 'genre' TEXT, 'year' INT, 'error' INT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, zdj_library_import_db );
    // Curation
    strcpy( sql, "CREATE TABLE 'Curation_Data_Entity' ('entity_id' TEXT NOT NULL, 'data_source_entity_id' TEXT, 'tag_links' TEXT, 'playlist_links' TEXT, 'error' INT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, zdj_library_import_db );
    // Performance
    strcpy( sql, "CREATE TABLE 'Performance_Data_Entity' ('entity_id' TEXT NOT NULL, 'data_source_entity_id' TEXT, 'length_in_samples' INT, 'key' INT, 'bpm' REAL, 'has_beat_grid' INT, 'beat_grid_start_sample' INT, 'cuepoint_links' TEXT, 'error' INT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, zdj_library_import_db );
    // Song
    strcpy( sql, "CREATE TABLE 'Song_Entity' ('entity_id' TEXT NOT NULL, 'catalog_data_links' TEXT, 'catalog_data_entity_id' TEXT, 'performance_data_links' TEXT, 'performance_data_entity_id' TEXT, 'curation_data_links' TEXT, 'curation_data_entity_id' TEXT, 'audio_data_links' TEXT, 'audio_data_entity_id' TEXT, 'analysis_state' INT, 'analysis_progress' REAL, 'has_error' INT, 'error_flags' INT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, zdj_library_import_db );

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
    zdj_library_import_db = NULL;
    if( res ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    } else {
        return ZDJ_HEALTH_STATUS_OKAY;
    }
}
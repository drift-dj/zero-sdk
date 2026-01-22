#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <uuid.h>

#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/health/zdj_health_type.h>
#include <zerodj/library/zdj_library.h>
#include <zerodj/system/sql/zdj_sql.h>


sqlite3 * zdj_library_db;
sqlite3 * zdj_library_import_db;
zdj_library_config_t * library_config;
static char _sql[ 1024 ];

static void _zdj_library_create_db_tables( sqlite3 * db );

char * zdj_library_get_uuid( ) {
    uuid_t uuid;
    uuid_generate( uuid );
    char uuid_str[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
    char uuid_str_no_dash[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
    uuid_unparse_lower( uuid, uuid_str );
    int n = 0;
    for( int i=0; i<ZDJ_LIBRARY_ENTITY_ID_LEN; i++ ) {
        if( uuid_str[ i ] != '-' ) {
            uuid_str_no_dash[ n ] = uuid_str[ i ];
            n++;
        }
    }
    return strdup( uuid_str_no_dash );
}

void zdj_library_put_uuid( char * str ) {
    uuid_t uuid;
    uuid_generate( uuid );
    char uuid_str[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
    char uuid_str_no_dash[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
    uuid_unparse_lower( uuid, uuid_str );
    int n = 0;
    for( int i=0; i<ZDJ_LIBRARY_ENTITY_ID_LEN; i++ ) {
        if( uuid_str[ i ] != '-' ) {
            uuid_str_no_dash[ n ] = uuid_str[ i ];
            n++;
        }
    }
    strcpy( str, uuid_str_no_dash );
}

zdj_health_status_t zdj_import_db_flush( void ) {
    if( zdj_library_import_db ) {
        sqlite3_db_cacheflush( zdj_library_import_db );
        return ZDJ_HEALTH_STATUS_OKAY;
    }
}

zdj_health_status_t zdj_library_db_flush( void ) {
    if( zdj_library_db ) {
        sqlite3_db_cacheflush( zdj_library_db );
        return ZDJ_HEALTH_STATUS_OKAY;
    }
}

zdj_health_status_t zdj_library_health( void ) {
    // Confirm lib db exits.
    if( access( ZDJ_LIBRARY_DB_PATH, F_OK ) != 0 ) { return ZDJ_HEALTH_STATUS_MISSING_LIBRARY_DB; }
}

static void _zdj_library_create_db_tables( sqlite3 * db ) {
    // Create tables
    char sql[ 2048 ];
    // Audio
    strcpy( sql, "CREATE TABLE 'Audio_Data_Entity' ( 'entity_id' TEXT NOT NULL, 'song_entity_id' TEXT, 'data_source_entity_id' TEXT, 'filepath' TEXT, 'file_checksum' TEXT, 'has_procedural_edit' INT, 'procedural_edit_filepath' TEXT, 'av_codec_id' INT, 'av_stream_index' INT, 'av_sample_rate' INT, 'av_sample_format' INT, 'av_channel_count' INT, 'duration_sec' REAL, 'duration_pcm' INT, 'timebase' REAL, 'error' INT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, db );
    // Catalog
    strcpy( sql, "CREATE TABLE 'Catalog_Data_Entity' ('entity_id' TEXT NOT NULL, 'song_entity_id' TEXT, 'data_source_entity_id' TEXT, 'title' TEXT, 'artist' TEXT, 'album' TEXT, 'label' TEXT, 'genre' TEXT, 'year' INT, 'error' INT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, db );
    // Curation
    strcpy( sql, "CREATE TABLE 'Curation_Data_Entity' ('entity_id' TEXT NOT NULL, 'parent_entity_id' TEXT, 'data_source_entity_id' TEXT, 'tag_links' TEXT, 'playlist_links' TEXT, 'error' INT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, db );
    // Performance
    strcpy( sql, "CREATE TABLE 'Performance_Data_Entity' ('entity_id' TEXT NOT NULL, 'song_entity_id' TEXT, 'data_source_entity_id' TEXT, 'length_in_samples' INT, 'key' INT, 'bpm' REAL, 'has_beat_grid' INT, 'beat_grid_start_sample' INT, 'cuepoint_links' TEXT, 'error' INT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, db );
    // Song
    strcpy( sql, "CREATE TABLE 'Song_Entity' ('entity_id' TEXT NOT NULL, 'catalog_data_entity_id' TEXT, 'performance_data_entity_id' TEXT, 'curation_data_entity_id' TEXT, 'audio_data_entity_id' TEXT, 'analysis_state' INT, 'analysis_progress' REAL, 'has_error' INT, 'error_flags' INT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, db );
    // Library
    strcpy( sql, "CREATE TABLE 'Library_Entity' ('entity_id' TEXT NOT NULL, 'name' TEXT, 'song_links' TEXT, 'playlist_links' TEXT, 'curation_data_links' TEXT, 'setting_links' TEXT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, db );
    // Library Config
    strcpy( sql, "CREATE TABLE 'Library_Config_Entity' ('entity_id' TEXT NOT NULL, 'entity_counter' INT, 'current_lib_entity_id' TEXT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, db );
    // Setting
    strcpy( sql, "CREATE TABLE 'Setting_Entity' ('entity_id' TEXT NOT NULL, 'library_entity_id' TEXT, 'type' INT, 'i_val' INT, 'b_val' INT, 'f_val' REAL, 'c_val' TEXT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, db );
    // Data Source
    strcpy( sql, "CREATE TABLE 'Data_Source_Entity' ('entity_id' TEXT NOT NULL, 'name' TEXT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, db );
    // Cuepoints
    strcpy( sql, "CREATE TABLE 'Cuepoint_Entity' ('entity_id' TEXT NOT NULL, 'performance_entity_id' TEXT, 'name' TEXT, 'sample' INT, 'is_loop' INT, 'loop_len' INT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, db );
    // Playlists
    strcpy( sql, "CREATE TABLE 'Playlist_Entity' ('entity_id' TEXT NOT NULL, 'name' TEXT, 'ordered_song_links' TEXT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, db );
    // Tags
    strcpy( sql, "CREATE TABLE 'Tag_Entity' ('entity_id' TEXT NOT NULL, 'name' TEXT, PRIMARY KEY('entity_id'))" );
    zdj_sql_exec( (char*)&sql, db );
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

void zdj_library_reset_db( void ) {
    // printf( "zdj_library_reset_db\n" );
    // Remove existing db
    remove( ZDJ_LIBRARY_DB_PATH );

    // Remove config ref
    if( library_config ) { free( library_config ); }
    library_config = NULL;

    // Create new db
    zdj_library_db = zdj_sql_open( ZDJ_LIBRARY_DB_PATH );
    if( !zdj_library_db ) { 
        printf( "failed to open zero db\n" );
        return; 
    }

    // Setup Tables
    _zdj_library_create_db_tables( zdj_library_db );

    // Create a new Library Config record
    zdj_library_get_config( );

    // Create a new Library record + store in config
    zdj_library_new( );

    // Create the default Data Source records
    zdj_library_create_default_data_sources( );

    // Create a new screenshot counter
    zdj_library_set_int_setting( 
        zdj_library_config_get_current_library_id( ), ZDJ_LIBRARY_SETTING_SCREENSHOT_COUNTER, 0
    );

    // Flush cache to disk so everything's in sync.
    zdj_library_db_flush( );
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
        remove( ZDJ_LIBRARY_IMPORT_DB_PATH );
    }

    // Create new import db
    zdj_library_import_db = zdj_sql_open( ZDJ_LIBRARY_IMPORT_DB_PATH );
    if( !zdj_library_import_db ) { 
        printf( "failed to open zero import db\n" );
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR; 
    }

    _zdj_library_create_db_tables( zdj_library_import_db );

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
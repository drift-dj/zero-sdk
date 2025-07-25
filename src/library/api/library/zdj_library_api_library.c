#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sqlite3.h>

#include <zerodj/health/zdj_health_type.h>
#include <zerodj/library/zdj_library.h>
#include <zerodj/system/sql/zdj_sql.h>

static char _sql[ 1024 ];

int zdj_library_count_libraries( void ) {
    return zdj_sql_rows_in_table ( ZDJ_LIBRARY_TABLE_LIBRARY, NULL, zdj_library_db );
}

// Populate an array of pointers to each library in Library_Entity table
void zdj_library_get_all_libraries( 
    zdj_library_t ** libraries, 
    int result_limit 
) {
    int row = 0;
    int sql_res;
    sprintf( _sql, "select * from %s", ZDJ_LIBRARY_TABLE_LIBRARY );
    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( _sql, zdj_library_db );
    if( stmt ) {
        while ( (( sql_res = sqlite3_step( stmt ) ) == SQLITE_ROW) && row < result_limit ) {
            zdj_library_t * lib = calloc( 1, sizeof( zdj_library_t ) );
            lib->entity_id = strdup( (char*)sqlite3_column_text ( stmt, 0 ) );
            lib->name = strdup( (char*)sqlite3_column_text ( stmt, 1 ) );
            lib->song_links = strdup( (char*)sqlite3_column_text ( stmt, 2 ) );
            lib->curation_data_links = strdup( (char*)sqlite3_column_text ( stmt, 3 ) );
            lib->setting_links = strdup( (char*)sqlite3_column_text ( stmt, 4 ) );
            libraries[ row ] = lib;
            row++;
        }
        sqlite3_finalize( stmt );
    }
}

zdj_library_t * zdj_library_get_library_for_entity_id( char * library_entity_id ) {
    int sql_res;
    zdj_library_t * lib = NULL;
    sprintf( _sql, "select * from %s where entity_id like \'%s\'", ZDJ_LIBRARY_TABLE_LIBRARY, library_entity_id );
    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( _sql, zdj_library_db );
    if( stmt ) {
        while ( ( sql_res = sqlite3_step( stmt ) ) == SQLITE_ROW ) {
            lib = calloc( 1, sizeof( zdj_library_t ) );
            lib->entity_id = strdup( (char*)sqlite3_column_text ( stmt, 0 ) );
            lib->name = strdup( (char*)sqlite3_column_text ( stmt, 1 ) );
            lib->song_links = strdup( (char*)sqlite3_column_text ( stmt, 2 ) );
            lib->curation_data_links = strdup( (char*)sqlite3_column_text ( stmt, 3 ) );
            lib->setting_links = strdup( (char*)sqlite3_column_text ( stmt, 4 ) );
        }
        sqlite3_finalize( stmt );
    }
    return lib;
}


int zdj_library_set_current( int library_entity_id ) {
    return 0;
}

bool zdj_library_is_current( zdj_library_t * library ) {
    return true;
}

zdj_health_status_t zdj_library_new( void ) {
    // printf( "zdj_library_new\n" );
    // Insert a new Library_Entity record
    int lib_count = zdj_sql_rows_in_table ( "Library_Entity", NULL, zdj_library_db );
    char * new_lib_entity_id = zdj_library_get_uuid( );
    snprintf( _sql, sizeof( _sql ), "INSERT INTO Library_Entity VALUES(\"%s\", \"Library %d\", \"Song_Links_%s\", \"Playlist_Links_%s\", \"Curation_Data_Links_%s\", \"Setting_Links_%s\");\n",
        new_lib_entity_id,
        lib_count,
        new_lib_entity_id,
        new_lib_entity_id,
        new_lib_entity_id,
        new_lib_entity_id
    );

    zdj_sql_exec( (char *)&_sql, zdj_library_db );

    // Add a Song_Links table
    snprintf( _sql, sizeof( _sql ), "CREATE TABLE 'Song_Links_%s' ( 'entity_id' TEXT NOT NULL, PRIMARY KEY('entity_id'))",
        new_lib_entity_id 
    );
    zdj_sql_exec( (char*)&_sql, zdj_library_db );

    // Set current lib to the new lib
    zdj_library_config_set_current_library_id( new_lib_entity_id );

    // Make Settings for new lib
    zdj_library_set_bool_setting( new_lib_entity_id, ZDJ_LIBRARY_SETTING_SHOW_ARTISTS_MENU, true );
    zdj_library_set_bool_setting( new_lib_entity_id, ZDJ_LIBRARY_SETTING_SHOW_GENRES_MENU, true );
    zdj_library_set_bool_setting( new_lib_entity_id, ZDJ_LIBRARY_SETTING_SHOW_YEARS_MENU, false );
    zdj_library_set_bool_setting( new_lib_entity_id, ZDJ_LIBRARY_SETTING_SHOW_BPM_MENU, true );

    free( new_lib_entity_id );

    // Flush the db
    zdj_library_db_flush( );

    return ZDJ_HEALTH_STATUS_OKAY;
}

zdj_health_status_t zdj_library_duplicate_library( int library_entity_id ) {
    // Copy song links table
    // Copy curation data links table
    // copy setting links table
    return ZDJ_HEALTH_STATUS_OKAY;
}

zdj_health_status_t zdj_library_remove_library( int library_entity_id ) {
    return ZDJ_HEALTH_STATUS_OKAY;
}

zdj_health_status_t zdj_library_rename_library( int library_entity_id, char * name ) {
    return ZDJ_HEALTH_STATUS_OKAY;
}

zdj_health_status_t zdj_library_add_song_link( char * library_entity_id, zdj_library_song_t * song, sqlite3 * db ) {
    // Build insert based on links table name in dto
    int count = 0;
    int res;
    char sql[ 4096 ];
    // Set up for prepared stmt w/binds to use built-in string escaping.
    snprintf( sql, sizeof( sql ), 
        // Insert new record
        "INSERT INTO Song_Links_%s VALUES('%s')\n"

        // Or update existing record
        "ON CONFLICT(entity_id) DO UPDATE SET entity_id='%s'",

        // Table Name
        library_entity_id,
        song->entity_id,
        song->entity_id
    );
    res = zdj_sql_exec( sql, db );

    if( res != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR; 
    } else {
        return ZDJ_HEALTH_STATUS_OKAY;
    }
}

void zdj_library_deinit_library( zdj_library_t * library ) {
    free( library->name );
    free( library->song_links );
    free( library->curation_data_links );
    free( library->setting_links );
}
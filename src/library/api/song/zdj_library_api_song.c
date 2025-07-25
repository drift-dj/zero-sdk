#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sqlite3.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>

#include <zerodj/health/zdj_health_type.h>
#include <zerodj/library/zdj_library.h>
#include <zerodj/sql/zdj_sql.h>


zdj_library_song_t * zdj_library_create_song_dto( void ) {
    zdj_library_song_t * song = calloc( 1, sizeof( zdj_library_song_t ) );
    song->entity_id = zdj_library_get_uuid( );
    return song;
}

zdj_library_song_t * zdj_library_fetch_song_dto_for_entity_id( 
    char * entity_id, 
    sqlite3 * db 
) {
    int res;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "select * from %s where entity_id like \'%s\'", 
        ZDJ_LIBRARY_TABLE_SONG,
        entity_id
    );

    int _eid_col = 0;
    int _clt_col = 1;
    int _cceid_col = 2;
    int _plt_col = 3;
    int _cpeid_col = 4;
    int _cult_col = 5;
    int _ccueid_col = 6;
    int _alt_col = 7;
    int _caeid_col = 8;
    int _as_col = 9;
    int _ap_col = 10;
    zdj_library_song_t * song = NULL;

    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
    if( stmt ) {
        while ( ( res = sqlite3_step( stmt ) ) == SQLITE_ROW ) { 
            song = calloc( 1, sizeof( zdj_library_song_t ) );
            song->entity_id = strdup( (char*)sqlite3_column_text ( stmt, _eid_col ) );
            song->catalog_links_table = strdup( (char*)sqlite3_column_text ( stmt, _clt_col ) );
            song->current_catalog_entity_id = strdup( (char*)sqlite3_column_text ( stmt, _cceid_col ) );
            song->performance_links_table = strdup( (char*)sqlite3_column_text ( stmt, _plt_col ) );
            song->current_performance_entity_id = strdup( (char*)sqlite3_column_text ( stmt, _cpeid_col ) );
            song->curation_links_table = strdup( (char*)sqlite3_column_text ( stmt, _cult_col ) );
            song->current_curation_entity_id = strdup( (char*)sqlite3_column_text ( stmt, _ccueid_col ) );
            song->audio_links_table = strdup( (char*)sqlite3_column_text ( stmt, _alt_col ) );
            song->current_audio_entity_id = strdup( (char*)sqlite3_column_text ( stmt, _caeid_col ) );
            song->analysis_state = sqlite3_column_int ( stmt, _as_col );
            song->analysis_progress = sqlite3_column_double ( stmt, _ap_col );
        }
        sqlite3_finalize( stmt );
    }

    return song;
}

zdj_health_status_t zdj_library_free_song_dto( 
    zdj_library_song_t * song 
) {

}

zdj_health_status_t zdj_library_store_song( 
    zdj_library_song_t * song, 
    sqlite3 * db 
) {
    int count = 0;
    int res;
    char sql[ 4096 ];
    snprintf( sql, sizeof( sql ), 
        // Insert new record
        "INSERT INTO %s(entity_id,catalog_data_links,catalog_data_entity_id,performance_data_links,performance_data_entity_id,curation_data_links,curation_data_entity_id,audio_data_links,audio_data_entity_id,analysis_state,analysis_progress,has_error,error_flags) VALUES('%s','%s','%s','%s','%s','%s','%s','%s','%s',%d,%f,%d,%d)\n"
        // Or update existing record
        "ON CONFLICT(entity_id) DO UPDATE SET entity_id='%s',catalog_data_links='%s',catalog_data_entity_id='%s',performance_data_links='%s',performance_data_entity_id='%s',curation_data_links='%s',curation_data_entity_id='%s',audio_data_links='%s',audio_data_entity_id='%s',analysis_state=%d,analysis_progress=%f,has_error=%d,error_flags=%d",

        // Table name
        ZDJ_LIBRARY_TABLE_SONG,

        // Insert new record
        song->entity_id,
        song->catalog_links_table,
        song->current_catalog_entity_id,
        song->performance_links_table,
        song->current_performance_entity_id,
        song->curation_links_table,
        song->current_curation_entity_id,
        song->audio_links_table,
        song->current_audio_entity_id,
        song->analysis_state,
        song->analysis_progress,
        song->has_error,
        song->error_flags,

        // Update existing record
        song->entity_id,
        song->catalog_links_table,
        song->current_catalog_entity_id,
        song->performance_links_table,
        song->current_performance_entity_id,
        song->curation_links_table,
        song->current_curation_entity_id,
        song->audio_links_table,
        song->current_audio_entity_id,
        song->analysis_state,
        song->analysis_progress,
        song->has_error,
        song->error_flags
    );
    zdj_sql_exec( (char*)&sql, db );
    
    return ZDJ_HEALTH_STATUS_OKAY;
}

int zdj_library_count_songs_in_library( char * library_entity_id, sqlite3 * db ) {
    int res;
    int count = 0;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "select entity_id from %s", ZDJ_LIBRARY_TABLE_SONG );
    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
    if( stmt ) {
        while ( ( res = sqlite3_step( stmt ) ) == SQLITE_ROW ) { 
            count++;
        }
        sqlite3_finalize( stmt );
    }
    return count;
}

int zdj_library_count_songs_in_db( sqlite3 * db ) {
    int res;
    int count = 0;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "select entity_id from %s", ZDJ_LIBRARY_TABLE_SONG );
    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
    if( stmt ) {
        while ( ( res = sqlite3_step( stmt ) ) == SQLITE_ROW ) { 
            count++;
        }
        sqlite3_finalize( stmt );
    }
    return count;
}

// Alloc/populate an array with all entity_ids in db
zdj_health_status_t zdj_library_fetch_song_entity_ids( 
    char ** arr,
    int count,
    sqlite3 * db 
) {
    int res;
    int row = 0;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "select entity_id from %s", ZDJ_LIBRARY_TABLE_SONG );
    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
    if( stmt ) {
        while ( ( res = sqlite3_step( stmt ) ) == SQLITE_ROW ) { 
            if( row < count ) {
                arr[ row ] = strdup( (char*)sqlite3_column_text ( stmt, 0 ) );
            }
            row++;
        }
        sqlite3_finalize( stmt );
    }
}

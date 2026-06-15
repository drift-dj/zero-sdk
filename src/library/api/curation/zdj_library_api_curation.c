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
#include <zerodj/system/sql/zdj_sql.h>

zdj_library_curation_t * zdj_library_create_curation_dto( void ) {
    zdj_library_curation_t * curation = calloc( 1, sizeof( zdj_library_curation_t ) );
    zdj_library_put_uuid( curation->entity_id );
    return curation;
}

zdj_library_curation_t * zdj_library_fetch_current_curation_dto_for_song( 
    zdj_library_song_t * song, 
    sqlite3 * db 
) {
    int res;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "select * from %s where entity_id like \'%s\'", 
        ZDJ_LIBRARY_TABLE_CURATION_DATA,
        song->current_curation_entity_id
    );

    int _eid_col = 0;
    int _dseid_col = 1;
    int _tl_col = 2;
    int _pl_col = 3;
    zdj_library_curation_t * curation = NULL;

    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
    if( stmt ) {
        while ( ( res = sqlite3_step( stmt ) ) == SQLITE_ROW ) { 
            curation = calloc( 1, sizeof( zdj_library_curation_t ) );
            strcpy( curation->entity_id, (char*)sqlite3_column_text ( stmt, _eid_col ) );
            
            char * data_source_entity_id = (char*)sqlite3_column_text ( stmt, _dseid_col );
            if( data_source_entity_id ) { strcpy( curation->data_source_entity_id, data_source_entity_id ); }

            char * tags_links_table = (char*)sqlite3_column_text ( stmt, _tl_col );
            if( tags_links_table ) { strcpy( curation->tags_links_table, tags_links_table ); }

            char * playlists_links_table = (char*)sqlite3_column_text ( stmt, _pl_col );
            if( playlists_links_table ) { strcpy( curation->playlists_links_table, playlists_links_table ); }
        }
        sqlite3_finalize( stmt );
    }

    return curation;
}

// WARNING - this is VERY expensive - we query every playlist in the library.
zdj_error_state_t zdj_library_populate_playlist_eids_for_song(
    zdj_library_song_t * song,
    sqlite3 * db 
) {
    // Build a count of all playlist links tables containing row w/song's EID.
    // Allocate array of storage on the song's curation DTO's playlist_eids field.
    // Add a playlist EID to the array for every playlist links table containing a row with this song's EID.

}

zdj_health_status_t zdj_library_free_curation_dto( 
    zdj_library_curation_t * curation 
) {

}

zdj_health_status_t zdj_library_store_curation( 
    zdj_library_curation_t * curation, 
    sqlite3 * db 
) {
    int count = 0;
    int res;
    char sql[ 4096 ];
    snprintf( sql, sizeof( sql ), 
        // Insert new record
        "INSERT INTO %s(entity_id,data_source_entity_id,tag_links,playlist_links,error) VALUES(\'%s\',\'%s\',\'%s\',\'%s\',%d)\n"

        // Or update existing record
        "ON CONFLICT(entity_id) DO UPDATE SET entity_id=\'%s\',data_source_entity_id=\'%s\',tag_links=\'%s\',playlist_links=\'%s\',error=%d",

        // Table name
        ZDJ_LIBRARY_TABLE_CURATION_DATA,

        // Insert new record
        curation->entity_id,
        curation->data_source_entity_id,
        curation->tags_links_table,
        curation->playlists_links_table,
        curation->error,

        // Update existing record
        curation->entity_id,
        curation->data_source_entity_id,
        curation->tags_links_table,
        curation->playlists_links_table,
        curation->error
    );
    zdj_sql_exec( sql, db );
    
    zdj_sql_db_flush( db );
    
    return ZDJ_HEALTH_STATUS_OKAY;
}

zdj_health_status_t zdj_library_delete_curation( 
    zdj_library_curation_t * curation, 
    sqlite3 * db 
) {
    int res;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "delete from %s where entity_id like \'%s\'", 
        ZDJ_LIBRARY_TABLE_CURATION_DATA,
        curation->entity_id
    );
    zdj_sql_exec( sql, db );
    return ZDJ_HEALTH_STATUS_OKAY;
}
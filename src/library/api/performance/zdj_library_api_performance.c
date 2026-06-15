#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sqlite3.h>

#include <zerodj/health/zdj_health_type.h>
#include <zerodj/library/zdj_library.h>
#include <zerodj/system/sql/zdj_sql.h>

zdj_library_performance_t * zdj_library_create_performance_dto( void ) {
    zdj_library_performance_t * performance = calloc( 1, sizeof( zdj_library_performance_t ) );
    zdj_library_put_uuid( performance->entity_id );

    // Make cuepoint_links table
    return performance;
}

zdj_health_status_t zdj_library_delete_performance( 
    zdj_library_performance_t * performance, 
    sqlite3 * db 
) {
    int res;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "delete from %s where entity_id like \'%s\'", 
        ZDJ_LIBRARY_TABLE_PERFORMANCE_DATA,
        performance->entity_id
    );
    zdj_sql_exec( sql, db );
    return ZDJ_HEALTH_STATUS_OKAY;
}

zdj_library_performance_t * zdj_library_fetch_current_performance_dto_for_song( 
    zdj_library_song_t * song, 
    sqlite3 * db 
) {
    int res;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "select * from %s where entity_id like \'%s\'", 
        ZDJ_LIBRARY_TABLE_PERFORMANCE_DATA,
        song->current_performance_entity_id
    );

    int _eid_col = 0;
    int _sid_col = 1;
    int _dseid_col = 2;
    int _lis_col = 3;
    int _k_col = 4;
    int _bpm_col = 5;
    int _hbg_col = 6;
    int _bgs_col = 7;
    int _cl_col = 8;
    zdj_library_performance_t * performance = NULL;

    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
    if( stmt ) {
        while ( ( res = sqlite3_step( stmt ) ) == SQLITE_ROW ) { 
            performance = calloc( 1, sizeof( zdj_library_performance_t ) );
            strcpy( performance->entity_id, (char*)sqlite3_column_text ( stmt, _eid_col ) );
            
            char * song_entity_id = (char*)sqlite3_column_text ( stmt, _sid_col );
            if( song_entity_id ) { strcpy( performance->song_entity_id, song_entity_id ); }

            char * data_source_entity_id = (char*)sqlite3_column_text ( stmt, _dseid_col );
            if( data_source_entity_id ) { strcpy( performance->data_source_entity_id, data_source_entity_id ); }

            performance->sample_length = sqlite3_column_int ( stmt, _lis_col );
            performance->key = sqlite3_column_int ( stmt, _k_col );
            performance->bpm = sqlite3_column_double ( stmt, _bpm_col );
            performance->has_beat_grid = sqlite3_column_int ( stmt, _hbg_col );
            performance->beat_grid_start_sample = sqlite3_column_int ( stmt, _bgs_col );

            char * cuepoints_links_table = (char*)sqlite3_column_text ( stmt, _cl_col );
            strcpy( performance->cuepoints_links_table, cuepoints_links_table );
            performance->cuepoint_count = 0;
            
        }
        sqlite3_finalize( stmt );
    }

    return performance;
}

zdj_health_status_t zdj_library_free_performance_dto( 
    zdj_library_performance_t * performance 
) {

}

zdj_health_status_t zdj_library_store_performance( 
    zdj_library_performance_t * performance, 
    sqlite3 * db 
) {
    int count = 0;
    int res;
    char sql[ 4096 ];
    snprintf( sql, sizeof( sql ), 
        // Insert new record
        "INSERT INTO %s(entity_id,song_entity_id,data_source_entity_id,length_in_samples,key,bpm,has_beat_grid,beat_grid_start_sample,cuepoint_links,error) VALUES(\'%s\',\'%s\',\'%s\',%d,%d,%f,%d,%d,\'%s\',%d)\n"

        // Or update existing record
        "ON CONFLICT(entity_id) DO UPDATE SET entity_id=\'%s\',song_entity_id=\'%s\',data_source_entity_id=\'%s\',length_in_samples=%d,key=%d,bpm=%f,has_beat_grid=%d,beat_grid_start_sample=%d,cuepoint_links=\'%s\',error=%d",

        // Table name
        ZDJ_LIBRARY_TABLE_PERFORMANCE_DATA,
 
        // Insert new record
        performance->entity_id,
        performance->song_entity_id,
        performance->data_source_entity_id,
        performance->sample_length,
        performance->key,
        performance->bpm,
        performance->has_beat_grid,
        performance->beat_grid_start_sample,
        performance->cuepoints_links_table,
        performance->error,

        // Update existing record
        performance->entity_id,
        performance->song_entity_id,
        performance->data_source_entity_id,
        performance->sample_length,
        performance->key,
        performance->bpm,
        performance->has_beat_grid,
        performance->beat_grid_start_sample,
        performance->cuepoints_links_table,
        performance->error
    );
    zdj_sql_exec( sql, db );
    
    zdj_sql_db_flush( db );
    
    return ZDJ_HEALTH_STATUS_OKAY;
}
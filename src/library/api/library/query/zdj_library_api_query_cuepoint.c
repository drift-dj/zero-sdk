#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sqlite3.h>

#include <zerodj/health/zdj_health_type.h>
#include <zerodj/library/zdj_library.h>
#include <zerodj/system/sql/zdj_sql.h>

// static char _sql[ 1024 ];

int zdj_library_query_count_cuepoints_for_song( zdj_library_song_t * song, sqlite3 * db ) {
    // printf( "counting cuepoints for perf_eid: %s\n", song->performance->entity_id );
    int res;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "select count(*) from %s where performance_entity_id like \'%s\'", 
        ZDJ_LIBRARY_TABLE_CUEPOINT,
        song->performance->entity_id
    );

    int count = 0;
    sqlite3_stmt * c_stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
    if( c_stmt ) {
        while ( ( res = sqlite3_step( c_stmt ) ) == SQLITE_ROW ) {
            count = sqlite3_column_int ( c_stmt, 0 );
        }
        sqlite3_finalize( c_stmt );
    }
    return count;
}

zdj_error_type_t zdj_library_query_cuepoints_for_song( 
    zdj_library_song_t * song, 
    char ** cuepoints, 
    int count, 
    sqlite3 * db 
) {
    int res;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "select entity_id from %s where performance_entity_id like \'%s\'", 
        ZDJ_LIBRARY_TABLE_CUEPOINT,
        song->performance->entity_id
    );

    int row = 0;
    sqlite3_stmt * c_stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
    if( c_stmt ) {
        while ( ( res = sqlite3_step( c_stmt ) ) == SQLITE_ROW ) {
            if( row < count ) {
                cuepoints[ row ] = strdup( (char*)sqlite3_column_text ( c_stmt, 0 ) );
            }
            row++;
        }
        sqlite3_finalize( c_stmt );
    }
    
    return ZDJ_ERROR_LIBRARY_QUERY_OKAY;
}
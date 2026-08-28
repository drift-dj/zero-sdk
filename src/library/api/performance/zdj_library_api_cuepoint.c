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

zdj_library_cuepoint_t * zdj_library_create_cuepoint_dto( void ) {
    zdj_library_cuepoint_t * cuepoint = calloc( 1, sizeof( zdj_library_cuepoint_t ) );
    zdj_library_put_uuid( cuepoint->entity_id );
    return cuepoint;
}

zdj_library_cuepoint_t * zdj_library_fetch_cuepoint_dto_for_entity_id( char * entity_id, sqlite3 * db ) {
    // printf( "zdj_library_fetch_cuepoint_dto_for_entity_id: %s\n", entity_id );
    int res;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "select * from %s where entity_id like \'%s\' order by num", 
        ZDJ_LIBRARY_TABLE_CUEPOINT,
        entity_id
    );

    int col_count = 0;
    int _eid_col = col_count;
    int _peid_col = ++col_count;
    int _n_col = ++col_count;
    int _nm_col = ++col_count;
    int _s_col = ++col_count;
    int _il_col = ++col_count;
    int _ll_col = ++col_count;
    zdj_library_cuepoint_t * cuepoint = NULL;


    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
    if( stmt ) {
        while ( ( res = sqlite3_step( stmt ) ) == SQLITE_ROW ) { 
            cuepoint = calloc( 1, sizeof( zdj_library_cuepoint_t ) );
            
            strcpy( cuepoint->entity_id, (char*)sqlite3_column_text ( stmt, _eid_col ) );
            
            char * performance_entity_id = (char*)sqlite3_column_text ( stmt, _peid_col );
            if( performance_entity_id ) { strcpy( cuepoint->performance_entity_id, performance_entity_id ); }

            char * name = (char*)sqlite3_column_text ( stmt, _n_col );
            if( name ) { strcpy( cuepoint->name, name ); }

            cuepoint->num = sqlite3_column_int ( stmt, _nm_col );
            if( strlen( name ) < 1 ) { sprintf( cuepoint->name, "%d", cuepoint->num+1 ); }

            cuepoint->sample = sqlite3_column_int ( stmt, _s_col );
            cuepoint->is_loop = sqlite3_column_int ( stmt, _il_col );
            cuepoint->loop_len = sqlite3_column_int ( stmt, _ll_col );
        }
        sqlite3_finalize( stmt );
    }

    return cuepoint;
}

void zdj_library_free_cuepoint_dto( zdj_library_cuepoint_t * cuepoint ) {
    free( cuepoint );
}

zdj_error_type_t zdj_library_store_cuepoint( zdj_library_cuepoint_t * cuepoint, sqlite3 * db ) {
    int count = 0;
    int res;
    char sql[ 4096 ];

    // printf( "zdj_library_store_cuepoint: %p - %s, %s, %s, %d \n", 
    //     cuepoint,
    //     cuepoint->entity_id,
    //     cuepoint->performance_entity_id,
    //     cuepoint->name,
    //     cuepoint->num
    // );

    snprintf( sql, sizeof( sql ), 
        // Insert new record
        "INSERT INTO %s(entity_id,performance_entity_id,name,num,sample,is_loop,loop_len) VALUES(\'%s\',\'%s\',\'%s\',%d,%ld,%d,%ld)\n"

        // Or update existing record
        "ON CONFLICT(entity_id) DO UPDATE SET entity_id=\'%s\',performance_entity_id=\'%s\',name=\'%s\',num=%d,sample=%ld,is_loop=%d,loop_len=%ld",

        // Table name
        ZDJ_LIBRARY_TABLE_CUEPOINT,
 
        // Insert new record
        cuepoint->entity_id,
        cuepoint->performance_entity_id,
        cuepoint->name,
        cuepoint->num,
        cuepoint->sample,
        cuepoint->is_loop,
        cuepoint->loop_len,

        // Update existing record
        cuepoint->entity_id,
        cuepoint->performance_entity_id,
        cuepoint->name,
        cuepoint->num,
        cuepoint->sample,
        cuepoint->is_loop,
        cuepoint->loop_len
    );
    zdj_sql_exec( sql, db );
    
    // printf( "zdj_library_store_cuepoint done\n" );

    return ZDJ_ERROR_OKAY;
}

void zdj_library_remove_cuepoints_for_performance_eid( char * perf_eid, sqlite3 * db ) {
    snprintf( _sql, sizeof( _sql ), 
        "DELETE FROM Cuepoint_Entity WHERE performance_entity_id=\'%s\';",
        perf_eid
    );
    zdj_sql_exec( _sql, zdj_library_db );
}

void zdj_library_remove_cuepoint( zdj_library_cuepoint_t * cuepoint ) {
    snprintf( _sql, sizeof( _sql ), 
        "DELETE FROM Cuepoint_Entity WHERE entity_id=\'%s\';",
        cuepoint->entity_id
    );
    zdj_sql_exec( _sql, zdj_library_db );
}
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sqlite3.h>

#include <zerodj/health/zdj_health_type.h>
#include <zerodj/library/zdj_library.h>
#include <zerodj/sql/zdj_sql.h>

static char _sql[ 1024 ];

// Config
zdj_library_config_t * zdj_library_get_config( void ) {
    // printf( "zdj_library_get_config\n" );
    if( library_config ) { return library_config; }

    zdj_library_config_t * cfg;
    zdj_library_t * lib;

    // Grab all the values from db
    int sql_res;
    sprintf( _sql, "select * from %s LIMIT 1", ZDJ_LIBRARY_TABLE_LIBRARY_CONFIG );
    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( _sql, zdj_library_db );
    if( stmt ) {
        while ( ( sql_res = sqlite3_step( stmt ) ) == SQLITE_ROW ) {
            cfg = calloc( 1, sizeof( zdj_library_config_t ) );
            cfg->entity_id = sqlite3_column_int ( stmt, 0 );
            cfg->entity_counter = sqlite3_column_int ( stmt, 1 );
            cfg->current_lib_entity_id = sqlite3_column_int ( stmt, 2 );
        }
        sqlite3_finalize( stmt );
    }
    library_config = cfg;
    return cfg;
}

int zdj_library_config_get_current_library_id( void ) {
    zdj_library_config_t * cfg = zdj_library_get_config( );
    if( cfg ) {
        return cfg->current_lib_entity_id;
    } else {
        return 0;
    }
}

zdj_health_status_t zdj_library_config_set_current_library_id( int entity_id ) {
    zdj_library_config_t * cfg = zdj_library_get_config( );
    if( !cfg ) { return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR; }
    cfg->current_lib_entity_id = entity_id;

    snprintf( _sql, sizeof( _sql ), "UPDATE Library_Config_Entity SET entity_id=%d, entity_counter=%d, current_lib_entity_id=%d;\n",
        cfg->entity_id,
        cfg->entity_counter,
        cfg->current_lib_entity_id
    );
    int res = zdj_sql_exec( (char *)&_sql, zdj_library_db );
    if( res == SQLITE_OK ) { 
        return ZDJ_HEALTH_STATUS_OKAY;
    } else {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
}
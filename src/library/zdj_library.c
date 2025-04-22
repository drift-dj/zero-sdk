#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include <zerodj/health/zdj_health_type.h>
#include <zerodj/library/zdj_library.h>
#include <zerodj/sql/zdj_sql.h>

sqlite3 * zdj_library_db;
sqlite3 * zdj_library_import_db;
zdj_library_config_t * library_config;
static char _sql[ 1024 ];

zdj_health_status_t zdj_library_init( void ) {
    // Confirm lib db exits.
    if( access( ZDJ_LIBRARY_DB_PATH, F_OK ) != 0 ) { return ZDJ_HEALTH_STATUS_MISSING_LIBRARY_DB; }

    // Confirm lib db opens.
    if( zdj_library_db_init( ) > ZDJ_HEALTH_STATUS_OKAY ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }

    return ZDJ_HEALTH_STATUS_OKAY;
}

zdj_health_status_t zdj_library_db_init( void ) {
    zdj_library_db = zdj_sql_open( ZDJ_LIBRARY_DB_PATH );
    if( !zdj_library_db ) { 
        printf( "failed to open zero db\n" );
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR; 
    }
    return ZDJ_HEALTH_STATUS_OKAY;
}

zdj_health_status_t zdj_library_db_flush( void ) {
    sqlite3_db_cacheflush( zdj_library_db );
    return ZDJ_HEALTH_STATUS_OKAY;
}

zdj_health_status_t zdj_library_health( void ) {
    // Confirm lib db exits.
    if( access( ZDJ_LIBRARY_DB_PATH, F_OK ) != 0 ) { return ZDJ_HEALTH_STATUS_MISSING_LIBRARY_DB; }
}

int zdj_library_increment_entity_count( void ) {
    // printf( "zdj_library_increment_entity_count\n" );
    zdj_library_config_t * config = zdj_library_get_config( );
    config->entity_counter++;
    int sql_res;
    sprintf( _sql, "update Library_Config_Entity set entity_counter=%d", config->entity_counter );
    zdj_sql_exec( _sql, zdj_library_db );
    return config->entity_counter;
}
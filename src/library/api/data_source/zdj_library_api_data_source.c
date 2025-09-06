#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sqlite3.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/sql/zdj_sql.h>

static char _sql[ 1024 ];

void zdj_library_create_default_data_sources( void ) {
    snprintf( _sql, sizeof( _sql ), "INSERT INTO \"%s\" VALUES(\"%s\", \"Zero Edits\");\n",
        ZDJ_LIBRARY_TABLE_DATA_SOURCE,
        ZDJ_LIBRARY_DATA_SOURCE_ZERO
    );
    zdj_sql_exec( (char *)&_sql, zdj_library_db );

    snprintf( _sql, sizeof( _sql ), "INSERT INTO \"%s\" VALUES(\"%s\", \"ID3 Data\");\n",
        ZDJ_LIBRARY_TABLE_DATA_SOURCE,
        ZDJ_LIBRARY_DATA_SOURCE_ID3
    );
    zdj_sql_exec( (char *)&_sql, zdj_library_db );
}

zdj_library_data_source_t * zdj_library_fetch_data_source_for_entity_id( char * entity_id, sqlite3 * db ) {

}

void zdj_library_store_data_source( zdj_library_data_source_t * data_source, sqlite3 * db ) {
    int count = 0;
    int res;
    char sql[ 4096 ];
    snprintf( sql, sizeof( sql ), 
        // Insert new record
        "INSERT INTO %s(entity_id,name) VALUES('%s','%s')\n"
        // Or update existing record
        "ON CONFLICT(entity_id) DO UPDATE SET entity_id='%s',name='%s'",

        // Table name
        ZDJ_LIBRARY_TABLE_DATA_SOURCE,

        // Insert new record
        data_source->entity_id,
        data_source->name,

        // Update existing record
        data_source->entity_id,
        data_source->name
    );
    zdj_sql_exec( (char*)&sql, db );
}
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <sqlite3.h>

#include <zerodj/sql/zdj_sql.h>

static char _zdj_sql[4096];

sqlite3 * zdj_sql_open( char * path ) {
    sqlite3 * db;
    int rc = sqlite3_open( path, &db );
    if ( rc != SQLITE_OK ) {
        fprintf( stderr, "Cannot open data_model database: %s\n", sqlite3_errmsg( db ) );
        sqlite3_close( db );
        return NULL;
    }
    return db;
}

int zdj_sql_exec( char * sql, sqlite3 * db ) {
    printf( "executing sql: %s\n", sql );
    char * err_msg;
    int rc = sqlite3_exec( db, sql, NULL, NULL, &err_msg);
    if ( rc != SQLITE_OK ) {
        printf( "rc: %d, SQL error: %s\n", rc, err_msg );
        printf( "sql: %s\n", sql );
    }
    return rc;
}

int zdj_sql_rows_in_table ( char * table, char * distinct, sqlite3 * db ) {
    // printf( "zdj_sql_rows_in_table: %s, %s\n", table, distinct );
    // Build SQL count statement
    if( distinct ) {
        snprintf( _zdj_sql, sizeof( _zdj_sql ), "select count(distinct %s) from %s", distinct, table );
    } else {
        snprintf( _zdj_sql, sizeof( _zdj_sql ), "select count(*) from %s", table );
    }
   
    // Execute row stepper
    int count = 0;
    int res;
    sqlite3_stmt * c_stmt = zdj_sql_prep_row_stepper( (char*)&_zdj_sql, db );
    if( c_stmt ) {
        while ( ( res = sqlite3_step( c_stmt ) ) == SQLITE_ROW ) {
            count = sqlite3_column_int ( c_stmt, 0 );
        }
        sqlite3_finalize( c_stmt );
    }
    return count;
}

sqlite3_stmt * zdj_sql_prep_row_stepper( char * sql, sqlite3 * db ) {
    sqlite3_stmt *stmt;
    int res = sqlite3_prepare_v2( db, strdup( sql ), -1, &stmt, NULL );
    int result_number = 0;
    if ( res != SQLITE_OK ){ 
        printf( "prep row error: %s\n", sqlite3_errmsg( db ) ); 
        return NULL;
    }
    return stmt;
}
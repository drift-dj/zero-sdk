#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sqlite3.h>

#include <zerodj/health/zdj_health_type.h>
#include <zerodj/library/zdj_library.h>
#include <zerodj/system/sql/zdj_sql.h>

zdj_library_catalog_t * zdj_library_create_catalog_dto( void ) {
    zdj_library_catalog_t * catalog = calloc( 1, sizeof( zdj_library_catalog_t ) );
    zdj_library_put_uuid( catalog->entity_id );
    return catalog;
}

zdj_health_status_t zdj_library_delete_catalog( 
    zdj_library_catalog_t * catalog, 
    sqlite3 * db 
) {
    int res;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "delete from %s where entity_id like \'%s\'", 
        ZDJ_LIBRARY_TABLE_CATALOG_DATA,
        catalog->entity_id
    );
    zdj_sql_exec( sql, db );
    return ZDJ_HEALTH_STATUS_OKAY;
}

zdj_library_catalog_t * zdj_library_fetch_current_catalog_dto_for_song( 
    zdj_library_song_t * song, 
    sqlite3 * db 
) {
    // printf( "zdj_library_fetch_current_catalog_dto_for_song: %s / cat id: %s\n", song->entity_id, song->current_catalog_entity_id );

    int res;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "select * from %s where entity_id like \'%s\'", 
        ZDJ_LIBRARY_TABLE_CATALOG_DATA,
        song->current_catalog_entity_id
    );

    int _eid_col = 0;
    int _seid_col = 1;
    int _dseid_col = 2;
    int _t_col = 3;
    int _ar_col = 4;
    int _al_col = 5;
    int _l_col = 6;
    int _g_col = 7;
    int _y_col = 8;
    zdj_library_catalog_t * catalog = NULL;

    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
    if( stmt ) {
        while ( ( res = sqlite3_step( stmt ) ) == SQLITE_ROW ) { 
            catalog = calloc( 1, sizeof( zdj_library_catalog_t ) );
            strcpy( catalog->entity_id, (char*)sqlite3_column_text ( stmt, _eid_col ) );
            char * song_entity_id = (char*)sqlite3_column_text ( stmt, _seid_col );
            if( song_entity_id ) { strcpy( catalog->song_entity_id, song_entity_id ); }
            char * data_source_entity_id = (char*)sqlite3_column_text ( stmt, _dseid_col );
            if( data_source_entity_id ) { strcpy( catalog->data_source_entity_id, data_source_entity_id ); }
            char * title = (char*)sqlite3_column_text ( stmt, _t_col );
            if( title ) { strcpy( catalog->title, title ); }
            char * artist = (char*)sqlite3_column_text ( stmt, _ar_col );
            if( artist ) { strcpy( catalog->artist, artist ); }
            char * album = (char*)sqlite3_column_text ( stmt, _al_col );
            if( album ) { strcpy( catalog->album, album ); }
            char * label = (char*)sqlite3_column_text ( stmt, _l_col );
            if( label ) { strcpy( catalog->label, label ); }
            char * genre = (char*)sqlite3_column_text ( stmt, _g_col );
            if( genre ) { strcpy( catalog->genre, genre ); }
            catalog->year = sqlite3_column_int ( stmt, _y_col );
        }
        sqlite3_finalize( stmt );
    }
    
    return catalog;
}

zdj_health_status_t zdj_library_free_catalog_dto( 
    zdj_library_catalog_t * catalog 
) {

}

zdj_health_status_t zdj_library_store_catalog( 
    zdj_library_catalog_t * catalog, 
    sqlite3 * db 
) {
    int count = 0;
    int res;
    char sql[ 4096 ];

    // Set up for prepared stmt w/binds to use built-in string escaping.
    snprintf( sql, sizeof( sql ), 
        // Insert new record
        "INSERT INTO %s (entity_id,song_entity_id,data_source_entity_id,title,artist,album,label,genre,year,error) VALUES(?,?,?,?,?,?,?,?,?,?)\n"

        // Or update existing record
        "ON CONFLICT(entity_id) DO UPDATE SET entity_id=?,song_entity_id=?,data_source_entity_id=?,title=?,artist=?,album=?,label=?,genre=?,year=?,error=?",

        // Table Name
        ZDJ_LIBRARY_TABLE_CATALOG_DATA
    );

    res = sqlite3_prepare_v2( db, sql, -1, &catalog->store_stmt, 0 );
    if ( res != SQLITE_OK ) {
        printf("\nCould not prepare statement: (%d) %s\n", res, sql);
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }

    // Insert Binds
    if( sqlite3_bind_text( catalog->store_stmt, 1, catalog->entity_id, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( catalog->store_stmt, 2, catalog->song_entity_id, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( catalog->store_stmt, 3, catalog->data_source_entity_id, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( catalog->store_stmt, 4, catalog->title, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( catalog->store_stmt, 5, catalog->artist, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( catalog->store_stmt, 6, catalog->album, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( catalog->store_stmt, 7, catalog->label, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( catalog->store_stmt, 8, catalog->genre, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_int( catalog->store_stmt, 9, catalog->year ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_int( catalog->store_stmt, 10, catalog->error ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    

    // Update Binds
    if( sqlite3_bind_text( catalog->store_stmt, 11, catalog->entity_id, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( catalog->store_stmt, 12, catalog->song_entity_id, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( catalog->store_stmt, 13, catalog->data_source_entity_id, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( catalog->store_stmt, 14, catalog->title, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( catalog->store_stmt, 15, catalog->artist, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( catalog->store_stmt, 16, catalog->album, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( catalog->store_stmt, 17, catalog->label, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( catalog->store_stmt, 18, catalog->genre, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_int( catalog->store_stmt, 19, catalog->year ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_int( catalog->store_stmt, 20, catalog->error ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }

    if ( sqlite3_step( catalog->store_stmt ) != SQLITE_DONE ) {
        printf( "\nCould not step (execute) stmt.\n" );
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    
    sqlite3_finalize( catalog->store_stmt );

    zdj_sql_db_flush( db );
    
    return ZDJ_HEALTH_STATUS_OKAY;
}
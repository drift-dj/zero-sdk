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

// SELECT * from Song_Entity where entity_id IN (SELECT entity_id from Song_Links_6cc78253d5a4445d8950558b2123e895)
// "select distinct Artist from Songs order by Artist"
// "select count(distinct Artist) from Songs"
int zdj_library_query_count_all_artists( 
	char * library_entity_id, 
	sqlite3 * db 
) {
    // Build a count of all distinct artists in given library
    snprintf( _sql, sizeof( _sql ), 
        "SELECT count(DISTINCT artist) from\n"
        "(SELECT * from Song_Entity\n"
        "INNER JOIN Catalog_Data_Entity ON Song_Entity.catalog_data_entity_id = Catalog_Data_Entity.entity_id\n"
        "WHERE Song_Entity.entity_id IN (select entity_id from Song_Links_%s))\n"
        "ORDER BY artist",
        library_entity_id
    );
    int count = 0;
    int res;
    sqlite3_stmt * c_stmt = zdj_sql_prep_row_stepper( (char*)&_sql, db );
    if( c_stmt ) {
        while ( ( res = sqlite3_step( c_stmt ) ) == SQLITE_ROW ) {
            count = sqlite3_column_int ( c_stmt, 0 );
        }
        sqlite3_finalize( c_stmt );
    }
    return count;
}

zdj_error_type_t zdj_library_query_all_artists( 
    char * library_entity_id, 
    char ** artists, 
    int count, 
    sqlite3 * db 
) {
    printf( "zdj_library_query_all_artists\n" );
    // Populate list of artists
    snprintf( _sql, sizeof( _sql ), 
        "SELECT DISTINCT artist from\n"
        "(SELECT * from Song_Entity INNER JOIN Catalog_Data_Entity ON Song_Entity.catalog_data_entity_id = Catalog_Data_Entity.entity_id\n"
        "WHERE Song_Entity.entity_id IN (select entity_id from Song_Links_%s))\n"
        "ORDER BY artist",
        library_entity_id
    );
    int row = 0;
    int res;
    sqlite3_stmt * a_stmt = zdj_sql_prep_row_stepper( (char*)&_sql, db );
    if( a_stmt ) {
        while ( ( res = sqlite3_step( a_stmt ) ) == SQLITE_ROW ) {
            // Bound results by count input
            if( row < count ) {
                artists[ row ] = strdup( (char*)sqlite3_column_text ( a_stmt, 0 ) );
            }
            row++;
        }
        sqlite3_finalize( a_stmt );
    }
    
    return ZDJ_ERROR_LIBRARY_QUERY_OKAY;
}

int zdj_library_query_count_songs_by_artist( 
	char * library_entity_id, 
	char * artist,
	sqlite3 * db 
) {
    printf( "zdj_library_query_count_songs_by_artist\n" );
    // Build a count of all songs by an artist in a given library

    snprintf( _sql, sizeof( _sql ),
        "SELECT count(*) FROM\n"
        "(SELECT * from Song_Entity\n"
        "INNER JOIN Catalog_Data_Entity ON Song_Entity.catalog_data_entity_id = Catalog_Data_Entity.entity_id\n"
        "WHERE Song_Entity.entity_id IN (select entity_id from Song_Links_%s) AND artist like \'%s\')\n",
        library_entity_id,
        artist
    );

    int count = 0;
    int res;
    sqlite3_stmt * c_stmt = zdj_sql_prep_row_stepper( (char*)&_sql, db );
    if( c_stmt ) {
        while ( ( res = sqlite3_step( c_stmt ) ) == SQLITE_ROW ) {
            count = sqlite3_column_int ( c_stmt, 0 );
        }
        sqlite3_finalize( c_stmt );
    }
    return count;
}

zdj_error_type_t zdj_library_query_songs_by_artist( 
	char * library_entity_id, 
	char * artist,
	zdj_library_song_t ** songs, 
	int count, 
	sqlite3 * db 
) {
    printf( "zdj_library_query_songs_by_artist\n" );
    // Populate list of artists
    snprintf( _sql, sizeof( _sql ), 
        "SELECT * from Song_Entity\n"
        "INNER JOIN Catalog_Data_Entity ON Song_Entity.catalog_data_entity_id = Catalog_Data_Entity.entity_id\n"
        "WHERE Song_Entity.entity_id IN (select entity_id from Song_Links_%s) AND artist like '%s'\n",
        library_entity_id,
        artist
    );
    int row = 0;
    int res;
    sqlite3_stmt * a_stmt = zdj_sql_prep_row_stepper( (char*)&_sql, db );
    if( a_stmt ) {
        while ( ( res = sqlite3_step( a_stmt ) ) == SQLITE_ROW ) {
            // Bound results by count input
            if( row < count ) {
                songs[ row ] = zdj_library_fetch_song_dto_for_entity_id( (char*)sqlite3_column_text ( a_stmt, 0 ), db );
                printf( "songs[%d]: %p\n", row, songs[ row ] );
                if( songs[ row ] ) {
                    zdj_library_fetch_menu_song_graph( songs[ row ], db );
                    printf( "catalog: %p\n", songs[ row ]->catalog );
                }
            }
            row++;
        }
        sqlite3_finalize( a_stmt );
    }
    
    return ZDJ_ERROR_LIBRARY_QUERY_OKAY;
}
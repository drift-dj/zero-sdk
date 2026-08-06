#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sqlite3.h>

#include <zerodj/health/zdj_health_type.h>
#include <zerodj/library/zdj_library.h>
#include <zerodj/system/sql/zdj_sql.h>

static char _sql[ 4096 ];

void zdj_library_refresh_menu_query_table( sqlite3 * db ) {
    snprintf( _sql, sizeof( _sql ), "DROP TABLE IF EXISTS Menu_Query; \
        CREATE TABLE \'Menu_Query\' ( \
            \'song_entity_id\'	TEXT NOT NULL, \
            \'artist\'	TEXT, \
            \'title\'	TEXT, \
            \'genre\'	TEXT, \
            \'year\'	INTEGER, \
            \'filepath\' TEXT, \
            \'bpm\'	REAL, \
            \'key\'	INTEGER, \
            \'has_error\' INTEGER \
        ); \
        INSERT INTO Menu_Query (song_entity_id, artist, title, genre, year, filepath, bpm, key, has_error) \
        SELECT \
            s.entity_id, \
            cat.artist, \
            cat.title, \
            cat.genre, \
            cat.year, \
            a.filepath, \
            perf.bpm, \
            perf.key, \
            s.has_error \
        FROM Song_Entity s \
        LEFT JOIN Audio_Data_Entity a ON s.entity_id = a.song_entity_id \
        LEFT JOIN Catalog_Data_Entity cat ON s.entity_id = cat.song_entity_id \
        LEFT JOIN Performance_Data_Entity perf ON s.entity_id = perf.song_entity_id;"
    );
    zdj_sql_exec( _sql, zdj_library_db );
}

int zdj_library_query_count_all_songs( 
	sqlite3 * db 
) {
    snprintf( _sql, sizeof( _sql ), "SELECT count(*) from Menu_Query" );
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

zdj_error_type_t zdj_library_query_artist_menu( 
	zdj_library_menu_row_t * rows, 
	int count,
	sqlite3 * db 
) {
    snprintf( _sql, sizeof( _sql ), "SELECT * from Menu_Query" );
    int res;
    int row = 0;
    sqlite3_stmt * c_stmt = zdj_sql_prep_row_stepper( (char*)&_sql, db );
    if( c_stmt ) {
        while ( ( res = sqlite3_step( c_stmt ) ) == SQLITE_ROW ) {
            if( row < count ) {
                char * eid = (char*)sqlite3_column_text ( c_stmt, 0 );
                if( eid ) { strcpy( rows[ row ].song_entity_id, eid ); }
                char * artist = (char*)sqlite3_column_text ( c_stmt, 1 );
                if( artist ) { strcpy( rows[ row ].artist, artist ); }
                char * title = (char*)sqlite3_column_text ( c_stmt, 2 );
                if( title ) { strcpy( rows[ row ].title, title ); }
                char * filepath = (char*)sqlite3_column_text ( c_stmt, 3 );
                if( filepath ) { strcpy( rows[ row ].filepath, filepath ); }
                rows[ row ].bpm = sqlite3_column_double ( c_stmt, 4 );
                rows[ row ].key = sqlite3_column_int ( c_stmt, 5 );
                rows[ row ].has_error = sqlite3_column_int ( c_stmt, 6 );
                row++;
            }
        }
        sqlite3_finalize( c_stmt );
    }
    return ZDJ_ERROR_LIBRARY_QUERY_OKAY;
}

zdj_error_type_t zdj_library_query_bpm_menu( 
	zdj_library_menu_row_t * rows, 
	int count,
	sqlite3 * db 
) {
    snprintf( _sql, sizeof( _sql ), "SELECT * FROM Menu_Query ORDER BY bpm" );
    int res;
    int row = 0;
    sqlite3_stmt * c_stmt = zdj_sql_prep_row_stepper( (char*)&_sql, db );
    if( c_stmt ) {
        while ( ( res = sqlite3_step( c_stmt ) ) == SQLITE_ROW ) {
            if( row < count ) {
                char * eid = (char*)sqlite3_column_text ( c_stmt, 0 );
                if( eid ) { strcpy( rows[ row ].song_entity_id, eid ); }
                char * artist = (char*)sqlite3_column_text ( c_stmt, 1 );
                if( artist ) { strcpy( rows[ row ].artist, artist ); }
                char * title = (char*)sqlite3_column_text ( c_stmt, 2 );
                if( title ) { strcpy( rows[ row ].title, title ); }
                char * filepath = (char*)sqlite3_column_text ( c_stmt, 3 );
                if( filepath ) { strcpy( rows[ row ].filepath, filepath ); }
                rows[ row ].bpm = sqlite3_column_double ( c_stmt, 4 );
                rows[ row ].key = sqlite3_column_int ( c_stmt, 5 );
                rows[ row ].has_error = sqlite3_column_int ( c_stmt, 6 );
                row++;
            }
        }
        sqlite3_finalize( c_stmt );
    }
    return ZDJ_ERROR_LIBRARY_QUERY_OKAY;
}

int zdj_library_query_count_all_artists( 
	char * library_entity_id, 
	sqlite3 * db 
) {
    // Build a count of all distinct artists in given library
    snprintf( _sql, sizeof( _sql ), 
        "SELECT count(DISTINCT artist) from Menu_Query"
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

int zdj_library_query_count_all_genres( sqlite3 * db ) {
    // Build a count of all distinct genres in given library
    snprintf( _sql, sizeof( _sql ), 
        "SELECT count(DISTINCT genre) from Menu_Query"
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

zdj_error_type_t zdj_library_query_genres_menu( 
	char ** rows, 
	int count,
	sqlite3 * db 
) {
    // Build a count of all distinct artists in given library
    snprintf( _sql, sizeof( _sql ), 
        "SELECT DISTINCT genre from Menu_Query"
    );
    int row = 0;
    int res;
    sqlite3_stmt * c_stmt = zdj_sql_prep_row_stepper( (char*)&_sql, db );
    if( c_stmt ) {
        while ( ( res = sqlite3_step( c_stmt ) ) == SQLITE_ROW ) {
            char * genre = (char*)sqlite3_column_text ( c_stmt, 0 );
            if( genre ) { rows[ row ] = strdup( (char*)sqlite3_column_text ( c_stmt, 0 ) ); }
            row++;
        }
        sqlite3_finalize( c_stmt );
    }
    return ZDJ_ERROR_LIBRARY_QUERY_OKAY;
}

int zdj_library_query_count_songs_in_genre( sqlite3 * db, char * genre ) {
    // Build a count of all distinct genres in given library
    snprintf( _sql, sizeof( _sql ), 
        "SELECT count(*) from Menu_Query where genre=\'%s\'", genre
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

zdj_error_type_t zdj_library_query_genre_menu( 
	zdj_library_menu_row_t * rows, 
	char * genre,
	int count,
	sqlite3 * db 
) { 
    snprintf( _sql, sizeof( _sql ), "SELECT * from Menu_Query where genre=\'%s\'", genre );
    int res;
    int row = 0;
    sqlite3_stmt * c_stmt = zdj_sql_prep_row_stepper( (char*)&_sql, db );
    if( c_stmt ) {
        while ( ( res = sqlite3_step( c_stmt ) ) == SQLITE_ROW ) {
            if( row < count ) {
                char * eid = (char*)sqlite3_column_text ( c_stmt, 0 );
                if( eid ) { strcpy( rows[ row ].song_entity_id, eid ); }
                char * artist = (char*)sqlite3_column_text ( c_stmt, 1 );
                if( artist ) { strcpy( rows[ row ].artist, artist ); }
                char * title = (char*)sqlite3_column_text ( c_stmt, 2 );
                if( title ) { strcpy( rows[ row ].title, title ); }
                char * filepath = (char*)sqlite3_column_text ( c_stmt, 3 );
                if( filepath ) { strcpy( rows[ row ].filepath, filepath ); }
                rows[ row ].bpm = sqlite3_column_double ( c_stmt, 4 );
                rows[ row ].key = sqlite3_column_int ( c_stmt, 5 );
                rows[ row ].has_error = sqlite3_column_int ( c_stmt, 6 );
                row++;
            }
        }
        sqlite3_finalize( c_stmt );
    }
    return ZDJ_ERROR_LIBRARY_QUERY_OKAY;
}

int zdj_library_query_count_songs_in_key( sqlite3 * db, int key ) {
    // Build a count of all distinct genres in given library
    snprintf( _sql, sizeof( _sql ), 
        "SELECT count(*) from Menu_Query where key=%d", key
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

zdj_error_type_t zdj_library_query_key_menu( 
	zdj_library_menu_row_t * rows, 
	int key,
	int count,
	sqlite3 * db 
) { 
    snprintf( _sql, sizeof( _sql ), "SELECT * from Menu_Query where key=%d", key );
    int res;
    int row = 0;
    sqlite3_stmt * c_stmt = zdj_sql_prep_row_stepper( (char*)&_sql, db );
    if( c_stmt ) {
        while ( ( res = sqlite3_step( c_stmt ) ) == SQLITE_ROW ) {
            if( row < count ) {
                char * eid = (char*)sqlite3_column_text ( c_stmt, 0 );
                if( eid ) { strcpy( rows[ row ].song_entity_id, eid ); }
                char * artist = (char*)sqlite3_column_text ( c_stmt, 1 );
                if( artist ) { strcpy( rows[ row ].artist, artist ); }
                char * title = (char*)sqlite3_column_text ( c_stmt, 2 );
                if( title ) { strcpy( rows[ row ].title, title ); }
                char * filepath = (char*)sqlite3_column_text ( c_stmt, 3 );
                if( filepath ) { strcpy( rows[ row ].filepath, filepath ); }
                rows[ row ].bpm = sqlite3_column_double ( c_stmt, 4 );
                rows[ row ].key = sqlite3_column_int ( c_stmt, 5 );
                rows[ row ].has_error = sqlite3_column_int ( c_stmt, 6 );
                row++;
            }
        }
        sqlite3_finalize( c_stmt );
    }
    return ZDJ_ERROR_LIBRARY_QUERY_OKAY;
}
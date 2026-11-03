#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sqlite3.h>

#include <zerodj/health/zdj_health_type.h>
#include <zerodj/library/zdj_library.h>
#include <zerodj/system/log/zdj_log.h>
#include <zerodj/system/sql/zdj_sql.h>

static char _sql[ 4096 ];

typedef struct {
    int version;
} _zdj_library_query_table_version_t;
static _zdj_library_query_table_version_t * _query_table_version = NULL;

static _zdj_library_query_table_version_t * _get_query_table_version( void ) {
    if( !_query_table_version ) { 
        _query_table_version = calloc( 1, sizeof( _zdj_library_query_table_version_t ) ); 
        _query_table_version->version = 0;
    }
    return _query_table_version;
}

int zdj_library_get_query_table_version( void ) {
    return _get_query_table_version( )->version;
}

void zdj_library_refresh_menu_query_table( sqlite3 * db ) {
    zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_MSG, "Refresh Menu Query: %d", _get_query_table_version( )->version + 1 );

    zdj_sql_db_flush( db );

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
            \'has_error\' INTEGER, \
            \'error_flags\' INTEGER \
        ); \
        INSERT INTO Menu_Query (song_entity_id, artist, title, genre, year, filepath, bpm, key, has_error, error_flags) \
        SELECT \
            s.entity_id, \
            cat.artist, \
            cat.title, \
            cat.genre, \
            cat.year, \
            a.filepath, \
            perf.bpm, \
            perf.key, \
            s.has_error, \
            s.error_flags \
        FROM Song_Entity s \
        LEFT JOIN Audio_Data_Entity a ON s.entity_id = a.song_entity_id \
        LEFT JOIN Catalog_Data_Entity cat ON s.entity_id = cat.song_entity_id \
        LEFT JOIN Performance_Data_Entity perf ON s.entity_id = perf.song_entity_id;"
    );
    zdj_sql_exec( _sql, zdj_library_db );

    // Update the table version so the front end can track updates
    _get_query_table_version( )->version++;
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

int zdj_library_query_count_all_song_errors( sqlite3 * db ) {
    snprintf( _sql, sizeof( _sql ), "SELECT count(*) from Menu_Query WHERE has_error=1" );
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

bool zdj_library_query_song_eid_exists( char * song_eid, sqlite3 * db ) {
    snprintf( _sql, sizeof( _sql ), "SELECT count(*) from Song_Entity where entity_id=\'%s\'", song_eid );
    int count = 0;
    int res;
    sqlite3_stmt * c_stmt = zdj_sql_prep_row_stepper( (char*)&_sql, db );
    if( c_stmt ) {
        while ( ( res = sqlite3_step( c_stmt ) ) == SQLITE_ROW ) {
            count = sqlite3_column_int ( c_stmt, 0 );
        }
        sqlite3_finalize( c_stmt );
    }
    return count > 0;
}

bool zdj_library_query_entity_id_for_origin_id( char * entity_id, char * origin_id, sqlite3 * db ) {
    if( !origin_id || !entity_id ) { return false; }
    
    bool found = false;
    snprintf( _sql, sizeof( _sql ), 
        "SELECT entity_id FROM Song_Entity WHERE origin_id = \'%s\' LIMIT 1",
        origin_id
    );
    
    int res;
    sqlite3_stmt * c_stmt = zdj_sql_prep_row_stepper( (char*)&_sql, db );
    if( c_stmt ) {
        while ( ( res = sqlite3_step( c_stmt ) ) == SQLITE_ROW ) {
            char * eid = (char*)sqlite3_column_text ( c_stmt, 0 );
            if( eid ) { 
                printf( "found origin match: %s/%s\n", origin_id, eid );
                snprintf( entity_id, ZDJ_LIBRARY_ENTITY_ID_LEN, "%s", eid ); 
                found = true;
            }
        }
        sqlite3_finalize( c_stmt );
    }

    return found;
}

bool zdj_library_query_import_song_match( 
    zdj_library_song_t * song, 
    char * match_eid, 
    sqlite3 * db 
) {
    if( !song->catalog ) { return false; }
    bool found = false;

    int res;
    sqlite3_stmt * c_stmt;
    res = sqlite3_prepare_v2( 
        db, 
        "SELECT s.entity_id FROM Song_Entity s \
        LEFT JOIN Catalog_Data_Entity cat ON s.entity_id = cat.song_entity_id \
        WHERE cat.title=?1 AND cat.artist=?2 AND cat.album=?3 LIMIT 1;", 
        -1, 
        &c_stmt, 
        0
    );
    if ( ( res != SQLITE_OK ) || !c_stmt ) { return false; }

    sqlite3_bind_text( c_stmt, 1, song->catalog->title, -1, SQLITE_STATIC );
    sqlite3_bind_text( c_stmt, 2, song->catalog->artist, -1, SQLITE_STATIC );
    sqlite3_bind_text( c_stmt, 3, song->catalog->album, -1, SQLITE_STATIC );

    res = sqlite3_step( c_stmt );
    if ( res == SQLITE_ROW ) {
        char * eid = (char*)sqlite3_column_text ( c_stmt, 0 );
        if( eid ) { 
            strcpy( match_eid, eid ); 
            found = true;
        }
    }
    sqlite3_finalize( c_stmt );

    if( found ) {
        zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Match[%s]: %s - %s - %s\n", 
            match_eid,
            song->catalog->title, 
            song->catalog->artist,
            song->catalog->album  
        );
    }

    return found;
}

zdj_error_type_t zdj_library_query_artist_menu( 
	zdj_library_menu_row_t ** rows, 
	sqlite3 * db 
) {
    // printf( "zdj_library_query_artist_menu\n" );
    // This query is designed to give us results for both section titles AND songs by artist
    snprintf( _sql, sizeof( _sql ), "SELECT *, \
            CASE \
                WHEN lag(artist) OVER (ORDER BY artist) IS NULL THEN 0 \
                WHEN artist <> lag(artist) OVER (ORDER BY artist) THEN 1 \
                ELSE 0 \
            END AS needs_section \
        from Menu_Query ORDER BY artist, title;" 
    );
    int res;
    int row = 0;
    zdj_library_menu_row_t * section_row = NULL;
    zdj_library_menu_row_t * song_row = NULL;
    zdj_library_menu_row_t * prev_row = NULL;
    // printf( "sql: %s\n", _sql );
    sqlite3_stmt * c_stmt = zdj_sql_prep_row_stepper( (char*)&_sql, db );
    if( c_stmt ) {
        while ( ( res = sqlite3_step( c_stmt ) ) == SQLITE_ROW ) {
            if( row == 0 ) {
                // Add an artist row
                section_row = calloc( 1, sizeof( zdj_library_menu_row_t ) );
                char * artist = (char*)sqlite3_column_text ( c_stmt, 1 );
                // if( artist ) { strcpy( section_row->artist, artist ); }
                if( artist ) { snprintf( section_row->artist, ZDJ_LIBRARY_ARTIST_LEN, "%s", artist ); }
                section_row->is_section = true;
                *rows = section_row;
                prev_row = section_row;
                row++;

                // printf( "added first section: %p %p/%s\n", section_row, *rows, section_row->artist );

                song_row = calloc( 1, sizeof( zdj_library_menu_row_t ) );
                char * eid = (char*)sqlite3_column_text ( c_stmt, 0 );
                if( eid ) { strcpy( song_row->song_entity_id, eid ); }
                // if( artist ) { strcpy( song_row->artist, artist ); }
                // char * title = (char*)sqlite3_column_text ( c_stmt, 2 );
                // if( title ) { strcpy( song_row->title, title ); }

                if( artist ) { snprintf( song_row->artist, ZDJ_LIBRARY_ARTIST_LEN, "%s", artist ); }
                char * title = (char*)sqlite3_column_text ( c_stmt, 2 );
                if( title ) { snprintf( song_row->title, ZDJ_LIBRARY_TITLE_LEN, "%s", title ); }

                song_row->bpm = sqlite3_column_double ( c_stmt, 6 );
                song_row->key = sqlite3_column_int ( c_stmt, 7 );
                song_row->has_error = sqlite3_column_int ( c_stmt, 8 );
                song_row->error_flags = sqlite3_column_int ( c_stmt, 9 );

                // zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Row err:%d - %s", song_row->has_error, song_row->title );

                row++;
                section_row->prev = NULL;
                section_row->next = song_row;
                song_row->prev = section_row;
                prev_row = song_row;
            } else {
                // printf( "row: %s\n", (char*)sqlite3_column_text ( c_stmt, 2 ) );
                // Add a section if needed
                if( sqlite3_column_int ( c_stmt, 10 ) == 1 ) {
                    section_row = calloc( 1, sizeof( zdj_library_menu_row_t ) );
                    char * artist = (char*)sqlite3_column_text ( c_stmt, 1 );
                    
                    // if( artist ) { strcpy( section_row->artist, artist ); }
                    if( artist ) { snprintf( section_row->artist, ZDJ_LIBRARY_ARTIST_LEN, "%s", artist ); }

                    section_row->is_section = true;
                    prev_row->next = section_row;
                    section_row->prev = prev_row;
                    prev_row = section_row;
                    row++;
                }

                song_row = calloc( 1, sizeof( zdj_library_menu_row_t ) );
                char * eid = (char*)sqlite3_column_text ( c_stmt, 0 );
                if( eid ) { strcpy( song_row->song_entity_id, eid ); }
                char * artist = (char*)sqlite3_column_text ( c_stmt, 1 );

                // if( artist ) { strcpy( song_row->artist, artist ); }
                // char * title = (char*)sqlite3_column_text ( c_stmt, 2 );
                // if( title ) { strcpy( song_row->title, title ); }
                if( artist ) { snprintf( song_row->artist, ZDJ_LIBRARY_ARTIST_LEN, "%s", artist ); }
                char * title = (char*)sqlite3_column_text ( c_stmt, 2 );
                if( title ) { snprintf( song_row->title, ZDJ_LIBRARY_TITLE_LEN, "%s", title ); }


                song_row->bpm = sqlite3_column_double ( c_stmt, 6 );
                song_row->key = sqlite3_column_int ( c_stmt, 7 );
                song_row->has_error = sqlite3_column_int ( c_stmt, 8 );
                song_row->error_flags = sqlite3_column_int ( c_stmt, 9 );
                row++;

                // zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Row err:%d - %s", song_row->has_error, song_row->title );
                
                prev_row->next = song_row;
                song_row->prev = prev_row;
                prev_row = song_row;
            }
        }
        sqlite3_finalize( c_stmt );
    }
    zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "[Q]artist: %d", row );
    // printf( "sql done\n" );
    return ZDJ_ERROR_LIBRARY_QUERY_OKAY;
}

zdj_error_type_t zdj_library_query_bpm_menu( 
	zdj_library_menu_row_t ** rows, 
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
                rows[ row ] = calloc( 1, sizeof( zdj_library_menu_row_t ) );
                char * eid = (char*)sqlite3_column_text ( c_stmt, 0 );
                if( eid ) { strcpy( rows[ row ]->song_entity_id, eid ); }
                // char * artist = (char*)sqlite3_column_text ( c_stmt, 1 );
                // if( artist ) { strcpy( rows[ row ]->artist, artist ); }
                // char * title = (char*)sqlite3_column_text ( c_stmt, 2 );
                // if( title ) { strcpy( rows[ row ]->title, title ); }
                // char * genre = (char*)sqlite3_column_text ( c_stmt, 3 );
                // if( genre ) { strcpy( rows[ row ]->genre, genre ); }

                char * artist = (char*)sqlite3_column_text ( c_stmt, 1 );
                if( artist ) { snprintf( rows[ row ]->artist, ZDJ_LIBRARY_ARTIST_LEN, "%s", artist ); }
                char * title = (char*)sqlite3_column_text ( c_stmt, 2 );
                if( title ) { snprintf( rows[ row ]->title, ZDJ_LIBRARY_TITLE_LEN, "%s", title ); }
                char * genre = (char*)sqlite3_column_text ( c_stmt, 3 );
                if( genre ) { snprintf( rows[ row ]->genre, ZDJ_LIBRARY_GENRE_LEN, "%s", genre ); }

                rows[ row ]->year = sqlite3_column_int ( c_stmt, 4 );
                char * filepath = (char*)sqlite3_column_text ( c_stmt, 5 );
                if( filepath ) { strcpy( rows[ row ]->filepath, filepath ); }
                rows[ row ]->bpm = sqlite3_column_double ( c_stmt, 6 );
                rows[ row ]->key = sqlite3_column_int ( c_stmt, 7 );
                rows[ row ]->has_error = sqlite3_column_int ( c_stmt, 8 );
                rows[ row ]->error_flags = sqlite3_column_int ( c_stmt, 9 );
                row++;
            }
        }
        sqlite3_finalize( c_stmt );
    }
    return ZDJ_ERROR_LIBRARY_QUERY_OKAY;
}

zdj_error_type_t zdj_library_query_error_menu( 
	zdj_library_menu_row_t ** rows, 
    int count,
	sqlite3 * db 
) {
    snprintf( _sql, sizeof( _sql ), "SELECT * FROM Menu_Query WHERE has_error=1 ORDER BY title" );
    int res;
    int row_num = 0;
    zdj_library_menu_row_t * row = NULL;
    zdj_library_menu_row_t * prev_row = NULL;
    sqlite3_stmt * c_stmt = zdj_sql_prep_row_stepper( (char*)&_sql, db );
    if( c_stmt ) {
        while ( ( res = sqlite3_step( c_stmt ) ) == SQLITE_ROW ) {
            if( row_num < count ) {
                row = calloc( 1, sizeof( zdj_library_menu_row_t ) );
                if( row_num == 0 ){ *rows = row; }

                char * eid = (char*)sqlite3_column_text ( c_stmt, 0 );
                if( eid ) { strcpy( row->song_entity_id, eid ); }
                char * artist = (char*)sqlite3_column_text ( c_stmt, 1 );
                if( artist ) { snprintf( row->artist, ZDJ_LIBRARY_ARTIST_LEN, "%s", artist ); }
                char * title = (char*)sqlite3_column_text ( c_stmt, 2 );
                if( title ) { snprintf( row->title, ZDJ_LIBRARY_TITLE_LEN, "%s", title ); }
                char * genre = (char*)sqlite3_column_text ( c_stmt, 3 );
                if( genre ) { snprintf( row->genre, ZDJ_LIBRARY_GENRE_LEN, "%s", genre ); }
                row->year = sqlite3_column_int ( c_stmt, 4 );
                char * filepath = (char*)sqlite3_column_text ( c_stmt, 5 );
                if( filepath ) { strcpy( row->filepath, filepath ); }
                row->bpm = sqlite3_column_double ( c_stmt, 6 );
                row->key = sqlite3_column_int ( c_stmt, 7 );
                row->has_error = sqlite3_column_int ( c_stmt, 8 );
                row->error_flags = sqlite3_column_int ( c_stmt, 9 );

                row->prev = prev_row;
                if( prev_row ){ prev_row->next = row; }
                prev_row = row;
                row_num++;
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
            if( genre ) { 
                rows[ row ] = strdup( (char*)sqlite3_column_text ( c_stmt, 0 ) ); 
            }
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
	zdj_library_menu_row_t ** rows, 
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
                rows[ row ] = calloc( 1, sizeof( zdj_library_menu_row_t ) );
                char * eid = (char*)sqlite3_column_text ( c_stmt, 0 );
                if( eid ) { strcpy( rows[ row ]->song_entity_id, eid ); }
                // char * artist = (char*)sqlite3_column_text ( c_stmt, 1 );
                // if( artist ) { strcpy( rows[ row ]->artist, artist ); }
                // char * title = (char*)sqlite3_column_text ( c_stmt, 2 );
                // if( title ) { strcpy( rows[ row ]->title, title ); }
                // char * genre = (char*)sqlite3_column_text ( c_stmt, 3 );
                // if( genre ) { strcpy( rows[ row ]->genre, genre ); }

                char * artist = (char*)sqlite3_column_text ( c_stmt, 1 );
                if( artist ) { snprintf( rows[ row ]->artist, ZDJ_LIBRARY_ARTIST_LEN, "%s", artist ); }
                char * title = (char*)sqlite3_column_text ( c_stmt, 2 );
                if( title ) { snprintf( rows[ row ]->title, ZDJ_LIBRARY_TITLE_LEN, "%s", title ); }
                char * genre = (char*)sqlite3_column_text ( c_stmt, 3 );
                if( genre ) { snprintf( rows[ row ]->genre, ZDJ_LIBRARY_GENRE_LEN, "%s", genre ); }

                rows[ row ]->year = sqlite3_column_int ( c_stmt, 4 );
                char * filepath = (char*)sqlite3_column_text ( c_stmt, 5 );
                if( filepath ) { strcpy( rows[ row ]->filepath, filepath ); }
                rows[ row ]->bpm = sqlite3_column_double ( c_stmt, 6 );
                rows[ row ]->key = sqlite3_column_int ( c_stmt, 7 );
                rows[ row ]->has_error = sqlite3_column_int ( c_stmt, 8 );
                rows[ row ]->error_flags = sqlite3_column_int ( c_stmt, 9 );
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
	zdj_library_menu_row_t ** rows, 
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
                rows[ row ] = calloc( 1, sizeof( zdj_library_menu_row_t ) );
                char * eid = (char*)sqlite3_column_text ( c_stmt, 0 );
                if( eid ) { strcpy( rows[ row ]->song_entity_id, eid ); }
                // char * artist = (char*)sqlite3_column_text ( c_stmt, 1 );
                // if( artist ) { strcpy( rows[ row ]->artist, artist ); }
                // char * title = (char*)sqlite3_column_text ( c_stmt, 2 );
                // if( title ) { strcpy( rows[ row ]->title, title ); }
                // char * genre = (char*)sqlite3_column_text ( c_stmt, 3 );
                // if( genre ) { strcpy( rows[ row ]->genre, genre ); }

                char * artist = (char*)sqlite3_column_text ( c_stmt, 1 );
                if( artist ) { snprintf( rows[ row ]->artist, ZDJ_LIBRARY_ARTIST_LEN, "%s", artist ); }
                char * title = (char*)sqlite3_column_text ( c_stmt, 2 );
                if( title ) { snprintf( rows[ row ]->title, ZDJ_LIBRARY_TITLE_LEN, "%s", title ); }
                char * genre = (char*)sqlite3_column_text ( c_stmt, 3 );
                if( genre ) { snprintf( rows[ row ]->genre, ZDJ_LIBRARY_GENRE_LEN, "%s", genre ); }

                rows[ row ]->year = sqlite3_column_int ( c_stmt, 4 );
                char * filepath = (char*)sqlite3_column_text ( c_stmt, 5 );
                if( filepath ) { strcpy( rows[ row ]->filepath, filepath ); }
                rows[ row ]->bpm = sqlite3_column_double ( c_stmt, 6 );
                rows[ row ]->key = sqlite3_column_int ( c_stmt, 7 );
                rows[ row ]->has_error = sqlite3_column_int ( c_stmt, 8 );
                rows[ row ]->error_flags = sqlite3_column_int ( c_stmt, 9 );
                row++;
            }
        }
        sqlite3_finalize( c_stmt );
    }
    return ZDJ_ERROR_LIBRARY_QUERY_OKAY;
}
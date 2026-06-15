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

int zdj_library_query_count_songs_in_key( 
	char * library_entity_id, 
	zdj_library_key_t key, 
	sqlite3 * db 
) {
    // Build an array of song entity_ids within range by querying performance table
    char sql[ 1024 ];
    sprintf( sql, "SELECT count(*) from %s WHERE key = %d",
        ZDJ_LIBRARY_TABLE_PERFORMANCE_DATA,
        key
    );
    int res;
    int count = 0;
    sqlite3_stmt * c_stmt = zdj_sql_prep_row_stepper( sql, db );
    if( c_stmt ) {
        while ( ( res = sqlite3_step( c_stmt ) ) == SQLITE_ROW ) { 
            printf( "performance count res\n" );
            count = sqlite3_column_int ( c_stmt, 0 );
        }
        sqlite3_finalize( c_stmt );
    }

    printf( "found count: %d for key %s\n",
        count,
        zdj_library_key_name[ key ]
    );
    return count;
}

zdj_error_type_t zdj_library_query_songs_in_key( 
	char * library_entity_id, 
	zdj_library_key_t key,
	zdj_library_song_t ** songs, 
	int count, 
	sqlite3 * db 
) {
    if( !songs ){ return ZDJ_ERROR_LIBRARY_NO_RESULTS; }

    int res;
    int row;

    // Build an array of song entity_ids within range by querying performance table
    char sql[ 1024 ];
    char song_ids[ count ][ ZDJ_LIBRARY_ENTITY_ID_LEN ];
    sprintf( sql, "SELECT * from %s WHERE key = %d",
        ZDJ_LIBRARY_TABLE_PERFORMANCE_DATA,
        key
    );
    row = 0;
    sqlite3_stmt * c_stmt = zdj_sql_prep_row_stepper( sql, db );
    if( c_stmt ) {
        while ( ( res = sqlite3_step( c_stmt ) ) == SQLITE_ROW ) {
            if( row < count ) { 
                strncpy( 
                    song_ids[ row ], 
                    (char*)sqlite3_column_text ( c_stmt, 1 ), 
                    ZDJ_LIBRARY_ENTITY_ID_LEN 
                );
            }
            row++;
        }
        sqlite3_finalize( c_stmt );
    }

    
    row = 0;
    for( int i=0; i<count; i++ ) {
        sprintf( sql, "select * from %s where entity_id like \'%s\'",
            ZDJ_LIBRARY_TABLE_SONG,
            song_ids[ i ]
        );
        sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( sql, db );
        if( stmt ) {
            while ( ( res = sqlite3_step( stmt ) ) == SQLITE_ROW ) { 
                if( row < count ) {
                    zdj_library_song_t * _song = zdj_library_fetch_song_dto_for_entity_id( 
                        (char*)sqlite3_column_text ( stmt, 0 ), db
                    );
                    zdj_library_fetch_menu_song_graph( _song, db );
                    songs[ row ] = _song;
                    // printf( "zdj_library_query_songs_in_key found song: %d %s\n", row, _song->catalog->title );
                }
                row++;
            }
            sqlite3_finalize( stmt );
        }
    }
    return ZDJ_ERROR_LIBRARY_QUERY_OKAY;
}
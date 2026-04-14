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

int zdj_library_query_count_songs_in_bpm_range( 
	char * library_entity_id, 
	zdj_library_bpm_range_t range, 
	sqlite3 * db 
) {
    // Count all performance records with bpm values falling within range
    float range_min = 0.0;
    float range_max = 0.0;

    switch ( range ) {
        case ZDJ_LIBRARY_BPM_0_40: range_min = 0.0; range_max = 40.0; break;
        case ZDJ_LIBRARY_BPM_40_100: range_min = 40.0; range_max = 100.0; break;
        case ZDJ_LIBRARY_BPM_100_130: range_min = 100.0; range_max = 130.0; break;
        case ZDJ_LIBRARY_BPM_130_145: range_min = 130.0; range_max = 145.0; break;
        case ZDJ_LIBRARY_BPM_145_180: range_min = 145.0; range_max = 180.0; break;
        case ZDJ_LIBRARY_BPM_180: range_min = 180.0; range_max = 10000.0; break;
        default: break;
    }

    // Build an array of song entity_ids within range by querying performance table
    char sql[ 1024 ];
    sprintf( sql, "SELECT count(*) from %s WHERE bpm >= %f AND bpm < %f",
        ZDJ_LIBRARY_TABLE_PERFORMANCE_DATA,
        range_min,
        range_max
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

    printf( "found count: %d for %s\n",
        count,
        zdj_library_bpm_name[ range ]
    );
    return count;
}

zdj_error_type_t zdj_library_query_songs_in_bpm_range( 
	char * library_entity_id, 
	zdj_library_bpm_range_t range,
	zdj_library_song_t ** songs, 
	int count, 
	sqlite3 * db 
) {
    if( !songs ){ return ZDJ_ERROR_LIBRARY_NO_RESULTS; }

    float range_min = 0.0;
    float range_max = 0.0;

    switch ( range ) {
        case ZDJ_LIBRARY_BPM_0_40: range_min = 0.0; range_max = 40.0; break;
        case ZDJ_LIBRARY_BPM_40_100: range_min = 40.0; range_max = 100.0; break;
        case ZDJ_LIBRARY_BPM_100_130: range_min = 100.0; range_max = 130.0; break;
        case ZDJ_LIBRARY_BPM_130_145: range_min = 130.0; range_max = 145.0; break;
        case ZDJ_LIBRARY_BPM_145_180: range_min = 145.0; range_max = 180.0; break;
        case ZDJ_LIBRARY_BPM_180: range_min = 180.0; range_max = 10000.0; break;
        default: break;
    }

    int res;
    int row;

    // Build an array of song entity_ids within range by querying performance table
    char sql[ 1024 ];
    char song_ids[ count ][ ZDJ_LIBRARY_ENTITY_ID_LEN ];
    sprintf( sql, "SELECT * from %s WHERE bpm >= %f AND bpm < %f",
        ZDJ_LIBRARY_TABLE_PERFORMANCE_DATA,
        range_min,
        range_max
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
                    printf( "zdj_library_query_songs_in_bpm_range found song: %d %p\n", row, _song );
                    printf( "%s\n", _song->catalog->title );
                }
                row++;
            }
            sqlite3_finalize( stmt );
        }
    }
    return ZDJ_ERROR_LIBRARY_QUERY_OKAY;
}
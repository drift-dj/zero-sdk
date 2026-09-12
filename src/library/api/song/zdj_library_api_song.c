#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sqlite3.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>

#include <zerodj/health/zdj_health_type.h>
#include <zerodj/library/zdj_library.h>
#include <zerodj/system/sql/zdj_sql.h>


zdj_library_song_t * zdj_library_create_song_dto( void ) {
    zdj_library_song_t * song = calloc( 1, sizeof( zdj_library_song_t ) );
    zdj_library_put_uuid( song->entity_id );
    return song;
}

zdj_library_song_t * zdj_library_fetch_song_dto_for_entity_id( 
    char * entity_id, 
    sqlite3 * db 
) {
    int res;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "select * from %s where entity_id like \'%s\'", 
        ZDJ_LIBRARY_TABLE_SONG,
        entity_id
    );

    int _eid_col = 0;
    int _cceid_col = 1;
    int _cpeid_col = 2;
    int _ccueid_col = 3;
    int _caeid_col = 4;
    int _as_col = 5;
    int _ap_col = 6;
    int _he_col = 7;
    int _ef_col = 8;


    zdj_library_song_t * song = NULL;

    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
    if( stmt ) {
        while ( ( res = sqlite3_step( stmt ) ) == SQLITE_ROW ) { 
            song = calloc( 1, sizeof( zdj_library_song_t ) );
            strcpy( song->entity_id, (char*)sqlite3_column_text ( stmt, _eid_col ) );
            strcpy( song->current_catalog_entity_id, (char*)sqlite3_column_text ( stmt, _cceid_col ) );
            strcpy( song->current_performance_entity_id, (char*)sqlite3_column_text ( stmt, _cpeid_col ) );
            strcpy( song->current_curation_entity_id, (char*)sqlite3_column_text ( stmt, _ccueid_col ) );
            strcpy( song->current_audio_entity_id, (char*)sqlite3_column_text ( stmt, _caeid_col ) );
            song->analysis_state = sqlite3_column_int ( stmt, _as_col );
            song->analysis_progress = sqlite3_column_double ( stmt, _ap_col );
            song->has_error = sqlite3_column_int ( stmt, _he_col );
            song->error_flags = sqlite3_column_int ( stmt, _ef_col );
        }
        sqlite3_finalize( stmt );
    }

    return song;
}

zdj_health_status_t zdj_library_free_song_dto( 
    zdj_library_song_t * song 
) {
    free( song );
}

zdj_health_status_t zdj_library_store_song( 
    zdj_library_song_t * song, 
    sqlite3 * db 
) {
    int count = 0;
    int res;
    char sql[ 4096 ];
    snprintf( sql, sizeof( sql ), 
        // Insert new record
        "INSERT INTO %s(entity_id,catalog_data_entity_id,performance_data_entity_id,curation_data_entity_id,audio_data_entity_id,analysis_state,analysis_progress,has_error,error_flags) VALUES('%s','%s','%s','%s','%s',%d,%f,%d,%d)\n"
        // Or update existing record
        "ON CONFLICT(entity_id) DO UPDATE SET entity_id='%s',catalog_data_entity_id='%s',performance_data_entity_id='%s',curation_data_entity_id='%s',audio_data_entity_id='%s',analysis_state=%d,analysis_progress=%f,has_error=%d,error_flags=%d",

        // Table name
        ZDJ_LIBRARY_TABLE_SONG,

        // Insert new record
        song->entity_id,
        song->current_catalog_entity_id,
        song->current_performance_entity_id,
        song->current_curation_entity_id,
        song->current_audio_entity_id,
        song->analysis_state,
        song->analysis_progress,
        song->has_error,
        song->error_flags,

        // Update existing record
        song->entity_id,
        song->current_catalog_entity_id,
        song->current_performance_entity_id,
        song->current_curation_entity_id,
        song->current_audio_entity_id,
        song->analysis_state,
        song->analysis_progress,
        song->has_error,
        song->error_flags
    );
    zdj_sql_exec( sql, db );
    
    zdj_sql_db_flush( db );
    
    return ZDJ_HEALTH_STATUS_OKAY;
}

int zdj_library_count_songs_in_library( char * library_entity_id, sqlite3 * db ) {
    int res;
    int count = 0;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "select entity_id from %s", ZDJ_LIBRARY_TABLE_SONG );
    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
    if( stmt ) {
        while ( ( res = sqlite3_step( stmt ) ) == SQLITE_ROW ) { 
            count++;
        }
        sqlite3_finalize( stmt );
    }
    return count;
}

int zdj_library_count_songs_in_db( sqlite3 * db ) {
    int res;
    int count = 0;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "select entity_id from %s", ZDJ_LIBRARY_TABLE_SONG );
    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
    if( stmt ) {
        while ( ( res = sqlite3_step( stmt ) ) == SQLITE_ROW ) { 
            count++;
        }
        sqlite3_finalize( stmt );
    }
    return count;
}

// Alloc/populate an array with all entity_ids in db
zdj_health_status_t zdj_library_fetch_song_entity_ids( 
    char ** arr,
    int count,
    sqlite3 * db 
) {
    int res;
    int row = 0;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "select entity_id from %s", ZDJ_LIBRARY_TABLE_SONG );
    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
    if( stmt ) {
        while ( ( res = sqlite3_step( stmt ) ) == SQLITE_ROW ) { 
            if( row < count ) {
                arr[ row ] = strdup( (char*)sqlite3_column_text ( stmt, 0 ) );
            }
            row++;
        }
        sqlite3_finalize( stmt );
    }
}

void zdj_library_merge_song_with_entity_id( 
    zdj_library_song_t * song, 
    char * eid,
    sqlite3 * db 
) {
    int res;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "SELECT \
        s.entity_id, \
        s.current_audio_entity_id, \
        s.current_catalog_entity_id, \
        s.current_performance_entity_id, \
        s.current_curation_entity_id, \
        cat.artist, \
        cat.title, \
        cat.genre, \
        cat.year, \
        perf.bpm, \
        perf.key, \
        perf.has_beat_grid, \
        perf.beat_grid_start_sample, \
        FROM Song_Entity s \
        LEFT JOIN Audio_Data_Entity a ON s.entity_id = a.song_entity_id \
        LEFT JOIN Catalog_Data_Entity cat ON s.entity_id = cat.song_entity_id \
        LEFT JOIN Performance_Data_Entity perf ON s.entity_id = perf.song_entity_id \
        WHERE s.entity_id = \'%s\';",
        eid
    );

    int song_eid_col = 0;
    int audio_eid_col = 1;
    int cat_eid_col = 2;
    int perf_eid_col = 3;
    int cur_eid_col = 4;
    int cat_artist_col = 5;
    int cat_title_col = 6;
    int cat_genre_col = 7;
    int cat_year_col = 8;
    int perf_bpm_col = 9;
    int perf_key_col = 10;
    int perf_bg_col = 11;
    int perf_bg_start_col = 12;

    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
    if( stmt ) {
        while ( ( res = sqlite3_step( stmt ) ) == SQLITE_ROW ) { 
            char * song_eid = (char*)sqlite3_column_text ( stmt, song_eid_col );
            if( song_eid ) { 
                strcpy( song->entity_id, song_eid );
                strcpy( song->audio->song_entity_id, song_eid );
                strcpy( song->catalog->song_entity_id, song_eid );
                strcpy( song->performance->song_entity_id, song_eid );
                strcpy( song->curation->parent_entity_id, song_eid );
            }
            char * audio_eid = (char*)sqlite3_column_text ( stmt, audio_eid_col );
            if( audio_eid ) { 
                strcpy( song->audio->entity_id, audio_eid );
                strcpy( song->current_audio_entity_id, audio_eid );
            }

            char * cat_eid = (char*)sqlite3_column_text ( stmt, cat_eid_col );
            if( cat_eid ) { 
                strcpy( song->catalog->entity_id, cat_eid );
                strcpy( song->current_catalog_entity_id, cat_eid );
            }
            char * artist = (char*)sqlite3_column_text ( stmt, cat_artist_col );
            if( artist ) { strcpy( song->catalog->artist, artist ); }
            char * title = (char*)sqlite3_column_text ( stmt, cat_title_col );
            if( title ) { strcpy( song->catalog->title, title ); }
            char * genre = (char*)sqlite3_column_text ( stmt, cat_genre_col );
            if( genre ) { strcpy( song->catalog->genre, genre ); }
            song->catalog->year = sqlite3_column_int ( stmt, cat_year_col );

            char * perf_eid = (char*)sqlite3_column_text ( stmt, perf_eid_col );
            if( perf_eid ) { 
                strcpy( song->performance->entity_id, perf_eid );
                strcpy( song->current_performance_entity_id, perf_eid );
            }
            song->performance->bpm = sqlite3_column_double ( stmt, perf_bpm_col );
            song->performance->key = sqlite3_column_int ( stmt, perf_key_col );
            song->performance->has_beat_grid = sqlite3_column_int ( stmt, perf_bg_col );
            song->performance->beat_grid_start_sample = sqlite3_column_int ( stmt, perf_bg_start_col );

            char * cur_eid = (char*)sqlite3_column_text ( stmt, cur_eid_col );
            if( cur_eid ) { 
                strcpy( song->curation->entity_id, cur_eid );
                strcpy( song->current_curation_entity_id, cur_eid );
            }
        }
        sqlite3_finalize( stmt );
    }
}

void zdj_library_replace_entity_id_with_song( 
    zdj_library_song_t * song, 
    char * eid, 
    sqlite3 * db 
) {
    int res;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "SELECT \
        s.entity_id, \
        s.current_audio_entity_id, \
        s.current_catalog_entity_id, \
        s.current_performance_entity_id, \
        s.current_curation_entity_id, \
        cat.artist, \
        cat.title, \
        cat.genre, \
        cat.year, \
        perf.bpm, \
        perf.key, \
        perf.has_beat_grid, \
        perf.beat_grid_start_sample, \
        FROM Song_Entity s \
        LEFT JOIN Audio_Data_Entity a ON s.entity_id = a.song_entity_id \
        LEFT JOIN Catalog_Data_Entity cat ON s.entity_id = cat.song_entity_id \
        LEFT JOIN Performance_Data_Entity perf ON s.entity_id = perf.song_entity_id \
        WHERE s.entity_id = \'%s\';",
        eid
    );

    int song_eid_col = 0;
    int audio_eid_col = 1;
    int cat_eid_col = 2;
    int perf_eid_col = 3;
    int cur_eid_col = 4;
    int cat_artist_col = 5;
    int cat_title_col = 6;
    int cat_genre_col = 7;
    int cat_year_col = 8;
    int perf_bpm_col = 9;
    int perf_key_col = 10;
    int perf_bg_col = 11;
    int perf_bg_start_col = 12;

    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
    if( stmt ) {
        while ( ( res = sqlite3_step( stmt ) ) == SQLITE_ROW ) { 
            char * song_eid = (char*)sqlite3_column_text ( stmt, song_eid_col );
            if( song_eid ) { 
                strcpy( song->entity_id, song_eid );
                strcpy( song->audio->song_entity_id, song_eid );
                strcpy( song->catalog->song_entity_id, song_eid );
                strcpy( song->performance->song_entity_id, song_eid );
                strcpy( song->curation->parent_entity_id, song_eid );
            }
            char * audio_eid = (char*)sqlite3_column_text ( stmt, audio_eid_col );
            if( audio_eid ) { 
                strcpy( song->audio->entity_id, audio_eid );
                strcpy( song->current_audio_entity_id, audio_eid );
            }
            char * cat_eid = (char*)sqlite3_column_text ( stmt, cat_eid_col );
            if( cat_eid ) { 
                strcpy( song->catalog->entity_id, cat_eid );
                strcpy( song->current_catalog_entity_id, cat_eid );
            }
            char * perf_eid = (char*)sqlite3_column_text ( stmt, perf_eid_col );
            if( perf_eid ) { 
                strcpy( song->performance->entity_id, perf_eid );
                strcpy( song->current_performance_entity_id, perf_eid );
            }
            char * cur_eid = (char*)sqlite3_column_text ( stmt, cur_eid_col );
            if( cur_eid ) { 
                strcpy( song->curation->entity_id, cur_eid );
                strcpy( song->current_curation_entity_id, cur_eid );
            }
        }
        sqlite3_finalize( stmt );
    }
}

zdj_health_status_t zdj_library_delete_song( 
    zdj_library_song_t * song, 
    sqlite3 * db 
) {
    int count = 0;
    int res;
    char sql[ 4096 ];
    snprintf( sql, sizeof( sql ), 
        // Delete record
        "DELETE FROM %s WHERE entity_id LIKE '%s'",
        ZDJ_LIBRARY_TABLE_SONG,
        song->entity_id
    );
    zdj_sql_exec( sql, db );
    
    zdj_sql_db_flush( db );
    
    return ZDJ_HEALTH_STATUS_OKAY;
}

bool zdj_library_song_has_error_flag( zdj_library_song_t * song, zdj_library_song_error_t flag ) {
    return (song->error_flags >> flag) & 0x1;
}

bool zdj_library_song_can_play( zdj_library_song_t * song ) {
    if( song->has_error ) {
        if( zdj_library_song_has_error_flag( song, ZDJ_LIBRARY_SONG_ERROR_FLAG_BAD_FORMAT ) ||
	        zdj_library_song_has_error_flag( song, ZDJ_LIBRARY_SONG_ERROR_FLAG_BAD_ENCODING ) ||
	        zdj_library_song_has_error_flag( song, ZDJ_LIBRARY_SONG_ERROR_FLAG_FILE_MISSING ) ||
	        zdj_library_song_has_error_flag( song, ZDJ_LIBRARY_SONG_ERROR_FLAG_DECODE_FAILED ) 
        ) {
            return false;
        }
    }
    return true;
}
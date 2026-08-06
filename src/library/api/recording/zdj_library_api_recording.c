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
#include <zerodj/signal/pipeline/node/audio/record/zdj_audio_record_node.h>
#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/system/settings/zdj_settings.h>
#include <zerodj/system/sql/zdj_sql.h>


void zdj_library_remove_all_recordings( void ) {
    // Count, query and delete all records with a filename matching the internal naming scheme

    char library_entity_id[ ZDJ_LIBRARY_ENTITY_ID_LEN ];
    zdj_library_config_put_current_library_id( library_entity_id );

    // Count recording records to build a list of song eids
    char _sql[ 1024 ];
    snprintf( _sql, sizeof( _sql ), "SELECT count(*) from Menu_Query where artist=\'%s\'", ZDJ_RECORDING_ARTIST );
    int count = 0;
    int res;
    sqlite3_stmt * c_stmt = zdj_sql_prep_row_stepper( (char*)&_sql, zdj_library_db );
    if( c_stmt ) {
        while ( ( res = sqlite3_step( c_stmt ) ) == SQLITE_ROW ) {
            count = sqlite3_column_int ( c_stmt, 0 );
        }
        sqlite3_finalize( c_stmt );
    }
    
    // Exit if there aren't any recordings
    if( count == 0 ) { return; }

    // Pull row records for the recordings
    zdj_library_menu_row_t * rows = calloc( count, sizeof( zdj_library_menu_row_t ) );
    snprintf( _sql, sizeof( _sql ), "SELECT * from Menu_Query where artist=\'%s\'", ZDJ_RECORDING_ARTIST );
    int row = 0;
    sqlite3_stmt * r_stmt = zdj_sql_prep_row_stepper( (char*)&_sql, zdj_library_db );
    if( r_stmt ) {
        while ( ( res = sqlite3_step( r_stmt ) ) == SQLITE_ROW ) {
            if( row < count ) {
                char * eid = (char*)sqlite3_column_text ( r_stmt, 0 );
                if( eid ) { strcpy( rows[ row ].song_entity_id, eid ); }
                char * artist = (char*)sqlite3_column_text ( r_stmt, 1 );
                if( artist ) { strcpy( rows[ row ].artist, artist ); }
                char * title = (char*)sqlite3_column_text ( r_stmt, 2 );
                if( title ) { strcpy( rows[ row ].title, title ); }
                rows[ row ].bpm = sqlite3_column_double ( r_stmt, 3 );
                rows[ row ].key = sqlite3_column_int ( r_stmt, 4 );
                row++;
            }
        }
        sqlite3_finalize( r_stmt );
    }
    
    // Make song graphs, then delete the records
    for( int r=0; r<count; r++ ) {
        zdj_library_song_t * song = zdj_library_fetch_edit_song_graph( 
            rows[ r ].song_entity_id, zdj_library_db 
        );
        if( song ) { zdj_library_delete_song_graph( song, zdj_library_db ); }
        zdj_library_free_song_dto( song );
    }

    // zdj_library_song_t ** songs = calloc( recording_count, sizeof( zdj_library_song_t* ) );
    // zdj_error_type_t err = zdj_library_query_songs_by_artist(
    //     library_entity_id,
    //     ZDJ_RECORDING_ARTIST,
    //     songs,
    //     recording_count,
    //     zdj_library_db
    // );
    

    printf( "clearing recordings folder\n" );
    zdj_fs_remove_dir( ZDJ_RECORDING_DIR );
    zdj_fs_mkdir_p( ZDJ_RECORDING_DIR );
    sync( );

    // Reset recording counter
    zdj_setting_set_int( ZDJ_SETTING_RECORDING_COUNTER, 0 );
}

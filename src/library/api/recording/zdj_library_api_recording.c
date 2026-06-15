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
    int recording_count = zdj_library_query_count_songs_by_artist( 
        library_entity_id, ZDJ_RECORDING_ARTIST, zdj_library_db 
    );

    zdj_library_song_t ** songs = calloc( recording_count, sizeof( zdj_library_song_t* ) );
    zdj_error_type_t err = zdj_library_query_songs_by_artist(
        library_entity_id,
        ZDJ_RECORDING_ARTIST,
        songs,
        recording_count,
        zdj_library_db
    );

    // printf( "found %d songs for %s\n", recording_count, artists[ i ] );

    // Delete all records
    char sql[ 1024 ];
    for( int s=0; s<recording_count; s++ ) {
        // printf( "deleting recording: %p\n", songs[ s ] );
        zdj_library_delete_song_graph( songs[ s ], zdj_library_db );
    }

    printf( "clearing recordings folder\n" );
    zdj_fs_remove_dir( ZDJ_RECORDING_DIR );
    zdj_fs_mkdir_p( ZDJ_RECORDING_DIR );
    sync( );

    // Reset recording counter
    zdj_setting_set_int( ZDJ_SETTING_RECORDING_COUNTER, 0 );
}

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
#include <zerodj/system/log/zdj_log.h>
#include <zerodj/system/sql/zdj_sql.h>

// Song graph population/persist routines.
// A song DTO serves many use cases, each requiring a different subset
// of the total data in the database.
// To avoid expensive logical joins when not needed, only join a minimum
// amount of data to a song suitable to a specific task.
// These ops are non-destructive and cumulative.  So a song DTO which
// was previously populated for listing in a menu, can be re-populated
// for playback upon selection from library, for ex.

// Populate song graph dto for import from file

// Populate song graph dto for import from RB xml lib

// Populate song graph dto for listing in menu

// Populate song graph dto for playback

// Populate song graph dto for audio+metadata editing


// Crawl a song + it's linked data records, persisting everything to db
zdj_health_status_t zdj_library_store_song_graph( 
    zdj_library_song_t * song, 
    sqlite3 * db
) {
    // printf( "zdj_library_store_song_graph: %p/%s - %s %s %s %s\n", song, song->entity_id, song->audio->entity_id, song->catalog->entity_id, song->curation->entity_id, song->performance->entity_id );
    if( !song ) { return ZDJ_HEALTH_STATUS_MISSING_SONG; }

    zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Storing song graph[%s]: %s", song->entity_id, song->catalog->title );
    
    // Insert record into db.
    if( song->audio ) {
        zdj_health_status_t res = zdj_library_store_audio( song->audio, db );
        if( res != ZDJ_HEALTH_STATUS_OKAY ) {
            song->has_db_error = true;
            zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to store audio[%s]", song->entity_id );
        }
    }
    if( song->catalog ) {
        zdj_health_status_t res = zdj_library_store_catalog( song->catalog, db );
        if( res != ZDJ_HEALTH_STATUS_OKAY ) {
            song->has_db_error = true;
            zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to store catalog[%s]: %s", song->entity_id, song->catalog->title );
        }
    }

    if( song->curation ) {
        zdj_health_status_t res = zdj_library_store_curation( song->curation, db );
        if( res != ZDJ_HEALTH_STATUS_OKAY ) {
            song->has_db_error = true;
            zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to store curation[%s]", song->entity_id );
        }
    }
    if( song->performance ) {
        zdj_health_status_t res = zdj_library_store_performance( song->performance, db );
        if( res != ZDJ_HEALTH_STATUS_OKAY ) {
            song->has_db_error = true;
            zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to store performance[%s]", song->entity_id );
        }
    }
    zdj_library_store_song( song, db );
    zdj_library_db_flush( );

    return ZDJ_HEALTH_STATUS_OKAY;
}

zdj_health_status_t zdj_library_delete_song_graph( 
    zdj_library_song_t * song, 
    sqlite3 * db 
) {
    if( !song ) { return ZDJ_HEALTH_STATUS_MISSING_SONG; }

    // Remove song eid from all playlists
    zdj_library_playlist_remove_song_from_all_playlists( song->entity_id );

    // Remove graph from db.
    if( song->audio ) {
        zdj_health_status_t res = zdj_library_delete_audio( song->audio, db );
        if( res != ZDJ_HEALTH_STATUS_OKAY ) {
            song->has_db_error = true;
            zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to delete audio[%s]", song->entity_id );
        }
    }
    if( song->catalog ) {
        zdj_health_status_t res = zdj_library_delete_catalog( song->catalog, db );
        if( res != ZDJ_HEALTH_STATUS_OKAY ) {
            song->has_db_error = true;
            zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to delete catalog[%s]: %s", song->entity_id, song->catalog->title );
        }
    }

    if( song->curation ) {
        zdj_health_status_t res = zdj_library_delete_curation( song->curation, db );
        if( res != ZDJ_HEALTH_STATUS_OKAY ) {
            song->has_db_error = true;
            zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to delete curation[%s]", song->entity_id );
        }
    }
    if( song->performance ) {
        zdj_health_status_t res = zdj_library_delete_performance( song->performance, db );
        if( res != ZDJ_HEALTH_STATUS_OKAY ) {
            song->has_db_error = true;
            zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to delete performance[%s]", song->entity_id );
        }
    }
    if( song->has_db_error ) {
        zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to delete song graph[%s]", song->entity_id );
    } else {
        zdj_library_delete_song( song, db );
    }
    
    return ZDJ_HEALTH_STATUS_OKAY;
}

zdj_library_song_t * zdj_library_create_file_import_song_graph( 
    char * filepath, 
    sqlite3 * db 
) {
    // Make song
    zdj_library_song_t * song = zdj_library_create_song_dto( );
    
    // Make audio data
    zdj_library_audio_t * audio = zdj_library_create_audio_dto( );
    strcpy( audio->filepath, filepath );
    strcpy( song->current_audio_entity_id, audio->entity_id );
    strcpy( audio->song_entity_id, song->entity_id );
    song->audio = audio;

    // Capture missing file error
    if( access( filepath, F_OK ) != 0 ) { 
        zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_MSG, "New import graph missing: %s", filepath );
        song->has_error = true;
        song->error_flags |= 0x1 << ZDJ_LIBRARY_SONG_ERROR_FLAG_FILE_MISSING;
    }
    
    // Make catalog data
    zdj_library_catalog_t * catalog = zdj_library_create_catalog_dto( );
    strcpy( song->current_catalog_entity_id, catalog->entity_id );
    strcpy( catalog->song_entity_id, song->entity_id );
    song->catalog = catalog;
    
    // Make curation data
    zdj_library_curation_t * curation = zdj_library_create_curation_dto( );
    strcpy( song->current_curation_entity_id, curation->entity_id );
    strcpy( curation->parent_entity_id, song->entity_id );
    song->curation = curation;
    
    // Make performance data
    zdj_library_performance_t * performance = zdj_library_create_performance_dto( );
    strcpy( song->current_performance_entity_id, performance->entity_id );
    strcpy( performance->song_entity_id, song->entity_id );
    song->performance = performance;
    
    return song;
}

zdj_library_song_t * zdj_library_fetch_file_import_song_graph( 
    char * song_entity_id, 
    sqlite3 * db 
) {
    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_DB;
    // printf( "zdj_library_fetch_file_import_song_graph\n" );

    // Fetch Song DTO
    zdj_library_song_t * song = zdj_library_fetch_song_dto_for_entity_id( song_entity_id, db );
    if( !song ) { return NULL; }

    // Fetch Audio DTO
    song->audio = zdj_library_fetch_current_audio_dto_for_song( song, db );
    if( !song->audio ) {
        zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to load audio[%s]", song->entity_id );
        song->has_db_error = true;
    }
    // Fetch Catalog DTO - it may not exist during first stages of import scan
    song->catalog = zdj_library_fetch_current_catalog_dto_for_song( song, db );
    if( !song->catalog ) {
        zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to load catalog[%s]", song->entity_id );
    }
    // Fetch Curation DTO - Force refresh, even if it exists already.
    // No point in fetching curation links during song import - won't be any 
    song->curation = zdj_library_fetch_current_curation_dto_for_song( song, db );
    if( !song->curation ) {
        zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to load curation[%s]", song->entity_id );
        song->has_db_error = true;
    }

    // Fetch Performance DTO - it may not exist during first stages of import scan
    song->performance = zdj_library_fetch_current_performance_dto_for_song( song, db );
    if( !song->performance ) {
        zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to load performance[%s]", song->entity_id   );
    }

    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_UNCLAIMED;
    // Standup Graph
    return song;
}

zdj_library_song_t * zdj_library_create_rb_xml_import_song_graph( 
    char * track_id,
    sqlite3 * db 
) {
    // Make song - using the Rekordbox TrackID field as entity_id
    zdj_library_song_t * song = zdj_library_create_song_dto( );
    strcpy( song->entity_id, track_id );
    
    // Make audio data
    zdj_library_audio_t * audio = zdj_library_create_audio_dto( );
    strcpy( song->current_audio_entity_id, audio->entity_id );
    strcpy( audio->song_entity_id, song->entity_id );
    song->audio = audio;

    // Make catalog data
    zdj_library_catalog_t * catalog = zdj_library_create_catalog_dto( );
    strcpy( song->current_catalog_entity_id, catalog->entity_id );
    strcpy( catalog->song_entity_id, song->entity_id );
    song->catalog = catalog;
    
    // Make curation data
    zdj_library_curation_t * curation = zdj_library_create_curation_dto( );
    strcpy( song->current_curation_entity_id, curation->entity_id );
    strcpy( curation->parent_entity_id, song->entity_id );
    song->curation = curation;
    
    // Make performance data
    zdj_library_performance_t * performance = zdj_library_create_performance_dto( );
    strcpy( song->current_performance_entity_id, performance->entity_id );
    strcpy( performance->song_entity_id, song->entity_id );
    song->performance = performance;
    
    return song;
}

zdj_library_song_t * zdj_library_fetch_rb_xml_import_song_graph( 
    zdj_library_song_t * song, 
    sqlite3 * db 
) {

}

zdj_library_song_t * zdj_library_fetch_menu_song_graph( 
    zdj_library_song_t * song, 
    sqlite3 * db 
) {
    if( !song ) { return NULL; }

    // Fetch Audio DTO
    song->audio = zdj_library_fetch_current_audio_dto_for_song( song, db );
    if( !song->audio ) {
        zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to load audio[%s]", song->entity_id );
        song->has_db_error = true;
    }

    // Fetch Catalog DTO - it may not exist during first stages of import scan
    song->catalog = zdj_library_fetch_current_catalog_dto_for_song( song, db );
    if( !song->catalog ) {
        zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to load catalog[%s]", song->entity_id );
    }

    // Fetch Performance DTO
    song->performance = zdj_library_fetch_current_performance_dto_for_song( song, db );
    if( !song->performance ) {
        zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to load performance[%s]", song->entity_id );
    }

    // Standup Graph
    return song;
}

zdj_library_song_t * zdj_library_fetch_playback_song_graph( 
    zdj_library_song_t * song, 
    sqlite3 * db 
) {
    // Fetch Audio DTO
    if( !song->audio ){ song->audio = zdj_library_fetch_current_audio_dto_for_song( song, db ); }
    if( !song->audio ) {
        zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to load audio[%s]", song->entity_id );
        song->has_db_error = true;
    }
    
    // Fetch Catalog DTO
    if( !song->catalog ){ song->catalog = zdj_library_fetch_current_catalog_dto_for_song( song, db ); }
    if( !song->catalog ) {
        zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to load catalog[%s]", song->entity_id );
    }

    // Fetch Curation DTO
    // Curation links aren't needed during playback
    if( !song->curation ){ song->curation = zdj_library_fetch_current_curation_dto_for_song( song, db ); }
    if( !song->curation ) {
        zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to load curation[%s]", song->entity_id );
    }
    

    // Fetch Performance DTO
    if( !song->performance ){ song->performance = zdj_library_fetch_current_performance_dto_for_song( song, db ); }
    if( !song->performance ) {
        zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to load performance[%s]", song->entity_id );
    }

    // Fetch and populate cuepoints
    song->performance->cuepoint_count = zdj_library_query_count_cuepoints_for_song( song, db );
    if( song->performance->cuepoint_count > 0 ) {
        char * cuepoint_ids[ song->performance->cuepoint_count ];
        zdj_library_query_cuepoints_for_song( 
            song, cuepoint_ids, song->performance->cuepoint_count, db 
        );
        song->performance->cuepoints = malloc( 
            sizeof( zdj_library_cuepoint_t * ) * song->performance->cuepoint_count 
        );
        for( int i=0; i<song->performance->cuepoint_count; i++ ) {
            song->performance->cuepoints[ i ] = zdj_library_fetch_cuepoint_dto_for_entity_id(
                cuepoint_ids[ i ],
                db
            );
        }
    }
}

zdj_library_song_t * zdj_library_fetch_edit_song_graph( 
    char * song_entity_id, 
    sqlite3 * db 
) {
    zdj_library_song_t * song = zdj_library_fetch_song_dto_for_entity_id( song_entity_id, db );
    if( !song ) { return NULL; }

    // Fetch Audio DTO
    song->audio = zdj_library_fetch_current_audio_dto_for_song( song, db );
    if( !song->audio ) {
        zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to load audio[%s]", song->entity_id );
        song->has_db_error = true;
    }
    
    // Fetch Catalog DTO
    song->catalog = zdj_library_fetch_current_catalog_dto_for_song( song, db );
    if( !song->catalog ) {
        zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to load catalog[%s]", song->entity_id );
    }

    // Fetch Curation DTO
    song->curation = zdj_library_fetch_current_curation_dto_for_song( song, db );
    if( !song->curation ) {
        zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to load curation[%s]", song->entity_id );
    } else {
        // Populate playlist/tag links tables
        // WARNING - this is VERY expensive - use sparingly.
        zdj_library_populate_playlist_eids_for_song( song, db );
    }

    // Fetch Performance DTO
    song->performance = zdj_library_fetch_current_performance_dto_for_song( song, db );
    if( !song->performance ) {
        zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to load performance[%s]", song->entity_id );
    }

    // Standup Graph
    return song;
}

zdj_library_song_t * zdj_library_fetch_migration_song_graph( 
    char * song_entity_id, 
    sqlite3 * db 
) {
    // Fetch Song DTO
    zdj_library_song_t * song = zdj_library_fetch_song_dto_for_entity_id( song_entity_id, db );
    if( !song ) { return NULL; }

    // Fetch Audio DTO
    song->audio = zdj_library_fetch_current_audio_dto_for_song( song, db );
    if( !song->audio ) {
        zdj_log( ZDJ_LOG_LIBRARY, ZDJ_LOG_DEBUG, "Failed to load audio[%s]", song->entity_id );
        song->has_db_error = true;
    }

    // Fetch Catalog DTO - it may not exist during first stages of import scan
    song->catalog = zdj_library_fetch_current_catalog_dto_for_song( song, db );

    // Standup Graph
    return song;
}

zdj_health_status_t zdj_library_free_song_graph( 
    zdj_library_song_t * song 
) {
    if( song->audio ){ zdj_library_free_audio_dto( song->audio ); }
    if( song->catalog ){ zdj_library_free_catalog_dto( song->catalog ); }
    if( song->curation ){ zdj_library_free_curation_dto( song->curation ); }
    if( song->performance ){ zdj_library_free_performance_dto( song->performance ); }
    zdj_library_free_song_dto( song );
}
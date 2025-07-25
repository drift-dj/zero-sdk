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
#include <zerodj/sql/zdj_sql.h>

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
    if( !song ) { return ZDJ_HEALTH_STATUS_MISSING_SONG; }
    
    // Insert record into db.
    if( song->audio ) {
        zdj_health_status_t res = zdj_library_store_audio( song->audio, db );
        if( res != ZDJ_HEALTH_STATUS_OKAY ) {
            song->has_error = true;
            printf( "Library failed to store audio: %s\n", song->audio->filepath );
        }
    }
    if( song->catalog ) {
        zdj_health_status_t res = zdj_library_store_catalog( song->catalog, db );
        if( res != ZDJ_HEALTH_STATUS_OKAY ) {
            song->has_error = true;
            printf( "Library failed to store catalog: %s\n", song->catalog->title );
        }
    }
    zdj_library_store_song( song, db );
    
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
    audio->filepath = strdup( filepath );
    // Link everything together
    song->audio = audio;
    song->current_audio_entity_id = audio->entity_id;
    return song;
}

zdj_library_song_t * zdj_library_fetch_file_import_song_graph( 
    char * song_entity_id, 
    sqlite3 * db 
) {
    // Fetch Song DTO
    zdj_library_song_t * song = zdj_library_fetch_song_dto_for_entity_id( song_entity_id, db );
    if( !song ) { return NULL; }

    // Fetch Audio DTO
    song->audio = zdj_library_fetch_current_audio_dto_for_song( song, db );
    if( !song->audio ) {
        printf( "audio failed to load for %p\n", song );
        song->has_error = true;
    }

    // Fetch Catalog DTO - it may not exist during first stages of import scan
    song->catalog = zdj_library_fetch_current_catalog_dto_for_song( song, db );
    // if( !song->catalog ) {
    //     printf( "catalog failed to load for %p\n", song );
    // }
    // Standup Graph
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
        printf( "audio failed to load for %p\n", song );
        song->has_error = true;
    }

    // Fetch Catalog DTO - it may not exist during first stages of import scan
    song->catalog = zdj_library_fetch_current_catalog_dto_for_song( song, db );
    if( !song->catalog ) {
        printf( "catalog failed to load for %p\n", song );
    }
    // Standup Graph
    return song;
}

zdj_library_song_t * zdj_library_fetch_playback_song_graph( 
    zdj_library_song_t * song, 
    sqlite3 * db 
) {

}

zdj_library_song_t * zdj_library_fetch_edit_song_graph( 
    zdj_library_song_t * song, 
    sqlite3 * db 
) {

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
        printf( "audio failed to load for %p\n", song );
        song->has_error = true;
    }

    // Fetch Catalog DTO - it may not exist during first stages of import scan
    song->catalog = zdj_library_fetch_current_catalog_dto_for_song( song, db );
    // if( !song->catalog ) {
    //     printf( "catalog failed to load for %p\n", song );
    // }
    // Standup Graph
    return song;
}

zdj_health_status_t zdj_library_free_song_graph( 
    zdj_library_song_t * song 
) {

}
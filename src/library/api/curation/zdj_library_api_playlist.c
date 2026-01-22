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

zdj_library_playlist_t * zdj_library_create_playlist_dto( void ) {
    zdj_library_playlist_t * playlist = calloc( 1, sizeof( zdj_library_playlist_t ) );
    char uuid[ 64 ];
    zdj_library_put_uuid( uuid );
    snprintf( playlist->table_name, sizeof( playlist->table_name ), 
        "Playlist_%s", uuid
    );
    playlist->next = NULL;
    playlist->prev = NULL;
    return playlist;
}

zdj_library_playlist_t * zdj_library_make_playlist_dto_for_table_name( char * library_entity_id, char * playlist_table_name, sqlite3 * db ) {
    zdj_library_playlist_t * playlist = calloc( 1, sizeof( zdj_library_playlist_t ) );
    playlist->songs = NULL;

    strcpy( playlist->table_name, playlist_table_name );
    zdj_library_put_playlist_name_for_playlist( playlist, db );

    // Store total cong count
    playlist->song_count = zdj_sql_rows_in_table( playlist->table_name, NULL, db );

    if( playlist->song_count > 0 ) {
        char * song_eids[ playlist->song_count ];
        
        int res;
        int row = 0;
        char sql[ 2048 ];
        snprintf( sql, sizeof( sql ), "select * from %s", playlist->table_name );

        zdj_library_song_t * _prev_song = NULL;
        sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
        if( stmt ) {
            while ( ( res = sqlite3_step( stmt ) ) == SQLITE_ROW ) { 
                zdj_library_song_t * _song = zdj_library_fetch_song_dto_for_entity_id( 
                    (char*)sqlite3_column_text ( stmt, 0 ), db
                );
                zdj_library_fetch_menu_song_graph( _song, db );
                if( !playlist->songs ) { playlist->songs = _song; }
                if( _prev_song ) { _prev_song->next = _song; }
                _song->prev = _prev_song;
                _prev_song = _song;
            }
            sqlite3_finalize( stmt );
        }
    }
    
    return playlist;
}

void zdj_library_put_playlist_name_for_playlist( zdj_library_playlist_t * playlist, sqlite3 * db ) {
    zdj_library_t * library = zdj_library_get_current( );
    int res;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "select * from %s where table_name='%s'",
        library->playlist_links_table,
        playlist->table_name
    );

    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
    if( stmt ) {
        while ( ( res = sqlite3_step( stmt ) ) == SQLITE_ROW ) { 
            printf( "found table: %s\n", sqlite3_column_text ( stmt, 1 ) );
            strcpy( playlist->title, (char*)sqlite3_column_text ( stmt, 1 ) );
        }
        sqlite3_finalize( stmt );
    }
}

void zdj_library_free_playlist_dto( zdj_library_playlist_t * playlist ) {

}

// Add a row to the lib's playlist_link table,
// and create a new playlist table for the DTO's data.
zdj_error_type_t zdj_library_store_playlist( 
    char * library_entity_id, 
    zdj_library_playlist_t * playlist, 
    sqlite3 * db 
) {
    // printf( "zdj_library_store_playlist\n" );
    int res;
    char sql[ 4096 ];

    // Insert playlist table's name into lib's playlist_links
    snprintf( sql, sizeof( sql ), 
        // Insert new record
        "INSERT INTO Playlist_Links_%s VALUES('%s', '%s')\n"

        // Or update existing record
        "ON CONFLICT(table_name) DO UPDATE SET table_name='%s', display_name='%s'",

        // Table Name
        library_entity_id,
        playlist->table_name,
        playlist->title,
        playlist->table_name,
        playlist->title
    );
    res = zdj_sql_exec( sql, db );

    if( res != SQLITE_OK ) { return ZDJ_ERROR_LIBRARY_DB_ERROR; } 

    // Create a new playlist table
    snprintf( sql, sizeof( sql ), "CREATE TABLE IF NOT EXISTS '%s' ( 'entity_id' TEXT NOT NULL, PRIMARY KEY('entity_id'))",
        playlist->table_name 
    );
    zdj_sql_exec( (char*)&sql, db );

    if( res != SQLITE_OK ) { return ZDJ_ERROR_LIBRARY_DB_ERROR; } 

    // Insert song eids into new playlist table.
    zdj_library_song_t * song = playlist->songs;
    while( song ) {
        // Set up for prepared stmt w/binds to use built-in string escaping.
        snprintf( sql, sizeof( sql ), 
            // Insert new record
            "INSERT INTO %s VALUES('%s')\n"

            // Or update existing record
            "ON CONFLICT(entity_id) DO UPDATE SET entity_id='%s'",

            // Table Name
            playlist->table_name,
            song->entity_id,
            song->entity_id
        );
        res = zdj_sql_exec( sql, db );
        if( res != SQLITE_OK ) { return ZDJ_ERROR_LIBRARY_DB_ERROR; }

        song = song->next;
    } 
    
    // printf( "zdj_library_store_playlist done\n" );

    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t zdj_library_delete_playlist( char * library_entity_id, zdj_library_playlist_t * playlist, sqlite3 * db ) {
    int res;
    char sql[ 4096 ];
    // Remove record for playlist eid from library's playlist links table
    snprintf( sql, sizeof( sql ), "DELETE FROM Playlist_Links_%s WHERE table_name = '%s'",
        library_entity_id,
        playlist->table_name 
    );
    zdj_sql_exec( (char*)&sql, db );

    // Drop playlist's song_links table
    snprintf( sql, sizeof( sql ), "DROP TABLE IF EXISTS %s",
        playlist->table_name 
    );
    zdj_sql_exec( (char*)&sql, db );
}

void zdj_library_playlist_add_song( 
    zdj_library_playlist_t * playlist, 
    zdj_library_song_t * song, 
    sqlite3 * db 
) {
    zdj_library_song_t * _song = playlist->songs;
    if( !_song ) { playlist->songs = song; return; }
    while ( _song ) {
        if( !_song->next ) { 
            _song->next = song;
            _song = NULL;
        } else {
            _song = _song->next;
        }
    }
    playlist->song_count++;
}

void zdj_library_playlist_remove_song( 
    zdj_library_playlist_t * playlist, 
    zdj_library_song_t * song, 
    sqlite3 * db 
) {
    zdj_library_song_t * _song = playlist->songs;

    // Catch one song/1st song cases
    if( !_song->next && !_song->prev ) {
        // Only 1 song in playlist
        playlist->songs = NULL;
        playlist->song_count = 0;
        zdj_library_free_song_graph( _song );
        return;
    } else if ( _song->next && !_song->prev ) {
        // First song in playlist
        playlist->songs = _song->next;
        playlist->song_count--;
        zdj_library_free_song_graph( _song );
        return;
    }

    while ( _song ) {
        zdj_library_song_t * next_song = _song->next;
        if( !strcmp( _song->entity_id, song->entity_id ) ) {
            printf( "found matching song eid - removing %p %p\n", next_song, _song->prev );

            if( next_song != NULL ) {
                printf( "next_song: %p\n", next_song );
                next_song->prev = _song->prev; 
            }
            if( _song->prev != NULL ) { 
                printf( "song->prev: %p\n", _song->prev );
                _song->prev->next = next_song; 
            }
            zdj_library_free_song_graph( _song );
            playlist->song_count--;
        }
        _song = _song->next;
    }
}

void zdj_library_playlist_move_song( 
    zdj_library_playlist_t * playlist, 
    zdj_library_song_t * song, 
    int dir, 
    sqlite3 * db 
) {
    
}
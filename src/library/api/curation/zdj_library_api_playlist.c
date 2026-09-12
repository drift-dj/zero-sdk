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

static void _renumber_playlist_rows( zdj_library_playlist_t * playlist );

zdj_library_playlist_t * zdj_library_create_playlist_dto( void ) {
    zdj_library_playlist_t * playlist = calloc( 1, sizeof( zdj_library_playlist_t ) );
    char uuid[ 64 ];
    zdj_library_put_uuid( uuid );
    snprintf( playlist->table_name, sizeof( playlist->table_name ), 
        "Playlist_%s", uuid
    );
    playlist->row_count = 0;
    playlist->row_head = NULL;
    playlist->row_tail = NULL;
    playlist->next = NULL;
    playlist->prev = NULL;
    return playlist;
}

zdj_library_playlist_t * zdj_library_make_playlist_dto_for_table_name( char * library_entity_id, char * playlist_table_name, sqlite3 * db ) {
    printf( "zdj_library_make_playlist_dto_for_table_name: %s\n", playlist_table_name );
    zdj_library_playlist_t * playlist = calloc( 1, sizeof( zdj_library_playlist_t ) );
    playlist->row_head = NULL;
    playlist->row_tail = NULL;

    strcpy( playlist->table_name, playlist_table_name );
    zdj_library_put_playlist_name_for_playlist( playlist, db );

    // Store total cong count
    playlist->row_count = zdj_sql_rows_in_table( playlist->table_name, NULL, db );

    if( playlist->row_count > 0 ) {
        int res;
        int counter = 0;
        char sql[ 2048 ];
        snprintf( sql, sizeof( sql ), "SELECT \
            pt.entity_id, \
            pt.num, \
            cat.artist, \
            cat.title, \
            cat.genre, \
            cat.year, \
            a.filepath, \
            perf.bpm, \
            perf.key, \
            s.has_error, \
            s.error_flags \
            FROM %s pt \
            LEFT JOIN Song_Entity s ON pt.entity_id = s.entity_id \
            LEFT JOIN Audio_Data_Entity a ON s.entity_id = a.song_entity_id \
            LEFT JOIN Catalog_Data_Entity cat ON s.entity_id = cat.song_entity_id \
            LEFT JOIN Performance_Data_Entity perf ON s.entity_id = perf.song_entity_id \
            order by pt.num;",
            playlist->table_name 
        );
        
        zdj_library_menu_row_t * prev_row = NULL;
        sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
        if( stmt ) {
            while ( ( res = sqlite3_step( stmt ) ) == SQLITE_ROW ) { 
                if( counter < playlist->row_count ) {
                    zdj_library_menu_row_t * row = calloc( 1, sizeof( zdj_library_menu_row_t ) );
                    if( playlist->row_head == NULL ) { playlist->row_head = row; }
                    else { prev_row->next = row; row->prev = prev_row; }

                    char * eid = (char*)sqlite3_column_text ( stmt, 0 );
                    if( eid ) { strcpy( row->song_entity_id, eid ); }
                    row->num = sqlite3_column_int ( stmt, 1 );
                    char * artist = (char*)sqlite3_column_text ( stmt, 2 );
                    if( artist ) { strcpy( row->artist, artist ); }
                    char * title = (char*)sqlite3_column_text ( stmt, 3 );
                    if( title ) { strcpy( row->title, title ); }
                    char * genre = (char*)sqlite3_column_text ( stmt, 4 );
                    if( genre ) { strcpy( row->genre, genre ); }
                    row->year = sqlite3_column_int ( stmt, 5 );
                    char * filepath = (char*)sqlite3_column_text ( stmt, 6 );
                    if( filepath ) { strcpy( row->filepath, filepath ); }
                    row->bpm = sqlite3_column_double ( stmt, 7 );
                    row->key = sqlite3_column_int ( stmt, 8 );
                    row->has_error = sqlite3_column_int ( stmt, 9 );
                    row->error_flags = sqlite3_column_int ( stmt, 10 );

                    prev_row = row;
                    counter++;
                }
            }
            sqlite3_finalize( stmt );
        }
        playlist->row_tail = prev_row;
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
            // printf( "found table: %s\n", sqlite3_column_text ( stmt, 1 ) );
            strcpy( playlist->title, (char*)sqlite3_column_text ( stmt, 1 ) );
        }
        sqlite3_finalize( stmt );
    }
}

// Playlist DTO owns all rows
void zdj_library_free_playlist_dto( zdj_library_playlist_t * playlist, bool free_rows ) {
    if( free_rows ) {
        zdj_library_menu_row_t * _row = (zdj_library_menu_row_t*)playlist->row_head;
        while( _row ) {
            // printf( "freeing row\n" );
            zdj_library_menu_row_t * next_row = _row->next;
            free( _row );
            _row = next_row;
        } 
    }
    free( playlist );
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

    // Set up for prepared stmt w/binds to use built-in string escaping.
    snprintf( sql, sizeof( sql ), 
        // Insert new record
        "INSERT INTO Playlist_Links (table_name,display_name) VALUES(?,?)\n"
        // Or update existing record
        "ON CONFLICT(table_name) DO UPDATE SET table_name=?,display_name=?"
    );

    sqlite3_stmt * store_stmt;

    res = sqlite3_prepare_v2( db, sql, -1, &store_stmt, 0 );
    if ( res != SQLITE_OK ) {
        printf("\nCould not prepare statement: (%d) %s\n", res, sql);
        return ZDJ_ERROR_LIBRARY_DB_ERROR;
    }

    // Insert Binds
    if( sqlite3_bind_text( store_stmt, 1, playlist->table_name, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_ERROR_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( store_stmt, 2, playlist->title, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_ERROR_LIBRARY_DB_ERROR;
    }    

    // Update Binds
    if( sqlite3_bind_text( store_stmt, 3, playlist->table_name, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_ERROR_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( store_stmt, 4, playlist->title, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_ERROR_LIBRARY_DB_ERROR;
    }
    
    if ( sqlite3_step( store_stmt ) != SQLITE_DONE ) {
        printf( "\nCould not step (execute) stmt.\n" );
        return ZDJ_ERROR_LIBRARY_DB_ERROR;
    }
    
    sqlite3_finalize( store_stmt );

    zdj_sql_db_flush( db );

    // Create a new playlist table
    snprintf( sql, sizeof( sql ), "CREATE TABLE IF NOT EXISTS '%s' ( 'entity_id' TEXT NOT NULL, 'num' INTEGER, PRIMARY KEY('entity_id'))",
        playlist->table_name 
    );
    zdj_sql_exec( (char*)&sql, db );

    // Insert song eids into new playlist table.
    zdj_library_menu_row_t * row = (zdj_library_menu_row_t*)playlist->row_head;

    res = zdj_sql_exec( "BEGIN TRANSACTION;", db );
    if( res != SQLITE_OK ) { return ZDJ_ERROR_LIBRARY_DB_ERROR; }

    while( row ) {
        // Ignore rows for songs which don't exist
        if( zdj_library_query_song_eid_exists( row->song_entity_id, db ) ) {
            // Set up for prepared stmt w/binds to use built-in string escaping.
            // Group multiple 'upserts' together to reduce disk caching bottleneck
            snprintf( sql, sizeof( sql ), 
                // Insert new record
                "INSERT INTO %s VALUES('%s', %d)\n"

                // Or update existing record
                "ON CONFLICT(entity_id) DO UPDATE SET entity_id='%s', num=%d",

                // Table Name
                playlist->table_name,
                row->song_entity_id,
                row->num,
                row->song_entity_id,
                row->num
            );
            res = zdj_sql_exec( sql, db );
            if( res != SQLITE_OK ) { return ZDJ_ERROR_LIBRARY_DB_ERROR; }
        }

        row = row->next;
    }

    res = zdj_sql_exec( "COMMIT;", db );
    if( res != SQLITE_OK ) { return ZDJ_ERROR_LIBRARY_DB_ERROR; }
    
    // printf( "zdj_library_store_playlist done\n" );

    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t zdj_library_delete_playlist( char * library_entity_id, zdj_library_playlist_t * playlist, sqlite3 * db ) {
    int res;
    char sql[ 4096 ];
    // Remove record for playlist eid from library's playlist links table
    snprintf( sql, sizeof( sql ), "DELETE FROM Playlist_Links WHERE table_name = '%s'",
        playlist->table_name 
    );
    zdj_sql_exec( (char*)&sql, db );

    // Drop playlist's song_links table
    snprintf( sql, sizeof( sql ), "DROP TABLE IF EXISTS %s",
        playlist->table_name 
    );
    zdj_sql_exec( (char*)&sql, db );
}

zdj_error_type_t zdj_library_delete_playlist_named( 
	char * playlist_table_name, sqlite3 * db 
) {
    int res;
    if( !playlist_table_name ){ return ZDJ_ERROR_OKAY; }
    char sql[ 4096 ];
    // Remove record for playlist eid from library's playlist links table
    snprintf( sql, sizeof( sql ), "DELETE FROM Playlist_Links WHERE table_name = '%s'",
        playlist_table_name 
    );
    zdj_sql_exec( (char*)&sql, db );

    // Drop playlist's song_links table
    snprintf( sql, sizeof( sql ), "DROP TABLE IF EXISTS %s",
        playlist_table_name 
    );
    zdj_sql_exec( (char*)&sql, db );
}

bool zdj_library_playlist_title_exists_in_db( char * title, char * table_name, sqlite3 * db ) {
    bool found = false;
    int res;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "select table_name from Playlist_Links where display_name='%s'",
        title
    );

    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
    if( stmt ) {
        while ( ( res = sqlite3_step( stmt ) ) == SQLITE_ROW ) { 
            found = true;
            char * table_name_str = (char*)sqlite3_column_text ( stmt, 0 );
            if( table_name_str ) { strcpy( table_name, table_name_str ); }
        }
        sqlite3_finalize( stmt );
    }
    return found;
}

void zdj_library_merge_playlist( zdj_library_playlist_t * playlist, char * merge_source, sqlite3 * db ) {
    // Currently hard-coded to replace until merge logic is determined
    zdj_library_replace_playlist( playlist, merge_source, db );
}

void zdj_library_replace_playlist( zdj_library_playlist_t * playlist, char * target_table_name, sqlite3 * db ) {
    // Steal the existing table name
    strcpy( playlist->table_name, target_table_name );

    // Delete existing Playlist table
    zdj_library_delete_playlist_named( target_table_name, db );
}

void zdj_library_playlist_append_row( 
    zdj_library_playlist_t * playlist, 
    zdj_library_menu_row_t * row
) {
    // printf( "zdj_library_playlist_append_row: %p\n", row );
    // Copy the row since we don't know who owns the mem
    zdj_library_menu_row_t * _row = malloc( sizeof( zdj_library_menu_row_t ) );
    memcpy( _row, row, sizeof( zdj_library_menu_row_t ) );
    _row->next = NULL;
    _row->prev = NULL;

    if( !playlist->row_head ) { 
        playlist->row_head = _row;
        playlist->row_tail = _row;
    } else {
        zdj_library_menu_row_t * _row_tail = (zdj_library_menu_row_t*)playlist->row_tail;
        _row_tail->next = _row;
        _row->prev = _row_tail;
        playlist->row_tail = _row;
    }
    playlist->row_count++;
}

void zdj_library_playlist_remove_row( 
    zdj_library_playlist_t * playlist, 
    zdj_library_menu_row_t * row
) {
    // TEMPORARY - Crash to indicate dev isn't finished
    // Need to update tail logic
    exit( 1 );

    zdj_library_menu_row_t * _row_head = (zdj_library_menu_row_t*)playlist->row_head;
    if( _row_head == NULL ) {
        // error - trying to remove a row from an empty list
        return;
    } 
    
    // This row may not be owned by playlist
    // Scan thru playlist's rows to find matching row
    zdj_library_menu_row_t * local_row = NULL;
    while( _row_head ) {
        // Compare rows by string-matching song eid
        if( strcmp( row->song_entity_id, _row_head->song_entity_id ) == 0 ) { 
            local_row = _row_head;
            break; 
        }
    }

    // No matching row - bug out...
    if( !local_row ) { return; }

    // Rewind row_head to first row
    _row_head = (zdj_library_menu_row_t*)playlist->row_head;
    // Remove first row
    if( _row_head == local_row ) {
        // Remove first and only row in playlist
        if( !_row_head->next ) {
            playlist->row_head = NULL;
            playlist->row_tail = NULL;
            playlist->row_count = 0;
            return;
        
        // Remove first row and stitch next row to first
        } else {
            _row_head->next->prev = NULL;
            playlist->row_head = _row_head->next;
            _renumber_playlist_rows( playlist );
            return;
        }

    // Loop thru rows to find and remove a middle/end row
    } else {
        while( _row_head ) {
            if( strcmp( _row_head->song_entity_id, local_row->song_entity_id ) == 0 ) {
                if( _row_head->prev ) { _row_head->prev->next = _row_head->next; }
                if( _row_head->next ) { _row_head->next->prev = _row_head->prev; }
                _renumber_playlist_rows( playlist );
                return;
            }
            _row_head = _row_head->next;
        }        
    }
}

// Move a song to a new index in a playlist
void zdj_library_playlist_move_row( 
    zdj_library_playlist_t * playlist, 
    zdj_library_menu_row_t * row, 
    int index
) {
    // TEMPORARY - Crash to indicate dev isn't finished
    // Need to update tail logic
    exit( 1 );

    // If index > row count, just append the row
    if( index >= playlist->row_count ) { zdj_library_playlist_append_row( playlist, row ); return; }

    // Scan the linked list of rows to find one matching the index
    zdj_library_menu_row_t * _row_head = (zdj_library_menu_row_t*)playlist->row_head;
    int counter = 0;
    while( _row_head ) {
        // Ignore rows before index
        if( _row_head->num == index ) {
            if( _row_head->prev ) { _row_head->prev->next = row; }
            if( _row_head->next ) { _row_head->next->prev = row; }
            row->prev = _row_head->prev;
            row->next = _row_head->next;
            row->num = index;
            counter = index + 1;
            // loop forward thru rest of rows
            _row_head = row->next;
            while( _row_head ) {
                _row_head->num = counter;
                _row_head = _row_head->next;
                counter++;
            }
            // Exit loop
            break;
        }
    }
}

// Traverse linked list of rows and renumber them in order
static void _renumber_playlist_rows( zdj_library_playlist_t * playlist ) {
    zdj_library_menu_row_t * _row_head = (zdj_library_menu_row_t*)playlist->row_head;
    int counter = 0;

    while( _row_head ) {
        _row_head->num = counter;
        counter++;
        _row_head = _row_head->next;
    }

    playlist->row_count = counter;
}
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sqlite3.h>

#include <zerodj/health/zdj_health_type.h>
#include <zerodj/library/zdj_library.h>
#include <zerodj/sql/zdj_sql.h>

static char _sql[ 1024 ];

int zdj_library_count_songs( int library_entity_id ) {
    return 0;
}


// Import API
// Assume each of these are running in their own thread.
// No persistent codec (ffmpeg) will be available - make and release for each invocation.

// Build an initial record for a path. 
zdj_library_song_t * zdj_library_import_song_for_filepath( char * path, sqlite3 * db ) {
    // Get a type based on file extension - fail if extension not recognized.

    // Stand up codec
    // Attempt to open/read header to match extension - fail if unreadable/mismatch.

    // Read inital metadata into new song instance.

    // Insert record into db.

    // Tear down codec.
}

// zdj_library_song_t * zdj_library_import_song_for_rekordbox_node( xmlNode * node ) {

// }
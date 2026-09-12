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

int zdj_library_query_count_all_playlists( 
	char * library_entity_id, 
	sqlite3 * db 
) {
	printf( "zdj_library_query_count_all_playlists\n" );
    return 0;
}

zdj_error_type_t zdj_library_query_all_playlists( 
	char * library_entity_id, 
	char ** playlist_eids, 
	int count,
	sqlite3 * db 
) {
	return ZDJ_ERROR_OKAY;
}

int zdj_library_query_count_playlists_for_song(
	char * library_entity_id, 
	zdj_library_song_t * song,
	sqlite3 * db
) {
	return 0;
}
zdj_error_type_t zdj_library_query_playlists_for_song(
	char * library_entity_id, 
	char ** playlist_eids, 
	int count,
	zdj_library_song_t * song,
	sqlite3 * db
) {
	return ZDJ_ERROR_OKAY;
}
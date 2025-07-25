#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <sqlite3.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <mutils/mhash.h>

#include <zerodj/health/zdj_health_type.h>
#include <zerodj/library/zdj_library.h>
#include <zerodj/sql/zdj_sql.h>

zdj_library_audio_t * zdj_library_create_audio_dto( void ) {
    zdj_library_audio_t * audio = calloc( 1, sizeof( zdj_library_audio_t ) );
    audio->entity_id = zdj_library_get_uuid( );
    return audio;
}

zdj_library_audio_t * zdj_library_fetch_current_audio_dto_for_song( 
    zdj_library_song_t * song, 
    sqlite3 * db 
) {
    int res;
    char sql[ 2048 ];
    snprintf( sql, sizeof( sql ), "select * from %s where entity_id like \'%s\'", 
        ZDJ_LIBRARY_TABLE_AUDIO_DATA,
        song->current_audio_entity_id
    );

    int _eid_col = 0;
    int _dseid_col = 1;
    int _fp_col = 2;
    int _fcs_col = 3;
    int _hpe_col = 4;
    int _pef_col = 5;
    int _cid_col = 6;
    int _sid_col = 7;
    int _br_col = 8;
    int _sf_col = 9;
    int _cc_col = 10;
    int _dur_col = 11;
    zdj_library_audio_t * audio = NULL;

    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( (char*)&sql, db );
    if( stmt ) {
        while ( ( res = sqlite3_step( stmt ) ) == SQLITE_ROW ) { 
            audio = calloc( 1, sizeof( zdj_library_audio_t ) );
            audio->entity_id = strdup( (char*)sqlite3_column_text ( stmt, _eid_col ) );
            char * data_source_entity_id = (char*)sqlite3_column_text ( stmt, _dseid_col );
            if( data_source_entity_id ) { audio->data_source_entity_id = strdup( data_source_entity_id ); }
            audio->filepath = strdup( (char*)sqlite3_column_text ( stmt, _fp_col ) );
            char * file_checksum = (char*)sqlite3_column_text ( stmt, _fcs_col );
            if( file_checksum ) { audio->file_checksum = strdup( file_checksum ); }
            audio->has_procedural_edit = sqlite3_column_int ( stmt, _hpe_col );
            char * procedural_edit_filepath = (char*)sqlite3_column_text ( stmt, _pef_col );
            if( procedural_edit_filepath ) { audio->procedural_edit_filepath = strdup( procedural_edit_filepath ); }
            audio->av_codec_id = sqlite3_column_int ( stmt, _cid_col );
            audio->av_stream_index = sqlite3_column_int ( stmt, _sid_col );
            audio->av_sample_rate = sqlite3_column_int ( stmt, _br_col );
            audio->av_sample_format = sqlite3_column_int ( stmt, _sf_col );
            audio->av_channel_count = sqlite3_column_int ( stmt, _cc_col );
            audio->duration = sqlite3_column_double ( stmt, _dur_col );
        }
        sqlite3_finalize( stmt );
    }

    return audio;
}

zdj_health_status_t zdj_library_free_audio_dto( 
    zdj_library_audio_t * audio 
) {

}

zdj_health_status_t zdj_library_store_audio( 
    zdj_library_audio_t * audio, 
    sqlite3 * db 
) {
    int count = 0;
    int res;
    char sql[ 4096 ];
    // Set up for prepared stmt w/binds to use built-in string escaping.
    snprintf( sql, sizeof( sql ), 
        // Insert new record
        "INSERT INTO %s VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?)\n"

        // Or update existing record
        "ON CONFLICT(entity_id) DO UPDATE SET entity_id=?,data_source_entity_id=?,filepath=?,file_checksum=?,has_procedural_edit=?,procedural_edit_filepath=?,av_codec_id=?,av_stream_index=?,av_sample_rate=?,av_sample_format=?,av_channel_count=?,duration=?,error=?",

        // Table Name
        ZDJ_LIBRARY_TABLE_AUDIO_DATA
    );

    if ( sqlite3_prepare( db, (char*)&sql, -1, &audio->store_stmt, 0 ) != SQLITE_OK ) {
        printf("\nCould not prepare statement: %s\n", (char*)&sql);
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }

    // Insert Binds
    if( sqlite3_bind_text( audio->store_stmt, 1, audio->entity_id, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( audio->store_stmt, 2, audio->data_source_entity_id, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( audio->store_stmt, 3, audio->filepath, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( audio->store_stmt, 4, audio->file_checksum, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_int( audio->store_stmt, 5, audio->has_procedural_edit ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( audio->store_stmt, 6, audio->procedural_edit_filepath, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_int( audio->store_stmt, 7, audio->av_codec_id ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_int( audio->store_stmt, 8, audio->av_stream_index ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_int( audio->store_stmt, 9, audio->av_sample_rate ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_int( audio->store_stmt, 10, audio->av_sample_format ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_int( audio->store_stmt, 11, audio->av_channel_count ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_double( audio->store_stmt, 12, audio->duration ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_int( audio->store_stmt, 13, audio->error ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }

    // Update Binds
    if( sqlite3_bind_text( audio->store_stmt, 14, audio->entity_id, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( audio->store_stmt, 15, audio->data_source_entity_id, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( audio->store_stmt, 16, audio->filepath, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( audio->store_stmt, 17, audio->file_checksum, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_int( audio->store_stmt, 18, audio->has_procedural_edit ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_text( audio->store_stmt, 19, audio->procedural_edit_filepath, -1, SQLITE_STATIC ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_int( audio->store_stmt, 20, audio->av_codec_id ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_int( audio->store_stmt, 21, audio->av_stream_index ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_int( audio->store_stmt, 22, audio->av_sample_rate ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_int( audio->store_stmt, 23, audio->av_sample_format ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_int( audio->store_stmt, 24, audio->av_channel_count ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_double( audio->store_stmt, 25, audio->duration ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    if( sqlite3_bind_int( audio->store_stmt, 26, audio->error ) != SQLITE_OK ) {
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }

    if ( sqlite3_step( audio->store_stmt ) != SQLITE_DONE ) {
        printf( "\nCould not step (execute) stmt.\n" );
        return ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR;
    }
    
    sqlite3_finalize( audio->store_stmt );

    return ZDJ_HEALTH_STATUS_OKAY;
}

// Open audio's filepath in FFmpeg and attempt to read encoding data into audio DTO.
zdj_error_type_t zdj_library_audio_parse_encoding_info( zdj_library_audio_t * audio ) {
    return ZDJ_ERROR_OKAY;
}

bool zdj_library_audio_is_raw_pcm( zdj_library_audio_t * audio ) {
    return ( audio->av_codec_id >= AV_CODEC_ID_PCM_S16LE ) &&
           ( audio->av_codec_id <= AV_CODEC_ID_PCM_SGA );
}

char * zdj_library_audio_file_crc( char * path ) {
    // printf( "zdj_library_audio_file_crc\n" );
    int i;
    int hash_block_size = 64000;
    unsigned char buffer[ 64000 ];

    // Stand up mhash
    MHASH td = mhash_init( MHASH_CRC32B );
    if ( td == MHASH_FAILED ){ return NULL; }

    // Stand up file
    FILE * fd = fopen( path, "r" );
    if( !fd ) { return NULL; }

    // Run file thru the checksum generator
    while ( fread( &buffer, hash_block_size, 1, fd ) == 1 ) {
        mhash( td, &buffer, hash_block_size );
    }
    unsigned char * hash_out;
    char result[ mhash_get_block_size( MHASH_CRC32B ) * 2 + 1 ];
    hash_out = mhash_end( td );
    for (i = 0; i < mhash_get_block_size( MHASH_CRC32B ); i++) {
        sprintf( &result[ i*2 ], "%.2x", hash_out[ i ] );
    }
    return strdup( result );
}
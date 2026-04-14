#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include <mutils/mhash.h>

#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/system/hash/zdj_hash.h>

int zdj_put_file_hash( char * filepath, char * str ) {
    // printf( "zdj_library_audio_file_crc\n" );
    int i;
    int hash_block_size = 64000;
    unsigned char buffer[ 64000 ];

    // Stand up mhash
    MHASH td = mhash_init( MHASH_CRC32B );
    // MHASH td = mhash_init( MHASH_CRC32 );
    if ( td == MHASH_FAILED ){ return -1; }

    // Stand up file
    FILE * fd = fopen( filepath, "r" );
    if( !fd ) { return -1; }

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

    fclose( fd );
    // Copy the result to the target string
    strcpy( str, result );

    return 0;
}


int zdj_put_crc32_file_hash( char * filepath, char * str ) {
    // printf( "zdj_put_crc32_file_hash: %s\n", filepath );
    // Call out to the commandline crc32 algo - mHash won't match
    // the hashes created during zero-build ops
    char cmd[ 256 ];
    char res[ 128 ];
    sprintf( cmd, "crc32 %s", filepath );
    zdj_fs_get_popen( cmd, res );
    // Only get the frist 8 chars of the result
    snprintf( str, 9, "%s", res );
    return 0;
}


bool zdj_hashes_match( char * hash_a, char * hash_b ) {
    if( !strcmp( hash_a, hash_b ) ) {
        return true;
    } else {
        return false;
    }
}
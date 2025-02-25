#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>


#include <zerodj/fs/zdj_fs.h>

bool _zdj_fs_filename_match( char * filename, zdj_fs_scan_pattern_t * pattern );

void zdj_fs_scan_dir( 
    char * path,
    bool recursive,
    zdj_fs_scan_pattern_t * pattern,
    zdj_fs_result_cb result_cb
) {
    DIR *dir;
    struct dirent *entry;
    int id = 0;

    // Don't scan non-dir paths.
    if ( !( dir = opendir( path ) ) ) { return; }

    // Look at every entry in dir.
    while ( ( entry = readdir( dir ) ) != NULL ) {
        if ( entry->d_type == DT_DIR ) {
            // We've found a directory. 
            // If this is not a recursive scan, skip to the next entry in dir.
            if( !recursive ) { continue; }

            // Skip .+.. dirs
            if ( !strcmp( entry->d_name, "." ) || !strcmp( entry->d_name, ".." ) ) { continue; }
            
            // Build a path string and recurse into dir.
            char new_path[ 2048 ];
            snprintf( new_path, sizeof( new_path ), "%s%s", path, entry->d_name );
            zdj_fs_scan_dir( new_path, recursive, pattern, result_cb );
        } else {
            // Disregard invisible files
            if( entry->d_name[ 0 ] == '.' ) continue;

            // Invoke CB for matches
            if( _zdj_fs_filename_match( entry->d_name, pattern ) ) {
                char new_path[ 2048 ];
                snprintf( new_path, sizeof( new_path ), "%s%s", path, entry->d_name );
                result_cb( new_path );
            }            
        }
    }
    closedir( dir );
}

bool _zdj_fs_filename_match( char * filename, zdj_fs_scan_pattern_t * pattern ) {
    // No pattern means '*'
    if( !pattern ) { return true; }

    return false;
}

int zdj_fs_get_size( char * filepath ) {
    // Get file length in bytes
    FILE * fp = fopen( filepath, "r" );
    fseek( fp, 0, SEEK_END );
    int file_len = ftell( fp );
    fclose( fp );
    return file_len;
}
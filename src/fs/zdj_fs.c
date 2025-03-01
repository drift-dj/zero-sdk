#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <libgen.h>
#include <ftw.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <errno.h>

#include <zerodj/fs/zdj_fs.h>
#include <zerodj/health/zdj_health_type.h>

bool _zdj_fs_filename_match( char * filename, zdj_fs_scan_pattern_t * pattern );
static int _zdj_maybe_mkdir(const char* path, mode_t mode);

// Copy a file from source to destination.
zdj_health_status_t zdj_fs_copy_file( char * src, char * dst, bool overwrite ) {
    printf( "zdj_fs_copy_file: %s -> %s, o/w: %d\n", src, dst, overwrite );
    // TODO - Add free space check

    // Confirm source file exists.
    FILE * src_fd = fopen( src, "r" );
    if( !src_fd ) { return ZDJ_HEALTH_STATUS_FILE_MISSING; }

    if( overwrite && ( access(dst, F_OK) == 0 ) ) {
        // If overwrite, remove dest file if it exists and perms are okay.
        remove( dst );
    } else if( !overwrite && ( access(dst, F_OK) == 0 ) ) {
        // If no overwrite, return error if dest file exists.
        return ZDJ_HEALTH_STATUS_FILE_EXISTS;
    }
    
    // Ensure dest dir exists
    char * tmp = strdup( dst );
    char * dir_name = dirname( tmp );
    zdj_fs_mkdir_p( dir_name );
    free( tmp );

    // Copy bytes from source to destination
    FILE * dst_fd = fopen( dst, "w" );
    printf( "opend dst_fd: %s, %p\n", dst, dst_fd );
    if( !dst_fd ) { return ZDJ_HEALTH_STATUS_BAD_DIR; }
    int a;
    while ( ( a = fgetc( src_fd ) ) != EOF ) {
        fputc( a, dst_fd );
    }

    // Set permissions for new file
    printf( "chmod 0744 %s\n", dst );
    if( chmod( dst, 0744 ) == -1 ) {
        return ZDJ_HEALTH_STATUS_BAD_PERMS;
    }

    fclose( dst_fd );
    fclose( src_fd );
    
    return ZDJ_HEALTH_STATUS_OKAY;
}

zdj_health_status_t zdj_fs_extract_file_from_binary( 
    char * bin, 
    char * dst, 
    unsigned long long offset, 
    unsigned long long len, 
    bool overwrite
) {
    // Delete existing if overwrite
    if( overwrite ) { remove( dst ); }

    // TODO - Add free space check

    FILE * bin_fd = fopen( bin, "r" );
    if( !bin_fd ) { return ZDJ_HEALTH_STATUS_MISSING; }
    fseek( bin_fd, offset, SEEK_SET );
    FILE * dst_fd = fopen( dst, "w" );
    if( !dst_fd ) { return ZDJ_HEALTH_STATUS_BAD_DIR; }
    int i=0;
    char a;
    while ( i++ < len ) {
        a = fgetc( bin_fd );
        fputc( a, dst_fd );
    }
    fclose( dst_fd );
    fclose( bin_fd );

    return ZDJ_HEALTH_STATUS_OKAY;
}

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

int zdj_fs_mkdir_p( char * path ) {
    printf( "zdj_fs_mkdir_p: %s\n", path );
    /* Adapted from http://stackoverflow.com/a/2336245/119527 */
    char * _path = NULL;
    char * p; 
    int result = -1;
    mode_t mode = 0777;
    errno = 0;

    /* Copy string so it's mutable */
    _path = strdup( path );
    if ( _path == NULL ) { goto out; }

    /* Iterate the string */
    for ( p=_path+1; *p; p++ ) {
        if ( *p == '/' ) {
            /* Temporarily truncate */
            *p = '\0';
            if ( _zdj_maybe_mkdir(_path, mode) != 0 ) { goto out; }
            *p = '/';
        }
    } 

    if ( _zdj_maybe_mkdir( _path, mode ) != 0 ) { goto out; }
    result = 0;

    out:
    free(_path);
    return result;
}

// Make a directory; already existing dir okay
static int _zdj_maybe_mkdir(const char* path, mode_t mode) {
    printf( "_zdj_maybe_mkdir: %s\n", path );
    struct stat st;
    errno = 0;
   
    // Try to make the directory
    if (mkdir(path, mode) == 0) { return 0; }
    
    // If it fails for any reason but EEXIST, fail
    if (errno != EEXIST) { return -1; }
    
    // Check if the existing path is a directory
    if (stat(path, &st) != 0) { return -1; }
    
    // If not, fail with ENOTDIR
    if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        return -1;
    }
    errno = 0;
    return 0;
}

int _zdj_fs_remove_dir_nftw(
    const char *filename, 
    const struct stat *statptr,
    int fileflags, 
    struct FTW *pfwt
) {
    int rv = remove( filename );
    if ( rv ) { perror( filename ); }
    return rv;
}

void zdj_fs_remove_dir( char * path ) {
    int fd_limit = 5;
    int flags = FTW_CHDIR | FTW_DEPTH | FTW_MOUNT;
    int ret;
    ret = nftw( path, _zdj_fs_remove_dir_nftw, fd_limit, flags );
}

bool _zdj_fs_filename_match( char * filename, zdj_fs_scan_pattern_t * pattern ) {
    // No pattern means '*'
    if( !pattern ) { return true; }

    return false;
}

int zdj_fs_get_size( char * filepath ) {
    // Get file length in bytes
    FILE * fp = fopen( filepath, "r" );
    if( !fp ){ return 0; }
    fseek( fp, 0, SEEK_END );
    int file_len = ftell( fp );
    fclose( fp );
    return file_len;
}

// Available space on system drive
unsigned long long zdj_fs_sys_space( void ) {
    struct statvfs vfs;
    char drive[ ] = "/usr/";

    if ( statvfs( drive, &vfs ) == 0 ) {
        return vfs.f_bavail * vfs.f_frsize;
    }
    return 0;
}

// Available space on media drive
unsigned long long zdj_fs_media_space( void ) {
    struct statvfs vfs;
    char drive[ ] = "/media/internal";

    if ( statvfs( drive, &vfs ) == 0 ) {
        return vfs.f_bavail * vfs.f_frsize;
    }
    return 0;
}

// Alloc, read and return a char * from a given file path
char * zdj_fs_read_buffer( char * path, int limit ) {  
    FILE * fp = fopen( path, "r" );
    if( !fp ) { return NULL; }

    char *buffer = NULL;
    size_t buffer_size = 16384;
    size_t valid_bytes_in_buffer = 0;

    while ( 1 ) {
        size_t bytes_to_read, bytes_read;
        char *temp;

        // Attempt to realloc() buffer for this read.
        temp = realloc( buffer, buffer_size );
        if ( temp == NULL ) { free( buffer ); fclose( fp ); return NULL; }
        buffer = temp;

        // Set up the byte count for this read (leave room for null term char).
        bytes_to_read = buffer_size - valid_bytes_in_buffer - 1;

        // Ask for a block of bytes from the file pointer.
        bytes_read = fread( buffer + valid_bytes_in_buffer, 1, bytes_to_read, fp );

        // Exit loop when we get no new data.
        if ( bytes_read == 0 ) { break; }
           
        // Prep the next loop.
        valid_bytes_in_buffer += bytes_read;
        buffer_size *= 2;        
    }

    // Bug out on error.
    if ( ferror( fp ) ) {
        fprintf( stderr, "File I/O error occurred!" );
        free( buffer );
        fclose( fp );
        return NULL;
    }

    // Null-terminate the buffer data.
    buffer[ valid_bytes_in_buffer-1 ] = '\0';

    // Shrink mem to buffer len
    char *temp;
    temp = realloc( buffer, valid_bytes_in_buffer );
    if ( temp != NULL ) { buffer = temp; }

    fclose( fp );
    return buffer;
}
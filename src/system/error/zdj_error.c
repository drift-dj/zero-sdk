#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <execinfo.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/system/log/zdj_log.h>
#include <zerodj/system/settings/zdj_settings.h>

static bool _put_remap_line_for_addr( char * addr, char * remap_line, FILE * remap_file );

///////////////////////////////////////////////////////
// Do some hacking to fix wonky library dependencies //
///////////////////////////////////////////////////////
unsigned long __stack_chk_guard;
void __stack_chk_guard_setup(void) {
    __stack_chk_guard = 0xBAAAAAAD; // Use a random or magic number
}
void __stack_chk_fail(void) {
    /* Handle the error (e.g., print message and exit) */
}

int __isoc23_sscanf(const char *str, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vsscanf(str, format, args);
    va_end(args);
    return ret;
}

zdj_error_state_t * _zdj_error_state = NULL;

static char * _zdj_error_string[ ZDJ_ERROR_COUNT ] = {
    "Unknown", //ZDJ_ERROR_UNKNOWN,
    "Unknown", // ZDJ_ERROR_OKAY,
    "Unknown", // ZDJ_ERROR_NOT_INITIALIZED,
    "Unknown", // ZDJ_ERROR_MISSING,
    "Unknown", // ZDJ_ERROR_MISSING_DEPENDENCY,
    "Unknown", // ZDJ_ERROR_EMPTY,
    "Unknown", // ZDJ_ERROR_FULL,
    "Unknown", // ZDJ_ERROR_SYS_ERROR,
    "Unknown", // ZDJ_ERROR_NO_REG_RECORD,
    "Unknown", // ZDJ_ERROR_BAD_REG_RECORD,
    "Unknown", // ZDJ_ERROR_CRASHED,
    "Unknown", // ZDJ_ERROR_NOEXEC,
    "Unknown", // ZDJ_ERROR_FIRST_CRASH,
    "Unknown", // ZDJ_ERROR_SDL_FAILED,
    "Unknown", // ZDJ_ERROR_MISSING_GFX_RESOURCE,
    "Unknown", // ZDJ_ERROR_NO_SPACE,
    "Unknown", // ZDJ_ERROR_BAD_FILESIZE,
    "Unknown", // ZDJ_ERROR_BAD_DIR,
    "Unknown", // ZDJ_ERROR_BAD_PERMS,
    "Unknown", // ZDJ_ERROR_FILE_EXISTS,
    "Unknown", // ZDJ_ERROR_FILE_MISSING,
    "Unknown", // ZDJ_ERROR_BAD_MANIFEST,
    "Unknown", // ZDJ_ERROR_INVALID_EXE,
    "Unknown", // ZDJ_ERROR_CORRUPT,
    "Unknown", // ZDJ_ERROR_ROLLBACK,
    "Unknown", // ZDJ_ERROR_LIBRARY_DB_ERROR,
    "Unknown", // ZDJ_ERROR_MISSING_LIBRARY_DB,
    "Unknown", // ZDJ_ERROR_LIBRARY_EMPTY,
    "Unknown", // ZDJ_ERROR_LIBRARY_BAD_METADATA,
    "Unknown", // ZDJ_ERROR_LIBRARY_BAD_FILEPATH,
    "Unknown", // ZDJ_ERROR_MISSING_SONG,
};

static char * _zdj_error_marker_string[ ZDJ_ERROR_MARKER_COUNT ] = {
    "unclaimed", // ZDJ_ERROR_MARKER_UNCLAIMED,
    "add view", // ZDJ_ERROR_MARKER_VIEW_ADD,
    "remove view", // ZDJ_ERROR_MARKER_VIEW_REMOVE,
    "deinit view", // ZDJ_ERROR_MARKER_VIEW_DEINIT,
    "draw view", // ZDJ_ERROR_MARKER_VIEW_DRAW,
    "db access", // ZDJ_ERROR_MARKER_DB,
    "deck init", // ZDJ_ERROR_MARKER_DECK_INIT,
    "deck update", // ZDJ_ERROR_MARKER_DECK_UPDATE,
    "decode update", // ZDJ_ERROR_MARKER_DECODE_UPDATE,
    "decode window", // ZDJ_ERROR_MARKER_DECODE_WINDOW,
    "event handling", // ZDJ_ERROR_MARKER_HANDLE_EVENT,
    "record update", // ZDJ_ERROR_MARKER_RECORD_UPDATE,
    "debug" // ZDJ_ERROR_MARKER_DEBUG,
};

// Print (hopefully) helpful info, write a crash log if enabled and exit.
void _zdj_error_sig( int code ) {
    if( code == SIGSEGV ) {
        char path[ 512 ];
        sprintf( path, "%s/crash_log_%03d.txt", ZDJ_CRASH_LOG_DIR, zdj_new_log_num( ZDJ_LOG_CRASH_REPORT ) );

        printf( "\n===================\n SIGSEGV:\n [%s]\n %s\n===================\n",
            _zdj_error_marker_string[ zdj_error_state( )->marker ],
            basename( path )
        );
        
        // Open error log
        FILE * log_fp = fopen( path, "w" );
        if( !log_fp ) { printf( "FAILED TO WRITE CRASH LOG!!!\n" ); exit( code ); }
        FILE * remap_file = fopen( "/usr/bin/zero-dj/zero-dj.remap", "r" );
        if ( !remap_file ) { printf( "FAILED TO OPEN REMAP!!!\n" ); }

        printf( " BACKTRACE:\n" );
        printf( "===================\n" );

        int max_frames = 100;
        void *callstack[ max_frames ];
        int frames;
        char **strings;

        frames = backtrace( callstack, max_frames );
        strings = backtrace_symbols( callstack, frames );

        char output_line[ 256 ];
        char addr_str[ 64 ];
        char symbol[ 128 ];
        char address[ 32 ];
        for (int i = 0; i < frames; ++i) {
            int parse_count = sscanf( strings[ i ], "%[^ (](%[^)]) [%[^]]]", path, symbol, address );
            // We're only concerned with lines for zero-dj binary, skipping the zdj_error_sig() line
            if ( parse_count == 3 && 
                strstr( path, "dj" ) != NULL &&
                !strstr( symbol, "error_sig" )
            ) {
                
                // Remap lines which don't have function names
                if ( sscanf( strings[ i ], "%*[^(](%*[+]%31[^)])", addr_str ) == 1 ) {
                    
                    // Just write raw log if remap file is missing
                    if( !remap_file ) {
                        printf( "%s\n", strings[ i ] );
                        fprintf( log_fp, "%s\n", strings[ i ] );
    
                    // Rewrite log line with function name and source file
                    } else if( _put_remap_line_for_addr( addr_str, output_line, remap_file ) ) {
                        printf( "%s\n", output_line );
                        fprintf( log_fp, "%s\n", output_line );
                    }

                // Reformat lines with function names
                } else {
                    int matched = sscanf( strings[ i ], "%*[^(](%127[^+]+%31[^)]", symbol, address );
                    if ( matched == 2 ) {
                        printf( "%s() + %s\n", symbol, address );
                        fprintf( log_fp, "%s() + %s\n", symbol, address );
                    }
                }
                
            } else {
                // Only print non-zdj lines to console
                printf( "%s\n", strings[ i ] );
            }
        }

        // Finish up the crash log file
        if( log_fp ) { fclose( log_fp ); }
        
        printf( "===================\n\n" );
        exit( code );
    }
}

static bool _put_remap_line_for_addr( char * addr, char * remap_line, FILE * remap_file ) {
    bool found_remap_line = false;
    // Make int from addr
    long result = strtol( addr, NULL, 16 );
    // Scan remap file until address is captured
    char line[ 512 ];
    char source_file[ 128 ];
    char function_name[ 128 ];
    long vma = 0;
    char prev_source_file[ 128 ];
    char prev_function_name[ 128 ];
    long prev_vma = 0;
    // Find first address/func in remap file
    rewind( remap_file );
    fgets( line, sizeof( line ), remap_file );
    int res = sscanf( line, "%lx %s %s", &vma, (char*)&source_file, (char*)&function_name );
    if( res == 3 ) {
        strcpy( prev_function_name, function_name );
        strcpy( prev_source_file, source_file );
        prev_vma = vma;
    }

    // Start full scan of remap file
    rewind( remap_file );
    while ( fgets( line, sizeof( line ), remap_file ) ) {
        int res = sscanf( line, "%lx %s %s", &vma, (char*)&source_file, (char*)&function_name );
        if( res == 3 ) {
            if( result > prev_vma && result < vma ) { 
                snprintf( remap_line, 256, "%s() + 0x%lx [%s]",
                    prev_function_name,
                    result - prev_vma,
                    prev_source_file
                );
                found_remap_line = true;
                // Exit the loop early when we find the line
                break;
            }
            strcpy( prev_function_name, function_name );
            strcpy( prev_source_file, source_file );
            prev_vma = vma;
        }
    }
    return found_remap_line;
}

zdj_error_state_t * zdj_error_state( void ) {
    if( !_zdj_error_state ) { 
        signal( SIGSEGV, _zdj_error_sig );
        _zdj_error_state = calloc( 1, sizeof( zdj_error_state_t* ) );
        _zdj_error_state->marker = ZDJ_ERROR_MARKER_UNCLAIMED;
    }

    return _zdj_error_state;
}

void zdj_error_init( char * binary_path ) {
    // __bt_state = backtrace_create_state( binary_path, 0, bt_error_callback_create, NULL );
}

void zdj_print_error( zdj_error_type_t error ) {

}

void zdj_reset_logs( void ) {
    zdj_fs_remove_dir( ZDJ_LOG_DIR );
    zdj_fs_mkdir_p( ZDJ_LOG_DIR );
    zdj_fs_mkdir_p( ZDJ_CRASH_LOG_DIR );
    zdj_fs_mkdir_p( ZDJ_USB_LOG_DIR );
    zdj_fs_mkdir_p( ZDJ_ACTIVITY_LOG_DIR );
    zdj_fs_mkdir_p( ZDJ_DEBUG_LOG_DIR );
    sync( );
}
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <execinfo.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/system/settings/zdj_settings.h>

static int _inc_current_log_num( zdj_log_type_t type );

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
    printf( "SIG! %d\n", code );
    if( code == SIGSEGV ) {
        printf( "SIGSEGV during %s process.\n", _zdj_error_marker_string[ zdj_error_state( )->marker ] );
        
        // Open error log
        char path[ 512 ];
        sprintf( path, "%s/crash_log_%03d.txt", ZDJ_CRASH_LOG_DIR, zdj_new_log_num( ZDJ_LOG_TYPE_CRASH ) );
        FILE * log_fp = fopen( path, "w" );
        if( !log_fp ) { printf( "FAILED TO WRITE CRASH LOG!!!\n" ); exit( code ); }
        printf( "writing crash log: %p %s\n", log_fp, path );
        fprintf( log_fp, "### RAW CRASH LOG ###\n" );

        int max_frames = 100;
        void *callstack[ max_frames ];
        int frames;
        char **strings;
       
        frames = backtrace( callstack, max_frames );
        strings = backtrace_symbols( callstack, frames );

        for (int i = 0; i < frames; ++i) {
            printf( "%s\n", strings[ i ] );
            // Write to current error log file
            fprintf( log_fp, "%s\n", strings[ i ] );
        }

        // Finish up the crash log file
        if( log_fp ) { fclose( log_fp ); }

        // Tag the crash so re-launch can trigger a debug modal
        exit( code );
    }
}

int zdj_cur_log_num( zdj_log_type_t type ) {
    char log_dir[ 512 ];
    char count_path[ 512 ];
    switch ( type ) {
        case ZDJ_LOG_TYPE_CRASH:
            strcpy( log_dir, ZDJ_CRASH_LOG_DIR );
            strcpy( count_path, ZDJ_CRASH_LOG_COUNT );
            break;
        case ZDJ_LOG_TYPE_USB:
            strcpy( log_dir, ZDJ_USB_LOG_DIR );
            strcpy( count_path, ZDJ_USB_LOG_COUNT );
            break;
        case ZDJ_LOG_TYPE_LIBRARY:
            strcpy( log_dir, ZDJ_ACTIVITY_LOG_DIR );
            strcpy( count_path, ZDJ_ACTIVITY_LOG_COUNT );
            break;
        case ZDJ_LOG_TYPE_DEBUG:
            strcpy( log_dir, ZDJ_DEBUG_LOG_DIR );
            strcpy( count_path, ZDJ_DEBUG_LOG_COUNT );
            break;
        default: return 0;
    }

    // Create the logs dir if it's missing
    if( access( log_dir, F_OK ) != 0 ) { zdj_fs_mkdir_p( log_dir ); }
    // Open the log counter, create if missing
    int num = 0;
    FILE * count_fp = fopen( count_path, "r" );
    if( count_fp ) {
        fread( &num, sizeof( int ), 1, count_fp );
        fclose( count_fp );
    }
    return num;
}

static int _inc_current_log_num( zdj_log_type_t type ) {
    char log_dir[ 512 ];
    char count_path[ 512 ];
    switch ( type ) {
        case ZDJ_LOG_TYPE_CRASH:
            strcpy( log_dir, ZDJ_CRASH_LOG_DIR );
            strcpy( count_path, ZDJ_CRASH_LOG_COUNT );
            break;
        case ZDJ_LOG_TYPE_USB:
            strcpy( log_dir, ZDJ_USB_LOG_DIR );
            strcpy( count_path, ZDJ_USB_LOG_COUNT );
            break;
        case ZDJ_LOG_TYPE_LIBRARY:
            strcpy( log_dir, ZDJ_ACTIVITY_LOG_DIR );
            strcpy( count_path, ZDJ_ACTIVITY_LOG_COUNT );
            break;
        case ZDJ_LOG_TYPE_DEBUG:
            strcpy( log_dir, ZDJ_DEBUG_LOG_DIR );
            strcpy( count_path, ZDJ_DEBUG_LOG_COUNT );
            break;
        default: return 0;
    }

    // Create the logs dir if it's missing, fail to 0 
    if( access( log_dir, F_OK ) != 0 ) { zdj_fs_mkdir_p( log_dir ); }
    if( access( log_dir, F_OK ) != 0 ) { printf( "FAILED TO CREATE LOG DIR: %s\n", log_dir ); return 0; }
    // Open the log counter, create if missing
    int num = 0;
    FILE * count_fp = fopen( count_path, "r" );
    if( count_fp ) {
        fread( &num, sizeof( int ), 1, count_fp );
        fclose( count_fp );
        
        // Increment and write the new num to counter
        num++;
        count_fp = fopen( count_path, "w" );
        if( count_fp ) {
            fwrite( &num, sizeof( int ), 1, count_fp );
            fclose( count_fp );
        }
    }
    return num;
}

int zdj_new_log_num( zdj_log_type_t type ) {
    return _inc_current_log_num( type );
}

void zdj_put_cur_log( zdj_log_type_t type, char * str_1, char * str_2, char * str_3 ) {
    int cur_log_num = zdj_cur_log_num( type );

    char log_path[ 512 ];
    switch ( type ) {
        case ZDJ_LOG_TYPE_CRASH:
            sprintf( log_path, "%s/crash_log_%03d.txt", ZDJ_CRASH_LOG_DIR, cur_log_num );
            break;
        case ZDJ_LOG_TYPE_USB:
            sprintf( log_path, "%s/usb_log_%03d.txt", ZDJ_USB_LOG_DIR, cur_log_num );
            break;
        case ZDJ_LOG_TYPE_LIBRARY:
            sprintf( log_path, "%s/lib_log_%03d.txt", ZDJ_CRASH_LOG_DIR, cur_log_num );
            break;
        case ZDJ_LOG_TYPE_DEBUG:
            sprintf( log_path, "%s/debug_log_%03d.txt", ZDJ_CRASH_LOG_DIR, cur_log_num );
            break;
        default: return;
    }

    if( access( log_path, F_OK ) != 0 ) { return; }
    FILE * log = fopen( log_path, "r" );
    if( !log ) { return; }
    
    printf( "reading log: %s\n", log_path );
    char line[ 512 ];
    if( str_1 && fgets( line, 512, log ) ) {
        strcpy( str_1, line );
    }
    if( str_2 && fgets( line, 512, log ) ) {
        strcpy( str_2, line );
    }
    if( str_3 && fgets( line, 512, log ) ) {
        strcpy( str_3, line );
    }

    fclose( log );
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
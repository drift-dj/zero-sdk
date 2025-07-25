#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include <zerodj/error/zdj_error.h>

static zdj_error_state_t * _zdj_error_state = NULL;

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
    "draw view" // ZDJ_ERROR_MARKER_VIEW_DRAW,
};

// Print (hopefully) helpful info and exit.
void _zdj_error_sig( int code ) {
    if( code == SIGSEGV ) {
        printf( "SIGSEGV during %s process.\n", _zdj_error_marker_string[ zdj_error_state( )->marker ] );
        exit( code );
    }
}

zdj_error_state_t * zdj_error_state( void ) {
    if( !_zdj_error_state ) { 
        signal( SIGSEGV, _zdj_error_sig );
        _zdj_error_state = calloc( 1, sizeof( zdj_error_state_t* ) );
        _zdj_error_state->marker = ZDJ_ERROR_MARKER_UNCLAIMED;
    }

    return _zdj_error_state;
}

void zdj_error_init( void ) {
    
}

void zdj_print_error( zdj_error_type_t error ) {

}
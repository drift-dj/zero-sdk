#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <signal.h>
#include <execinfo.h>
// #include <backtrace.h>

#include <zerodj/system/error/zdj_error.h>


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

// int bt_callback(void *, uintptr_t, const char *filename, int lineno, const char *function) {
//   /// demangle function name
//   const char *func_name = function;
// //   int status;
// //   char *demangled = abi::__cxa_demangle(function, nullptr, nullptr, &status);
// //   if (status == 0) {
// //     func_name = demangled;
// //   }

//   /// print
//   printf("%s:%d in function %s\n", filename, lineno, func_name);
//   return 0;
// }

// void bt_error_callback(void *, const char *msg, int errnum) {
//   printf("Error %d occurred when getting the stacktrace: %s", errnum, msg);
// }

// void bt_error_callback_create(void *, const char *msg, int errnum) {
//   printf("Error %d occurred when initializing the stacktrace: %s", errnum, msg);
// }

// void *__bt_state = NULL;

// Print (hopefully) helpful info and exit.
void _zdj_error_sig( int code ) {
    if( code == SIGSEGV ) {
        printf( "SIGSEGV during %s process.\n", _zdj_error_marker_string[ zdj_error_state( )->marker ] );
        
        int max_frames = 100;
        void *callstack[ max_frames ];
        int frames;
        char **strings;
       
        frames = backtrace( callstack, max_frames );
        strings = backtrace_symbols( callstack, frames );

        for (int i = 0; i < frames; ++i) {
            printf("%s\n", strings[i]);
        }
        
        // printf( "SIGSEGV during %s process.\n", _zdj_error_marker_string[ zdj_error_state( )->marker ] );
        
        // if (__bt_state) { /// make sure init_back_trace() is called
        //     // backtrace_full((backtrace_state *) __bt_state, 0, bt_callback, bt_error_callback, nullptr);
        //     backtrace_full(__bt_state, 0, bt_callback, bt_error_callback, NULL);
        // }

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

void zdj_error_init( char * binary_path ) {
    // __bt_state = backtrace_create_state( binary_path, 0, bt_error_callback_create, NULL );
}

void zdj_print_error( zdj_error_type_t error ) {

}
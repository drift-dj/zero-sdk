// Copyright (c) 2025 Drift DJ Industries

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ZDJ_ERROR_H
#define ZDJ_ERROR_H

#define ZDJ_LOG_DIR "/media/internal/logs"
#define ZDJ_CRASH_LOG_DIR "/media/internal/logs/crash"
#define ZDJ_CRASH_LOG_COUNT "/media/internal/logs/crash/.count"
#define ZDJ_USB_LOG_DIR "/media/internal/logs/usb"
#define ZDJ_USB_LOG_COUNT "/media/internal/logs/usb/.count"
#define ZDJ_ACTIVITY_LOG_DIR "/media/internal/logs/activity"
#define ZDJ_ACTIVITY_LOG_COUNT "/media/internal/logs/activity/.count"
#define ZDJ_DEBUG_LOG_DIR "/media/internal/logs/debug"
#define ZDJ_DEBUG_LOG_COUNT "/media/internal/logs/debug/.count"

typedef enum {
    ZDJ_ERROR_UNKNOWN,
    ZDJ_ERROR_OKAY,
    // Status > OKAY == error state
    ZDJ_ERROR_NOT_INITIALIZED,
    ZDJ_ERROR_MISSING,
    ZDJ_ERROR_MISSING_DEPENDENCY,
    ZDJ_ERROR_EMPTY,
    ZDJ_ERROR_FULL,
    ZDJ_ERROR_SYS_ERROR,
    // Registry stuff
    ZDJ_ERROR_NO_REG_RECORD,
    ZDJ_ERROR_BAD_REG_RECORD,
    ZDJ_ERROR_CRASHED,
    ZDJ_ERROR_NOEXEC,
    ZDJ_ERROR_FIRST_CRASH,
    // UI/display stuff
    ZDJ_ERROR_SDL_FAILED,
    ZDJ_ERROR_MISSING_GFX_RESOURCE,
    // File-system stuff
    ZDJ_ERROR_NO_SPACE,
    ZDJ_ERROR_BAD_FILESIZE,
    ZDJ_ERROR_BAD_DIR,
    ZDJ_ERROR_BAD_PERMS,
    ZDJ_ERROR_FILE_EXISTS,
    ZDJ_ERROR_FILE_MISSING,
    ZDJ_ERROR_FAILED_COPY,
    // Installer stuff
    ZDJ_ERROR_BAD_MANIFEST,
    ZDJ_ERROR_INVALID_EXE,
    ZDJ_ERROR_CORRUPT,
    ZDJ_ERROR_ROLLBACK,
    // Library stuff
    ZDJ_ERROR_LIBRARY_DB_ERROR,
    ZDJ_ERROR_MISSING_LIBRARY_DB,
    ZDJ_ERROR_LIBRARY_EMPTY,
    ZDJ_ERROR_LIBRARY_BAD_METADATA,
    ZDJ_ERROR_LIBRARY_BAD_FILEPATH,
    ZDJ_ERROR_LIBRARY_MISSING_SONG,
    ZDJ_ERROR_LIBRARY_QUERY_OKAY,
    ZDJ_ERROR_LIBRARY_NO_RESULTS,
    // Codec stuff
    ZDJ_ERROR_BAD_AUDIO_ENCODING,
    ZDJ_ERROR_COUNT,
    // USB stuff
    ZDJ_ERROR_BAD_COMMAND,
    ZDJ_ERROR_BAD_STATUS,
    ZDJ_USB_ALREADY_IN_MODE,
    ZDJ_ERROR_USB_BAD_GADGET_CONFIG,
    ZDJ_ERROR_USB_DEVICE_DB_ERROR,
    ZDJ_ERROR_MISSING_USB_DEVICE_DB,
    ZDJ_ERROR_USB_DEVICE_EMPTY,
    // Sundcard stuff
    ZDJ_ERROR_SOUNDCARD_DB_ERROR,
    ZDJ_ERROR_SOUNDCARD_DB_NO_RECORD
} zdj_error_type_t;

typedef enum {
    ZDJ_ERROR_MARKER_UNCLAIMED,
    ZDJ_ERROR_MARKER_VIEW_ADD,
    ZDJ_ERROR_MARKER_VIEW_REMOVE,
    ZDJ_ERROR_MARKER_VIEW_DEINIT,
    ZDJ_ERROR_MARKER_VIEW_DRAW,
    ZDJ_ERROR_MARKER_DB,
    ZDJ_ERROR_MARKER_DECK_INIT,
    ZDJ_ERROR_MARKER_DECK_UPDATE,
    ZDJ_ERROR_MARKER_DECODE_UPDATE,
    ZDJ_ERROR_MARKER_DECODE_WINDOW,
    ZDJ_ERROR_MARKER_HANDLE_EVENT,
    ZDJ_ERROR_MARKER_DEBUG,
    ZDJ_ERROR_MARKER_RECORD_UPDATE,
    ZDJ_ERROR_MARKER_COUNT
} zdj_error_marker_t;

typedef enum {
    ZDJ_LOG_TYPE_NONE,
    ZDJ_LOG_TYPE_CRASH,
    ZDJ_LOG_TYPE_USB,
    ZDJ_LOG_TYPE_LIBRARY,
    ZDJ_LOG_TYPE_DEBUG
} zdj_log_type_t;

typedef struct {
    zdj_error_marker_t marker;
} zdj_error_state_t;

extern zdj_error_state_t * _zdj_error_state;

void zdj_error_init( char * binary_path );
zdj_error_state_t * zdj_error_state( void );
void zdj_print_error( zdj_error_type_t error );

int zdj_new_log_num( zdj_log_type_t type );
int zdj_cur_log_num( zdj_log_type_t type );
void zdj_reset_logs( void );
void zdj_put_cur_log( zdj_log_type_t type, char * str_1, char * str_2, char * str_3 );
void zdj_process_latest_crash_log_file( void );

#endif
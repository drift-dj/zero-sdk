// Copyright (c) 2026 Drift DJ Industries

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

#ifndef ZDJ_LOG_H
#define ZDJ_LOG_H

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
    ZDJ_LOG_NONE,
    ZDJ_LOG_DEBUG,
    ZDJ_LOG_MSG,
    ZDJ_LOG_ERROR,
    ZDJ_LOG_CRASH
} zdj_log_level_t;

// These are used as normal identieifers and as bitmasks
// Ensure any new entries work as bitmasks.
typedef enum {
    ZDJ_LOG_ANY,
    ZDJ_LOG_PLAYBACK,
    ZDJ_LOG_UI,
    ZDJ_LOG_USB,
    ZDJ_LOG_LIBRARY,
    ZDJ_LOG_INIT,
    ZDJ_LOG_FS,
    ZDJ_LOG_RECORDING,
    ZDJ_LOG_MIXER,
    ZDJ_LOG_SUBJECT_COUNT
} zdj_log_subject_t;

static int zdj_log_subject_mask[ ZDJ_LOG_SUBJECT_COUNT ] = {
    0x1, // ZDJ_LOG_ANY
    0x2, // ZDJ_LOG_PLAYBACK
    0x4, // ZDJ_LOG_UI
    0x8, // ZDJ_LOG_USB
    0x10, // ZDJ_LOG_LIBRARY
    0x20, // ZDJ_LOG_INIT
    0x40, // ZDJ_LOG_FS
    0x80, // ZDJ_LOG_RECORDING
    0x100 // ZDJ_LOG_MIXER
};

static char * zdj_log_prefix[ ZDJ_LOG_SUBJECT_COUNT ] = {
    "[ANY]", // ZDJ_LOG_ANY = 0,
    "[PLY]", // ZDJ_LOG_PLAYBACK,
    "[UI]", // ZDJ_LOG_UI,
    "[USB]", // ZDJ_LOG_USB,
    "[LIB]", // ZDJ_LOG_LIBRARY,
    "[INI]", // ZDJ_LOG_INIT,
    "[FS]", // ZDJ_LOG_FS
    "[RCD]", // ZDJ_LOG_RECORDING
    "[MIX]" // ZDJ_LOG_MIXER
};

typedef struct {
    zdj_log_level_t log_level;
    int subject_masks; // bitfield of subject_masks enabled for logging
    bool log_printf;
    bool log_to_file;
} zdj_log_settings_t;

extern zdj_log_settings_t * zdj_log_settings;

void zdj_log_init( void );
void zdj_log_refresh_settings( void );

void zdj_set_log_level( zdj_log_level_t );
void zdj_log( zdj_log_subject_t subject, zdj_log_level_t level, const char* fmt, ... );

int zdj_new_log_num( zdj_log_subject_t subject );
int zdj_cur_log_num( zdj_log_subject_t subject );
void zdj_reset_logs( void );
void zdj_put_cur_log( zdj_log_subject_t subject, char * str_1, char * str_2, char * str_3 );
void zdj_process_latest_crash_log_file( void );

#endif

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

#ifndef ZDJ_HEALTH_TYPE_H
#define ZDJ_HEALTH_TYPE_H

typedef enum {
    ZDJ_HEALTH_STATUS_UNKNOWN,
    ZDJ_HEALTH_STATUS_OKAY,
    // Status > OKAY == error state
    ZDJ_HEALTH_STATUS_NOT_INITIALIZED,
    ZDJ_HEALTH_STATUS_MISSING,
    ZDJ_HEALTH_STATUS_MISSING_DEPENDENCY,
    ZDJ_HEALTH_STATUS_SYS_ERROR,
    // Registry stuff
    ZDJ_HEALTH_STATUS_NO_REG_RECORD,
    ZDJ_HEALTH_STATUS_BAD_REG_RECORD,
    ZDJ_HEALTH_STATUS_CRASHED,
    ZDJ_HEALTH_STATUS_NOEXEC,
    ZDJ_HEALTH_STATUS_FIRST_CRASH,
    // UI/display stuff
    ZDJ_HEALTH_STATUS_SDL_FAILED,
    ZDJ_HEALTH_STATUS_MISSING_GFX_RESOURCE,
    // File-system stuff
    ZDJ_HEALTH_STATUS_NO_SPACE,
    ZDJ_HEALTH_STATUS_BAD_FILESIZE,
    ZDJ_HEALTH_STATUS_BAD_DIR,
    ZDJ_HEALTH_STATUS_BAD_PERMS,
    ZDJ_HEALTH_STATUS_FILE_EXISTS,
    ZDJ_HEALTH_STATUS_FILE_MISSING,
    // Installer stuff
    ZDJ_HEALTH_STATUS_BAD_MANIFEST,
    ZDJ_HEALTH_STATUS_INVALID_EXE,
    ZDJ_HEALTH_STATUS_CORRUPT,
    ZDJ_HEALTH_STATUS_ROLLBACK,
    // Library stuff
    ZDJ_HEALTH_STATUS_LIBRARY_DB_ERROR,
    ZDJ_HEALTH_STATUS_MISSING_LIBRARY_DB,
    ZDJ_HEALTH_STATUS_LIBRARY_EMPTY,
    ZDJ_HEALTH_STATUS_LIBRARY_BAD_METADATA,
    ZDJ_HEALTH_STATUS_LIBRARY_BAD_FILEPATH,
    ZDJ_HEALTH_STATUS_MISSING_SONG
} zdj_health_status_t;

#endif
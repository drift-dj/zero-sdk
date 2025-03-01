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

#ifndef ZDJ_FS_H
#define ZDJ_FS_H

#include <stdbool.h>
#include <zerodj/health/zdj_health_type.h>

typedef struct {
    char * substr;
} zdj_fs_scan_pattern_t;

typedef void ( *zdj_fs_result_cb )( char * );

zdj_health_status_t zdj_fs_copy_file( char * src, char * dst, bool overwrite );
zdj_health_status_t zdj_fs_extract_file_from_binary( 
    char * bin, 
    char * dst, 
    unsigned long long offset, 
    unsigned long long len,
    bool overwrite
);


void zdj_fs_scan_dir( 
    char * path,
    bool recursive,
    zdj_fs_scan_pattern_t * pattern,
    zdj_fs_result_cb result_cb
);
int zdj_fs_mkdir_p( char * path );
void zdj_fs_remove_dir( char * path );
int zdj_fs_get_size( char * filepath );
unsigned long long zdj_fs_sys_space( void );
unsigned long long zdj_fs_media_space( void );
char * zdj_fs_read_buffer( char * path, int limit );

#endif
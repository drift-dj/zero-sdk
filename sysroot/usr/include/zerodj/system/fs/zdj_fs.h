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
    char substr[ 512 ];
} zdj_fs_scan_pattern_t;

typedef void ( *zdj_fs_result_cb )( char *, void * );

float zdj_fs_get_free_media_space( void );
float zdj_fs_get_filesize( char * path );

zdj_health_status_t zdj_fs_copy_file( char * src, char * dst, bool overwrite );
zdj_health_status_t zdj_fs_copy_dir_contents( char * src, char * dst, bool overwrite );
int zdj_fs_copy_file_with_hash( char * src, char * dst, bool overwrite );
zdj_health_status_t zdj_fs_extract_file_from_binary( 
    char * bin, 
    char * dst, 
    unsigned long long offset, 
    unsigned long long len,
    bool overwrite
);

char * zdj_fs_get_file_extension( char * filepath );

bool zdj_fs_path_is_dir( char * path );
bool zdj_fs_path_is_audio_dir( char * path );
bool zdj_fs_path_is_external_database_filename( char * path );
bool zdj_fs_path_is_external_database_dir( char * path );
bool zdj_fs_path_is_audio_filename( char * path );
bool zdj_fs_path_is_image_filename( char * path );
bool zdj_fs_path_is_media_partition( char * path );
bool zdj_fs_path_is_attached_msd( char * path );
bool zdj_fs_path_is_dir_with_files( char * path );
bool zdj_fs_path_is_logfile( char * path );

zdj_health_status_t zdj_fs_put_parent_dir( char * path, char * dir );
void zdj_fs_scan_dir( 
    char * path,
    bool recursive,
    zdj_fs_scan_pattern_t * pattern,
    zdj_fs_result_cb result_cb,
    void * user_data
);
int zdj_fs_mkdir_p( char * path );
void zdj_fs_remove_dir( char * path );
void zdj_fs_remove_dir_contents( char * path );
int zdj_fs_get_size( char * filepath );
unsigned long long zdj_fs_sys_space( void );
unsigned long long zdj_fs_media_space( void );
char * zdj_fs_read_buffer( char * path, int limit );
int zdj_fs_write_buffer( char * path, char * buffer );

void zdj_fs_get_popen( char * cmd, char * res );

#endif
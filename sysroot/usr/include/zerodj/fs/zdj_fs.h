#ifndef ZDJ_FS_H
#define ZDJ_FS_H

typedef struct {
    char * substr;
} zdj_fs_scan_pattern_t;

typedef void ( *zdj_fs_result_cb )( char * );

void zdj_fs_scan_dir( 
    char * path,
    bool recursive,
    zdj_fs_scan_pattern_t * pattern,
    zdj_fs_result_cb result_cb
);

#endif
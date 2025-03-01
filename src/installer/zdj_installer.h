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

#ifndef ZDJ_INSTALLER_H
#define ZDJ_INSTALLER_H

#include <stdint.h>

#include <zerodj/zdj_data_type.h>
#include <zerodj/registry/zdj_registry.h>

#define ZDJ_INSTALLER_HEADER_START 4000 // Byte location of installer header in binary
#define ZDJ_INSTALLER_EXTRACT_DIR "/etc/zero-sdk/installer"
#define ZDJ_INSTALLER_ROLLBACK_DIR "/etc/zero-sdk/rollback"
#define ZDJ_INSTALLER_MANIFEST_PATH "/etc/zero-sdk/installer/manifest_db"

typedef struct {
    char tag[4];
    int header_offset;
    int manifest_offset;
    int manifest_length;
    int binary_offset;
    int binary_length;
} zdj_installer_offsets_t;

typedef struct {
    zdj_install_t install;
    zdj_installer_offsets_t data_offsets;
    char path[ 4096 ];
    void * fd;
    void * manifest_db;
    bool valid;
    zdj_health_status_t status;
    struct zdj_installer_t * next;
    struct zdj_installer_t * prev;
} zdj_installer_t;

typedef struct {
    char dest_path[ 4096 ];
    char extract_path[ 4096 ];
    char rollback_path[ 4096 ];
    int blob_start;
    int blob_length;
} zdj_installer_manifest_item_t;

typedef void ( *zdj_installer_manifest_item_cb )( zdj_installer_manifest_item_t *, void * );

zdj_installer_t * zdj_installer_for_filepath( char * filepath );
int zdj_installer_file_count( zdj_installer_t * installer );
bool zdj_installer_extract_manifest( zdj_installer_t * installer );
void zdj_installer_iterate_manifest( zdj_installer_t * installer, zdj_installer_manifest_item_cb cb, void * data );
bool zdj_installer_extract_files( zdj_installer_t * installer );
bool zdj_installer_validate_files( zdj_installer_t * installer );
bool zdj_installer_commit( zdj_installer_t * installer );
bool zdj_installer_cleanup( zdj_installer_t * installer );

#endif
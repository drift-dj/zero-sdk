#ifndef ZDJ_INSTALLER_H
#define ZDJ_INSTALLER_H

#include <stdint.h>

#include <zerodj/zdj_data_type.h>
#include <zerodj/registry/zdj_registry.h>

#define ZDJ_INSTALLER_HEADER_START 4000 // Byte location of installer header in binary
#define ZDJ_INSTALLER_EXTRACT_DIR "/etc/zero-sdk/installer"
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
    void * fd;
    void * manifest_db;
    struct zdj_installer_t * next;
    struct zdj_installer_t * prev;
} zdj_installer_t;

typedef struct {
    char filepath[ 4096 ];
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
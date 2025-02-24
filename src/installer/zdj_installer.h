#ifndef ZDJ_INSTALLER_H
#define ZDJ_INSTALLER_H

#include <stdint.h>

#include <zerodj/zdj_data_type.h>
#include <zerodj/registry/zdj_registry.h>

#define ZDJ_INSTALLER_HEADER_START 4000 // Byte location of installer header in binary

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
    void * manifest;
    struct zdj_installer_t * next;
    struct zdj_installer_t * prev;
} zdj_installer_t;

zdj_installer_t * zdj_installer_for_filepath( char * filepath );

#endif
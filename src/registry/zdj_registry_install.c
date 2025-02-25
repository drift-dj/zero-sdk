#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>


#include <zerodj/zdj_data_type.h>
#include <zerodj/registry/zdj_registry.h>
#include <zerodj/fs/zdj_fs.h>

static zdj_install_t * _zdj_registry_all_installs = NULL;

void _zdj_registry_install_scan_cb( char * path );

// Retrieve all installs in the registry
zdj_install_t * zdj_registry_installs( void ) {
    if( !_zdj_registry_all_installs ) {
        // Do a non-recursive scan for '*' (pattern=NULL) in the reg install dir.
        // Invoke _zdj_registry_install_scan_cb for all hits.
        zdj_fs_scan_dir( "/etc/registry/install/", false, NULL, &_zdj_registry_install_scan_cb );
    }
    return _zdj_registry_all_installs;
}

// Build a new set of install instances matching a given category
zdj_install_t * zdj_registry_installs_for_category( char * category ) {
    zdj_install_t * res = NULL;
    zdj_install_t * install = zdj_registry_installs( );
    while( install ) {
        if( !strcmp( install->category, category ) ) {
            zdj_install_t * new_install = zdj_registry_install_for_install_name( install->install_name );
            new_install->next = NULL;
            new_install->prev = NULL;
            if( new_install->health <= ZDJ_REGISTRY_HEALTH_UNKNOWN ) {
                if( res ) {
                    res->prev = new_install;
                    new_install->next = res; 
                }
                res = new_install;
            }
        }
        install = install->next;
    }
    return res;
}

// Create an install record from params
zdj_install_t * zdj_registry_create_install( 
    char * install_name,
    char * display_name,
    char * bin_path,
    char * desc,
    uint8_t v_maj,
    uint8_t v_min,
    uint8_t v_hf,
    uint16_t v_b
) {
    // Make UUID
    // Make version string
    // Set initial health state
    zdj_install_t * install = calloc( 1, sizeof( zdj_install_t ) );
    strcpy( install->install_name, install_name );
    strcpy( install->display_name, display_name );
    strcpy( install->bin_path, bin_path );
    
    return install;
}

zdj_install_t * zdj_registry_install_for_install_name( char * name ) {
    FILE * fp;
    char install_path[ 1024 ];
    zdj_install_t * install = calloc( 1, sizeof( zdj_install_t ) );
    snprintf( install_path, sizeof( install_path ), "/etc/registry/install/%s", name );
    if ( access( install_path, F_OK ) == 0 ) {
        fp = fopen( install_path, "r" );
        int br = fread( install, sizeof( zdj_install_t ), 1, fp );
        fclose( fp );
        if( br == 1 ) {
            install->health = ZDJ_REGISTRY_HEALTH_UNKNOWN;
        } else {
            install->health = ZDJ_REGISTRY_HEALTH_BAD_RECORD;
        }
    } else {
        install->health = ZDJ_REGISTRY_HEALTH_NO_RECORD;
    }
    return install;
}

zdj_install_t * zdj_registry_install_for_filepath( char * path ) {
    FILE * fp;
    zdj_install_t * install = calloc( 1, sizeof( zdj_install_t ) );
    if ( access( path, F_OK ) == 0 ) {
        fp = fopen( path, "r" );
        int br = fread( install, sizeof( zdj_install_t ), 1, fp );
        fclose( fp );
        if( br == 1 ) {
            install->health = ZDJ_REGISTRY_HEALTH_UNKNOWN;
        } else {
            install->health = ZDJ_REGISTRY_HEALTH_BAD_RECORD;
        }
    } else {
        install->health = ZDJ_REGISTRY_HEALTH_NO_RECORD;
    }
    return install;
}

void _zdj_registry_install_scan_cb( char * path ) {
    // Link in a new install item into the list of known installs
    zdj_install_t * install = zdj_registry_install_for_filepath( path );
    if( _zdj_registry_all_installs ) {
        // Slip the new install into the base of the stack.
        _zdj_registry_all_installs->prev = install;
        install->next = _zdj_registry_all_installs; 
    }
    _zdj_registry_all_installs = install;
}
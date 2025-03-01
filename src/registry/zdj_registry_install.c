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
            zdj_install_t * new_install = zdj_registry_install_for_name( install->registry_name );
            new_install->next = NULL;
            new_install->prev = NULL;
            if( new_install->health <= ZDJ_HEALTH_STATUS_UNKNOWN ) {
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
    char * registry_name,
    char * display_name,
    char * bin_dir,
    char * desc,
    uint8_t v_maj,
    uint8_t v_min,
    uint8_t v_hf,
    uint16_t v_b
) {
    // Make UUID
    // Make version string
    // Set initial health state
    zdj_install_t * install = malloc( sizeof( zdj_install_t ) );
    strcpy( install->registry_name, registry_name );
    strcpy( install->display_name, display_name );
    strcpy( install->bin_dir, bin_dir );
    strcpy( install->version.desc, desc );
    install->version.major = v_maj;
    install->version.minor = v_min;
    install->version.hotfix = v_hf;
    install->version.build = v_b;
    return install;
}


zdj_install_t * zdj_registry_install_for_name( char * name ) {
    FILE * fp;
    char install_path[ 1024 ];
    zdj_install_t * install = calloc( 1, sizeof( zdj_install_t ) );
    snprintf( install_path, sizeof( install_path ), "/etc/registry/install/%s", name );
    if ( access( install_path, F_OK ) == 0 ) {
        fp = fopen( install_path, "r" );
        int br = fread( install, sizeof( zdj_install_t ), 1, fp );
        fclose( fp );
        if( br == 1 ) {
            install->health = ZDJ_HEALTH_STATUS_UNKNOWN;
        } else {
            return NULL; 
        }
    } else {
        return NULL;
    }
    install->next = NULL;
    install->prev = NULL;
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
            install->health = ZDJ_HEALTH_STATUS_UNKNOWN;
        } else {
            install->health = ZDJ_HEALTH_STATUS_BAD_REG_RECORD;
        }
    } else {
        install->health = ZDJ_HEALTH_STATUS_NO_REG_RECORD;
    }
    install->next = NULL;
    install->prev = NULL;
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

// Update an existing install or create a new one
void zdj_registry_commit_install( zdj_install_t * install ) {
    zdj_registry_write_install( install );
}

int zdj_registry_write_install( zdj_install_t * install ) {
    char path[ 2048 ];
    snprintf( path, sizeof( path ), "%s/%s", "/etc/registry/install", install->registry_name );
    FILE * fp = fopen( path, "w" );
    int bw = 0;
    if( fp ) {
        bw = fwrite( install, sizeof( zdj_install_t ), 1, fp );
        fclose( fp );
    }
    return bw;
}

// Remove an installed app
void zdj_registry_remove_install( zdj_install_t * install ) {
    // Don't delete protected system apps
    if( install->protected ) { return; } 
    
    // Delete the app's directory
    zdj_fs_remove_dir( install->bin_dir );
    // Delete the registry record
    char path[ 2048 ];
    snprintf( path, sizeof( path ), "%s/%s", "/etc/registry/install", install->registry_name );
    remove( path );
    // If this is the normal_mode boot app, switch norm_mode req to fallback and persist
    zdj_launch_req_t * normal_req = zdj_registry_load_launch_req( "/etc/registry/boot/normal_req" );
    if( !strcmp( normal_req->app_install.registry_name, install->registry_name ) ) {
        zdj_registry_launch_req_switch_to_fallback( normal_req );
        zdj_registry_write_launch_req( "/etc/registry/boot/normal_req", normal_req );
    }
}

// Release mem for an install instance
void zdj_registry_free_install( zdj_install_t * install ) {

}
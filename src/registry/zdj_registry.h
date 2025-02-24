#ifndef ZDJ_REGISTRY_H
#define ZDJ_REGISTRY_H

#include <stdint.h>
#include <zerodj/zdj_data_type.h>

typedef enum {
    ZDJ_REGISTRY_HEALTH_OKAY, // Everything checked and known-good
    ZDJ_REGISTRY_HEALTH_UNKNOWN, // Not everyhthing checked - may be good or bad
    // --- anything > UNKNOWN represents error state
    ZDJ_REGISTRY_HEALTH_NO_RECORD, // Can't find a matching record
    ZDJ_REGISTRY_HEALTH_BAD_RECORD, // Found matching malformed record
    ZDJ_REGISTRY_HEALTH_CRASHED, // App has crashed
    ZDJ_REGISTRY_HEALTH_NOEXEC // zerod failed to execve() this app
} zdj_registry_health_t;

typedef struct {
    char install_name[128];
    char display_name[128];
    char short_name[16];
    char description[1024];
    char category[32];
    char uuid[32];
    char bin_path[2048];
    zdj_version_t version;
    zdj_registry_health_t health;
    struct zdj_install_t * next;
    struct zdj_install_t * prev;
} zdj_install_t;

typedef struct {
    char * app_install_name;
    char stored_app_install_name[128];
    zdj_install_t * app_install;
    char * fallback_install_name;
    char stored_fallback_install_name[128];
    zdj_install_t * fallback_install;
    zdj_registry_health_t health;
} zdj_launch_req_t;

// Install
zdj_install_t * zdj_registry_create_install( 
    char * install_name,
    char * display_name,
    char * bin_path,
    char * desc,
    uint8_t v_maj,
    uint8_t v_min,
    uint8_t v_hf,
    uint16_t v_b
);
zdj_install_t * zdj_registry_installs( void );
zdj_install_t * zdj_registry_installs_for_category( char * category );
void zdj_registry_install_for_filename( char * name, zdj_install_t * install );
void zdj_registry_install_for_filepath( char * path, zdj_install_t * install );
void zdj_registry_commit_install( zdj_install_t * install );
void zdj_registry_free_install( zdj_install_t * install );

// Installer

// Launch Request
zdj_launch_req_t * zdj_registry_create_launch_req( 
    char * app_install_name, 
    char * fallback_install_name 
);
int zdj_registry_commit_launch_req( zdj_launch_req_t * launch_req );
zdj_launch_req_t * zdj_registry_process_new_launch_req( char * path );
zdj_launch_req_t * zdj_registry_load_launch_req( char * path );
void zdj_registry_retrieve_launch_req_file( char * path, zdj_launch_req_t * launch_req );
void zdj_registry_load_launch_req_installs( zdj_launch_req_t * launch_req );
void zdj_registry_free_launch_req( zdj_launch_req_t * launch_req );
void zdj_registry_launch_req_switch_to_fallback( zdj_launch_req_t * launch_req );

#endif
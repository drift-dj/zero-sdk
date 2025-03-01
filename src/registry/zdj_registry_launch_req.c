#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <zerodj/registry/zdj_registry.h>


// Create a new launch_req from params.
// This must be suitable for immediate use AND for 
// writing to the drive as a stored launch_req.
// Request for a missing app should return a launch_req
// with undefined installs, and health = NO_REG_RECORD
zdj_launch_req_t * zdj_registry_create_launch_req( 
    char * app_install_name, 
    char * fallback_install_name 
) {
    zdj_launch_req_t * req = calloc( 1, sizeof( zdj_launch_req_t ) );
    req->health = ZDJ_HEALTH_STATUS_UNKNOWN;

    zdj_install_t * app_install = zdj_registry_install_for_name( app_install_name );
    app_install->health = ZDJ_HEALTH_STATUS_UNKNOWN;
    zdj_install_t * fallback_install = zdj_registry_install_for_name( fallback_install_name );

    // Copy install data into launch req
    if( !app_install ) {
        req->health = ZDJ_HEALTH_STATUS_NO_REG_RECORD;
        return req;
    } else {
        memcpy(&req->app_install, app_install, sizeof( zdj_install_t ));
    }
    if( fallback_install ) {
        memcpy(&req->fallback_install, fallback_install, sizeof( zdj_install_t ));
    }
    
    return req;
}

// Write a zdj_launch_req_t to /etc/registry/launch_req/new_launch_req.
// Once a launch_req has been committed, an app can exit without
// triggering a crash-to-fallback response from zerod.
int zdj_registry_commit_launch_req( zdj_launch_req_t * launch_req ) {
    int bw = zdj_registry_write_launch_req( ZDJ_REGISTRY_NEW_LAUNCH_REQ_PATH, launch_req );
}

// TODO, fix assumed new_launch_req filename here.
zdj_launch_req_t * zdj_registry_process_new_launch_req( void ) {
    // Overwrite launch_req w/new_launch_req.
    zdj_launch_req_t * req = zdj_registry_load_launch_req( ZDJ_REGISTRY_NEW_LAUNCH_REQ_PATH );
    if( !zdj_registry_write_launch_req( ZDJ_REGISTRY_LAUNCH_REQ_PATH, req ) ) { 
        // Exit with error if we can't overwrite launch_req.
        exit( EINVAL ); 
    }
    // Remove new_launch_req so we don't attempt to process it again.
    remove( ZDJ_REGISTRY_NEW_LAUNCH_REQ_PATH );
    // Reload launch_req from drive.
    return zdj_registry_load_launch_req( ZDJ_REGISTRY_LAUNCH_REQ_PATH );
}

// Load a launch request from Zero's local storage.
// Get the app install names from the local drive, 
// then populate the reg_install pointers by loading
// a matching persisted zdj_install_t from storage.
zdj_launch_req_t * zdj_registry_load_launch_req( char * path ) {
    // Load the request from storage
    FILE * fp;
    zdj_launch_req_t * req = calloc( 1, sizeof( zdj_launch_req_t ) );
    if ( access( path, F_OK ) == 0 ) {
        fp = fopen( path, "r" );
        int br = fread( req, sizeof( zdj_launch_req_t ), 1, fp );
        fclose( fp );
        if( br == 1 ) {
            req->health = ZDJ_HEALTH_STATUS_UNKNOWN;
        } else {
            req->health = ZDJ_HEALTH_STATUS_BAD_REG_RECORD;
        }
    } else {
        req->health = ZDJ_HEALTH_STATUS_NO_REG_RECORD;
    }
    // Null if there was a problem loading
    if( req->health > ZDJ_HEALTH_STATUS_OKAY ) { return NULL; }

    return req;
}

int zdj_registry_write_launch_req( 
    char * path, 
    zdj_launch_req_t * launch_req 
) {
    FILE * fp = fopen( path, "w" );
    printf( "fp: %p\n", fp );
    int bw = 0;
    if( fp ) {
        bw = fwrite( launch_req, sizeof( zdj_launch_req_t ), 1, fp );
        fclose( fp );
    }
    return bw;
}

void zdj_registry_free_launch_req( zdj_launch_req_t * launch_req ) {

}

// Move a launch request into its fallback state.
// Move the fallback app into the primary app and reload the installs from storage.
void zdj_registry_launch_req_switch_to_fallback( zdj_launch_req_t * launch_req ) {
    // If fallback_install is missing, set health to NO_REG_RECORD and return
    if( !strlen( launch_req->fallback_install.registry_name ) ) {
        launch_req->health = ZDJ_HEALTH_STATUS_NO_REG_RECORD;
        return;
    }

    // copy fallback_install to app_install
    memcpy( &launch_req->fallback_install, &launch_req->app_install, sizeof( zdj_install_t ) );

    // clear fallback_install
    memset( &launch_req->fallback_install, 0, sizeof( zdj_install_t ) );
}
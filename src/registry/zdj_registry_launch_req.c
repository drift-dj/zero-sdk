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
zdj_launch_req_t * zdj_registry_create_launch_req( 
    char * app_install_name, 
    char * fallback_install_name 
) {
    zdj_launch_req_t * req = malloc( sizeof( zdj_launch_req_t ) );
    req->health = ZDJ_REGISTRY_HEALTH_UNKNOWN;

    // Populate install names + name_storage
    req->app_install_name = app_install_name;
    strcpy( req->stored_app_install_name, app_install_name );
    
    if( fallback_install_name ) {
        req->fallback_install_name = fallback_install_name;
        strcpy( req->stored_fallback_install_name, fallback_install_name );
    } else {
        req->fallback_install_name = NULL;
    }

    // Attach installs from storage + set req health
    zdj_registry_load_launch_req_installs( req );
    
    return req;
}

// Write a zdj_launch_req_t to /etc/registry/launch_req/new_launch_req.
// Once a launch_req has been committed, an app can exit without
// triggering a crash-to-fallback response from zerod.
int zdj_registry_commit_launch_req( zdj_launch_req_t * launch_req ) {
    FILE * fd = fopen( "/etc/registry/launch_req/new_launch_req", "w" );
    int bw = fwrite( launch_req, sizeof( zdj_launch_req_t ), 1, fd );
    fclose( fd );
    if( bw == 1 ) {
        return 0;
    } else  {
        return 1;
    }
}

zdj_launch_req_t * zdj_registry_process_new_launch_req( char * path ) {
    // Overwrite launch_req w/new_launch_req.
    zdj_launch_req_t * req = zdj_registry_load_launch_req( path );
    FILE * lfp = fopen( "/etc/registry/launch_req/launch_req", "w" );
    int bw = fwrite( req, sizeof( zdj_launch_req_t ), 1, lfp );
    fclose( lfp );
    // Remove new_launch_req so we don't attempt to process it again.
    remove( "/etc/registry/launch_req/new_launch_req" );
    if( bw != 1 ) {
        exit( EINVAL );
    }
    // Reload launch_req from drive.
    return zdj_registry_load_launch_req( "/etc/registry/launch_req/launch_req" );
}

// Load a launch request from Zero's local storage.
// Get the app install names from the local drive, 
// then populate the reg_install pointers by loading
// a matching persisted zdj_install_t from storage.
zdj_launch_req_t * zdj_registry_load_launch_req( char * launch_req_path ) {
    // Load the request from storage
    zdj_launch_req_t * req = malloc( sizeof( zdj_launch_req_t ) );
    zdj_registry_retrieve_launch_req_file( launch_req_path, req );
    req->app_install_name = &req->stored_app_install_name;
    req->fallback_install_name = &req->stored_fallback_install_name;
    // Attach installs from storage + set req health
    zdj_registry_load_launch_req_installs( req );

    return req;
}

void zdj_registry_retrieve_launch_req_file( char * path, zdj_launch_req_t * launch_req ) {
    FILE * fp;
    if ( access( path, F_OK ) == 0 ) {
        fp = fopen( path, "r" );
        int br = fread( launch_req, sizeof( zdj_launch_req_t ), 1, fp );
        fclose( fp );
        if( br == 1 ) {
            launch_req->health = ZDJ_REGISTRY_HEALTH_UNKNOWN;
        } else {
            launch_req->health = ZDJ_REGISTRY_HEALTH_BAD_RECORD;
        }
    } else {
        launch_req->health = ZDJ_REGISTRY_HEALTH_NO_RECORD;
    }
}

// Populate app/fallback install records of a launch_req from storage.
void zdj_registry_load_launch_req_installs( zdj_launch_req_t * launch_req ) {
    // If we have a NULL app_install_name, we just tried
    // to switch to a non-existing fallback.
    // Declare the launch_req to be unhealthy and return.
    if( launch_req->app_install_name == NULL ) {
        launch_req->health = ZDJ_REGISTRY_HEALTH_NO_RECORD;
        return;
    }

    // Load main app install record from storage
    launch_req->app_install = zdj_registry_install_for_install_name( launch_req->app_install_name );
    if( launch_req->app_install->health > ZDJ_REGISTRY_HEALTH_UNKNOWN ) {
        launch_req->health = launch_req->app_install->health;
    }
    // Look for + load fallback install record
    // fallback is optional, don't break health if it's NULL
    if( launch_req->fallback_install_name ) {
        launch_req->fallback_install = zdj_registry_install_for_install_name( launch_req->fallback_install_name );
    } else {
        launch_req->fallback_install = NULL;
    }
}

void zdj_registry_free_launch_req( zdj_launch_req_t * launch_req ) {

}

// Move a launch request into its fallback state.
// Move the fallback app into the primary app and reload the installs from storage.
void zdj_registry_launch_req_switch_to_fallback( zdj_launch_req_t * launch_req ) {
    free( launch_req->app_install );
    launch_req->app_install_name = launch_req->fallback_install_name;
    launch_req->fallback_install_name = NULL;
    // Attach installs from storage + set req health
    zdj_registry_load_launch_req_installs( launch_req );
}
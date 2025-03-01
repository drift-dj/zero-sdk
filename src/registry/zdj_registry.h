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

#ifndef ZDJ_REGISTRY_H
#define ZDJ_REGISTRY_H

#include <stdint.h>

#include <zerodj/zdj_data_type.h>
#include <zerodj/health/zdj_health_type.h>

#define ZDJ_REGISTRY_LAUNCH_REQ_PATH "/etc/registry/launch-req/launch_req"
#define ZDJ_REGISTRY_NEW_LAUNCH_REQ_PATH "/etc/registry/launch-req/new_launch_req"

typedef struct {
    char registry_name[256];
    char display_name[128];
    char short_name[16];
    char description[1024];
    char category[32];
    char uuid[32];
    char bin_dir[2048];
    zdj_version_t version;
    zdj_health_status_t health;
    bool protected;
    struct zdj_install_t * next;
    struct zdj_install_t * prev;
} zdj_install_t;

typedef struct {
    zdj_install_t app_install;
    zdj_install_t fallback_install;
    zdj_health_status_t health;
} zdj_launch_req_t;

// Install
zdj_install_t * zdj_registry_create_install( 
    char * registry_name,
    char * display_name,
    char * bin_dir,
    char * desc,
    uint8_t v_maj,
    uint8_t v_min,
    uint8_t v_hf,
    uint16_t v_b
);
zdj_install_t * zdj_registry_installs( void );
zdj_install_t * zdj_registry_installs_for_category( char * category );
zdj_install_t * zdj_registry_install_for_name( char * name );
zdj_install_t * zdj_registry_install_for_filepath( char * path );
void zdj_registry_commit_install( zdj_install_t * install );
int zdj_registry_write_install( zdj_install_t * install );
void zdj_registry_remove_install( zdj_install_t * install );
void zdj_registry_free_install( zdj_install_t * install );

// Launch Request
zdj_launch_req_t * zdj_registry_create_launch_req( 
    char * app_install_name, 
    char * fallback_install_name 
);
int zdj_registry_commit_launch_req( zdj_launch_req_t * launch_req );
zdj_launch_req_t * zdj_registry_process_new_launch_req( void );
zdj_launch_req_t * zdj_registry_load_launch_req( char * path );
int zdj_registry_write_launch_req( char * path, zdj_launch_req_t * launch_req );

void zdj_registry_free_launch_req( zdj_launch_req_t * launch_req );
void zdj_registry_launch_req_switch_to_fallback( zdj_launch_req_t * launch_req );

#endif
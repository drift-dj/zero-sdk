#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "zerodj/boot/zdj_boot.h"

#define ZERO_BUILD_INSTALL_DIR "/usr/local/bin/zero-build"
#ifdef __APPLE__
    #define ZERO_BUILD_DATA_DIR "/opt/zero-build/"
    #define ZERO_BUILD_DATA_PATH "/opt/zero-build/zero_build_data"
    #define ZERO_BUILD_REG_DIR "/opt/zero-build/boot_registry/"
#elif __linux
    #define ZERO_BUILD_DATA_DIR "/var/local/zero-build/"
    #define ZERO_BUILD_DATA_PATH "/var/local/zero-build/zero_build_data"
    #define ZERO_BUILD_REG_DIR "/var/local/zero-build/boot_registry/"
#endif

typedef enum {
    BUILD_STATUS_UNKNOWN,
    BUILD_STATUS_OKAY,
    BUILD_STATUS_ERROR,
    BUILD_STATUS_NOT_LINKED,
    BUILD_STATUS_NOT_FOUND,
    BUILD_STATUS_NOT_AN_APP,
    BUILD_STATUS_SDK_NOT_FOUND,
    BUILD_STATUS_SDK_NOT_LINKED,
    BUILD_STATUS_SYSROOT_LINK_FAILED,
    BUILD_STATUS_DOCKER_NOT_CREATED,
    BUILD_STATUS_DOCKER_NOT_STARTED,
    BUILD_STATUS_DOCKER_ERROR,
    BUILD_STATUS_XML_PARSE_ERROR
} build_status_t;

typedef struct {
    bool build_sdk;
    bool build_apps[ 64 ];
    bool build_lib_db;
    bool build_m7;
    bool build_registry;
    bool build_os_bootloader;
    bool build_os_kernel_source;
    bool build_os_kernel_image;
    bool build_os_rootfs_source;
    bool build_os_rootfs_image;
    bool build_os_media_image;
    bool build_os_image;
    bool re_flash;
    bool upload;
    bool flash_drift_os;
    bool load_sdk;
    bool load_sdk_assets;
    bool load_apps[ 64 ];
    bool load_registry;
    bool load_zerodb;
    build_status_t status;
} workflow_model_t;

#define ZERO_BUILD_BOOT_APP_NUM 0
#define ZERO_BUILD_CONFIG_APP_NUM 1
#define ZERO_BUILD_DRIVE_APP_NUM 2
#define ZERO_BUILD_DEV_APP_NUM 3
#define ZERO_BUILD_LIB_APP_NUM 4
#define ZERO_BUILD_DJ_APP_NUM 5
#define ZERO_BUILD_SDK_TEST_APP_NUM 6

typedef struct {
    char name[ 256 ];
    bool link;
    char path[ 1024 ];
    bool stop_workflow_on_error;
    bool bootable;
    bool make_reg_install;
    build_status_t status;
} app_model_t;

typedef struct {
    char boot_app_name[ 256 ];
    zdj_boot_mode_t boot_mode;
    bool stop_workflow_on_error;
    build_status_t status;
} app_registry_model_t;

typedef struct {
    bool repo_link;
    char repo_path[ 1024 ];
    bool toolchain_link;
    char toolchain_path[ 1024 ];
    bool stop_workflow_on_error;
    build_status_t status;
} m7_model_t;

typedef enum {
    OS_BUILDER_STATE_NONE,
    OS_BUILDER_STATE_ATTEMPT_CREATE,
    OS_BUILDER_STATE_CREATE_FAILED,
    OS_BUILDER_STATE_CREATED,
    OS_BUILDER_STATE_OKAY,
    OS_BUILDER_STATE_UNREACHABLE
} os_builder_state_t;

typedef struct {
    bool repo_link;
    char repo_path[ 1024 ];
    bool builder_link;
    char builder_image_id[ 64 ];
    char bootloader_bin_path[ 256 ];
    char os_img_path[ 256 ];
    os_builder_state_t os_builder_state_t_state;
    bool stop_workflow_on_error;
    build_status_t status;
} os_model_t;

typedef struct {
    bool link;
    char path[ 1024 ];
    char sysroot[ 1024 ];
    bool stop_workflow_on_error;
    build_status_t status;
} sdk_model_t;

typedef struct {
    bool docker_link;
    char docker_image_path[ 256 ];
    bool uuu_link;
    char uuu_path[ 256 ];
    build_status_t status;
} toolchain_model_t;

typedef struct {
    workflow_model_t workflow;
    app_model_t apps[ 64 ];
    int app_count;
    app_registry_model_t reg;
    m7_model_t m7;
    os_model_t os;
    sdk_model_t sdk;
    toolchain_model_t toolchain;
} zero_build_data_t;

extern zero_build_data_t * build_data;
extern char app_dir[ 2048 ];

bool load_data( zero_build_data_t * data );
void store_data( zero_build_data_t * data );
void make_init_data( zero_build_data_t * data );
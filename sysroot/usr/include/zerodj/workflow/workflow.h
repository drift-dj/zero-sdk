#pragma once

#include "data.h"

void run_workflow( zero_build_data_t * data );

bool should_build_os( zero_build_data_t * data );
bool os_image_exists( zero_build_data_t * data );

build_status_t run_m7_build( zero_build_data_t * data );
build_status_t run_sdk_build( zero_build_data_t * data );
build_status_t run_app_build( zero_build_data_t * data, int app_num );
build_status_t run_registry_build( zero_build_data_t * data );
build_status_t run_drift_os_pre_install( zero_build_data_t * data );
build_status_t run_drift_os_build( zero_build_data_t * data );

build_status_t run_uploads( zero_build_data_t * data );
build_status_t run_os_reflash( zero_build_data_t * data );
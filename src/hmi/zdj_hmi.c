#include <zerodj/hmi/zdj_hmi.h>
#include <zerodj/hmi/zdj_hmi_shared_state_model.h>
#include <zerodj/m7/zdj_m7.h>

void zdj_hmi_activate( void ) {
    // Signal M7 core to begin scanning HMI muxes and collecting state data
    zdj_m7_msg( ZDJ_M7_ACTIVATE_HMI );
}

void zdj_hmi_deactivate( void ) {
    zdj_m7_msg( ZDJ_M7_DEACTIVATE_HMI );
}

void zdj_hmi_update( void ) {
    // Process M7's hmi input model from shared memory
    // Signal M7 to flush a new model into shared memory
    zdj_m7_msg( ZDJ_M7_FLUSH_HMI );
}
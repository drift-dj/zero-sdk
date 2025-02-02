#ifndef ZDJ_HMI_SHARED_STATE_MODEL_H
#define ZDJ_HMI_SHARED_STATE_MODEL_H

#include <stdint.h>

#define HMI_MODEL_TIMESLICE_COUNT 32

typedef struct {
    int8_t out_state[ HMI_MODEL_TIMESLICE_COUNT ];
    uint8_t out_pb_state;
    int8_t jog_state[ HMI_MODEL_TIMESLICE_COUNT ];
    uint8_t jog_pb_state;
    int8_t tone_1_state[ HMI_MODEL_TIMESLICE_COUNT ];
    uint8_t tone_1_pb_state;
    int8_t tone_2_state[ HMI_MODEL_TIMESLICE_COUNT ];
    uint8_t tone_2_pb_state;
    int8_t tone_3_state[ HMI_MODEL_TIMESLICE_COUNT ];
    uint8_t tone_3_pb_state;
    uint8_t hotcue_pb_state;
    uint8_t play_pb_state;
    uint8_t nav_pb_state;
    uint8_t fn_1_pb_state;
    uint8_t fn_2_pb_state;
    uint8_t fn_3_pb_state;
    int8_t fade_1_state;
    int8_t fade_2_state;
    int8_t xfade_state;
} zdj_hmi_shared_state_model_t;

#endif
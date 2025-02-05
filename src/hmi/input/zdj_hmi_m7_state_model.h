#ifndef ZDJ_HMI_SHARED_STATE_MODEL_H
#define ZDJ_HMI_SHARED_STATE_MODEL_H

#include <stdint.h>

typedef struct {
    int32_t out_state;
    int32_t out_pb_state;
    int32_t jog_state;
    int32_t jog_pb_state;
    int32_t tone_1_state;
    int32_t tone_1_pb_state;
    int32_t tone_2_state;
    int32_t tone_2_pb_state;
    int32_t tone_3_state;
    int32_t tone_3_pb_state;
    int32_t hotcue_pb_state;
    int32_t play_pb_state;
    int32_t nav_pb_state;
    int32_t fn_1_pb_state;
    int32_t fn_2_pb_state;
    int32_t fn_3_pb_state;
    int32_t fade_1_state;
    int32_t fade_2_state;
    int32_t xfade_state;
} zdj_hmi_m7_state_model_t;

#endif
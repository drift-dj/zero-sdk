#ifndef ZDJ_HMI_SHARED_STATE_MODEL_H
#define ZDJ_HMI_SHARED_STATE_MODEL_H

#include <stdint.h>

// Model structure for shared HMI state
typedef enum {
    ZDJ_HMI_ENCO_STATE_IDLE,
    ZDJ_HMI_ENCO_STATE_PRESS_1,
    ZDJ_HMI_ENCO_STATE_UP_1,
    ZDJ_HMI_ENCO_STATE_PRESS_2,
    ZDJ_HMI_ENCO_STATE_UP_2,
    ZDJ_HMI_ENCO_STATE_PRESS_TURN,
    ZDJ_HMI_ENCO_STATE_LONG_PRESS,
    ZDJ_HMI_ENCO_STATE_LONG_PRESS_UP,
    ZDJ_HMI_ENCO_STATE_TURN,
    ZDJ_HMI_ENCO_STATE_DEBOUNCE,
} zdj_hmi_enco_state_t;

typedef enum {
    ZDJ_HMI_ENCO_EVENT_NONE,
    ZDJ_HMI_ENCO_EVENT_PRESS,
    ZDJ_HMI_ENCO_EVENT_SHORT_RELEASE,
    ZDJ_HMI_ENCO_EVENT_DOUBLE_RELEASE,
    ZDJ_HMI_ENCO_EVENT_LONG_RELEASE,
    ZDJ_HMI_ENCO_EVENT_ADJUST,
    ZDJ_HMI_ENCO_EVENT_PRESS_ADJUST,
    ZDJ_HMI_ENCO_EVENT_PRESS_ADJUST_RELEASE
} zdj_hmi_enco_event_t;

typedef enum {
    ZDJ_HMI_PB_STATE_IDLE,
    ZDJ_HMI_PB_STATE_PRESS_1,
    ZDJ_HMI_PB_STATE_UP_1,
    ZDJ_HMI_PB_STATE_PRESS_2,
    ZDJ_HMI_PB_STATE_UP_2,
    ZDJ_HMI_PB_STATE_LONG_PRESS,
    ZDJ_HMI_PB_STATE_LONG_PRESS_UP,
    ZDJ_HMI_PB_STATE_DEBOUNCE,
} zdj_hmi_pb_state_t;

typedef enum {
    ZDJ_HMI_PB_EVENT_NONE,
    ZDJ_HMI_PB_EVENT_PRESS,
    ZDJ_HMI_PB_EVENT_MOMENTARY_RELEASE,
    ZDJ_HMI_PB_EVENT_SHORT_RELEASE,
    ZDJ_HMI_PB_EVENT_DOUBLE_RELEASE,
    ZDJ_HMI_PB_EVENT_LONG_RELEASE,
} zdj_hmi_pb_event_t;

typedef enum {
    ZDJ_HMI_POT_EVENT_NONE,
    ZDJ_HMI_POT_EVENT_ADJUST,
} zdj_hmi_pot_event_t;

typedef struct {
    uint8_t current_state;
    uint8_t current_event;
    uint8_t press_emitted;
    int16_t current_event_val;
    uint16_t turn_press_timer;
    uint16_t turn_hyst_timer;
    uint16_t long_press_timer;
    uint16_t double_press_timer;
    uint16_t debounce_timer;
} zdj_hmi_enco_state_model_t;

typedef struct {
    uint8_t current_state;
    uint8_t current_event;
    uint8_t press_emitted;
    uint8_t momentary_release_emitted;
    int16_t current_event_val;
    uint16_t long_press_timer;
    uint16_t double_press_timer;
    uint16_t debounce_timer;
} zdj_hmi_pb_state_model_t;

typedef struct {
    zdj_hmi_pot_event_t current_event;
    int16_t current_event_val;
} zdj_hmi_pot_state_model_t;

typedef struct {
    zdj_hmi_enco_state_model_t out_state;
    zdj_hmi_enco_state_model_t jog_state;
    zdj_hmi_enco_state_model_t tone_1_state;
    zdj_hmi_enco_state_model_t tone_2_state;
    zdj_hmi_enco_state_model_t tone_3_state;
    zdj_hmi_pb_state_model_t out_pb_state;
    zdj_hmi_pb_state_model_t jog_pb_state;
    zdj_hmi_pb_state_model_t tone_1_pb_state;
    zdj_hmi_pb_state_model_t tone_2_pb_state;
    zdj_hmi_pb_state_model_t tone_3_pb_state;
    zdj_hmi_pb_state_model_t hotcue_state;
    zdj_hmi_pb_state_model_t play_state;
    zdj_hmi_pb_state_model_t nav_state;
    zdj_hmi_pb_state_model_t fn_1_state;
    zdj_hmi_pb_state_model_t fn_2_state;
    zdj_hmi_pb_state_model_t fn_3_state;
} zdj_hmi_shared_state_model_t;

#endif
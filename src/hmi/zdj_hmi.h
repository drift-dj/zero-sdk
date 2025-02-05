#ifndef ZDJ_HMI_H
#define ZDJ_HMI_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    ZDJ_HMI_ENCO_1_VOL,
    ZDJ_HMI_ENCO_2_JOG,
    ZDJ_HMI_ENCO_3_TONE_1,
    ZDJ_HMI_ENCO_4_TONE_2,
    ZDJ_HMI_ENCO_5_TONE_3,
    ZDJ_HMI_PB_0_HOTCUE,
    ZDJ_HMI_PB_1_PLAY,
    ZDJ_HMI_PB_2_FN_1,
    ZDJ_HMI_PB_3_FN_2,
    ZDJ_HMI_PB_4_FN_3,
    ZDJ_HMI_PB_5_NAV,
    ZDJ_HMI_POT_0_CH_1,
    ZDJ_HMI_POT_1_CH_2,
    ZDJ_HMI_POT_2_XFADE,
    ZDJ_HMI_CONTROL_ID_COUNT
} zdj_hmi_control_id_t;

static char * zdj_hmi_control_name[ ZDJ_HMI_CONTROL_ID_COUNT ] = {
    "Volume",// ZDJ_HMI_ENCO_1_VOL,
    "Jog",// ZDJ_HMI_ENCO_2_JOG,
    "Tone 1",// ZDJ_HMI_ENCO_3_TONE_1,
    "Tone 2",// ZDJ_HMI_ENCO_4_TONE_2,
    "Tone 3",// ZDJ_HMI_ENCO_5_TONE_3,
    "Hotcue",// ZDJ_HMI_PB_0_HOTCUE,
    "Play",// ZDJ_HMI_PB_1_PLAY,
    "F(x) 1",// ZDJ_HMI_PB_2_FN_1,
    "F(x) 2",// ZDJ_HMI_PB_3_FN_2,
    "F(x) 3",// ZDJ_HMI_PB_4_FN_3,
    "Nav",// ZDJ_HMI_PB_5_NAV,
    "Fade 1",// ZDJ_HMI_POT_0_CH_1,
    "Fade 2",// ZDJ_HMI_POT_1_CH_2,
    "XFade"// ZDJ_HMI_POT_2_XFADE,
};

typedef enum {
    ZDJ_HMI_EVENT_PRESS,
    ZDJ_HMI_EVENT_RELEASE,
    ZDJ_HMI_EVENT_MOD_RELEASE,
    ZDJ_HMI_EVENT_LONG_PRESS,
    ZDJ_HMI_EVENT_LONG_RELEASE,
    ZDJ_HMI_EVENT_ADJUST,
    ZDJ_HMI_EVENT_PRESS_ADJUST,
    ZDJ_HMI_EVENT_PRESS_ADJUST_RELEASE,
    ZDJ_HMI_EVENT_TYPE_COUNT
} zdj_hmi_event_type_t;

static char * zdj_hmi_event_name[ ZDJ_HMI_EVENT_TYPE_COUNT ] = {
    "Press",// ZDJ_HMI_EVENT_PRESS,
    "Release",// ZDJ_HMI_EVENT_RELEASE,
    "Mod Release",// ZDJ_HMI_EVENT_MOD_RELEASE,
    "Long Press",// ZDJ_HMI_EVENT_LONG_PRESS,
    "Long Release",// ZDJ_HMI_EVENT_LONG_RELEASE,
    "Adjust",// ZDJ_HMI_EVENT_ADJUST,
    "Press Adjust",// ZDJ_HMI_EVENT_PRESS_ADJUST,
    "Press Adjust Release"// ZDJ_HMI_EVENT_PRESS_ADJUST_RELEASE
};

typedef struct {
    zdj_hmi_control_id_t id;
    zdj_hmi_event_type_t type;
    uint16_t mod_bitmap;
    bool blocked;
    int i_val;
    double d_val;
    bool b_val;
    struct zdj_hmi_event_t * next;
    struct zdj_hmi_event_t * prev;
} zdj_hmi_event_t;

typedef enum {
    ZDJ_HMI_STATE_IDLE,
    ZDJ_HMI_STATE_PRESS,
    ZDJ_HMI_STATE_MOD_PRESS,
    ZDJ_HMI_STATE_UP,
    ZDJ_HMI_STATE_MOD_UP,
    ZDJ_HMI_STATE_PRESS_TURN,
    ZDJ_HMI_STATE_LONG_PRESS,
    ZDJ_HMI_STATE_LONG_PRESS_UP,
    ZDJ_HMI_STATE_TURN,
    ZDJ_HMI_STATE_DEBOUNCE,
} zdj_hmi_control_state_type_t;

typedef struct {
    zdj_hmi_control_id_t id;
    zdj_hmi_control_state_type_t current_state;
    bool is_modifier;
    bool is_modified;
    bool press_emitted;
    bool long_press_emitted;
    uint16_t turn_press_timer;
    uint16_t turn_hyst_timer;
    uint16_t long_press_timer;
    uint16_t debounce_timer;
} zdj_hmi_control_state_t;


// Head of the linked list of unprocessed events
// Will be NULL if no events need to be processed
extern zdj_hmi_event_t * zdj_hmi_event_base;
extern zdj_hmi_event_t * zdj_hmi_event_tip;

// 
extern zdj_hmi_control_state_t ** zdj_hmi_control_states;

void zdj_hmi_init( void );
void zdj_hmi_activate( void );
void zdj_hmi_deactivate( void );
void zdj_hmi_create_events( bool should_flush );
void zdj_hmi_clear_events( void );

#endif
#ifndef ZDJ_HMI_INPUT_H
#define ZDJ_HMI_INPUT_H

// Bitmaps used to build modifier key status for each control
extern uint16_t zdj_hmi_mod_bitmap;

void zdj_hmi_init_input( void );
void zdj_hmi_process_input( void );

void zdj_hmi_input_push_event( zdj_hmi_event_t * event );
zdj_hmi_event_t * zdj_hmi_input_new_event( zdj_hmi_control_id_t id, zdj_hmi_event_type_t type, bool is_modified );
void zdj_hmi_input_clear_events( void );

void zdj_hmi_process_digital_control( zdj_hmi_control_state_t * control_state, int32_t enco_val, int32_t pb_state );
void zdj_hmi_process_analog_control( zdj_hmi_control_state_t * control_state, int32_t pot_val );

#endif
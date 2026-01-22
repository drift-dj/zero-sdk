#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include <zerodj/controls/hmi/zdj_hmi_input.h>

void zdj_control_process_hmi_analog_input( zdj_hmi_input_state_t * input, int32_t pot_val ) {
    zdj_hmi_input_event_t * event;

    // printf( "zdj_control_process_hmi_analog_input: %s %d\n", zdj_hmi_input_name[ input->id ], pot_val );

    switch ( input->current_state ) {
        case ZDJ_HMI_STATE_IDLE:
            // printf( "%s ZDJ_HMI_STATE_IDLE\n", zdj_hmi_input_name[ input->id ] );
            // Stay in IDLE until pot moves enough to exit hysteresis.
            // This keeps a noisy pot from constantly changing value.
            if( abs( pot_val - input->adjust_hyst_val) > 2 ) {
                input->turn_hyst_timer = 0;
                input->adjust_hyst_val = pot_val;
                input->current_state = ZDJ_HMI_STATE_TURN;
            }
            break;
        case ZDJ_HMI_STATE_TURN:
            // printf( "%s ZDJ_HMI_STATE_TURN\n", zdj_hmi_input_name[ input->id ] );
            // If val stays in hysteresis range for longer than [500 cycles], enter idle
            if( (abs( pot_val - input->adjust_hyst_val ) < 3) && 
                (input->turn_hyst_timer++ > 200)
            ) {
                // printf( "enter idle\n" );
                input->current_state = ZDJ_HMI_STATE_IDLE;
            } else {
                if( pot_val != input->adjust_prev_val ) {
                    input->adjust_prev_val = pot_val;
                    // printf( "emit %s adjust: %d\n", zdj_hmi_input_name[ input->id ], pot_val );
                    event = &zdj_hmi_input_event_buf[ zdj_get_next_hmi_input_event_ind( ) ];
                    event->id = input->id;
                    event->type = ZDJ_HMI_EVENT_ADJUST;
                    event->i_val = pot_val;
                }

                if(abs( pot_val - input->adjust_hyst_val ) > 2) {
                    input->turn_hyst_timer = 0;
                    input->adjust_hyst_val = pot_val;
                }
            }
            break;

    }   
}
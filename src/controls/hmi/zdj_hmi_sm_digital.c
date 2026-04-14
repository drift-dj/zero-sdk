#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <zerodj/controls/zdj_controls.h>
#include <zerodj/controls/hmi/zdj_hmi_input.h>

#define ZDJ_HMI_WINDOW_DEBOUNCE 3 // Wait time after button is released before returning to IDLE
#define ZDJ_HMI_WINDOW_TURN_PRESS 0 // Wait time after a new turn starts before emitting ADJUSTs -- lets us disregard micro-turns when user really wanted a press
#define ZDJ_HMI_WINDOW_TURN_HYST 40 // Remain in TURN without upvals for this long before returning to IDLE
#define ZDJ_HMI_THRESH_TURN 2 // upvals less than this will not promote a push into a push-turn -- basically a sensitivity adjust to prevent post-push micro-turns from taking over
#define ZDJ_HMI_WINDOW_LONG_PRESS 460 // Wait time to scan for a long press

void _zdj_hmi_promote_mods_for_control( zdj_hmi_input_state_t * control );

void zdj_control_process_hmi_digital_input( 
    zdj_hmi_input_state_t * input, 
    int32_t enco_val, 
    int32_t pb_state 
) {
    zdj_hmi_input_event_t * event;

    // Reset input's is_modified state if the mod key has been released.
    if( !zdj_hmi_mod_bitmap ) { input->is_modified = false; }
    
    switch ( input->current_state ) {
    case ZDJ_HMI_STATE_IDLE:
        // Look for turn
        if( enco_val != 0 ) {
            // Promote any controls in press state to modifiers
            _zdj_hmi_promote_mods_for_control( input );
            input->current_state = ZDJ_HMI_STATE_TURN;
            // If modifier keys are present, set modified
            if( zdj_hmi_mod_bitmap ) {
                input->is_modified = true;
            }
            event = &zdj_hmi_input_event_buf[ zdj_get_next_hmi_input_event_ind( ) ];
            event->id = input->id;
            // event->type = ZDJ_HMI_EVENT_ADJUST;
            if( input->is_modified ) { event->type = ZDJ_HMI_EVENT_MOD_ADJUST; } 
            else { event->type = ZDJ_HMI_EVENT_ADJUST; }
            event->i_val = enco_val;
        }
        // Look for press
        if( pb_state == true ) {
            // Promote any controls in press state to modifiers
            _zdj_hmi_promote_mods_for_control( input );
            input->current_state = ZDJ_HMI_STATE_PRESS;

            // If this is an enco, capture the adjust val at press.
            // Use this to detect press-turn condition.
            input->adjust_hyst_val = 0;

            // If modifier keys are present, set modified
            // printf( "mod bmp: %d\n", zdj_hmi_mod_bitmap );
            if( zdj_hmi_mod_bitmap ) {
                input->is_modified = true;
            }
        }
        break;
    case ZDJ_HMI_STATE_TURN:
        // printf( "%s ZDJ_HMI_STATE_TURN: is_mod=%d\n", zdj_hmi_input_name[ input->id ], input->is_modified );
        // increment turn-press debounce timer
        input->turn_press_timer++;
        input->turn_hyst_timer++;
        // If turn-press debounce timer has expired && turn has value update
        if( input->turn_press_timer > ZDJ_HMI_WINDOW_TURN_PRESS && enco_val != 0 ) {
            // Emit ADJUST
            event = &zdj_hmi_input_event_buf[ zdj_get_next_hmi_input_event_ind( ) ];
            event->id = input->id;
            if( input->is_modified ) { event->type = ZDJ_HMI_EVENT_MOD_ADJUST; } 
            else { event->type = ZDJ_HMI_EVENT_ADJUST; }
            event->i_val = enco_val;
            // Reset hysteresis timer every time there's an update
            input->turn_hyst_timer = 0;
        }
        // Stay in TURN until hysteresis expires with no update -- then enter IDLE
        if( input->turn_hyst_timer > ZDJ_HMI_WINDOW_TURN_HYST )
            input->current_state = ZDJ_HMI_STATE_IDLE;
        // Look for press -- Enter PRESS
        if( pb_state == true )
            input->current_state = ZDJ_HMI_STATE_PRESS;
        break;
    case ZDJ_HMI_STATE_PRESS: // First press
        // printf( "%s ZDJ_HMI_STATE_PRESS: %d turn: %d/%d\n", zdj_hmi_input_name[ input->id ], input->id );
        input->long_press_timer++; 
        // Emit PRESS only once
        if( !input->press_emitted ) {
            input->press_emitted = true;
            event = &zdj_hmi_input_event_buf[ zdj_get_next_hmi_input_event_ind( ) ];
            event->id = input->id;
            if( input->is_modified ) { event->type = ZDJ_HMI_EVENT_MOD_PRESS; } 
            else { event->type = ZDJ_HMI_EVENT_PRESS; }
        }
        // If turn magnitude rises above threshold while pressing -- Enter PRESS_TURN
        input->adjust_hyst_val += enco_val;
        if( abs(input->adjust_hyst_val) > ZDJ_HMI_THRESH_TURN )
            input->current_state = ZDJ_HMI_STATE_PRESS_TURN;
        // If long-press timer expires while pressing -- Enter LONG_PRESS
        if( input->long_press_timer > ZDJ_HMI_WINDOW_LONG_PRESS )
            // input->current_state = ZDJ_HMI_STATE_LONG_PRESS;
            if( input->is_modified ) { input->current_state = ZDJ_HMI_STATE_MOD_LONG_PRESS; } 
            else { input->current_state = ZDJ_HMI_STATE_LONG_PRESS; }
        // Look for release -- Enter UP
        if( pb_state == false )
            input->current_state = ZDJ_HMI_STATE_UP;
        break;
    case ZDJ_HMI_STATE_UP: // First release
        // printf( "%s ZDJ_HMI_STATE_UP mod:%d\n", zdj_hmi_input_name[ input->id ], input->is_modified );
        // Emit SHORT_RELEASE
        event = &zdj_hmi_input_event_buf[ zdj_get_next_hmi_input_event_ind( ) ];
        event->id = input->id;
        if( input->is_modified ) { event->type = ZDJ_HMI_EVENT_MOD_RELEASE; } 
        else { event->type = ZDJ_HMI_EVENT_RELEASE; }
        // Enter DEBOUNCE
        input->current_state = ZDJ_HMI_STATE_DEBOUNCE;
        break;
    case ZDJ_HMI_STATE_MOD_PRESS:
        // printf( "%s ZDJ_HMI_STATE_MOD_PRESS\n", zdj_hmi_input_name[ input->id ] );
        input->long_press_timer++; 

        if( pb_state == false )
            input->current_state = ZDJ_HMI_STATE_MOD_UP;
        break;
    case ZDJ_HMI_STATE_MOD_UP:
        // printf( "%s ZDJ_HMI_STATE_MOD_UP\n", zdj_hmi_input_name[ input->id ] );
        // Emit MOD_RELEASE
        // event = &zdj_hmi_input_event_buf[ zdj_get_next_hmi_input_event_ind( ) ];
        // event->id = input->id;
        // event->type = ZDJ_HMI_EVENT_MOD_RELEASE;
        // Enter DEBOUNCE
        zdj_hmi_mod_bitmap = 0;
        input->current_state = ZDJ_HMI_STATE_DEBOUNCE;
        break;
    case ZDJ_HMI_STATE_PRESS_TURN:
        // printf( "%s ZDJ_HMI_STATE_PRESS_TURN\n", zdj_hmi_input_name[ input->id ] );
        // Emit PRESS_ADJUST
        if( enco_val != 0 ) {
            event = &zdj_hmi_input_event_buf[ zdj_get_next_hmi_input_event_ind( ) ];
            event->id = input->id;
            event->i_val = enco_val;
            event->type = ZDJ_HMI_EVENT_PRESS_ADJUST;
            if( input->is_modified ) { event->type = ZDJ_HMI_EVENT_MOD_PRESS_ADJUST; } 
            else { event->type = ZDJ_HMI_EVENT_PRESS_ADJUST; }
        }
        // Look for release -- Emit ADJUST_RELEASE and enter DEBOUNCE
        if( pb_state == false ) {
            // printf( "ZDJ_HMI_STATE_PRESS_TURN pb_val: %d\n", pb_state );
            // Emit PRESS_ADJUST_RELEASE
            event = &zdj_hmi_input_event_buf[ zdj_get_next_hmi_input_event_ind( ) ];
            event->id = input->id;
            event->type = ZDJ_HMI_EVENT_PRESS_ADJUST_RELEASE;
            // Enter DEBOUNCE
            input->current_state = ZDJ_HMI_STATE_DEBOUNCE;
        }
        break;
    case ZDJ_HMI_STATE_LONG_PRESS:
        // printf( "%s ZDJ_HMI_STATE_LONG_PRESS\n", zdj_hmi_input_name[ input->id ] );
        // Emit PRESS only once
        if( !input->long_press_emitted ) {
            input->long_press_emitted = true;
            event = &zdj_hmi_input_event_buf[ zdj_get_next_hmi_input_event_ind( ) ];
            event->id = input->id;
            event->type = ZDJ_HMI_EVENT_LONG_PRESS;
        }
        // Look for release -- Enter LONG_PRESS_UP
        if( pb_state == false )
            input->current_state = ZDJ_HMI_STATE_LONG_PRESS_UP;
        break;
    case ZDJ_HMI_STATE_MOD_LONG_PRESS:
        // printf( "%s ZDJ_HMI_STATE_MOD_LONG_PRESS\n", zdj_hmi_input_name[ input->id ] );
        // Emit PRESS only once
        if( !input->long_press_emitted ) {
            input->long_press_emitted = true;
            event = &zdj_hmi_input_event_buf[ zdj_get_next_hmi_input_event_ind( ) ];
            event->id = input->id;
            event->type = ZDJ_HMI_EVENT_MOD_LONG_PRESS;
        }
        // Look for release -- Enter LONG_PRESS_UP
        if( pb_state == false )
            input->current_state = ZDJ_HMI_STATE_MOD_LONG_PRESS_UP;
        break;
    case ZDJ_HMI_STATE_LONG_PRESS_UP:
        // printf( "%s ZDJ_HMI_STATE_LONG_PRESS_UP\n", zdj_hmi_input_name[ input->id ] );
        // Emit LONG_RELEASE
        event = &zdj_hmi_input_event_buf[ zdj_get_next_hmi_input_event_ind( ) ];
        event->id = input->id;
        event->type = ZDJ_HMI_EVENT_LONG_RELEASE;
        // Enter DEBOUNCE
        input->current_state = ZDJ_HMI_STATE_DEBOUNCE;
        break;
    case ZDJ_HMI_STATE_MOD_LONG_PRESS_UP:
        // printf( "%s ZDJ_HMI_STATE_MOD_LONG_PRESS_UP\n", zdj_hmi_input_name[ input->id ] );
        // Emit LONG_RELEASE
        event = &zdj_hmi_input_event_buf[ zdj_get_next_hmi_input_event_ind( ) ];
        event->id = input->id;
        event->type = ZDJ_HMI_EVENT_MOD_LONG_RELEASE;
        // Enter DEBOUNCE
        input->current_state = ZDJ_HMI_STATE_DEBOUNCE;
        break;
    case ZDJ_HMI_STATE_DEBOUNCE:
        // printf( "%s ZDJ_HMI_STATE_DEBOUNCE\n", zdj_hmi_input_name[ input->id ] );
        // Wait for debounce timer to expire
        input->debounce_timer++;
        // Clear PRESS emitted flag
        input->press_emitted = false;
        // Clear LONG_PRESS emitted flag
        input->long_press_emitted = false;
        if( input->debounce_timer > ZDJ_HMI_WINDOW_DEBOUNCE ) {
            // Enter IDLE
            input->current_state = ZDJ_HMI_STATE_IDLE;
            
            input->turn_press_timer = 0;
            input->turn_hyst_timer = 0;
            input->long_press_timer = 0;
            input->debounce_timer = 0;
            if( input->is_modified ) {
                // zdj_hmi_mod_bitmap = 0;
                input->is_modified = false;
            }
            if( input->is_modifier ) {
                input->is_modifier = false;
            }
        }
        break;
    default:
        break;
    }
}

// Turn any controls currently in press state into modifiers
void _zdj_hmi_promote_mods_for_control( zdj_hmi_input_state_t * input ) {
    zdj_hmi_input_state_t * c;
    for( int i=0; i<ZDJ_HMI_CONTROL_ID_COUNT; i++ ) {
        // Don't allow Fn btns to become mods - required for momentary deck select function
        if( i == ZDJ_HMI_PB_2_FN_1 || i == ZDJ_HMI_PB_3_FN_2 || i == ZDJ_HMI_PB_4_FN_3 ) {
            continue;
        }
        c = zdj_hmi_input_states[ i ];
        if( c->id == input->id ) {
            // Do not promote the calling control to mod.
            // In the case of a press-turn, calling control will be in press state.
            continue;
        }
        if( c->current_state == ZDJ_HMI_STATE_PRESS ||
            c->current_state == ZDJ_HMI_STATE_LONG_PRESS
        ) {
            c->current_state = ZDJ_HMI_STATE_MOD_PRESS;
            c->is_modifier = true;
            c->is_modified = false;
            zdj_hmi_mod_bitmap |= (1 << i);
            printf( "promoting %s to mod:%d\n", zdj_hmi_input_name[ i ], zdj_hmi_mod_bitmap );
        }
    }
}

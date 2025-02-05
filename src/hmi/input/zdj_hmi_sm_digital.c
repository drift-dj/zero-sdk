#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <zerodj/hmi/zdj_hmi.h>
#include <zerodj/hmi/input/zdj_hmi_input.h>

#define ZDJ_HMI_WINDOW_DEBOUNCE 3 // Wait time after button is released before returning to IDLE
#define ZDJ_HMI_WINDOW_TURN_PRESS 0 // Wait time after a new turn starts before emitting ADJUSTs -- lets us disregard micro-turns when user really wanted a press
#define ZDJ_HMI_WINDOW_TURN_HYST 40 // Remain in TURN without upvals for this long before returning to IDLE
#define ZDJ_HMI_THRESH_TURN 2 // upvals less than this will not promote a push into a push-turn -- basically a sensitivity adjust to prevent post-push micro-turns from taking over
#define ZDJ_HMI_WINDOW_LONG_PRESS 100 // Wait time to scan for a long press

void _zdj_hmi_promote_mods_for_control( zdj_hmi_control_state_t * control );

void zdj_hmi_process_digital_control( 
    zdj_hmi_control_state_t * control, 
    int32_t enco_val, 
    int32_t pb_state 
    ) {
    zdj_hmi_event_t * event;

    switch ( control->current_state ) {
    case ZDJ_HMI_STATE_IDLE:
        // Look for turn
        if( enco_val != 0 ) {
            // Promote any controls in press state to modifiers
            _zdj_hmi_promote_mods_for_control( control );
            control->current_state = ZDJ_HMI_STATE_TURN;
            // If modifier keys are present, set modified
            if( zdj_hmi_mod_bitmap ) {
                control->is_modified = true;
            }
            event = zdj_hmi_input_new_event( control->id, ZDJ_HMI_EVENT_ADJUST, control->is_modified );
            event->i_val = enco_val;
            zdj_hmi_input_push_event( event );
        }
        // Look for press
        if( pb_state == true ) {
            // Promote any controls in press state to modifiers
            _zdj_hmi_promote_mods_for_control( control );
            control->current_state = ZDJ_HMI_STATE_PRESS;
            // If modifier keys are present, set modified
            if( zdj_hmi_mod_bitmap ) {
                control->is_modified = true;
            }
        }
        break;
    case ZDJ_HMI_STATE_TURN:
        // printf( "%s ZDJ_HMI_STATE_TURN\n", zdj_hmi_control_name[ control->id ] );
        // increment turn-press debounce timer
        control->turn_press_timer++;
        control->turn_hyst_timer++;
        // If turn-press debounce timer has expired && turn has value update
        if( control->turn_press_timer > ZDJ_HMI_WINDOW_TURN_PRESS && enco_val != 0 ) {
        // if( enco_val != 0 ) {
            // Emit ADJUST
            event = zdj_hmi_input_new_event( control->id, ZDJ_HMI_EVENT_ADJUST, control->is_modified );
            event->i_val = enco_val;
            zdj_hmi_input_push_event( event );
            // Reset hysteresis timer every time there's an update
            control->turn_hyst_timer = 0;
        }
        // Stay in TURN until hysteresis expires with no update -- then enter IDLE
        if( control->turn_hyst_timer > ZDJ_HMI_WINDOW_TURN_HYST )
            control->current_state = ZDJ_HMI_STATE_IDLE;
        // Look for press -- Enter PRESS
        if( pb_state == true )
            control->current_state = ZDJ_HMI_STATE_PRESS;
        break;
    case ZDJ_HMI_STATE_PRESS: // First press
        // printf( "%s ZDJ_HMI_STATE_PRESS\n", zdj_hmi_control_name[ control->id ] );
        control->long_press_timer++; 
        // Emit PRESS only once
        if( !control->press_emitted ) {
            control->press_emitted = true;
            event = zdj_hmi_input_new_event( control->id, ZDJ_HMI_EVENT_PRESS, control->is_modified );
            zdj_hmi_input_push_event( event );
        }
        // If turn magnitude rises above threshold while pressing -- Enter PRESS_TURN
        if( abs(enco_val) > ZDJ_HMI_THRESH_TURN )
            control->current_state = ZDJ_HMI_STATE_PRESS_TURN;
        // If long-press timer expires while pressing -- Enter LONG_PRESS
        if( control->long_press_timer > ZDJ_HMI_WINDOW_LONG_PRESS )
            control->current_state = ZDJ_HMI_STATE_LONG_PRESS;
        // Look for release -- Enter UP
        if( pb_state == false )
            control->current_state = ZDJ_HMI_STATE_UP;
        break;
    case ZDJ_HMI_STATE_UP: // First release
        // printf( "%s ZDJ_HMI_STATE_UP\n", zdj_hmi_control_name[ control->id ] );
        // Emit SHORT_RELEASE
        event = zdj_hmi_input_new_event( control->id, ZDJ_HMI_EVENT_RELEASE, control->is_modified );
        zdj_hmi_input_push_event( event );
        // Enter DEBOUNCE
        control->current_state = ZDJ_HMI_STATE_DEBOUNCE;
        break;
    case ZDJ_HMI_STATE_MOD_PRESS:
        // printf( "%s ZDJ_HMI_STATE_MOD_PRESS\n", zdj_hmi_control_name[ control->id ] );
        if( pb_state == false )
            control->current_state = ZDJ_HMI_STATE_MOD_UP;
        break;
    case ZDJ_HMI_STATE_MOD_UP:
        // printf( "%s ZDJ_HMI_STATE_MOD_UP\n", zdj_hmi_control_name[ control->id ] );
        // Emit MOD_RELEASE
        event = zdj_hmi_input_new_event( control->id, ZDJ_HMI_EVENT_MOD_RELEASE, control->is_modified );
        zdj_hmi_input_push_event( event );
        // Enter DEBOUNCE
        control->current_state = ZDJ_HMI_STATE_DEBOUNCE;
        break;
    case ZDJ_HMI_STATE_PRESS_TURN:
        // printf( "%s ZDJ_HMI_STATE_PRESS_TURN\n", zdj_hmi_control_name[ control->id ] );
        // Emit PRESS_ADJUST
        if( enco_val != 0 ) {
            event = zdj_hmi_input_new_event( control->id, ZDJ_HMI_EVENT_PRESS_ADJUST, control->is_modified );
            event->i_val = enco_val;
            zdj_hmi_input_push_event( event );
        }
        // Look for release -- Emit ADJUST_RELEASE and enter DEBOUNCE
        if( pb_state == false ) {
            // printf( "ZDJ_HMI_STATE_PRESS_TURN pb_val: %d\n", pb_state );
            // Emit PRESS_ADJUST_RELEASE
                        // control->current_event = HMI_ENCO_EVENT_PRESS_ADJUST_RELEASE;
            event = zdj_hmi_input_new_event( control->id, ZDJ_HMI_EVENT_PRESS_ADJUST_RELEASE, control->is_modified );
            zdj_hmi_input_push_event( event );
            // Enter DEBOUNCE
            control->current_state = ZDJ_HMI_STATE_DEBOUNCE;
        }
        break;
    case ZDJ_HMI_STATE_LONG_PRESS:
        // printf( "%s ZDJ_HMI_STATE_LONG_PRESS\n", zdj_hmi_control_name[ control->id ] );
        // Emit PRESS only once
        if( !control->long_press_emitted ) {
            control->long_press_emitted = true;
                        // control->current_event = ZDJ_HMI_EVENT_LONG_PRESS;
            zdj_hmi_event_t * event = zdj_hmi_input_new_event( control->id, ZDJ_HMI_EVENT_LONG_PRESS, control->is_modified );
            zdj_hmi_input_push_event( event );
        }
        // Look for release -- Enter LONG_PRESS_UP
        if( pb_state == false )
            control->current_state = ZDJ_HMI_STATE_LONG_PRESS_UP;
        break;
    case ZDJ_HMI_STATE_LONG_PRESS_UP:
        // printf( "%s ZDJ_HMI_STATE_LONG_PRESS_UP\n", zdj_hmi_control_name[ control->id ] );
        // Emit LONG_RELEASE
        event = zdj_hmi_input_new_event( control->id, ZDJ_HMI_EVENT_LONG_RELEASE, control->is_modified );
        zdj_hmi_input_push_event( event );
        // Enter DEBOUNCE
        control->current_state = ZDJ_HMI_STATE_DEBOUNCE;
        break;
    case ZDJ_HMI_STATE_DEBOUNCE:
        // printf( "%s ZDJ_HMI_STATE_DEBOUNCE\n", zdj_hmi_control_name[ control->id ] );
        // Wait for debounce timer to expire
        control->debounce_timer++;
        // Clear PRESS emitted flag
        control->press_emitted = false;
        if( control->debounce_timer > ZDJ_HMI_WINDOW_DEBOUNCE ) {
            // Enter IDLE
            control->current_state = ZDJ_HMI_STATE_IDLE;
            
            control->turn_press_timer = 0;
            control->turn_hyst_timer = 0;
            control->long_press_timer = 0;
            control->debounce_timer = 0;
            if( control->is_modified ) {
                zdj_hmi_mod_bitmap = 0;
                control->is_modified = false;
            }
            if( control->is_modifier ) {
                control->is_modifier = false;
            }
        }
        break;
    default:
        break;
    }
}

// Turn any controls currently in press state into modifiers
void _zdj_hmi_promote_mods_for_control( zdj_hmi_control_state_t * control ) {
    zdj_hmi_control_state_t * c;
    for( int i=0; i<ZDJ_HMI_CONTROL_ID_COUNT; i++ ) {
        c = zdj_hmi_control_states[ i ];
        if( c->id == control->id ) {
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
            // printf( "promoting %s to mod:%d\n", zdj_hmi_control_name[ i ], zdj_hmi_mod_bitmap );
        }
    }
}

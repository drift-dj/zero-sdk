#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <zerodj/hmi/zdj_hmi.h>
#include <zerodj/hmi/input/zdj_hmi_input.h>
#include <zerodj/hmi/input/zdj_hmi_m7_state_model.h>

volatile zdj_hmi_m7_state_model_t * zdj_hmi_m7_state_model;
zdj_hmi_control_state_t ** zdj_hmi_control_states;
uint16_t zdj_hmi_mod_bitmap;

zdj_hmi_event_t * zdj_hmi_event_base;
zdj_hmi_event_t * zdj_hmi_event_tip;

void _zdj_hmi_free_event( zdj_hmi_event_t * event );

void zdj_hmi_init_input( void ) {
    // Grab a ref to the hmi model memory region used by the m7
    int mem_fd = open( "/dev/mem", O_RDWR );
	zdj_hmi_m7_state_model = mmap(0, 0x20000, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, 0x55a31000);

    zdj_hmi_mod_bitmap = 0;

    // Init the individual control_state instances
    zdj_hmi_control_states = malloc( sizeof( zdj_hmi_control_state_t* ) * ZDJ_HMI_CONTROL_ID_COUNT );

    // Enco 1 - Out vol
    zdj_hmi_control_state_t * vol_enco = calloc( sizeof( zdj_hmi_control_state_t ), 1 );
    vol_enco->id = ZDJ_HMI_ENCO_1_VOL;
    zdj_hmi_control_states[ ZDJ_HMI_ENCO_1_VOL ] = vol_enco;
    // Enco 2 - Jog wheel
    zdj_hmi_control_state_t * jog_enco = calloc( sizeof( zdj_hmi_control_state_t ), 1 );
    jog_enco->id = ZDJ_HMI_ENCO_2_JOG;
    zdj_hmi_control_states[ ZDJ_HMI_ENCO_2_JOG ] = jog_enco;
    // Enco 3 - Tone 1
    zdj_hmi_control_state_t * tone_1_enco = calloc( sizeof( zdj_hmi_control_state_t ), 1 );
    tone_1_enco->id = ZDJ_HMI_ENCO_3_TONE_1;
    zdj_hmi_control_states[ ZDJ_HMI_ENCO_3_TONE_1 ] = tone_1_enco;
    // Enco 4 - Tone 2
    zdj_hmi_control_state_t * tone_2_enco = calloc( sizeof( zdj_hmi_control_state_t ), 1 );
    tone_2_enco->id = ZDJ_HMI_ENCO_4_TONE_2;
    zdj_hmi_control_states[ ZDJ_HMI_ENCO_4_TONE_2 ] = tone_2_enco;
    // Enco 5 - Tone 3
    zdj_hmi_control_state_t * tone_3_enco = calloc( sizeof( zdj_hmi_control_state_t ), 1 );
    tone_3_enco->id = ZDJ_HMI_ENCO_5_TONE_3;
    zdj_hmi_control_states[ ZDJ_HMI_ENCO_5_TONE_3 ] = tone_3_enco;

    // PB 0 - Hotcue
    zdj_hmi_control_state_t * hotcue_pb = calloc( sizeof( zdj_hmi_control_state_t ), 1 );
    hotcue_pb->id = ZDJ_HMI_PB_0_HOTCUE;
    zdj_hmi_control_states[ ZDJ_HMI_PB_0_HOTCUE ] = hotcue_pb;
    // PB 1 - Play
    zdj_hmi_control_state_t * play_pb = calloc( sizeof( zdj_hmi_control_state_t ), 1 );
    play_pb->id = ZDJ_HMI_PB_1_PLAY;
    zdj_hmi_control_states[ ZDJ_HMI_PB_1_PLAY ] = play_pb;
    // PB 2 - Fn 1
    zdj_hmi_control_state_t * fn_1_pb = calloc( sizeof( zdj_hmi_control_state_t ), 1 );
    fn_1_pb->id = ZDJ_HMI_PB_2_FN_1;
    zdj_hmi_control_states[ ZDJ_HMI_PB_2_FN_1 ] = fn_1_pb;
    // PB 3 - Fn 2
    zdj_hmi_control_state_t * fn_2_pb = calloc( sizeof( zdj_hmi_control_state_t ), 1 );
    fn_2_pb->id = ZDJ_HMI_PB_3_FN_2;
    zdj_hmi_control_states[ ZDJ_HMI_PB_3_FN_2 ] = fn_2_pb;
    // PB 4 - Fn 3
    zdj_hmi_control_state_t * fn_3_pb = calloc( sizeof( zdj_hmi_control_state_t ), 1 );
    fn_3_pb->id = ZDJ_HMI_PB_4_FN_3;
    zdj_hmi_control_states[ ZDJ_HMI_PB_4_FN_3 ] = fn_3_pb;
    // PB 5 - Nav
    zdj_hmi_control_state_t * nav_pb = calloc( sizeof( zdj_hmi_control_state_t ), 1 );
    nav_pb->id = ZDJ_HMI_PB_5_NAV;
    zdj_hmi_control_states[ ZDJ_HMI_PB_5_NAV ] = nav_pb;

    // Pot 0 - Ch 1 Fade
    zdj_hmi_control_state_t * fade_1_pot = calloc( sizeof( zdj_hmi_control_state_t ), 1 );
    fade_1_pot->id = ZDJ_HMI_POT_0_CH_1;
    zdj_hmi_control_states[ ZDJ_HMI_POT_0_CH_1 ] = fade_1_pot;
    // Pot 1 - Ch 2 Fade
    zdj_hmi_control_state_t * fade_2_pot = calloc( sizeof( zdj_hmi_control_state_t ), 1 );
    fade_2_pot->id = ZDJ_HMI_POT_1_CH_2;
    zdj_hmi_control_states[ ZDJ_HMI_POT_1_CH_2 ] = fade_2_pot;
    // Pot 2 - Crossfade
    zdj_hmi_control_state_t * xfade_pot = calloc( sizeof( zdj_hmi_control_state_t ), 1 );
    xfade_pot->id = ZDJ_HMI_POT_2_XFADE;
    zdj_hmi_control_states[ ZDJ_HMI_POT_2_XFADE ] = xfade_pot;
}

void zdj_hmi_process_input( void ) {
    // Read the most recent m7 HMI state model and generate events for changes
    zdj_hmi_process_digital_control( 
        zdj_hmi_control_states[ ZDJ_HMI_ENCO_1_VOL ], 
        zdj_hmi_m7_state_model->out_state,
        zdj_hmi_m7_state_model->out_pb_state
    );
    zdj_hmi_process_digital_control( 
        zdj_hmi_control_states[ ZDJ_HMI_ENCO_2_JOG ],
        zdj_hmi_m7_state_model->jog_state,
        zdj_hmi_m7_state_model->jog_pb_state  
    );
    zdj_hmi_process_digital_control( 
        zdj_hmi_control_states[ ZDJ_HMI_ENCO_3_TONE_1 ],
        zdj_hmi_m7_state_model->tone_1_state,
        zdj_hmi_m7_state_model->tone_1_pb_state 
    );
    zdj_hmi_process_digital_control( 
        zdj_hmi_control_states[ ZDJ_HMI_ENCO_4_TONE_2 ],
        zdj_hmi_m7_state_model->tone_2_state,
        zdj_hmi_m7_state_model->tone_2_pb_state
    );
    zdj_hmi_process_digital_control( 
        zdj_hmi_control_states[ ZDJ_HMI_ENCO_5_TONE_3 ],
        zdj_hmi_m7_state_model->tone_3_state,
        zdj_hmi_m7_state_model->tone_3_pb_state
    );

    zdj_hmi_process_digital_control( 
        zdj_hmi_control_states[ ZDJ_HMI_PB_0_HOTCUE ],
        0,
        zdj_hmi_m7_state_model->hotcue_pb_state 
    );
    zdj_hmi_process_digital_control( 
        zdj_hmi_control_states[ ZDJ_HMI_PB_1_PLAY ],
        0,
        zdj_hmi_m7_state_model->play_pb_state 
    );
    zdj_hmi_process_digital_control( 
        zdj_hmi_control_states[ ZDJ_HMI_PB_2_FN_1 ],
        0,
        zdj_hmi_m7_state_model->fn_1_pb_state 
    );
    zdj_hmi_process_digital_control( 
        zdj_hmi_control_states[ ZDJ_HMI_PB_3_FN_2 ],
        0,
        zdj_hmi_m7_state_model->fn_2_pb_state 
    );
    zdj_hmi_process_digital_control( 
        zdj_hmi_control_states[ ZDJ_HMI_PB_4_FN_3 ],
        0,
        zdj_hmi_m7_state_model->fn_3_pb_state 
    );
    zdj_hmi_process_digital_control( 
        zdj_hmi_control_states[ ZDJ_HMI_PB_5_NAV ],
        0,
        zdj_hmi_m7_state_model->nav_pb_state 
    );

    zdj_hmi_process_analog_control( 
        zdj_hmi_control_states[ ZDJ_HMI_POT_0_CH_1 ],
        zdj_hmi_m7_state_model->fade_1_state 
    );
    zdj_hmi_process_analog_control( 
        zdj_hmi_control_states[ ZDJ_HMI_POT_1_CH_2 ],
        zdj_hmi_m7_state_model->fade_2_state 
    );
    zdj_hmi_process_analog_control( 
        zdj_hmi_control_states[ ZDJ_HMI_POT_2_XFADE ],
        zdj_hmi_m7_state_model->xfade_state 
    );
}


// Push a new event to the tip of the stack
void zdj_hmi_input_push_event( zdj_hmi_event_t * event ) {
    event->next = event->prev = NULL;
    if( !zdj_hmi_event_base && !zdj_hmi_event_tip ) {
        zdj_hmi_event_base = zdj_hmi_event_tip = event;
    } else {
        zdj_hmi_event_tip->next = (struct zdj_hmi_event_t*)event;
        event->prev = zdj_hmi_event_tip;
        zdj_hmi_event_tip = event;
    }
}

// Create a new event with type/id
zdj_hmi_event_t * zdj_hmi_input_new_event( zdj_hmi_control_id_t id, zdj_hmi_event_type_t type, bool is_modified ) {
    zdj_hmi_event_t * e = malloc( sizeof( zdj_hmi_event_t ) );
    e->id = id;
    e->type = type;
    if( is_modified ) {
        e->mod_bitmap = zdj_hmi_mod_bitmap;
    }
    return e;
}

void zdj_hmi_input_clear_events( void ) {
    zdj_hmi_event_t * event = zdj_hmi_event_tip;
    while( event ) {
        _zdj_hmi_free_event( event );
        event = event->prev;
    }
    zdj_hmi_event_tip = zdj_hmi_event_base = NULL;
}

void _zdj_hmi_free_event( zdj_hmi_event_t * event ) {
    // Put the event list into a safe state before deleting the event
    zdj_hmi_event_t * next = (zdj_hmi_event_t *)event->next;
    zdj_hmi_event_t * prev = (zdj_hmi_event_t *)event->prev;
    if( next && prev ) { // If we're between 2 live events, splice the list back together
        next->prev = prev;
        prev = next;
    } else if( next && !prev ) { // If there's no prev, but there is a next (we're at base)
        next->prev = NULL;
        zdj_hmi_event_base = next;
    }  else if( !next && prev ) { // If there's no next, but there is a prev (we're at tip)
        prev->next = NULL;
        zdj_hmi_event_tip = prev;
    } else if( !next && !prev ) { // No next/prev - we're a singleton - clear base/tip
        zdj_hmi_event_base = zdj_hmi_event_tip = NULL;
    }

    // Clear the event
    event->next = NULL;
    event->prev = NULL;
    free( event );
}
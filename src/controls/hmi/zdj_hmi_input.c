#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>

#include <zerodj/controls/zdj_controls.h>
#include <zerodj/controls/hmi/zdj_hmi_input.h>
#include <zerodj/controls/hmi/zdj_hmi_m7_state_model.h>
#include <zerodj/system/perf/zdj_perf.h>

volatile zdj_hmi_m7_state_model_t * zdj_hmi_m7_state_model;
zdj_hmi_input_state_t ** zdj_hmi_input_states;
uint16_t zdj_hmi_mod_bitmap;

zdj_hmi_input_event_t zdj_hmi_input_event_buf[ ZDJ_HMI_INPUT_EVENT_BUF_LEN ];
int zdj_hmi_input_event_buf_write = 0;
int zdj_hmi_input_event_buf_read = 0;

void zdj_control_hmi_input_init( void ) {
    // Grab a ref to the hmi model memory region used by the m7
    int mem_fd = open( "/dev/mem", O_RDWR );
	zdj_hmi_m7_state_model = mmap(0, 0x20000, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, 0x55a11000);
    // memset( zdj_hmi_m7_state_model, 0, sizeof( zdj_hmi_m7_state_model_t ) );
    // Reset pushbutton states to inactive/hi
    zdj_hmi_m7_state_model->out_pb_state = 0;
    zdj_hmi_m7_state_model->out_state = 0;
    zdj_hmi_m7_state_model->jog_pb_state = 0;
    zdj_hmi_m7_state_model->jog_state = 0;
    zdj_hmi_m7_state_model->tone_1_pb_state = 0;
    zdj_hmi_m7_state_model->tone_1_state = 0;
    zdj_hmi_m7_state_model->tone_2_pb_state = 0;
    zdj_hmi_m7_state_model->tone_2_state = 0;
    zdj_hmi_m7_state_model->tone_3_pb_state = 0;
    zdj_hmi_m7_state_model->tone_3_state = 0;
    zdj_hmi_m7_state_model->hotcue_pb_state = 0;
    zdj_hmi_m7_state_model->play_pb_state = 0;
    zdj_hmi_m7_state_model->nav_pb_state = 0;
    zdj_hmi_m7_state_model->fn_1_pb_state = 0;
    zdj_hmi_m7_state_model->fn_2_pb_state = 0;
    zdj_hmi_m7_state_model->fn_3_pb_state = 0;

    zdj_hmi_mod_bitmap = 0;
    // hmi_scan_thread_flush = false;

    // Init the individual control_state instances
    zdj_hmi_input_states = malloc( sizeof( zdj_hmi_input_state_t* ) * ZDJ_HMI_CONTROL_ID_COUNT );

    // Enco 1 - Out vol
    zdj_hmi_input_state_t * vol_enco = calloc( sizeof( zdj_hmi_input_state_t ), 1 );
    vol_enco->id = ZDJ_HMI_ENCO_1_VOL;
    zdj_hmi_input_states[ ZDJ_HMI_ENCO_1_VOL ] = vol_enco;
    // Enco 2 - Jog wheel
    zdj_hmi_input_state_t * jog_enco = calloc( sizeof( zdj_hmi_input_state_t ), 1 );
    jog_enco->id = ZDJ_HMI_ENCO_2_JOG;
    zdj_hmi_input_states[ ZDJ_HMI_ENCO_2_JOG ] = jog_enco;
    // Enco 3 - Tone 1
    zdj_hmi_input_state_t * tone_1_enco = calloc( sizeof( zdj_hmi_input_state_t ), 1 );
    tone_1_enco->id = ZDJ_HMI_ENCO_3_TONE_1;
    zdj_hmi_input_states[ ZDJ_HMI_ENCO_3_TONE_1 ] = tone_1_enco;
    // Enco 4 - Tone 2
    zdj_hmi_input_state_t * tone_2_enco = calloc( sizeof( zdj_hmi_input_state_t ), 1 );
    tone_2_enco->id = ZDJ_HMI_ENCO_4_TONE_2;
    zdj_hmi_input_states[ ZDJ_HMI_ENCO_4_TONE_2 ] = tone_2_enco;
    // Enco 5 - Tone 3
    zdj_hmi_input_state_t * tone_3_enco = calloc( sizeof( zdj_hmi_input_state_t ), 1 );
    tone_3_enco->id = ZDJ_HMI_ENCO_5_TONE_3;
    zdj_hmi_input_states[ ZDJ_HMI_ENCO_5_TONE_3 ] = tone_3_enco;

    // PB 0 - Hotcue
    zdj_hmi_input_state_t * hotcue_pb = calloc( sizeof( zdj_hmi_input_state_t ), 1 );
    hotcue_pb->id = ZDJ_HMI_PB_0_HOTCUE;
    zdj_hmi_input_states[ ZDJ_HMI_PB_0_HOTCUE ] = hotcue_pb;
    // PB 1 - Play
    zdj_hmi_input_state_t * play_pb = calloc( sizeof( zdj_hmi_input_state_t ), 1 );
    play_pb->id = ZDJ_HMI_PB_1_PLAY;
    zdj_hmi_input_states[ ZDJ_HMI_PB_1_PLAY ] = play_pb;
    // PB 2 - Fn 1
    zdj_hmi_input_state_t * fn_1_pb = calloc( sizeof( zdj_hmi_input_state_t ), 1 );
    fn_1_pb->id = ZDJ_HMI_PB_2_FN_1;
    zdj_hmi_input_states[ ZDJ_HMI_PB_2_FN_1 ] = fn_1_pb;
    // PB 3 - Fn 2
    zdj_hmi_input_state_t * fn_2_pb = calloc( sizeof( zdj_hmi_input_state_t ), 1 );
    fn_2_pb->id = ZDJ_HMI_PB_3_FN_2;
    zdj_hmi_input_states[ ZDJ_HMI_PB_3_FN_2 ] = fn_2_pb;
    // PB 4 - Fn 3
    zdj_hmi_input_state_t * fn_3_pb = calloc( sizeof( zdj_hmi_input_state_t ), 1 );
    fn_3_pb->id = ZDJ_HMI_PB_4_FN_3;
    zdj_hmi_input_states[ ZDJ_HMI_PB_4_FN_3 ] = fn_3_pb;
    // PB 5 - Nav
    zdj_hmi_input_state_t * nav_pb = calloc( sizeof( zdj_hmi_input_state_t ), 1 );
    nav_pb->id = ZDJ_HMI_PB_5_NAV;
    zdj_hmi_input_states[ ZDJ_HMI_PB_5_NAV ] = nav_pb;

    // Pot 0 - Ch 1 Fade
    zdj_hmi_input_state_t * fade_1_pot = calloc( sizeof( zdj_hmi_input_state_t ), 1 );
    fade_1_pot->id = ZDJ_HMI_POT_0_CH_1;
    zdj_hmi_input_states[ ZDJ_HMI_POT_0_CH_1 ] = fade_1_pot;
    // Pot 1 - Ch 2 Fade
    zdj_hmi_input_state_t * fade_2_pot = calloc( sizeof( zdj_hmi_input_state_t ), 1 );
    fade_2_pot->id = ZDJ_HMI_POT_1_CH_2;
    zdj_hmi_input_states[ ZDJ_HMI_POT_1_CH_2 ] = fade_2_pot;
    // Pot 2 - Crossfade
    zdj_hmi_input_state_t * xfade_pot = calloc( sizeof( zdj_hmi_input_state_t ), 1 );
    xfade_pot->id = ZDJ_HMI_POT_2_XFADE;
    zdj_hmi_input_states[ ZDJ_HMI_POT_2_XFADE ] = xfade_pot;
}

void zdj_control_process_hmi_input( void ) {
    // Read the most recent m7 HMI state model and generate events for changes
    zdj_control_process_hmi_digital_input( 
        zdj_hmi_input_states[ ZDJ_HMI_ENCO_1_VOL ], 
        zdj_hmi_m7_state_model->out_state,
        zdj_hmi_m7_state_model->out_pb_state
    );
    zdj_control_process_hmi_digital_input( 
        zdj_hmi_input_states[ ZDJ_HMI_ENCO_2_JOG ],
        zdj_hmi_m7_state_model->jog_state,
        zdj_hmi_m7_state_model->jog_pb_state  
    );
    zdj_control_process_hmi_digital_input( 
        zdj_hmi_input_states[ ZDJ_HMI_ENCO_3_TONE_1 ],
        zdj_hmi_m7_state_model->tone_1_state,
        zdj_hmi_m7_state_model->tone_1_pb_state 
    );
    zdj_control_process_hmi_digital_input( 
        zdj_hmi_input_states[ ZDJ_HMI_ENCO_4_TONE_2 ],
        zdj_hmi_m7_state_model->tone_2_state,
        zdj_hmi_m7_state_model->tone_2_pb_state
    );
    zdj_control_process_hmi_digital_input( 
        zdj_hmi_input_states[ ZDJ_HMI_ENCO_5_TONE_3 ],
        zdj_hmi_m7_state_model->tone_3_state,
        zdj_hmi_m7_state_model->tone_3_pb_state
    );

    zdj_control_process_hmi_digital_input( 
        zdj_hmi_input_states[ ZDJ_HMI_PB_0_HOTCUE ],
        0,
        zdj_hmi_m7_state_model->hotcue_pb_state 
    );
    zdj_control_process_hmi_digital_input( 
        zdj_hmi_input_states[ ZDJ_HMI_PB_1_PLAY ],
        0,
        zdj_hmi_m7_state_model->play_pb_state 
    );
    zdj_control_process_hmi_digital_input( 
        zdj_hmi_input_states[ ZDJ_HMI_PB_2_FN_1 ],
        0,
        zdj_hmi_m7_state_model->fn_1_pb_state 
    );
    zdj_control_process_hmi_digital_input( 
        zdj_hmi_input_states[ ZDJ_HMI_PB_3_FN_2 ],
        0,
        zdj_hmi_m7_state_model->fn_2_pb_state 
    );
    zdj_control_process_hmi_digital_input( 
        zdj_hmi_input_states[ ZDJ_HMI_PB_4_FN_3 ],
        0,
        zdj_hmi_m7_state_model->fn_3_pb_state 
    );
    zdj_control_process_hmi_digital_input( 
        zdj_hmi_input_states[ ZDJ_HMI_PB_5_NAV ],
        0,
        zdj_hmi_m7_state_model->nav_pb_state 
    );

    zdj_control_process_hmi_analog_input( 
        zdj_hmi_input_states[ ZDJ_HMI_POT_0_CH_1 ],
        zdj_hmi_m7_state_model->fade_1_state 
    );
    zdj_control_process_hmi_analog_input( 
        zdj_hmi_input_states[ ZDJ_HMI_POT_1_CH_2 ],
        zdj_hmi_m7_state_model->fade_2_state 
    );
    zdj_control_process_hmi_analog_input( 
        zdj_hmi_input_states[ ZDJ_HMI_POT_2_XFADE ],
        zdj_hmi_m7_state_model->xfade_state 
    );
}

// Analyze each new event in HMI Input events ring buffer.
// Using current event_transform_state, make new UI/Deck Control events.
void zdj_control_transform_hmi_events( void ) {
    // Loop on new events
    if( zdj_hmi_input_event_buf_read != zdj_hmi_input_event_buf_write ) {
        int i = zdj_hmi_input_event_buf_read;
        zdj_control_event_t event;
        while ( i != zdj_hmi_input_event_buf_write ) {
            i++;
            i %= ZDJ_CONTROL_EVENT_BUF_LEN; // Loop i in ring buffer
            if( zdj_control_map_hmi_input_event( &zdj_hmi_input_event_buf[ i ], &event ) ) {
                if( zdj_control_event_is_ui_control( &event ) ) {
                    // If input event maps to an active UI control, make a UI Control event
                    int write_head = zdj_get_next_ui_event_ind( );
                    zdj_ui_event_buf[ write_head ].id = event.id;
                    zdj_ui_event_buf[ write_head ].blocked = event.blocked;
                    zdj_ui_event_buf[ write_head ].i_val = event.i_val;
                    zdj_ui_event_buf[ write_head ].d_val = event.d_val;
                    zdj_ui_event_buf[ write_head ].b_val = event.b_val;
                } else if( zdj_control_event_is_deck_control( &event ) ) {
                    // If input event maps to an active Deck control, make a Deck Control event
                    int write_head = zdj_get_next_deck_event_ind( );
                    zdj_ui_event_buf[ write_head ].id = event.id;
                    zdj_ui_event_buf[ write_head ].blocked = event.blocked;
                    zdj_ui_event_buf[ write_head ].i_val = event.i_val;
                    zdj_ui_event_buf[ write_head ].d_val = event.d_val;
                    zdj_ui_event_buf[ write_head ].b_val = event.b_val;
                }
                // Unmapped HMI Input events are ignored
            }
        }
    }
    // Bring read head to meet write head
    zdj_hmi_input_event_buf_read = zdj_hmi_input_event_buf_write;
}

// Increment and bound and return write head.
int zdj_get_next_hmi_input_event_ind( void ) {
    zdj_hmi_input_event_buf_write++;
    zdj_hmi_input_event_buf_write %= ZDJ_CONTROL_EVENT_BUF_LEN;
    return zdj_hmi_input_event_buf_write;
}
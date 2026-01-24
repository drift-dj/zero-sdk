#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>


#include <zerodj/controls/zdj_controls.h>
#include <zerodj/controls/hmi/zdj_hmi_input.h>


void _zdj_control_copy_hmi_to_control_event( 
    zdj_control_id_t id,
    zdj_hmi_input_event_t * in_e, 
    zdj_control_event_t * c_e 
);


// Main internal HMI input event mapping point.
// Events from the onboard HMI input scan are processed here.
// We decide whether an event should be mapped to a UI control or a deck control.
// Ex. depending on UI state, a jog turn will either scroll an onscreen menu,
// or scrub playback of one of the decks.
// Since UI, HMI, and playback threads are totally asynchronous, UI has to declare event
// capture intent on its thread, then we check capture state and route events from 
// here on the Controls thread.
bool zdj_control_map_hmi_input_event( zdj_hmi_input_event_t * in_e, zdj_control_event_t * c_e ) {

    // Jog Knob
    if( in_e->id == ZDJ_HMI_ENCO_2_JOG ) {
        // printf( "jog event: %s\n", zdj_hmi_input_event_name[ in_e->type ] );
        // First check for UI event capture
        if( in_e->type == ZDJ_HMI_EVENT_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_JOG_PRESS_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_JOG_PRESS_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_JOG_RELEASE_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_JOG_RELEASE_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_JOG_PRESS_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_JOG_PRESS_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_RELEASE ) {
            if ( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_JOG_RELEASE_1 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_JOG_RELEASE_1, in_e, c_e ); return true; 
            }
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_PRESS ) {
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TOGGLE_PANEL ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TOGGLE_PANEL, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_JOG_PRESS_2 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_JOG_PRESS_2, in_e, c_e ); return true; 
            }
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_JOG_RELEASE_2 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_JOG_RELEASE_2, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_LONG_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_JOG_PRESS_3 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_JOG_PRESS_3, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_JOG_RELEASE_3 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_JOG_RELEASE_3, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_ADJUST ) {
            // Sort Jog scroll events among UI/Decks
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_JOG_ADJUST_0 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_JOG_ADJUST_0, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_SCRUB ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_SCRUB, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_SCRUB ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_SCRUB, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_XPORT_CONTROL_SCRUB ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_XPORT_CONTROL_SCRUB, in_e, c_e ); return true; 
            }
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_ADJUST ) {
            // Sort Jog scroll events among UI/Decks
            if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_TEMPO_FINE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_TEMPO_FINE, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_TEMPO_FINE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_TEMPO_FINE, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_EXT_CONTROL_TEMPO_FINE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_EXT_CONTROL_TEMPO_FINE, in_e, c_e ); return true; 
            }  else if( zdj_control_active_state.controls[ ZDJ_DECK_XPORT_CONTROL_TEMPO_FINE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_XPORT_CONTROL_TEMPO_FINE, in_e, c_e ); return true; 
            } 
        } else if( in_e->type == ZDJ_HMI_EVENT_PRESS_ADJUST ) {
            // Sort Jog push-turn events among UI/Decks
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_JOG_ADJUST_1 ] ) {
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_JOG_ADJUST_1, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_TEMPO ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_TEMPO, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_TEMPO ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_TEMPO, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_EXT_CONTROL_TEMPO ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_EXT_CONTROL_TEMPO, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_XPORT_CONTROL_TEMPO ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_XPORT_CONTROL_TEMPO, in_e, c_e ); return true; 
            }  
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_PRESS_ADJUST ) {
            // Sort Jog scroll events among UI/Decks
            if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_TEMPO_FINE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_TEMPO, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_TEMPO ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_TEMPO, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_EXT_CONTROL_TEMPO ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_EXT_CONTROL_TEMPO, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_XPORT_CONTROL_TEMPO ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_XPORT_CONTROL_TEMPO, in_e, c_e ); return true; 
            } 
        }
    }
    
    
    // Out/Vol Knob
    else if( in_e->id == ZDJ_HMI_ENCO_1_VOL ) {
        if( in_e->type == ZDJ_HMI_EVENT_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_OUT_PRESS_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_OUT_PRESS_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_OUT_RELEASE_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_OUT_RELEASE_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_PRESS ) {
            
            if ( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_OUT_PRESS_1 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_OUT_PRESS_1, in_e, c_e ); return true; 
            }
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_RELEASE ) {
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_OUT_RELEASE_1 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_OUT_RELEASE_1, in_e, c_e ); return true; 
            }
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_PRESS ) {
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_OUT_PRESS_2 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_OUT_PRESS_2, in_e, c_e ); return true; 
            }
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_OUT_RELEASE_2 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_OUT_RELEASE_2, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_LONG_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_OUT_PRESS_3 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_OUT_PRESS_3, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_OUT_RELEASE_3 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_OUT_RELEASE_3, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_ADJUST ) {
            // Sort turn events among UI/Decks
            // printf( "out adjust - " );
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_OUT_ADJUST_0 ] ) { 
                // printf( "ui out\n" );
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_OUT_ADJUST_0, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_SCROLL_PANEL ] ) { 
                // printf( "scroll panel\n" );
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_SCROLL_PANEL, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_CONTROL_LR_VOL ] ) { 
                // printf( "vol\n" );
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_CONTROL_LR_VOL, in_e, c_e ); return true; 
            } else {
                // printf( "none\n" );
            }
        } else if( in_e->type == ZDJ_HMI_EVENT_PRESS_ADJUST ) {
            // Sort push-turn events among UI/Decks
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_OUT_ADJUST_2 ] ) {
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_OUT_ADJUST_2, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_CONTROL_CUE_VOL ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_CONTROL_CUE_VOL, in_e, c_e ); return true; 
            }  
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_ADJUST ) {
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_OUT_ADJUST_1 ] ) {
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_OUT_ADJUST_1, in_e, c_e ); return true; 
            }  
        }
    }
    
    // Tone 1
    else if( in_e->id == ZDJ_HMI_ENCO_3_TONE_1 ) {
        if( in_e->type == ZDJ_HMI_EVENT_PRESS ) {
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_1_PRESS_0 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_1_PRESS_0, in_e, c_e ); 
                return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_LOOP_TOGGLE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_LOOP_TOGGLE, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_LOOP_TOGGLE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_LOOP_TOGGLE, in_e, c_e ); return true; 
            }
        } else if( in_e->type == ZDJ_HMI_EVENT_RELEASE ) {
            // UI Release
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_1_RELEASE_0 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_1_RELEASE_0, in_e, c_e ); return true; 
            }

        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_1_PRESS_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_1_PRESS_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_1_RELEASE_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_1_RELEASE_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_1_PRESS_2 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_1_PRESS_2, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_1_RELEASE_2 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_1_RELEASE_2, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_LONG_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_1_PRESS_3 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_1_PRESS_3, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_1_RELEASE_3 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_1_RELEASE_3, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_ADJUST ){
            // UI Adjust
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_1_ADJUST_0 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_1_ADJUST_0, in_e, c_e ); return true;

            // EQ Lo
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_EQ_LO ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_EQ_LO, in_e, c_e ); return true;
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_EQ_LO ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_EQ_LO, in_e, c_e ); return true;
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_EXT_CONTROL_EQ_LO ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_EXT_CONTROL_EQ_LO, in_e, c_e ); return true;
            
            // Deck Trim
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_TRIM ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_TRIM, in_e, c_e ); return true;
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_TRIM ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_TRIM, in_e, c_e ); return true;
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_EXT_CONTROL_TRIM ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_EXT_CONTROL_TRIM, in_e, c_e ); return true;

            // Loop Length
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_LOOP_LENGTH ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_LOOP_LENGTH, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_LOOP_LENGTH ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_LOOP_LENGTH, in_e, c_e ); return true; 
            }

        } else if( in_e->type == ZDJ_HMI_EVENT_PRESS_ADJUST ) {
            // UI Press Adjust
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_1_ADJUST_1 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_1_ADJUST_1, in_e, c_e ); return true;

            // Bass Swap
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_2_BASS_SWAP ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_2_BASS_SWAP, in_e, c_e ); return true;
            } 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_ADJUST ) {
            // UI Shift Adjust
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_1_ADJUST_2 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_1_ADJUST_2, in_e, c_e ); return true;
                
            // Loop Start
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_LOOP_START ] ) {
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_LOOP_START, in_e, c_e ); return true;
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_LOOP_START ] ) {
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_LOOP_START, in_e, c_e ); return true;
            }
        }
    }

    // Tone 2
    else if( in_e->id == ZDJ_HMI_ENCO_4_TONE_2 ) {
        
        if( in_e->type == ZDJ_HMI_EVENT_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_2_PRESS_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_2_PRESS_0, in_e, c_e ); return true; 
        
        } else if( in_e->type == ZDJ_HMI_EVENT_RELEASE ) {
            // UI Release
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_2_RELEASE_0 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_2_RELEASE_0, in_e, c_e ); return true; 

            // PFL Mute
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_PFL_TOGGLE_MUTE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_PFL_TOGGLE_MUTE, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_PFL_TOGGLE_MUTE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_PFL_TOGGLE_MUTE, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_EXT_CONTROL_PFL_TOGGLE_MUTE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_EXT_CONTROL_PFL_TOGGLE_MUTE, in_e, c_e ); return true; 
            }
        
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_2_PRESS_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_2_PRESS_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_2_RELEASE_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_2_RELEASE_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_2_PRESS_2 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_2_PRESS_2, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_2_RELEASE_2 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_2_RELEASE_2, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_LONG_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_2_PRESS_3 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_2_PRESS_3, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_2_RELEASE_3 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_2_RELEASE_3, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_ADJUST ) {
            // UI Adjust
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_2_ADJUST_0 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_2_ADJUST_0, in_e, c_e ); return true; 
            
            // EQ Mid
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_EQ_MID ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_EQ_MID, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_EQ_MID ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_EQ_MID, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_EXT_CONTROL_EQ_MID ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_EXT_CONTROL_EQ_MID, in_e, c_e ); return true; 

            // PFL Trim
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_PFL_TRIM ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_PFL_TRIM, in_e, c_e ); return true;
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_PFL_TRIM ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_PFL_TRIM, in_e, c_e ); return true;
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_EXT_CONTROL_PFL_TRIM ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_EXT_CONTROL_PFL_TRIM, in_e, c_e ); return true;

            // Skip Len
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_SKIP_LENGTH ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_SKIP_LENGTH, in_e, c_e ); return true;
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_SKIP_LENGTH ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_SKIP_LENGTH, in_e, c_e ); return true;
            }
        
        } else if( in_e->type == ZDJ_HMI_EVENT_PRESS_ADJUST ) {
            // UI Press Adjust
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_2_ADJUST_1 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_2_ADJUST_1, in_e, c_e ); return true;
            
            }
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_ADJUST ) {
            // UI Shift Adjust
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_2_ADJUST_2 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_2_ADJUST_2, in_e, c_e ); return true;
            }
        }
    }

    // Tone 3
    else if( in_e->id == ZDJ_HMI_ENCO_5_TONE_3 ) {
        if( in_e->type == ZDJ_HMI_EVENT_PRESS ) {
            // UI Press
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_3_PRESS_0 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_3_PRESS_0, in_e, c_e ); return true;
            
            // Skip Reset
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_SKIP_RESET_TO_ORIGIN ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_SKIP_RESET_TO_ORIGIN, in_e, c_e ); return true;
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_SKIP_RESET_TO_ORIGIN ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_SKIP_RESET_TO_ORIGIN, in_e, c_e ); return true;

            // Sync Toggle
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_CONTROL_SYNC_TOGGLE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_CONTROL_SYNC_TOGGLE, in_e, c_e ); return true;
            }
        
        } else if( in_e->type == ZDJ_HMI_EVENT_RELEASE ) {
            // UI Release
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_3_RELEASE_0 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_3_RELEASE_0, in_e, c_e ); return true;
            
            }

        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_3_PRESS_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_3_PRESS_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_3_RELEASE_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_3_RELEASE_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_3_PRESS_2 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_3_PRESS_2, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_3_RELEASE_2 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_3_RELEASE_2, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_LONG_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_3_PRESS_3 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_3_PRESS_3, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_3_RELEASE_3 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_3_RELEASE_3, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_ADJUST ) {
            // UI Adjust
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_3_ADJUST_0 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_3_ADJUST_0, in_e, c_e ); return true;
            
            // EQ Hi
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_EQ_HI ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_EQ_HI, in_e, c_e ); return true;
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_EQ_HI ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_EQ_HI, in_e, c_e ); return true;
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_EXT_CONTROL_EQ_HI ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_EXT_CONTROL_EQ_HI, in_e, c_e ); return true;
            
            // Skip
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_SKIP ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_SKIP, in_e, c_e ); return true;
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_SKIP ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_SKIP, in_e, c_e ); return true;
            } 

        } else if( in_e->type == ZDJ_HMI_EVENT_PRESS_ADJUST ) {
            // UI Press Adjust
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_3_ADJUST_1 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_3_ADJUST_1, in_e, c_e ); return true;
            
            // Filter
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_FILTER_0 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_FILTER_0, in_e, c_e ); return true;
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_FILTER_0 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_FILTER_0, in_e, c_e ); return true;
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_EXT_CONTROL_FILTER_0 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_EXT_CONTROL_FILTER_0, in_e, c_e ); return true;
            }
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_ADJUST ) {
            // UI Shift Adjust
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_3_ADJUST_2 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_3_ADJUST_2, in_e, c_e ); return true;
            }
        }
    }

    // Hotcue Btn
    else if( in_e->id == ZDJ_HMI_PB_0_HOTCUE ) {
        if( in_e->type == ZDJ_HMI_EVENT_PRESS ) {
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_HOTCUE_PRESS_0 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_HOTCUE_PRESS_0, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_HOTCUE_START ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_HOTCUE_START, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_HOTCUE_START ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_HOTCUE_START, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_XPORT_CONTROL_HOTCUE_START ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_XPORT_CONTROL_HOTCUE_START, in_e, c_e ); return true; 
            }
        } else if( in_e->type == ZDJ_HMI_EVENT_RELEASE ) {
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_HOTCUE_RELEASE_0 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_HOTCUE_RELEASE_0, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_HOTCUE_END ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_HOTCUE_END, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_HOTCUE_END ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_HOTCUE_END, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_XPORT_CONTROL_HOTCUE_END ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_XPORT_CONTROL_HOTCUE_END, in_e, c_e ); return true; 
            }
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_HOTCUE_PRESS_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_HOTCUE_PRESS_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_LONG_PRESS ) {
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_HOTCUE_PRESS_3 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_HOTCUE_PRESS_3, in_e, c_e ); return true; 
            }
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_RELEASE ) {
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_HOTCUE_RELEASE_1 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_HOTCUE_RELEASE_1, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_HOTCUE_END ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_HOTCUE_END, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_HOTCUE_END ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_HOTCUE_END, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_XPORT_CONTROL_HOTCUE_END ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_XPORT_CONTROL_HOTCUE_END, in_e, c_e ); return true; 
            }
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_RELEASE ){
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_HOTCUE_RELEASE_2 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_HOTCUE_RELEASE_2, in_e, c_e ); return true; 
            }
        }
    }

    // Play Btn
    else if( in_e->id == ZDJ_HMI_PB_1_PLAY ) {
        // printf( "play event: %s\n", zdj_hmi_input_event_name[ in_e->type ] );
        if( in_e->type == ZDJ_HMI_EVENT_PRESS ) {
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_PLAY_PRESS_0 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_PLAY_PRESS_0, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_PLAY_PAUSE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_PLAY_PAUSE, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_PLAY_PAUSE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_PLAY_PAUSE, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_EXT_CONTROL_PLAY_PAUSE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_EXT_CONTROL_PLAY_PAUSE, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_XPORT_CONTROL_PLAY_PAUSE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_XPORT_CONTROL_PLAY_PAUSE, in_e, c_e ); return true; 
            }
        } else if( in_e->type == ZDJ_HMI_EVENT_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_PLAY_RELEASE_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_PLAY_RELEASE_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_PLAY_PRESS_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_PLAY_PRESS_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_LONG_PRESS ) {
            if( zdj_control_active_state.controls[ ZDJ_DECK_CONTROL_TOGGLE_RECORD ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_CONTROL_TOGGLE_RECORD, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_PLAY_PRESS_3 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_PLAY_PRESS_3, in_e, c_e ); return true; 
            }
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_PLAY_RELEASE_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_PLAY_RELEASE_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_RELEASE ) {
            if ( zdj_control_active_state.controls[ ZDJ_DECK_CONTROL_TOGGLE_RECORD ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_CONTROL_TOGGLE_RECORD, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_PLAY_RELEASE_2 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_PLAY_RELEASE_2, in_e, c_e ); return true; 
            }
        }
    }

    // Nav Btn
    else if( in_e->id == ZDJ_HMI_PB_5_NAV ) {
        if( in_e->type == ZDJ_HMI_EVENT_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_NAV_PRESS_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_NAV_PRESS_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_NAV_RELEASE_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_NAV_RELEASE_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_NAV_PRESS_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_NAV_PRESS_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_NAV_RELEASE_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_NAV_RELEASE_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_NAV_RELEASE_2 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_NAV_RELEASE_2, in_e, c_e ); return true; 
        }
    }

    // Fn 1
    else if( in_e->id == ZDJ_HMI_PB_2_FN_1 ) {
        if( in_e->type == ZDJ_HMI_EVENT_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_1_PRESS_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_1_PRESS_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_RELEASE ) {
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_PREV_PANEL ] ) { 
                printf( "prev panel\n" );
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_PREV_PANEL, in_e, c_e ); return true; 
            } else if ( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_1_RELEASE_0 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_1_RELEASE_0, in_e, c_e ); return true;
            } 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_1_PRESS_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_1_PRESS_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_1_RELEASE_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_1_RELEASE_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_1_RELEASE_2 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_1_RELEASE_2, in_e, c_e ); return true; 
        }
    }

    // Fn 2
    else if( in_e->id == ZDJ_HMI_PB_3_FN_2 ) {
        if( in_e->type == ZDJ_HMI_EVENT_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_2_PRESS_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_2_PRESS_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_2_RELEASE_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_2_RELEASE_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_2_PRESS_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_2_PRESS_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_2_RELEASE_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_2_RELEASE_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_2_RELEASE_2 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_2_RELEASE_2, in_e, c_e ); return true; 
        }
    }

    // Fn 3
    else if( in_e->id == ZDJ_HMI_PB_4_FN_3 ) {
        if( in_e->type == ZDJ_HMI_EVENT_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_3_PRESS_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_3_PRESS_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_RELEASE ) {
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_NEXT_PANEL ] ) { 
                printf( "next panel\n" );
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_NEXT_PANEL, in_e, c_e ); return true; 
            } else if ( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_3_RELEASE_0 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_3_RELEASE_0, in_e, c_e ); return true;
            }  
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_PRESS ) {
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_3_PRESS_1 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_3_PRESS_1, in_e, c_e ); 
                return true;
            } 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_3_RELEASE_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_3_RELEASE_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_PRESS ) {
            if ( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_3_PRESS_1 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_3_PRESS_1, in_e, c_e ); 
                return true; 
            }
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_RELEASE ) {
            if ( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_3_RELEASE_2 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_3_RELEASE_2, in_e, c_e ); 
                return true; 
            }
        }
    }

    // Fade 1
    else if( in_e->id == ZDJ_HMI_POT_0_CH_1 ) {
        if( in_e->type == ZDJ_HMI_EVENT_ADJUST ) {
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FADE_1_ADJUST_0 ] ) {
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FADE_1_ADJUST_0, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_FADE ] ) {
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_FADE, in_e, c_e ); return true; 
            } 
        }
    }
    // Fade 2
    else if( in_e->id == ZDJ_HMI_POT_1_CH_2 ) {
        if( in_e->type == ZDJ_HMI_EVENT_ADJUST ) {
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FADE_2_ADJUST_0 ] ) {
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FADE_2_ADJUST_0, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_FADE ] ) {
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_FADE, in_e, c_e ); return true; 
            } 
        }
    }
    // XFade
    else if( in_e->id == ZDJ_HMI_POT_2_XFADE ) {
        if( in_e->type == ZDJ_HMI_EVENT_ADJUST ) {
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_XFADE_ADJUST_0 ] ) {
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_XFADE_ADJUST_0, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_CONTROL_XFADE ] ) {
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_CONTROL_XFADE, in_e, c_e ); return true; 
            } 
        }
    }
    

    return false;
}

void _zdj_control_copy_hmi_to_control_event( 
    zdj_control_id_t id,
    zdj_hmi_input_event_t * in_e, 
    zdj_control_event_t * c_e 
) {
        c_e->id = id;
        c_e->blocked = false;
        c_e->i_val = in_e->i_val;
        c_e->d_val = in_e->d_val;
        c_e->b_val = in_e->b_val;
}

bool zdj_control_event_is_ui_control( zdj_control_event_t * event ) {
    switch ( event->id ) {
        case ZDJ_UI_CONTROL_JOG_ADJUST_0:
        case ZDJ_UI_CONTROL_JOG_ADJUST_1:
        case ZDJ_UI_CONTROL_JOG_ADJUST_2:
        case ZDJ_UI_CONTROL_JOG_ADJUST_3:
        case ZDJ_UI_CONTROL_JOG_ADJUST_4:
        case ZDJ_UI_CONTROL_JOG_ADJUST_5:
        case ZDJ_UI_CONTROL_JOG_ADJUST_6:
        case ZDJ_UI_CONTROL_JOG_ADJUST_7:
        case ZDJ_UI_CONTROL_JOG_PRESS_0:
        case ZDJ_UI_CONTROL_JOG_PRESS_1:
        case ZDJ_UI_CONTROL_JOG_PRESS_2:
        case ZDJ_UI_CONTROL_JOG_RELEASE_0:
        case ZDJ_UI_CONTROL_JOG_RELEASE_1:
        case ZDJ_UI_CONTROL_JOG_RELEASE_2:
        case ZDJ_UI_CONTROL_JOG_RELEASE_3:
        case ZDJ_UI_CONTROL_OUT_ADJUST_0:
        case ZDJ_UI_CONTROL_OUT_ADJUST_1:
        case ZDJ_UI_CONTROL_OUT_ADJUST_2:
        case ZDJ_UI_CONTROL_OUT_PRESS_0:
        case ZDJ_UI_CONTROL_OUT_PRESS_1:
        case ZDJ_UI_CONTROL_OUT_PRESS_2:
        case ZDJ_UI_CONTROL_OUT_PRESS_3:
        case ZDJ_UI_CONTROL_OUT_RELEASE_0:
        case ZDJ_UI_CONTROL_OUT_RELEASE_1:
        case ZDJ_UI_CONTROL_OUT_RELEASE_2:
        case ZDJ_UI_CONTROL_OUT_RELEASE_3:
        case ZDJ_UI_CONTROL_TONE_1_ADJUST_0:
        case ZDJ_UI_CONTROL_TONE_1_ADJUST_1:
        case ZDJ_UI_CONTROL_TONE_1_ADJUST_2:
        case ZDJ_UI_CONTROL_TONE_1_PRESS_0:
        case ZDJ_UI_CONTROL_TONE_1_PRESS_1:
        case ZDJ_UI_CONTROL_TONE_1_PRESS_2:
        case ZDJ_UI_CONTROL_TONE_1_PRESS_3:
        case ZDJ_UI_CONTROL_TONE_1_RELEASE_0:
        case ZDJ_UI_CONTROL_TONE_1_RELEASE_1:
        case ZDJ_UI_CONTROL_TONE_1_RELEASE_2:
        case ZDJ_UI_CONTROL_TONE_1_RELEASE_3:
        case ZDJ_UI_CONTROL_TONE_2_ADJUST_0:
        case ZDJ_UI_CONTROL_TONE_2_ADJUST_1:
        case ZDJ_UI_CONTROL_TONE_2_ADJUST_2:
        case ZDJ_UI_CONTROL_TONE_2_PRESS_0:
        case ZDJ_UI_CONTROL_TONE_2_PRESS_1:
        case ZDJ_UI_CONTROL_TONE_2_PRESS_2:
        case ZDJ_UI_CONTROL_TONE_2_PRESS_3:
        case ZDJ_UI_CONTROL_TONE_2_RELEASE_0:
        case ZDJ_UI_CONTROL_TONE_2_RELEASE_1:
        case ZDJ_UI_CONTROL_TONE_2_RELEASE_2:
        case ZDJ_UI_CONTROL_TONE_2_RELEASE_3: 
        case ZDJ_UI_CONTROL_TONE_3_ADJUST_0:
        case ZDJ_UI_CONTROL_TONE_3_ADJUST_1:
        case ZDJ_UI_CONTROL_TONE_3_ADJUST_2:
        case ZDJ_UI_CONTROL_TONE_3_PRESS_0:
        case ZDJ_UI_CONTROL_TONE_3_PRESS_1:
        case ZDJ_UI_CONTROL_TONE_3_PRESS_2:
        case ZDJ_UI_CONTROL_TONE_3_PRESS_3:
        case ZDJ_UI_CONTROL_TONE_3_RELEASE_0:
        case ZDJ_UI_CONTROL_TONE_3_RELEASE_1:
        case ZDJ_UI_CONTROL_TONE_3_RELEASE_2:
        case ZDJ_UI_CONTROL_TONE_3_RELEASE_3:
        case ZDJ_UI_CONTROL_PLAY_PRESS_0:
        case ZDJ_UI_CONTROL_PLAY_PRESS_1:
        case ZDJ_UI_CONTROL_PLAY_PRESS_2:
        case ZDJ_UI_CONTROL_PLAY_PRESS_3:
        case ZDJ_UI_CONTROL_PLAY_RELEASE_0:
        case ZDJ_UI_CONTROL_PLAY_RELEASE_1:
        case ZDJ_UI_CONTROL_PLAY_RELEASE_2:
        case ZDJ_UI_CONTROL_PLAY_RELEASE_3:
        case ZDJ_UI_CONTROL_HOTCUE_PRESS_0:
        case ZDJ_UI_CONTROL_HOTCUE_PRESS_1:
        case ZDJ_UI_CONTROL_HOTCUE_PRESS_2:
        case ZDJ_UI_CONTROL_HOTCUE_PRESS_3:
        case ZDJ_UI_CONTROL_HOTCUE_RELEASE_0:
        case ZDJ_UI_CONTROL_HOTCUE_RELEASE_1:
        case ZDJ_UI_CONTROL_HOTCUE_RELEASE_2:
        case ZDJ_UI_CONTROL_HOTCUE_RELEASE_3:
        case ZDJ_UI_CONTROL_NAV_PRESS_0:
        case ZDJ_UI_CONTROL_NAV_PRESS_1:
        case ZDJ_UI_CONTROL_NAV_PRESS_2:
        case ZDJ_UI_CONTROL_NAV_PRESS_3:
        case ZDJ_UI_CONTROL_NAV_RELEASE_0:
        case ZDJ_UI_CONTROL_NAV_RELEASE_1:
        case ZDJ_UI_CONTROL_NAV_RELEASE_2:
        case ZDJ_UI_CONTROL_NAV_RELEASE_3:
        case ZDJ_UI_CONTROL_FN_1_PRESS_0:
        case ZDJ_UI_CONTROL_FN_1_PRESS_1:
        case ZDJ_UI_CONTROL_FN_1_PRESS_2:
        case ZDJ_UI_CONTROL_FN_1_PRESS_3:
        case ZDJ_UI_CONTROL_FN_1_RELEASE_0:
        case ZDJ_UI_CONTROL_FN_1_RELEASE_1:
        case ZDJ_UI_CONTROL_FN_1_RELEASE_2:
        case ZDJ_UI_CONTROL_FN_1_RELEASE_3:
        case ZDJ_UI_CONTROL_FN_2_PRESS_0:
        case ZDJ_UI_CONTROL_FN_2_PRESS_1:
        case ZDJ_UI_CONTROL_FN_2_PRESS_2:
        case ZDJ_UI_CONTROL_FN_2_PRESS_3:
        case ZDJ_UI_CONTROL_FN_2_RELEASE_0:
        case ZDJ_UI_CONTROL_FN_2_RELEASE_1:
        case ZDJ_UI_CONTROL_FN_2_RELEASE_2:
        case ZDJ_UI_CONTROL_FN_2_RELEASE_3:
        case ZDJ_UI_CONTROL_FN_3_PRESS_0:
        case ZDJ_UI_CONTROL_FN_3_PRESS_1:
        case ZDJ_UI_CONTROL_FN_3_PRESS_2:
        case ZDJ_UI_CONTROL_FN_3_PRESS_3:
        case ZDJ_UI_CONTROL_FN_3_RELEASE_0:
        case ZDJ_UI_CONTROL_FN_3_RELEASE_1:
        case ZDJ_UI_CONTROL_FN_3_RELEASE_2:
        case ZDJ_UI_CONTROL_FN_3_RELEASE_3:
        case ZDJ_UI_CONTROL_FADE_1_ADJUST_0:
        case ZDJ_UI_CONTROL_FADE_1_ADJUST_1:
        case ZDJ_UI_CONTROL_FADE_2_ADJUST_0:
        case ZDJ_UI_CONTROL_FADE_2_ADJUST_1:
        case ZDJ_UI_CONTROL_XFADE_ADJUST_0:
        case ZDJ_UI_CONTROL_XFADE_ADJUST_1: 
        case ZDJ_UI_CONTROL_TOGGLE_PANEL:
        case ZDJ_UI_CONTROL_SCROLL_PANEL:
        case ZDJ_UI_CONTROL_NEXT_PANEL:
        case ZDJ_UI_CONTROL_PREV_PANEL:
        case ZDJ_UI_CONTROL_TOGGLE_ASSIST_PANEL:
        case ZDJ_UI_CONTROL_TOGGLE_RECORDING_PANEL:
        case ZDJ_UI_CONTROL_TOGGLE_SOUNDCARD_PANEL: return true;
        default: return false;
    }
}

bool zdj_control_event_is_deck_control( zdj_control_event_t * event ) {
    switch ( event->id ) {
        case ZDJ_DECK_CONTROL_LR_VOL:
        case ZDJ_DECK_CONTROL_CUE_VOL:
        case ZDJ_DECK_CONTROL_XFADE:
        case ZDJ_DECK_CONTROL_TOGGLE_RECORD:
        case ZDJ_DECK_CONTROL_START_RECORD:
        case ZDJ_DECK_CONTROL_PAUSE_RECORD:
        case ZDJ_DECK_CONTROL_SYNC_TOGGLE:
        case ZDJ_DECK_CONTROL_SYNC_ENABLE:
        case ZDJ_DECK_CONTROL_SYNC_DISABLE:
        case ZDJ_DECK_1_2_BASS_SWAP:

        case ZDJ_DECK_1_CONTROL_FADE:
        case ZDJ_DECK_1_CONTROL_TRIM:
        case ZDJ_DECK_1_CONTROL_EQ_LO:
        case ZDJ_DECK_1_CONTROL_EQ_MID:
        case ZDJ_DECK_1_CONTROL_EQ_HI:
        case ZDJ_DECK_1_CONTROL_PFL_TRIM:
        case ZDJ_DECK_1_CONTROL_PFL_TOGGLE_MUTE:
        case ZDJ_DECK_1_CONTROL_LOOP_TOGGLE:
        case ZDJ_DECK_1_CONTROL_LOOP_ON:
        case ZDJ_DECK_1_CONTROL_LOOP_OFF:
        case ZDJ_DECK_1_CONTROL_LOOP_START:
        case ZDJ_DECK_1_CONTROL_LOOP_END:
        case ZDJ_DECK_1_CONTROL_LOOP_LENGTH:
        case ZDJ_DECK_1_CONTROL_LOOP_RESET_TO_START:
        case ZDJ_DECK_1_CONTROL_SKIP:
        case ZDJ_DECK_1_CONTROL_SKIP_LENGTH:
        case ZDJ_DECK_1_CONTROL_SKIP_SET_ORIGIN:
        case ZDJ_DECK_1_CONTROL_SKIP_RESET_TO_ORIGIN:
        case ZDJ_DECK_1_CONTROL_FX_SELECT:
        case ZDJ_DECK_1_CONTROL_FX_0:
        case ZDJ_DECK_1_CONTROL_FX_1:
        case ZDJ_DECK_1_CONTROL_FX_2:
        case ZDJ_DECK_1_CONTROL_FX_3:
        case ZDJ_DECK_1_CONTROL_FX_4:
        case ZDJ_DECK_1_CONTROL_FX_5:
        case ZDJ_DECK_1_CONTROL_SYNC_MULT:
        case ZDJ_DECK_1_CONTROL_SCRUB:
        case ZDJ_DECK_1_CONTROL_TEMPO:
        case ZDJ_DECK_1_CONTROL_TEMPO_FINE:
        case ZDJ_DECK_1_CONTROL_PLAY_PAUSE:
        case ZDJ_DECK_1_CONTROL_PAUSE:
        case ZDJ_DECK_1_CONTROL_HOTCUE_START:
        case ZDJ_DECK_1_CONTROL_HOTCUE_END:
        case ZDJ_DECK_2_CONTROL_FADE:
        case ZDJ_DECK_2_CONTROL_TRIM:
        case ZDJ_DECK_2_CONTROL_EQ_LO:
        case ZDJ_DECK_2_CONTROL_EQ_MID:
        case ZDJ_DECK_2_CONTROL_EQ_HI:
        case ZDJ_DECK_2_CONTROL_PFL_TRIM:
        case ZDJ_DECK_2_CONTROL_PFL_TOGGLE_MUTE:
        case ZDJ_DECK_2_CONTROL_LOOP_TOGGLE:
        case ZDJ_DECK_2_CONTROL_LOOP_ON:
        case ZDJ_DECK_2_CONTROL_LOOP_OFF:
        case ZDJ_DECK_2_CONTROL_LOOP_START:
        case ZDJ_DECK_2_CONTROL_LOOP_END:
        case ZDJ_DECK_2_CONTROL_LOOP_LENGTH:
        case ZDJ_DECK_2_CONTROL_LOOP_RESET_TO_START:
        case ZDJ_DECK_2_CONTROL_SKIP:
        case ZDJ_DECK_2_CONTROL_SKIP_LENGTH:
        case ZDJ_DECK_2_CONTROL_SKIP_SET_ORIGIN:
        case ZDJ_DECK_2_CONTROL_SKIP_RESET_TO_ORIGIN:
        case ZDJ_DECK_2_CONTROL_FX_SELECT:
        case ZDJ_DECK_2_CONTROL_FX_0:
        case ZDJ_DECK_2_CONTROL_FX_1:
        case ZDJ_DECK_2_CONTROL_FX_2:
        case ZDJ_DECK_2_CONTROL_FX_3:
        case ZDJ_DECK_2_CONTROL_FX_4:
        case ZDJ_DECK_2_CONTROL_FX_5:
        case ZDJ_DECK_2_CONTROL_SYNC_MULT:
        case ZDJ_DECK_2_CONTROL_SCRUB:
        case ZDJ_DECK_2_CONTROL_TEMPO:
        case ZDJ_DECK_2_CONTROL_TEMPO_FINE:
        case ZDJ_DECK_2_CONTROL_PLAY_PAUSE:
        case ZDJ_DECK_2_CONTROL_PAUSE:
        case ZDJ_DECK_2_CONTROL_HOTCUE_START:
        case ZDJ_DECK_2_CONTROL_HOTCUE_END:
        case ZDJ_DECK_EXT_CONTROL_TRIM:
        case ZDJ_DECK_EXT_CONTROL_EQ_LO:
        case ZDJ_DECK_EXT_CONTROL_EQ_MID:
        case ZDJ_DECK_EXT_CONTROL_EQ_HI:
        case ZDJ_DECK_EXT_CONTROL_PFL_TRIM:
        case ZDJ_DECK_EXT_CONTROL_PFL_TOGGLE_MUTE:
        case ZDJ_DECK_EXT_CONTROL_FX_SELECT:
        case ZDJ_DECK_EXT_CONTROL_FX_0:
        case ZDJ_DECK_EXT_CONTROL_FX_1:
        case ZDJ_DECK_EXT_CONTROL_FX_2:
        case ZDJ_DECK_EXT_CONTROL_FX_3:
        case ZDJ_DECK_EXT_CONTROL_FX_4:
        case ZDJ_DECK_EXT_CONTROL_FX_5:
        case ZDJ_DECK_EXT_CONTROL_SYNC_MULT:
        case ZDJ_DECK_EXT_CONTROL_TEMPO:
        case ZDJ_DECK_EXT_CONTROL_TEMPO_FINE:
        case ZDJ_DECK_EXT_CONTROL_PLAY_PAUSE:
        case ZDJ_DECK_EXT_CONTROL_PAUSE: 
        
        // External Transport Deck
        case ZDJ_DECK_XPORT_CONTROL_SCRUB:
        case ZDJ_DECK_XPORT_CONTROL_SYNC_MULT:
        case ZDJ_DECK_XPORT_CONTROL_TEMPO:
        case ZDJ_DECK_XPORT_CONTROL_TEMPO_FINE:
        case ZDJ_DECK_XPORT_CONTROL_PLAY_PAUSE:
        case ZDJ_DECK_XPORT_CONTROL_PAUSE:
        case ZDJ_DECK_XPORT_CONTROL_HOTCUE_START:
        case ZDJ_DECK_XPORT_CONTROL_HOTCUE_STOP:
        case ZDJ_DECK_XPORT_CONTROL_HOTCUE_END: return true;
        default: return false;
    }
}
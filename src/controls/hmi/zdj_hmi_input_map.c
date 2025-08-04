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

bool zdj_control_map_hmi_input_event( zdj_hmi_input_event_t * in_e, zdj_control_event_t * c_e ) {

    // Jog Knob
    if( in_e->id == ZDJ_HMI_ENCO_2_JOG ) {
        // First check for UI event capture
        if( in_e->type == ZDJ_HMI_EVENT_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_JOG_PRESS_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_JOG_PRESS_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_JOG_RELEASE_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_JOG_RELEASE_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_JOG_PRESS_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_JOG_PRESS_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_JOG_RELEASE_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_JOG_RELEASE_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_JOG_RELEASE_2 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_JOG_RELEASE_2, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_ADJUST ) {
            // Sort Jog scroll events among UI/Decks
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_JOG_ADJUST_0 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_JOG_ADJUST_0, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_SCRUB ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_SCRUB, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_SCRUB ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_SCRUB, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_NUDGE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_NUDGE, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_NUDGE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_NUDGE, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_EXT_CONTROL_NUDGE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_EXT_CONTROL_NUDGE, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_TEMPO_FINE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_TEMPO_FINE, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_TEMPO_FINE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_TEMPO_FINE, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_EXT_CONTROL_TEMPO_FINE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_EXT_CONTROL_TEMPO_FINE, in_e, c_e ); return true; 
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
            }  
        }
        
    }
    
    
    // Out/Vol Knob
    else if( in_e->id == ZDJ_HMI_ENCO_1_VOL ) {
        if( in_e->type == ZDJ_HMI_EVENT_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_OUT_PRESS_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_OUT_PRESS_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_OUT_RELEASE_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_OUT_RELEASE_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_OUT_PRESS_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_OUT_PRESS_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_OUT_RELEASE_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_OUT_RELEASE_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_OUT_RELEASE_2 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_OUT_RELEASE_2, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_ADJUST ) {
            // Sort turn events among UI/Decks
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_OUT_ADJUST_0 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_OUT_ADJUST_0, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_CONTROL_LR_VOL ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_CONTROL_LR_VOL, in_e, c_e ); return true; 
            }
        } else if( in_e->type == ZDJ_HMI_EVENT_PRESS_ADJUST ) {
            // Sort push-turn events among UI/Decks
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_OUT_ADJUST_1 ] ) {
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_OUT_ADJUST_1, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_CONTROL_CUE_VOL ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_CONTROL_CUE_VOL, in_e, c_e ); return true; 
            }  
        }
    }
    
    // Tone 1
    else if( in_e->id == ZDJ_HMI_ENCO_3_TONE_1 ) {
        if( in_e->type == ZDJ_HMI_EVENT_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_1_PRESS_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_1_PRESS_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_1_RELEASE_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_1_RELEASE_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_1_PRESS_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_1_PRESS_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_1_RELEASE_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_1_RELEASE_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_1_RELEASE_2 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_1_RELEASE_2, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_ADJUST && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_1_ADJUST_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_1_ADJUST_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_PRESS_ADJUST && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_1_ADJUST_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_1_ADJUST_1, in_e, c_e ); return true; 
        }
    }

    // Tone 2
    else if( in_e->id == ZDJ_HMI_ENCO_4_TONE_2 ) {
        if( in_e->type == ZDJ_HMI_EVENT_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_2_PRESS_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_2_PRESS_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_2_RELEASE_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_2_RELEASE_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_2_PRESS_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_2_PRESS_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_2_RELEASE_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_2_RELEASE_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_2_RELEASE_2 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_2_RELEASE_2, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_ADJUST && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_2_ADJUST_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_2_ADJUST_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_PRESS_ADJUST && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_2_ADJUST_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_2_ADJUST_1, in_e, c_e ); return true; 
        }
    }

    // Tone 3
    else if( in_e->id == ZDJ_HMI_ENCO_5_TONE_3 ) {
        if( in_e->type == ZDJ_HMI_EVENT_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_3_PRESS_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_3_PRESS_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_3_RELEASE_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_3_RELEASE_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_3_PRESS_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_3_PRESS_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_3_RELEASE_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_3_RELEASE_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_3_RELEASE_2 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_3_RELEASE_2, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_ADJUST && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_3_ADJUST_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_3_ADJUST_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_PRESS_ADJUST && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_TONE_3_ADJUST_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_TONE_3_ADJUST_1, in_e, c_e ); return true; 
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
            }
        } else if( in_e->type == ZDJ_HMI_EVENT_RELEASE ) {
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_HOTCUE_RELEASE_0 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_HOTCUE_RELEASE_0, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_HOTCUE_END ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_HOTCUE_END, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_HOTCUE_END ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_HOTCUE_END, in_e, c_e ); return true; 
            }
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_HOTCUE_PRESS_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_HOTCUE_PRESS_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_HOTCUE_RELEASE_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_HOTCUE_RELEASE_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_HOTCUE_RELEASE_2 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_HOTCUE_RELEASE_2, in_e, c_e ); return true; 
        }
    }

    // Play Btn
    else if( in_e->id == ZDJ_HMI_PB_1_PLAY ) {
        if( in_e->type == ZDJ_HMI_EVENT_PRESS ) {
            if( zdj_control_active_state.controls[ ZDJ_UI_CONTROL_PLAY_PRESS_0 ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_PLAY_PRESS_0, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_1_CONTROL_PLAY_PAUSE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_1_CONTROL_PLAY_PAUSE, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_2_CONTROL_PLAY_PAUSE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_2_CONTROL_PLAY_PAUSE, in_e, c_e ); return true; 
            } else if( zdj_control_active_state.controls[ ZDJ_DECK_EXT_CONTROL_PLAY_PAUSE ] ) { 
                _zdj_control_copy_hmi_to_control_event( ZDJ_DECK_EXT_CONTROL_PLAY_PAUSE, in_e, c_e ); return true; 
            }
        } else if( in_e->type == ZDJ_HMI_EVENT_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_PLAY_RELEASE_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_PLAY_RELEASE_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_PLAY_PRESS_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_PLAY_PRESS_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_PLAY_RELEASE_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_PLAY_RELEASE_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_PLAY_RELEASE_2 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_PLAY_RELEASE_2, in_e, c_e ); return true; 
        }
    }

    // Nav Btn
    else if( in_e->id == ZDJ_HMI_PB_5_NAV ) {
        if( in_e->type == ZDJ_HMI_EVENT_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_NAV_RELEASE_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_NAV_RELEASE_0, in_e, c_e ); return true; 
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
        if( in_e->type == ZDJ_HMI_EVENT_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_1_RELEASE_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_1_RELEASE_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_1_RELEASE_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_1_RELEASE_0, in_e, c_e ); return true; 
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
        if( in_e->type == ZDJ_HMI_EVENT_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_2_RELEASE_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_2_RELEASE_0, in_e, c_e ); return true; 
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
        if( in_e->type == ZDJ_HMI_EVENT_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_3_RELEASE_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_3_RELEASE_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_3_RELEASE_0 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_3_RELEASE_0, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_PRESS && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_3_PRESS_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_3_PRESS_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_LONG_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_3_RELEASE_1 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_3_RELEASE_1, in_e, c_e ); return true; 
        } else if( in_e->type == ZDJ_HMI_EVENT_MOD_RELEASE && zdj_control_active_state.controls[ ZDJ_UI_CONTROL_FN_3_RELEASE_2 ] ) { 
            _zdj_control_copy_hmi_to_control_event( ZDJ_UI_CONTROL_FN_3_RELEASE_2, in_e, c_e ); return true; 
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
        case ZDJ_UI_CONTROL_OUT_RELEASE_0:
        case ZDJ_UI_CONTROL_OUT_RELEASE_1:
        case ZDJ_UI_CONTROL_OUT_RELEASE_2:
        case ZDJ_UI_CONTROL_TONE_1_ADJUST_0:
        case ZDJ_UI_CONTROL_TONE_1_ADJUST_1:
        case ZDJ_UI_CONTROL_TONE_1_ADJUST_2:
        case ZDJ_UI_CONTROL_TONE_1_PRESS_0:
        case ZDJ_UI_CONTROL_TONE_1_PRESS_1:
        case ZDJ_UI_CONTROL_TONE_1_PRESS_2:
        case ZDJ_UI_CONTROL_TONE_1_RELEASE_0:
        case ZDJ_UI_CONTROL_TONE_1_RELEASE_1:
        case ZDJ_UI_CONTROL_TONE_1_RELEASE_2:
        case ZDJ_UI_CONTROL_TONE_2_ADJUST_0:
        case ZDJ_UI_CONTROL_TONE_2_ADJUST_1:
        case ZDJ_UI_CONTROL_TONE_2_ADJUST_2:
        case ZDJ_UI_CONTROL_TONE_2_PRESS_0:
        case ZDJ_UI_CONTROL_TONE_2_PRESS_1:
        case ZDJ_UI_CONTROL_TONE_2_PRESS_2:
        case ZDJ_UI_CONTROL_TONE_2_RELEASE_0:
        case ZDJ_UI_CONTROL_TONE_2_RELEASE_1:
        case ZDJ_UI_CONTROL_TONE_2_RELEASE_2:
        case ZDJ_UI_CONTROL_TONE_3_ADJUST_0:
        case ZDJ_UI_CONTROL_TONE_3_ADJUST_1:
        case ZDJ_UI_CONTROL_TONE_3_ADJUST_2:
        case ZDJ_UI_CONTROL_TONE_3_PRESS_0:
        case ZDJ_UI_CONTROL_TONE_3_PRESS_1:
        case ZDJ_UI_CONTROL_TONE_3_PRESS_2:
        case ZDJ_UI_CONTROL_TONE_3_RELEASE_0:
        case ZDJ_UI_CONTROL_TONE_3_RELEASE_1:
        case ZDJ_UI_CONTROL_TONE_3_RELEASE_2:
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
        case ZDJ_UI_CONTROL_XFADE_ADJUST_1: return true;
        default: return false;
    }
}

bool zdj_control_event_is_deck_control( zdj_control_event_t * event ) {
    switch ( event->id ) {
        case ZDJ_DECK_CONTROL_LR_VOL:
        case ZDJ_DECK_CONTROL_CUE_VOL:
        case ZDJ_DECK_CONTROL_XFADE:
        case ZDJ_DECK_1_CONTROL_FADE:
        case ZDJ_DECK_1_CONTROL_TRIM:
        case ZDJ_DECK_1_CONTROL_EQ_LO:
        case ZDJ_DECK_1_CONTROL_EQ_MID:
        case ZDJ_DECK_1_CONTROL_EQ_HI:
        case ZDJ_DECK_1_CONTROL_PFL_TRIM:
        case ZDJ_DECK_1_CONTROL_PFL_MUTE:
        case ZDJ_DECK_1_CONTROL_FX_SELECT:
        case ZDJ_DECK_1_CONTROL_FX_0:
        case ZDJ_DECK_1_CONTROL_FX_1:
        case ZDJ_DECK_1_CONTROL_FX_2:
        case ZDJ_DECK_1_CONTROL_FX_3:
        case ZDJ_DECK_1_CONTROL_FX_4:
        case ZDJ_DECK_1_CONTROL_FX_5:
        case ZDJ_DECK_1_CONTROL_SCRUB:
        case ZDJ_DECK_1_CONTROL_NUDGE:
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
        case ZDJ_DECK_2_CONTROL_PFL_MUTE:
        case ZDJ_DECK_2_CONTROL_FX_SELECT:
        case ZDJ_DECK_2_CONTROL_FX_0:
        case ZDJ_DECK_2_CONTROL_FX_1:
        case ZDJ_DECK_2_CONTROL_FX_2:
        case ZDJ_DECK_2_CONTROL_FX_3:
        case ZDJ_DECK_2_CONTROL_FX_4:
        case ZDJ_DECK_2_CONTROL_FX_5:
        case ZDJ_DECK_2_CONTROL_SCRUB:
        case ZDJ_DECK_2_CONTROL_NUDGE:
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
        case ZDJ_DECK_EXT_CONTROL_PFL_MUTE:
        case ZDJ_DECK_EXT_CONTROL_FX_SELECT:
        case ZDJ_DECK_EXT_CONTROL_FX_0:
        case ZDJ_DECK_EXT_CONTROL_FX_1:
        case ZDJ_DECK_EXT_CONTROL_FX_2:
        case ZDJ_DECK_EXT_CONTROL_FX_3:
        case ZDJ_DECK_EXT_CONTROL_FX_4:
        case ZDJ_DECK_EXT_CONTROL_FX_5:
        case ZDJ_DECK_EXT_CONTROL_NUDGE:
        case ZDJ_DECK_EXT_CONTROL_TEMPO:
        case ZDJ_DECK_EXT_CONTROL_TEMPO_FINE:
        case ZDJ_DECK_EXT_CONTROL_PLAY_PAUSE:
        case ZDJ_DECK_EXT_CONTROL_PAUSE: return true;
        default: return false;
    }
}
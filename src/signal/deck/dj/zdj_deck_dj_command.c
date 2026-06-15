#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/dj/zdj_deck_dj.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/signal/pipeline/node/audio/tsm/zdj_tsm_pitch_node.h>
#include <zerodj/signal/pipeline/node/audio/tsm/zdj_tsm_tempo_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

// Public API
static void _update_command_req( zdj_deck_t * deck );
static bool _update_command_models( zdj_deck_t * deck );

// Internal API
// static void _new_loop( zdj_deck_t * deck );
// static void _enable_loop( zdj_deck_t * deck, zdj_library_cuepoint_t * cuepoint );
// static void _disable_loop( zdj_deck_t * deck );
// static void _move_loop( zdj_deck_t * deck, double val );
// static void _resize_loop( zdj_deck_t * deck, double req_len );
// static void _skip( zdj_deck_t * deck, double val );
// static void _stage_needledrop( zdj_deck_t * deck );
// static void _update_needledrop( zdj_deck_t * deck );

// Input Sensitivity Filter
static int _update_req_input( 
    zdj_deck_t * deck, 
    zdj_deck_control_command_request_state_t req, 
    int val 
);

void zdj_deck_dj_init_command( zdj_deck_t * deck ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;

    // Public API
    deck->update_command_req = &_update_command_req;
    deck->update_command_models = &_update_command_models;

    // Set request input sensitivites (higher numbers = less sensitive)
    int * discon_req_sens = (int *)&deck->command_req.req_sens;
    discon_req_sens[ ZDJ_DECK_COMMAND_REQUEST_MOVE_LOOP ] = 2;
    discon_req_sens[ ZDJ_DECK_COMMAND_REQUEST_RESIZE_LOOP ] = 4;
    discon_req_sens[ ZDJ_DECK_COMMAND_REQUEST_SKIP ] = 1;
    discon_req_sens[ ZDJ_DECK_COMMAND_REQUEST_CHANGE_SKIP_LENGTH ] = 6;
}

bool zdj_dj_deck_command_request( zdj_deck_t * deck, zdj_deck_control_command_request_state_t req ) {
    if( deck->command_req.state == ZDJ_DECK_COMMAND_REQUEST_NONE ) {
        deck->command_req.state = req;
        return true;
    } else {
        return false;
    }
}

/////////////////////////////////////////////////////////////
// NOT THREAD SAFE!                                        //
static void _update_command_req( zdj_deck_t * deck ) {
// Discon Request is updated from the HMI input thread.    //
// It must not directly set anything on the Discon Models. //
/////////////////////////////////////////////////////////////

    if( deck->command_req.state == ZDJ_DECK_COMMAND_REQUEST_NONE ) { return; }

    switch ( deck->command_req.state ) {
        case ZDJ_DECK_COMMAND_REQUEST_RECEIVED:
            deck->command_req.state = ZDJ_DECK_COMMAND_REQUEST_NONE;
            break;
    }
}


/////////////////////////////////////////////////////////////////////////////
// NOT THREAD SAFE!                                                        //
static bool _update_command_models( zdj_deck_t * deck ) {
// Discon Models are updated from the soundcard thread on buffer boundary. //
// If Discon Request model declares an update, pull state and              //
// declare the update to be received.                                      //
/////////////////////////////////////////////////////////////////////////////
    zdj_deck_control_command_request_t * command_req = &deck->command_req;
    if( command_req->state == ZDJ_DECK_COMMAND_REQUEST_NONE ) { return false; }

    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_tsm_pitch_node_state_t * tsm_pitch_state = (zdj_tsm_pitch_node_state_t*)deck_state->tsm_pitch_node->state;
    zdj_tsm_tempo_node_state_t * tsm_tempo_state = (zdj_tsm_tempo_node_state_t*)deck_state->tsm_tempo_node->state;

    int val = 0;
    switch ( command_req->state ) {
        
        case ZDJ_DECK_COMMAND_REQUEST_TOGGLE_LOOP:
            if( zdj_dj_deck_loop_is_enabled( deck ) ) {
                zdj_dj_deck_disable_loop( deck );
                if( deck->station == ZDJ_DECK_STATION_1 ) {
                    zdj_activate_control_map( ZDJ_CONTROL_MAP_STATION_1_LOOP_OFF );
                } else if( deck->station == ZDJ_DECK_STATION_2 ) {
                    zdj_activate_control_map( ZDJ_CONTROL_MAP_STATION_2_LOOP_OFF );
                }
            } else {
                zdj_dj_deck_new_loop( deck );
                if( deck->station == ZDJ_DECK_STATION_1 ) {
                    zdj_activate_control_map( ZDJ_CONTROL_MAP_STATION_1_LOOP_ON );
                } else if( deck->station == ZDJ_DECK_STATION_2 ) {
                    zdj_activate_control_map( ZDJ_CONTROL_MAP_STATION_2_LOOP_ON );
                }
            }
            break;

        case ZDJ_DECK_COMMAND_REQUEST_MOVE_LOOP:
            // Run inputs through input sensitivity filter
            val = _update_req_input( deck, ZDJ_DECK_COMMAND_REQUEST_MOVE_LOOP, command_req->event_i_val );
            if( val != 0 ) { zdj_dj_deck_move_loop( deck, val ); }
            break;

        case ZDJ_DECK_COMMAND_REQUEST_RESIZE_LOOP:
            // Run inputs through input sensitivity filter
            val = _update_req_input( deck, ZDJ_DECK_COMMAND_REQUEST_RESIZE_LOOP, command_req->event_i_val );
            if( val != 0 ) { zdj_dj_deck_resize_loop( deck, val ); }
            break;

        case ZDJ_DECK_COMMAND_REQUEST_JUMP_TO_LOOP_START:
            zdj_dj_deck_reset_to_loop_start( deck );
            break;
        case ZDJ_DECK_COMMAND_REQUEST_SKIP:
            val = _update_req_input( deck, ZDJ_DECK_COMMAND_REQUEST_SKIP, command_req->event_i_val );
            if( val != 0 && !deck->controls.skip_state.locked ) { zdj_dj_deck_skip( deck, val ); }
            break;
        case ZDJ_DECK_COMMAND_REQUEST_CHANGE_SKIP_LENGTH:
            val = _update_req_input( deck, ZDJ_DECK_COMMAND_REQUEST_CHANGE_SKIP_LENGTH, command_req->event_i_val );
            if( val != 0 ) { zdj_dj_deck_change_skip_length( deck, val ); }
            break;
        case ZDJ_DECK_COMMAND_REQUEST_SET_SKIP_ORIGIN:
            zdj_dj_deck_set_skip_origin( deck );
            break;
        case ZDJ_DECK_COMMAND_REQUEST_JUMP_TO_SKIP_ORIGIN:
            zdj_dj_deck_skip_to_origin( deck );
            break;
        case ZDJ_DECK_COMMAND_REQUEST_SET_CUEPOINT:
            zdj_dj_deck_set_cuepoint( deck );
            break;
        case ZDJ_DECK_COMMAND_REQUEST_NEXT_CUEPOINT:
            if( deck_state->song->performance && deck_state->song->performance->cuepoint_count > 0 ) {
                zdj_dj_deck_next_cuepoint( deck );
            }
            break;
        case ZDJ_DECK_COMMAND_REQUEST_CUE_START:
            if( zdj_dj_deck_loop_is_enabled( deck ) ) {
                zdj_dj_deck_skip_to_loop_start( deck );
            } else {
                deck->controls.cue_state.is_cueing = true;
                zdj_dj_deck_play_cuepoint( deck );
            }
            break;
        case ZDJ_DECK_COMMAND_REQUEST_CUE_END:
            if( deck->controls.cue_state.is_cueing ) {
                zdj_dj_deck_reset_to_cuepoint( deck );
            }
            break;
        case ZDJ_DECK_COMMAND_REQUEST_HOTCUE:
            if( deck_state->song->performance && deck_state->song->performance->cuepoint_count > 0 ) {
                zdj_dj_deck_hotcue( deck, command_req->hotcue_num );
            }
            break;
    }
    
    command_req->state = ZDJ_DECK_COMMAND_REQUEST_NONE;
    
    // Let deck know there's an update
    return true;
}

static int _update_req_input( 
    zdj_deck_t * deck, 
    zdj_deck_control_command_request_state_t req, 
    int val 
) {
    // Bounds-check the req array
    if( req < 1 || req >= ZDJ_DECK_COMMAND_REQUEST_COUNT ) { return false; }
    
    int res = 0;
    zdj_deck_control_command_request_t * command_req = &deck->command_req;
    command_req->req_sens_count[ req ] += val;
    if( abs( command_req->req_sens_count[ req ] ) >= command_req->req_sens[ req ] ) {
        res = ceil((float)command_req->req_sens_count[ req ] / (float)command_req->req_sens[ req ]);
        command_req->req_sens_count[ req ] = 0;
    }

    return res;
}


bool zdj_dj_deck_quantize_commands( zdj_deck_t * deck ) {
    return deck->controls.discon_quantize;
}

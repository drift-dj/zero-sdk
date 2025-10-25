#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <math.h>

#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/dj/zdj_deck_dj.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

zdj_deck_t * zdj_new_deck( zdj_deck_type_t type, zdj_deck_station_t station, void * resource ) {
    // printf( "zdj_new_deck %d %d %p\n", type, station, resource );
    zdj_deck_t * deck = calloc( 1, sizeof( zdj_deck_t ) );
    deck->type = type;
    deck->station = station;
    deck->status = ZDJ_DECK_STATUS_NEW;

    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_DECK_INIT;
    switch ( type ) {
        // case ZDJ_DECK_TYPE_LIB: zdj_new_lib_deck( deck, resource );
        case ZDJ_DECK_TYPE_DJ: zdj_new_dj_deck( deck, resource ); break;
        case ZDJ_DECK_TYPE_EXTERNAL: zdj_new_extern_deck( deck ); break;
        // case ZDJ_DECK_TYPE_TEST:
        //     // deck->get_edge_data = &_zdj_deck_get_test_data;
        //     // zdj_test_deck_state_t * state = calloc( 1, sizeof( zdj_test_deck_state_t ) );
        //     // deck->state = state;
        //     // state->s1_p = 0;
        //     // state->s1_f = 945;
        //     // state->s2_p = 0;
        //     // state->s2_f = 7;
        //     // state->s3_p = 0;
        //     // state->s3_f = 1021;
        //     // state->s4_p = 0;
        //     // state->s4_f = 3;
        //     break;
    }

    deck->handle_control_event = NULL;

    memset( &deck->controls, 0, sizeof( zdj_deck_control_state_t ) );
    
    deck->controls.platter.scratch_override = true;
    
    deck->controls.platter.motor.enabled = false;
    deck->controls.platter.motor.pitch_setting = 1.0f;
    deck->controls.platter.motor.spin_up_cycle_count = 0;
    deck->controls.platter.motor.spin_down_cycle_count = 60;

    deck->controls.platter.slip.slip_dwell = 40;
    deck->controls.platter.slip.set_val = 0.0f;
    deck->controls.platter.slip.instant_val = 0.0f;

    // Sim constants - adjust to taste
    deck->controls.platter.slip.sim_duration = (double)deck->controls.platter.slip.slip_dwell / 4.0;
    deck->controls.platter.slip.mass = 10.0;
    deck->controls.platter.slip.spring_k = 8.0;
    deck->controls.platter.slip.damp_c = 10.0;

    // Scratch/Nudge constants
    deck->controls.platter.nudge_coeff = 10;
    deck->controls.platter.scratch_coeff = 600.0;

    // Scratch while playing
    deck->controls.platter.scratch_override = false;

    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_UNCLAIMED;
    return deck;
}

// // Set the common params for a playback deck
// void zdj_deck_init_controls( zdj_deck_t * deck ) {
//     memset( &deck->controls, 0, sizeof( zdj_deck_control_state_t ) );
    
//     deck->controls.platter.scratch_override = true;
    
//     deck->controls.platter.motor.enabled = false;
//     deck->controls.platter.motor.pitch_setting = 1.0f;
//     deck->controls.platter.motor.spin_up_cycle_count = 0;
//     deck->controls.platter.motor.spin_down_cycle_count = 60;

//     deck->controls.platter.slip.slip_dwell = 40;
//     deck->controls.platter.slip.set_val = 0.0f;
//     deck->controls.platter.slip.instant_val = 0.0f;

//     // Sim constants - adjust to taste
//     deck->controls.platter.slip.sim_duration = (double)deck->controls.platter.slip.slip_dwell / 4.0;
//     deck->controls.platter.slip.mass = 10.0;
//     deck->controls.platter.slip.spring_k = 8.0;
//     deck->controls.platter.slip.damp_c = 10.0;

//     // Scratch/Nudge constants
//     // These define the number of song PCM samples covered by a single step of the jog encoder.
//     // Scratch should be orders of magnitude higher than nudge.
//     deck->controls.platter.nudge_coeff = 10;
//     deck->controls.platter.scratch_coeff = 600.0;

//     // Scratch while playing
//     deck->controls.platter.scratch_override = false;

//     // deck->controls.loop_state.phase = ZDJ_DECK_LOOP_PHASE_INACTIVE;
// }

// DEPRECATING


// DEPRECATING

// If no discons are present in decode node, needle IS pcm addr.
// Else, have to convert needle address to 
// int64_t zdj_deck_get_pcm_addr_for_needle_head( zdj_deck_t * deck, zdj_pipeline_node_t * decode_node ) {
//     zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)decode_node->state;
//     // Get offset from needle head to decode head
//     int64_t needle_offset = round(deck->controls.platter.needle.head) - decode_state->head_decode_addr;
//     // Get offset from decode window head to decode pcm head
//     return decode_state->head_pcm_addr + needle_offset;
// }
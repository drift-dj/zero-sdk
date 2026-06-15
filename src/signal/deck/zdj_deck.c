#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <math.h>

#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/xport/zdj_deck_xport.h>
#include <zerodj/signal/deck/dj/zdj_deck_dj.h>
#include <zerodj/signal/deck/extern/zdj_deck_extern.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/system/settings/zdj_settings.h>

zdj_deck_t * zdj_new_deck( 
    zdj_deck_type_t type, 
    zdj_deck_station_t station, 
    void * resource, 
    int win_buf_count  
) {
    // printf( "zdj_new_deck %d %d %p\n", type, station, resource );
    zdj_deck_t * deck = calloc( 1, sizeof( zdj_deck_t ) );
    deck->type = type;
    deck->station = station;
    deck->status = ZDJ_DECK_STATUS_NEW;

    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_DECK_INIT;
    switch ( type ) {
        case ZDJ_DECK_TYPE_DJ: zdj_new_dj_deck( deck, resource, win_buf_count ); break;
        case ZDJ_DECK_TYPE_EXTERNAL: zdj_new_extern_deck( deck ); break;
        case ZDJ_DECK_TYPE_XPORT: zdj_new_xport_deck( deck ); break;
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

    deck->controls.platter.motor.enabled = false;
    deck->controls.platter.motor.pitch_setting = 1.0f;
    deck->controls.platter.motor.spin_up_cycle_count = 0;
    deck->controls.platter.motor.spin_down_cycle_count = 60;

    // deck->controls.platter.slip.slip_dwell = 40;
    deck->controls.platter.slip.slip_dwell = 20;
    deck->controls.platter.slip.set_val = 0.0f;
    deck->controls.platter.slip.instant_val = 0.0f;

    // Sim constants - adjust to taste
    deck->controls.platter.slip.sim_duration = (double)deck->controls.platter.slip.slip_dwell / 4.0;
    deck->controls.platter.slip.mass = 8.0;
    deck->controls.platter.slip.spring_k = 40.0;
    deck->controls.platter.slip.damp_c = 15.0;

    // Scratch/Nudge constants
    deck->controls.platter.nudge_coeff = 4.0;
    deck->controls.platter.scratch_coeff = 800.0;
    deck->controls.platter.hyperscrub_coeff = 100000.0;

    // Scratch while playing
    deck->controls.platter.scratch_override = false;
    zdj_setting_t * setting = zdj_setting_get( ZDJ_SETTING_DECK_SCRATCH_OVERRIDE );
    if( setting ) { deck->controls.platter.scratch_override = setting->b_val; }

    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_UNCLAIMED;
    return deck;
}
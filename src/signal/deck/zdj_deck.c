#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <math.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/controls/hmi/zdj_hmi.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

static void _zdj_deck_get_playback_data( zdj_pipeline_node_t * data_pipe );
static void _zdj_deck_get_external_data( zdj_pipeline_node_t * data_pipe );
static void _zdj_deck_get_test_data( zdj_pipeline_node_t * data_pipe );

zdj_deck_t * zdj_new_deck( zdj_deck_type_t type, zdj_deck_num_t num ) {
    zdj_deck_t * deck = calloc( 1, sizeof( zdj_deck_t ) );
    deck->type = type;
    deck->num = num;

    switch ( type ) {
        case ZDJ_DECK_TYPE_PLAYBACK:
            deck->get_edge_data = &_zdj_deck_get_playback_data;
            deck->state = calloc( 1, sizeof( zdj_playback_deck_state_t ) );
            break;
        case ZDJ_DECK_TYPE_EXTERNAL:
            deck->get_edge_data = &_zdj_deck_get_external_data;
            deck->state = calloc( 1, sizeof( zdj_external_deck_state_t ) );
            break;
        case ZDJ_DECK_TYPE_TEST:
            deck->get_edge_data = &_zdj_deck_get_test_data;
            deck->state = calloc( 1, sizeof( zdj_test_deck_state_t ) );
            break;
    }
}

zdj_error_type_t zdj_deinit_deck( zdj_deck_t * deck ) {

}

void _zdj_deck_get_playback_data( zdj_pipeline_node_t * data_pipe ) {
    // Sip samples from delta_t's window to fill the pipe.
    // Sample count will be significantly less than delta_t's window size.
    // Also update playback discontinuity rendering system.
    // Signal the control update thread to pull a new set of events for processing next buffer.
}

void _zdj_deck_get_external_data( zdj_pipeline_node_t * data_pipe ) {
    // Inert at the moment.  Fill with zeros.
    // Signal the control update thread to pull a new set of events for processing next buffer.
}  

void _zdj_deck_get_test_data( zdj_pipeline_node_t * data_pipe ) {
    // Fill the pipe with test tone or zero based on transport state.
    // Signal the control update thread to pull a new set of events for processing next buffer.
}

// Invoked on Control update thread.
// Process inputs and update control simulations.
void _zdj_deck_update_transport(  ) {

}
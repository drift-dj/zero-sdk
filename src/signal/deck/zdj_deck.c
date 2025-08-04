#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <math.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/controls/zdj_controls.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

static void _zdj_deck_get_playback_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo );
static void _zdj_deck_get_external_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo );
static void _zdj_deck_get_test_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo );

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
            zdj_test_deck_state_t * state = calloc( 1, sizeof( zdj_test_deck_state_t ) );
            deck->state = state;
            state->s1_p = 0;
            state->s1_f = 945;
            state->s2_p = 0;
            state->s2_f = 7;
            state->s3_p = 0;
            state->s3_f = 1021;
            state->s4_p = 0;
            state->s4_f = 3;
            break;
    }
    return deck;
}

zdj_error_type_t zdj_deinit_deck( zdj_deck_t * deck ) {

}

void _zdj_deck_get_playback_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo ) {
    // Sip samples from delta_t's window to fill the pipe.
    // Sample count will be significantly less than delta_t's window size.
    // Also update playback discontinuity rendering system.
    // Signal the control update thread to pull a new set of events for processing next buffer.
}

void _zdj_deck_get_external_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo ) {
    // Inert at the moment.  Fill with zeros.
    // Signal the control update thread to pull a new set of events for processing next buffer.
}  

void _zdj_deck_get_test_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo ) {
    float * output_buffer = data_pipe->get_data( data_pipe );
    if( !output_buffer ) { return; }
    zdj_deck_t * deck = (zdj_deck_t*)_deck;
    zdj_test_deck_state_t * deck_state = (zdj_test_deck_state_t*)deck->state;
    // Fill the pipe with test tone or zero based on transport state.
    // Signal the control update thread to pull a new set of events for processing next buffer.
    float v, m;
    for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
        // Fundamental tone (-1.0->1.0)
        deck_state->s1_p += deck_state->s1_f / 44100.0f;
        v = SDL_sinf( deck_state->s1_p );
        // Modulation tone (0->1.0)
        deck_state->s2_p += deck_state->s2_f / 44100.0f;
        m = (SDL_sinf( deck_state->s2_p ) + 1.0f) / 2.0f;
        
        if( stereo ) {
            output_buffer[ i*2 ] = v*m;

            deck_state->s3_p += (deck_state->s3_f / 44100.0f);
            v = SDL_sinf( deck_state->s3_p );
            // Modulation tone (0->1.0)
            deck_state->s4_p += (deck_state->s4_f / 44100.0f);
            m = (SDL_sinf( deck_state->s4_p ) + 1.0f) / 2.0f;
            output_buffer[ i*2+1 ] = v*m;
            // printf( "%1.3f %1.1f/%1.1f=%1.3f\n", deck_state->s4_p, v, m, v*m );
        } else {
            output_buffer[ i ] = v*m;
        }
    }
}

// Invoked on Control update thread.
// Process inputs and update control simulations.
void _zdj_deck_update_transport(  ) {

}
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/zdj_deck_manager.h>
#include <zerodj/signal/deck/dj/zdj_deck_dj.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/signal/pipeline/node/audio/tsm/zdj_tsm_pitch_node.h>
#include <zerodj/signal/pipeline/node/audio/tsm/zdj_tsm_tempo_node.h>

static void _get_edge_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo );

void zdj_deck_dj_init_soundcard( zdj_deck_t * deck ) {
    deck->get_edge_data = &_get_edge_data;
}

static void _get_edge_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo ) {
    // printf( "dj get edge data\n" );
    zdj_deck_t * deck = (zdj_deck_t*)_deck;
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_tsm_pitch_node_state_t * tsm_pitch_state = (zdj_tsm_pitch_node_state_t*)deck_state->tsm_pitch_node->state;
    zdj_tsm_tempo_node_state_t * tsm_tempo_state = (zdj_tsm_tempo_node_state_t*)deck_state->tsm_tempo_node->state;

    float * soundcard_buf = data_pipe->get_data( data_pipe );

    int i;
    switch( deck_state->tsm_source ) {
        case ZDJ_DECK_TSM_SOURCE_PITCH:
            // Copy direct from pitch node
            if( stereo && tsm_pitch_state->channel_count == 2 ) { // Stereo
                memcpy( soundcard_buf, tsm_pitch_state->out_buffer, ZDJ_SOUNDCARD_BUF_LEN * 2 * sizeof( float ) );
            } else if( !stereo && tsm_pitch_state->channel_count == 1 ) { // Mono
                memcpy( soundcard_buf, tsm_pitch_state->out_buffer, ZDJ_SOUNDCARD_BUF_LEN * sizeof( float ) );
            }
            break;

        case ZDJ_DECK_TSM_SOURCE_TEMPO:
            // Copy direct from tempo node
            if( stereo && tsm_tempo_state->channel_count == 2 ) { // Stereo
                memcpy( soundcard_buf, tsm_tempo_state->out_buffer, ZDJ_SOUNDCARD_BUF_LEN * 2 * sizeof( float ) );
            } else if( !stereo && tsm_tempo_state->channel_count == 1 ) { // Mono
                memcpy( soundcard_buf, tsm_tempo_state->out_buffer, ZDJ_SOUNDCARD_BUF_LEN * sizeof( float ) );
            }
            break;
        
        default: break;
    }

    // printf( "dj get edge data done\n" );
    // Release another cycle of the pipeline_thread_main below.
    sem_post( &deck_state->start_cycle );
}
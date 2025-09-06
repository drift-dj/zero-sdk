#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <math.h>

#include <zerodj/controls/zdj_controls.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

static void _init( zdj_deck_t * deck );
static void _deinit( zdj_deck_t * deck );
static void _get_edge_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo );

zdj_error_type_t zdj_new_plugin_deck( zdj_deck_t * deck, void * resource ) {
    deck->get_edge_data = &_get_edge_data;
    zdj_plugin_deck_state_t * state = calloc( 1, sizeof( zdj_plugin_deck_state_t ) );
    deck->state = state;
    
    return ZDJ_ERROR_OKAY;
}

static void _init( zdj_deck_t * deck ) {

}

static void _deinit( zdj_deck_t * deck ) {

}

static void _get_edge_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo ) {

}
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/deck_dsp/eq/zdj_eq_node.h>

zdj_pipeline_node_t * zdj_new_basic_eq_node( void ) {
    zdj_pipeline_node_t * node = calloc( 1, sizeof( zdj_pipeline_node_t ) );

    zdj_basic_eq_state_t * state = calloc( 1, sizeof( zdj_basic_eq_state_t ) );

    node->state = state;

    return node;
}
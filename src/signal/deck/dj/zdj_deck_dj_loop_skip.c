#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/dj/zdj_deck_dj.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>

// Public API
static void _new_loop_req( zdj_deck_t * deck, int64_t len, bool quant );
static void _move_loop_req( zdj_deck_t * deck, int64_t len );
static void _resize_loop_req( zdj_deck_t * deck, int64_t len );
static void _disable_loop_req( zdj_deck_t * deck );
static void _new_skip_req( zdj_deck_t * deck, double units );

// Internal API
static void _new_loop( zdj_deck_t * deck );
static void _enable_loop( zdj_deck_t * deck, zdj_library_cuepoint_t * cuepoint );
static void _disable_loop( zdj_deck_t * deck );
static void _move_loop( zdj_deck_t * deck, int64_t distance, bool quant );
static void _resize_loop( zdj_deck_t * deck, int64_t offset, bool quant );
static void _stage_skip( zdj_deck_t * deck );
static void _update_skip( zdj_deck_t * deck );

void zdj_deck_dj_init_loop_skip( zdj_deck_t * deck ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;

    // Public API
    deck->new_loop = &_new_loop_req;
    deck->disable_loop = &_disable_loop_req;
    deck->move_loop = &_move_loop_req;
    deck->resize_loop = &_resize_loop_req;
    deck->new_skip = &_new_skip_req;

    // Internal API
    deck_state->handle_new_loop_req = &_new_loop;
    deck_state->handle_disable_loop_req = &_disable_loop;
    deck_state->handle_move_loop_req = &_move_loop;
    deck_state->handle_resize_loop_req = &_resize_loop;
    deck_state->stage_skip_req = &_stage_skip;
    deck_state->update_skip_req = &_update_skip;
}

//////////////////////////////////////////////////
// Public API                                   //
// These are all thread-safe.                   //
// Meant to be called from control/UI threads.  //
//////////////////////////////////////////////////

static void _new_loop_req( zdj_deck_t * deck, int64_t len, bool quant ) {
    // printf( "_new_loop_req\n" );
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;
    loop_state->phase = ZDJ_DECK_LOOP_PHASE_ACTIVATE;
    loop_state->pcm_len = len;
    loop_state->quantize = quant;
    // printf( "_new_loop_req done\n" );
}

static void _disable_loop_req( zdj_deck_t * deck ) {
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;
    loop_state->phase = ZDJ_DECK_LOOP_PHASE_DEACTIVATE;
}

static void _move_loop_req( zdj_deck_t * deck, int64_t len ) {
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;
    loop_state->phase = ZDJ_DECK_LOOP_PHASE_MOVE;
}

static void _resize_loop_req( zdj_deck_t * deck, int64_t len ) {
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;
    loop_state->phase = ZDJ_DECK_LOOP_PHASE_RESIZE;
}

static void _new_skip_req( zdj_deck_t * deck, double units ) {
    // printf( "zdj_deck_new_skip_req: %1.3f\n", units );
    zdj_deck_control_skip_state_t * skip_state = &deck->controls.skip_state;
    skip_state->skip_req_len = units;
    if( skip_state->phase == ZDJ_DECK_SKIP_PHASE_INACTIVE ) {
        skip_state->phase = ZDJ_DECK_SKIP_PHASE_ACTIVATE;
        // printf( "activating: %p->phase: %d\n", skip_state, skip_state->phase );
    }
}


//////////////////////////////////////////////
// Internal API                             //
// Only accessible on fast soundcard thread //
//////////////////////////////////////////////

static void _new_loop( zdj_deck_t * deck ) {
    // printf( "_new_loop\n" );
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_state_t * controls = &deck->controls;
    controls->loop_state.is_enabled = true;
    controls->loop_state.phase = ZDJ_DECK_LOOP_PHASE_RUN;
    decode_state->add_loop_discon( deck_state->decode_node, controls );
    // printf( "_new_loop done\n" );
}

static void _enable_loop( zdj_deck_t * deck, zdj_library_cuepoint_t * cuepoint ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_state_t * controls = &deck->controls;
    controls->loop_state.is_enabled = true;
    controls->loop_state.phase = ZDJ_DECK_LOOP_PHASE_RUN;
    decode_state->add_loop_discon( deck_state->decode_node, controls );
}

static void _disable_loop( zdj_deck_t * deck ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_state_t * controls = &deck->controls;
    deck->controls.loop_state.is_enabled = false;
    deck->controls.loop_state.phase = ZDJ_DECK_LOOP_PHASE_EXIT;
    decode_state->release_loop_discon( deck_state->decode_node, controls );
}

static void _move_loop( zdj_deck_t * deck, int64_t distance, bool quant ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_pipeline_node_t * decode_node = deck_state->decode_node;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;
    decode_state->move_loop_discon( decode_node, loop_state );
}

static void _resize_loop( zdj_deck_t * deck, int64_t offset, bool quant ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_pipeline_node_t * decode_node = deck_state->decode_node;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;
    decode_state->resize_loop_discon( decode_node, loop_state );
}

// Handle a skip request.
// Quantize the skip to the next available beatgrid unit.
static void _stage_skip( zdj_deck_t * deck ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_pipeline_node_t * decode_node = deck_state->decode_node;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_skip_state_t * skip_state = &deck->controls.skip_state;
    // Confirm a skip hasn't already been staged
    decode_state->add_skip_discon( decode_node, skip_state );
    skip_state->phase = ZDJ_DECK_SKIP_PHASE_STAGED;
}

// If depart_decode_addr is in window, add layer and advance state, otherwise stay in staged.
static void _update_skip( zdj_deck_t * deck ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_pipeline_node_t * decode_node = deck_state->decode_node;
    zdj_deck_control_skip_state_t * skip_state = &deck->controls.skip_state;
    // Check if skip has happened yet
    skip_state->phase = ZDJ_DECK_SKIP_PHASE_INACTIVE;
}

// void zdj_deck_new_skip( zdj_deck_t * deck ) {
//     decode_state->add_skip_discon( );
// }
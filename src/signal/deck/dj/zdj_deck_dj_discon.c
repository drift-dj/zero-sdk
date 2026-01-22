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

// Public API
static void _new_loop_req( zdj_deck_t * deck, double len, bool quant );
static void _move_loop_req( zdj_deck_t * deck, double offset );
static void _resize_loop_req( zdj_deck_t * deck, double offset );
static void _disable_loop_req( zdj_deck_t * deck );
static void _new_skip_req( zdj_deck_t * deck, double offset, zdj_deck_control_skip_type_t type );
static void _new_hyperscrub_req( zdj_deck_t * deck, double coord );
static void _new_needledrop_req( zdj_deck_t * deck, double coord );

// Internal API
static void _new_loop( zdj_deck_t * deck );
static void _enable_loop( zdj_deck_t * deck, zdj_library_cuepoint_t * cuepoint );
static void _disable_loop( zdj_deck_t * deck );
static void _move_loop( zdj_deck_t * deck );
static void _resize_loop( zdj_deck_t * deck );
static void _stage_skip( zdj_deck_t * deck );
static void _update_skip( zdj_deck_t * deck );
static void _stage_hyperscrub( zdj_deck_t * deck );
static void _update_hyperscrub( zdj_deck_t * deck );
static void _stage_needledrop( zdj_deck_t * deck );
static void _update_needledrop( zdj_deck_t * deck );

void zdj_deck_dj_init_discon( zdj_deck_t * deck ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;

    // Public API
    deck->new_loop = &_new_loop_req;
    deck->disable_loop = &_disable_loop_req;
    deck->move_loop = &_move_loop_req;
    deck->resize_loop = &_resize_loop_req;
    deck->new_skip = &_new_skip_req;
    deck->new_hyperscrub = &_new_hyperscrub_req;
    deck->new_needledrop = &_new_needledrop_req;

    // Internal API
    deck_state->handle_new_loop_req = &_new_loop;
    deck_state->handle_disable_loop_req = &_disable_loop;
    deck_state->handle_move_loop_req = &_move_loop;
    deck_state->handle_resize_loop_req = &_resize_loop;
    deck_state->stage_skip_req = &_stage_skip;
    deck_state->update_skip_req = &_update_skip;
    deck_state->stage_hyperscrub_req = &_stage_hyperscrub;
    deck_state->update_hyperscrub_req = &_update_hyperscrub;
    deck_state->stage_needledrop_req = &_stage_needledrop;
    deck_state->update_needledrop_req = &_update_needledrop;
}

//////////////////////////////////////////////////
// Public API                                   //
// These are all thread-safe.                   //
// Meant to be called from control/UI threads.  //
//////////////////////////////////////////////////

static void _new_loop_req( zdj_deck_t * deck, double len, bool quant ) {
    // printf( "_new_loop_req\n" );
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;
    loop_state->phase = ZDJ_DECK_LOOP_PHASE_ACTIVATE;
    loop_state->pcm_len = len;
    // printf( "_new_loop_req done\n" );
}

static void _disable_loop_req( zdj_deck_t * deck ) {
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;
    loop_state->phase = ZDJ_DECK_LOOP_PHASE_DEACTIVATE;
}

static void _move_loop_req( zdj_deck_t * deck, double offset ) {
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;
    loop_state->move_req_len = offset;
    loop_state->phase = ZDJ_DECK_LOOP_PHASE_MOVE;
}

static void _resize_loop_req( zdj_deck_t * deck, double offset ) {
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;
    loop_state->length_change_req_len = offset;
    loop_state->phase = ZDJ_DECK_LOOP_PHASE_RESIZE;
}

static void _new_skip_req( zdj_deck_t * deck, double offset, zdj_deck_control_skip_type_t type ) {
    zdj_deck_control_skip_state_t * skip_state = &deck->controls.skip_state;
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    // printf( "zdj_deck_new_skip_req: %1.3f typ: %d ph:%d\n", offset, skip_state->skip_req_type, skip_state->phase );
    // Only skip when:
    // - No other skip reqs are processing
    // - There is only 1 layer in decode node (no existing discons)
    if( skip_state->phase == ZDJ_DECK_SKIP_PHASE_INACTIVE &&
        decode_state->first_layer == decode_state->last_layer
    ) {
        // Zero out any requested scrub-skip - this feels hacky
        deck->controls.platter.slip.scrub_skip_offset = 0.0;
        skip_state->skip_req_type = type;
        skip_state->skip_req_len = offset;
        skip_state->phase = ZDJ_DECK_SKIP_PHASE_ACTIVATE;
        // printf( "activating: %p->phase: %d\n", skip_state, skip_state->phase );
    }
}

static void _new_hyperscrub_req( zdj_deck_t * deck, double offset ) {
    zdj_deck_control_hyperscrub_state_t * hyperscrub_state = &deck->controls.hyperscrub_state;
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    // // printf( "zdj_deck_new_skip_req: %1.3f typ: %d ph:%d\n", offset, skip_state->skip_req_type, skip_state->phase );
    // // Only skip when:
    // // - No other skip reqs are processing
    // // - There is only 1 layer in decode node (no existing discons)
    // if( hyperscrub_state->phase == ZDJ_DECK_HYPERSCRUB_PHASE_INACTIVE &&
    //     decode_state->first_layer == decode_state->last_layer
    // ) {
    //     // Zero out any requested scrub-skip - this feels hacky
    //     deck->controls.platter.slip.scrub_skip_offset = 0.0;
    //     hyperscrub_state->phase = ZDJ_DECK_HYPERSCRUB_PHASE_ACTIVATE;
    //     // printf( "activating: %p->phase: %d\n", skip_state, skip_state->phase );
    // }
}

static void _new_needledrop_req( zdj_deck_t * deck, double coord ) {
    // printf( "==> _new_needledrop_req: %1.3f\n", coord );
    zdj_deck_control_needledrop_state_t * needledrop_state = &deck->controls.needledrop_state;
    if( needledrop_state->phase == ZDJ_DECK_NEEDLEDROP_PHASE_INACTIVE ) {
        needledrop_state->origin_coord = coord;
        needledrop_state->phase = ZDJ_DECK_NEEDLEDROP_PHASE_ACTIVATE;
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
    decode_state->enable_loop_discon( deck_state->decode_node, controls );
    decode_state->refresh_loop_discon_layers( deck_state->decode_node, controls );
    // printf( "_new_loop done\n" );
}

static void _enable_loop( zdj_deck_t * deck, zdj_library_cuepoint_t * cuepoint ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_state_t * controls = &deck->controls;
    controls->loop_state.is_enabled = true;
    controls->loop_state.phase = ZDJ_DECK_LOOP_PHASE_RUN;
    decode_state->enable_loop_discon( deck_state->decode_node, controls );
    decode_state->refresh_loop_discon_layers( deck_state->decode_node, controls );
}

static void _disable_loop( zdj_deck_t * deck ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_state_t * controls = &deck->controls;
    deck->controls.loop_state.is_enabled = false;
    deck->controls.loop_state.phase = ZDJ_DECK_LOOP_PHASE_EXIT;
    decode_state->release_loop_discon( deck_state->decode_node, controls );
}

static void _move_loop( zdj_deck_t * deck ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_pipeline_node_t * decode_node = deck_state->decode_node;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;  
    zdj_tsm_tempo_node_state_t * tsm_tempo_state = (zdj_tsm_tempo_node_state_t*)deck_state->tsm_tempo_node->state;
    zdj_tsm_pitch_node_state_t * tsm_pitch_state = (zdj_tsm_pitch_node_state_t*)deck_state->tsm_pitch_node->state;  
    
    double req_offset = deck->controls.loop_state.move_req_len;

    // printf( "pre _move_loop offset: hd:%1.1f loop:[%1.1f > %1.1f] >>> %1.1f \n", 
    //     decode_state->head.origin_d,
    //     deck->controls.loop_state.start_origin_d,
    //     deck->controls.loop_state.end_origin_d,
    //     deck->controls.loop_state.move_req_len 
    // );
    // Offset loop by req_len
    decode_state->move_loop_discon( decode_node, &deck->controls );

    // printf( "post _move_loop offset: hd:%1.1f loop:[%1.1f > %1.1f] >>> %1.1f \n", 
    //     decode_state->head.origin_d,
    //     deck->controls.loop_state.start_origin_d,
    //     deck->controls.loop_state.end_origin_d,
    //     deck->controls.loop_state.move_req_len 
    // );

    // Moving the loop will step on the decode->head addr, and we're not playing,
    // so just needledrop and re-install the loop structure
    if( decode_state->move_loop_discon_will_move_head( decode_node, &deck->controls ) &&
        !deck->controls.platter.motor.enabled && 
        deck->controls.platter.motor.instant_rate < zdj_eps 
    ) {
        // printf( "===> moving head while stopped len:%1.1f\n", deck->controls.loop_state.move_req_len );
        // Do a cheat-y needledrop here
        zdj_decode_init_addr( &decode_state->head );

        // Hard-set the decode head
        if( req_offset > 0 ) {
            decode_state->addr_for_origin_d_coord_in_layer( decode_node, decode_state->first_layer, &decode_state->head, loop_state->start_origin_d + 1 );
        } else {
            decode_state->addr_for_origin_d_coord_in_layer( decode_node, decode_state->last_layer, &decode_state->head, loop_state->end_origin_d - 1000 );
        }
        
        // Reset platter
        zdj_dj_deck_reset_platter( &deck->controls.platter, decode_state->head.transport_d );
        // Reset tempo state/coords
        zdj_tsm_tempo_node_state_t * tsm_tempo_state = (zdj_tsm_tempo_node_state_t*)deck_state->tsm_tempo_node->state;
        tsm_tempo_state->decode_coord = decode_state->head.transport_d;
        zdj_reset_tsm_tempo_node( deck_state->tsm_tempo_node );
        // Reset pitch state/coords
        zdj_tsm_pitch_node_state_t * tsm_pitch_state = (zdj_tsm_pitch_node_state_t*) deck_state->tsm_pitch_node->state;
        tsm_pitch_state->decode_start_coord = decode_state->head.transport_d;
        tsm_pitch_state->decode_end_coord = tsm_pitch_state->decode_start_coord;
    }

    // Reset layer stack 
    // decode_state->enable_loop_discon( deck_state->decode_node, &deck->controls );
    decode_state->refresh_loop_discon_layers( deck_state->decode_node, &deck->controls );

    // Return to run mode
    deck->controls.loop_state.phase = ZDJ_DECK_LOOP_PHASE_RUN;
}

static void _resize_loop( zdj_deck_t * deck ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_pipeline_node_t * decode_node = deck_state->decode_node;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;
    
    decode_state->resize_loop_discon( decode_node, &deck->controls );

    if( !deck->controls.platter.motor.enabled &&
        decode_state->move_loop_discon_will_move_head( decode_node, &deck->controls ) 
    ) { 
        // Do a cheat-y needledrop here
        zdj_decode_init_addr( &decode_state->head );

        // Hard-set the decode head
        decode_state->addr_for_origin_d_coord_in_layer( decode_node, decode_state->last_layer, &decode_state->head, loop_state->end_origin_d - 1000 );

        // Reset platter
        zdj_dj_deck_reset_platter( &deck->controls.platter, decode_state->head.transport_d );

        // Reset tempo state/coords
        zdj_tsm_tempo_node_state_t * tsm_tempo_state = (zdj_tsm_tempo_node_state_t*)deck_state->tsm_tempo_node->state;
        tsm_tempo_state->decode_coord = decode_state->head.transport_d;
        zdj_reset_tsm_tempo_node( deck_state->tsm_tempo_node );

        // Reset pitch state/coords
        zdj_tsm_pitch_node_state_t * tsm_pitch_state = (zdj_tsm_pitch_node_state_t*) deck_state->tsm_pitch_node->state;
        tsm_pitch_state->decode_start_coord = decode_state->head.transport_d;
        tsm_pitch_state->decode_end_coord = tsm_pitch_state->decode_start_coord;
    }

    decode_state->refresh_loop_discon_layers( deck_state->decode_node, &deck->controls );

    deck->controls.loop_state.phase = ZDJ_DECK_LOOP_PHASE_RUN;
}

// Handle a skip request.
// Quantize the skip to the next available beatgrid unit.
static void _stage_skip( zdj_deck_t * deck ) {
    // printf( "_stage_skip\n" );
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_pipeline_node_t * decode_node = deck_state->decode_node;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_skip_state_t * skip_state = &deck->controls.skip_state;

    skip_state->counter = 0;

    if( deck->controls.loop_state.is_enabled ) {
        // printf( "loop-enabled skip\n" );
        // can we reset layer here?
        // Do we make a new layer
        
    } else { // If loop not enabled:
        // printf( "unbounded skip\n" );
        // If we're synced, stage the new layer at the next global beatgrid.
        // If we're not synced, stage the new layer ASAP.
        // printf( "add_skip_discon: %p %1.3f\n", skip_state, skip_state->skip_req_len );
        decode_state->add_skip_discon( decode_node, &deck->controls );
        skip_state->phase = ZDJ_DECK_SKIP_PHASE_STAGED;
    }
}

// If depart_decode_addr is in window, add layer and advance state, otherwise stay in staged.
static void _update_skip( zdj_deck_t * deck ) {
    // printf( "_update_skip\n" );
    zdj_deck_control_skip_state_t * skip_state = &deck->controls.skip_state;
    // Check if skip has happened yet
    if( skip_state->counter++ > 2 ) {
        // printf( "deactivating skip\n" );
        skip_state->phase = ZDJ_DECK_SKIP_PHASE_INACTIVE;
    }
    // printf( "_update_skip done\n" );
}

static void _stage_hyperscrub( zdj_deck_t * deck ) {
    // printf( "_stage_hyperscrub\n" );
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_pipeline_node_t * decode_node = deck_state->decode_node;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_hyperscrub_state_t * hyperscrub_state = &deck->controls.hyperscrub_state;

    hyperscrub_state->counter = 0;

    decode_state->add_hyperscrub_discon( decode_node, &deck->controls );
    hyperscrub_state->phase = ZDJ_DECK_HYPERSCRUB_PHASE_STAGED;
}

// If depart_decode_addr is in window, add layer and advance state, otherwise stay in staged.
static void _update_hyperscrub( zdj_deck_t * deck ) {
    // printf( "_update_hyperscrub\n" );
    zdj_deck_control_hyperscrub_state_t * hyperscrub_state = &deck->controls.hyperscrub_state;
    // Check if skip has happened yet
    if( hyperscrub_state->counter++ > 2 ) {
        // printf( "deactivating hyperscrub\n" );
        hyperscrub_state->phase = ZDJ_DECK_HYPERSCRUB_PHASE_INACTIVE;
    }
    // printf( "_update_hyperscrub done\n" );
}

static void _stage_needledrop( zdj_deck_t * deck ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_pipeline_node_t * decode_node = deck_state->decode_node;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_needledrop_state_t * needledrop_state = &deck->controls.needledrop_state;
    zdj_tsm_tempo_node_state_t * tsm_tempo_state = (zdj_tsm_tempo_node_state_t*)deck_state->tsm_tempo_node->state;
    zdj_tsm_pitch_node_state_t * tsm_pitch_state = (zdj_tsm_pitch_node_state_t*)deck_state->tsm_pitch_node->state;

    needledrop_state->counter = 0;
    
    printf( "stage_needledrop: %1.3f -> %1.3f\n", decode_state->head.origin_d, needledrop_state->origin_coord );

    // Reset decode head
    decode_node->reset_window( decode_node, needledrop_state->origin_coord );

    // Reset platter
    zdj_dj_deck_reset_platter( &deck->controls.platter, decode_state->head.transport_d );

    // Reset tempo state/coords
    tsm_tempo_state->decode_coord = decode_state->head.transport_d;
    zdj_reset_tsm_tempo_node( deck_state->tsm_tempo_node );

    // Reset pitch state/coords
    tsm_pitch_state->decode_start_coord = decode_state->head.transport_d;
    tsm_pitch_state->decode_end_coord = tsm_pitch_state->decode_start_coord;

    needledrop_state->phase = ZDJ_DECK_NEEDLEDROP_PHASE_STAGED;
}

static void _update_needledrop( zdj_deck_t * deck ) {
    printf( "update_needledrop\n" );
    zdj_deck_control_needledrop_state_t * needledrop_state = &deck->controls.needledrop_state;
    // if( needledrop_state->counter++ > 3 ) { // Debounce the needledrop for a few DAC cycles
        needledrop_state->phase = ZDJ_DECK_NEEDLEDROP_PHASE_INACTIVE;
    // }
}
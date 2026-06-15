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

static double _offset_and_wrap_origin_d_coord_in_loop ( 
    zdj_deck_control_loop_state_t * loop_state,  
    double origin_d,
    double offset_d
);

void zdj_dj_deck_skip( zdj_deck_t * deck, double val ) {
    // printf( "Skip: %1.3f\n", val );
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_pipeline_node_t * decode_node = deck_state->decode_node;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;  
    zdj_deck_control_skip_state_t * skip_state = &deck->controls.skip_state;  
    zdj_tsm_tempo_node_state_t * tsm_tempo_state = (zdj_tsm_tempo_node_state_t*)deck_state->tsm_tempo_node->state;
    zdj_tsm_pitch_node_state_t * tsm_pitch_state = (zdj_tsm_pitch_node_state_t*)deck_state->tsm_pitch_node->state;  

    ///////////////////////////////////////////////
    // Phase 1 - Build the PCM distance from the //
    // current beatgrid quantize setting.        //
    ///////////////////////////////////////////////
    double req_offset_d = 0;
    if( zdj_dj_deck_quantize_commands( deck ) && zdj_dj_deck_has_beatgrid( deck ) ){
        double bg_offset = deck->controls.discon_quantize_val * val;
        req_offset_d = decode_state->get_d_offset_for_beatgrid_dist( deck_state->decode_node, bg_offset );
    } else {
        req_offset_d = val * 7000;
    }

    /////////////////////////////////////////////////
    // Phase 2 - Build the departure origin coord. //
    // Quantize to beatgrid.
    // Constrain to song PCM coords.               //
    // Wrap within loop.                           //
    /////////////////////////////////////////////////
    // Quantize depart coord (if enabled)
    double depart_origin_d = 0;
    if( zdj_dj_deck_quantize_commands( deck ) && zdj_dj_deck_has_beatgrid( deck ) ) {
        depart_origin_d = zdj_decode_guantize_origin_d_for_beatgrid(
            decode_state->head.origin_d + 300, // give enough space for a crossfade
            deck->controls.discon_quantize_val,
            decode_state->song,
            ZDJ_DECODE_QUANTIZE_CEIL
        );
    } else {
        depart_origin_d = decode_state->head.origin_d + 300;
    }
    // Constrain
    if( depart_origin_d < 0.0 || depart_origin_d > decode_state->song_pcm_duration ) {
        return;
    }
    // Wrap in Loop (if enabled)
    if( zdj_dj_deck_loop_is_enabled( deck ) ) {
        depart_origin_d = _offset_and_wrap_origin_d_coord_in_loop( 
            loop_state, depart_origin_d, 0 // Depart coord doesn't need offset
        );
    }

    ///////////////////////////////////////////////////
    // Phase 3 - Build the destination origin coord. //
    // Wrap within loop.                             //
    ///////////////////////////////////////////////////
    double dest_origin_d = 0;
    // Wrap in Loop (if enabled)
    if( zdj_dj_deck_loop_is_enabled( deck ) ) {
        dest_origin_d = _offset_and_wrap_origin_d_coord_in_loop( 
            loop_state, depart_origin_d, req_offset_d 
        );
    } else {
        dest_origin_d = depart_origin_d + req_offset_d;
    }

    zdj_decode_discon_request_t req;
    memset( &req, 0, sizeof( zdj_decode_discon_request_t ) );

    //////////////////////////////////////////////
    // Phase 4 - If a loop is enabled, add loop //
    // bounds/enable to the request             //
    //////////////////////////////////////////////
    if( zdj_dj_deck_loop_is_enabled( deck ) ) {
        req.loop_coord_subspace = ZDJ_ADDR_SUBSPACE_D;
        req.loop_start_origin_d = loop_state->start_origin_d;
        req.loop_end_origin_d = loop_state->end_origin_d;
        req.enable_loop = true;
    }


    ////////////////////////////////////////////////////////
    // Phase 5 - If we're not playing just request reset. //
    // Observe presence of loop.                          //
    ////////////////////////////////////////////////////////
    if ( !zdj_dj_deck_platter_is_playing( deck ) ) {
        // We're going to reset the decode node, 
        // so just figure out new head origin coords.
        req.type = ZDJ_DECODE_DISCON_REQUEST_RESET;
        req.reset_coord_subspace = ZDJ_ADDR_SUBSPACE_D;
        req.reset_head_origin_d = dest_origin_d;
        decode_state->request_discon( deck_state->decode_node, &req );

        // Reset TSM nodes
        zdj_dj_deck_reset_tsm_nodes( deck );


    ///////////////////////////////////////////////
    // Phase 6 - If we're playing, request skip. //
    // Observe presence of loop.                 //
    ///////////////////////////////////////////////
    } else if ( zdj_dj_deck_platter_is_playing( deck ) ) {
        req.type = ZDJ_DECODE_DISCON_REQUEST_SKIP_PLAY_TO_PLAY;
        req.skip_coord_subspace = ZDJ_ADDR_SUBSPACE_D;
        req.skip_depart_origin_d = depart_origin_d;
        req.skip_dest_origin_d = dest_origin_d;

        // Update the skip_state so refresh_layers knows where to start the next layer.
        // Lock the skip_state until head has reached discon and processed new layers.
        skip_state->current_offset += req_offset_d;
        skip_state->dest_origin_d = dest_origin_d;
        skip_state->locked = true;

        decode_state->request_discon( deck_state->decode_node, &req );

    }
}

void zdj_dj_deck_change_skip_length( zdj_deck_t * deck, double val ) {
    printf( "Skip Length Change: %1.3f\n", val );
    // Sixteenth
    if( fabs( deck->controls.discon_quantize_val - 0.0625 ) < zdj_eps ) {
        if( val > 0.0 ) { deck->controls.discon_quantize_val = 0.125; }

    // Eighth
    } else if( fabs( deck->controls.discon_quantize_val - 0.125 ) < zdj_eps ) {
        if( val > 0.0 ) { deck->controls.discon_quantize_val = 0.250; } 
        else if( val < 0.0 ) { deck->controls.discon_quantize_val = 0.0625; }

    // Quarter
    } else if( fabs( deck->controls.discon_quantize_val - 0.250 ) < zdj_eps ) {
        if( val < 0.0 ) { deck->controls.discon_quantize_val = 0.125; }
    }
    deck->controls.skip_state.skip_req_len = deck->controls.discon_quantize_val;
}

void zdj_dj_deck_set_skip_origin( zdj_deck_t * deck ) {
    printf( "Skip set origin\n" );
    deck->controls.skip_state.current_offset = 0.0;
}

void zdj_dj_deck_skip_to_origin( zdj_deck_t * deck ) {
    printf( "Skip to origin\n" );
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_pipeline_node_t * decode_node = deck_state->decode_node;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;  
    zdj_deck_control_skip_state_t * skip_state = &deck->controls.skip_state;  
    zdj_tsm_tempo_node_state_t * tsm_tempo_state = (zdj_tsm_tempo_node_state_t*)deck_state->tsm_tempo_node->state;
    zdj_tsm_pitch_node_state_t * tsm_pitch_state = (zdj_tsm_pitch_node_state_t*)deck_state->tsm_pitch_node->state;  

    /////////////////////////////////////////////////
    // Phase 1 - Build the departure origin coord. //
    // Quantize to beatgrid.                       //
    // Constrain to song PCM coords.               //
    /////////////////////////////////////////////////
    // Quantize depart coord (if enabled)
    double depart_origin_d = 0;
    if( zdj_dj_deck_quantize_commands( deck ) && zdj_dj_deck_has_beatgrid( deck ) ) {
        depart_origin_d = zdj_decode_guantize_origin_d_for_beatgrid(
            decode_state->head.origin_d + 300, // give enough space for a crossfade
            deck->controls.discon_quantize_val,
            decode_state->song,
            ZDJ_DECODE_QUANTIZE_CEIL
        );
    } else {
        depart_origin_d = decode_state->head.origin_d + 300;
    }
    // Constrain
    if( depart_origin_d < 0.0 || depart_origin_d > decode_state->song_pcm_duration ) {
        return;
    }

    ///////////////////////////////////////////////////
    // Phase 2 - Build the destination origin coord. //
    ///////////////////////////////////////////////////
    double dest_origin_d = depart_origin_d - skip_state->current_offset;



    zdj_decode_discon_request_t req;
    memset( &req, 0, sizeof( zdj_decode_discon_request_t ) );


    ////////////////////////////////////////////////////////
    // Phase 3 - If we're not playing just request reset. //
    // Observe presence of loop.                          //
    ////////////////////////////////////////////////////////
    if ( !zdj_dj_deck_platter_is_playing( deck ) ) {
        // We're going to reset the decode node, 
        // so just figure out new head origin coords.
        req.type = ZDJ_DECODE_DISCON_REQUEST_RESET;
        req.reset_coord_subspace = ZDJ_ADDR_SUBSPACE_D;
        req.reset_head_origin_d = dest_origin_d;
        decode_state->request_discon( deck_state->decode_node, &req );

        // Reset TSM nodes
        zdj_dj_deck_reset_tsm_nodes( deck );


    ///////////////////////////////////////////////
    // Phase 4 - If we're playing, request skip. //
    // Observe presence of loop.                 //
    ///////////////////////////////////////////////
    } else if ( zdj_dj_deck_platter_is_playing( deck ) ) {
        req.type = ZDJ_DECODE_DISCON_REQUEST_SKIP_PLAY_TO_PLAY;
        req.skip_depart_origin_d = depart_origin_d;
        req.skip_dest_origin_d = dest_origin_d;
        req.skip_coord_subspace = ZDJ_ADDR_SUBSPACE_D;

        // Update the skip_state so refresh_layers knows where to start the next layer.
        // Lock the skip_state until head has reached discon and processed new layers.
        skip_state->current_offset = 0.0;
        skip_state->dest_origin_d = dest_origin_d;
        skip_state->locked = true;

        decode_state->request_discon( deck_state->decode_node, &req );

    }
}

// This is undefined if loop is not currently enabled
void zdj_dj_deck_skip_to_loop_start( zdj_deck_t * deck ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_pipeline_node_t * decode_node = deck_state->decode_node;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;  
    zdj_deck_control_skip_state_t * skip_state = &deck->controls.skip_state;  


    /////////////////////////////////////////////////
    // Phase 1 - Build the departure origin coord. //
    // Quantize to beatgrid.                       //
    // Constrain to song PCM coords.               //
    /////////////////////////////////////////////////
    // Quantize depart coord (if enabled)
    double depart_origin_d = 0;
    if( zdj_dj_deck_quantize_commands( deck ) && zdj_dj_deck_has_beatgrid( deck ) ) {
        depart_origin_d = zdj_decode_guantize_origin_d_for_beatgrid(
            decode_state->head.origin_d + 300, // give enough space for a crossfade
            deck->controls.discon_quantize_val,
            decode_state->song,
            ZDJ_DECODE_QUANTIZE_CEIL
        );
    } else {
        depart_origin_d = decode_state->head.origin_d + 300;
    }
    // Constrain
    if( depart_origin_d < 0.0 || depart_origin_d > decode_state->song_pcm_duration ) {
        return;
    }

    ///////////////////////////////////////////////////
    // Phase 2 - Build the destination origin coord. //
    ///////////////////////////////////////////////////
    double dest_origin_d = loop_state->start_origin_d;

    zdj_decode_discon_request_t req;
    memset( &req, 0, sizeof( zdj_decode_discon_request_t ) );
    ////////////////////////////////////////////////////////
    // Phase 3 - If we're not playing just request reset. //
    // Observe presence of loop.                          //
    ////////////////////////////////////////////////////////
    if ( !zdj_dj_deck_platter_is_playing( deck ) ) {
        // We're going to reset the decode node, 
        // so just figure out new head origin coords.
        req.type = ZDJ_DECODE_DISCON_REQUEST_RESET;
        req.reset_coord_subspace = ZDJ_ADDR_SUBSPACE_D;
        req.reset_head_origin_d = dest_origin_d;
        decode_state->request_discon( deck_state->decode_node, &req );

        // Reset TSM nodes
        zdj_dj_deck_reset_tsm_nodes( deck );


    ///////////////////////////////////////////////
    // Phase 4 - If we're playing, request skip. //
    // Observe presence of loop.                 //
    ///////////////////////////////////////////////
    } else if ( zdj_dj_deck_platter_is_playing( deck ) ) {
        req.type = ZDJ_DECODE_DISCON_REQUEST_SKIP_PLAY_TO_PLAY;
        req.skip_depart_origin_d = depart_origin_d;
        req.skip_dest_origin_d = dest_origin_d;
        req.skip_coord_subspace = ZDJ_ADDR_SUBSPACE_D;

        // Update the skip_state so refresh_layers knows where to start the next layer.
        // Lock the skip_state until head has reached discon and processed new layers.
        skip_state->current_offset = 0.0;
        skip_state->dest_origin_d = dest_origin_d;
        skip_state->locked = true;

        decode_state->request_discon( deck_state->decode_node, &req );
    }
}

static double _offset_and_wrap_origin_d_coord_in_loop ( 
    zdj_deck_control_loop_state_t * loop_state,  
    double origin_d,
    double offset_d
) {
    double wrapped_origin_d;
    // Wrap around loop end
    if( (origin_d+offset_d) > loop_state->end_origin_d ) {
        wrapped_origin_d = loop_state->start_origin_d + ( (origin_d+offset_d) - loop_state->end_origin_d );
        // req_offset = origin_d - wrapped_origin_d;
    
    // Wrap around loop start
    } else if( (origin_d+offset_d) < loop_state->start_origin_d ) {
        wrapped_origin_d = loop_state->end_origin_d - (loop_state->start_origin_d - (origin_d+offset_d));
        // req_offset = wrapped_origin_d - decode_state->head.origin_d;
    }
    return wrapped_origin_d;
}
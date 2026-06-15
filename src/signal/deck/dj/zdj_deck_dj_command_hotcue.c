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

void zdj_dj_deck_hotcue( zdj_deck_t * deck, int num ) {
    printf( "hotcue: %d\n", num );
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_pipeline_node_t * decode_node = deck_state->decode_node;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;  
    zdj_deck_control_skip_state_t * skip_state = &deck->controls.skip_state;  
    zdj_tsm_tempo_node_state_t * tsm_tempo_state = (zdj_tsm_tempo_node_state_t*)deck_state->tsm_tempo_node->state;
    zdj_tsm_pitch_node_state_t * tsm_pitch_state = (zdj_tsm_pitch_node_state_t*)deck_state->tsm_pitch_node->state;  
    zdj_library_performance_t * perf = deck_state->song->performance;

    // Skip (Quantized) if deck is playing
    if( zdj_dj_deck_platter_is_playing( deck ) ) {

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

        ///////////////////////////////////////////////////
        // Phase 3 - Build the destination origin coord. //
        // Wrap within loop.                             //
        ///////////////////////////////////////////////////
        double dest_origin_d = (double)perf->cuepoints[ num ]->sample;

        zdj_decode_discon_request_t req;
        memset( &req, 0, sizeof( zdj_decode_discon_request_t ) );

        ////////////////////////////////////////////////
        // Phase 4 - If a cuepoint is a loop, be sure //
        // to enable loop in new skip layer           //
        ////////////////////////////////////////////////
        // if( zdj_dj_deck_loop_is_enabled( deck ) ) {
        //     req.loop_coord_subspace = ZDJ_ADDR_SUBSPACE_D;
        //     req.loop_start_origin_d = loop_state->start_origin_d;
        //     req.loop_end_origin_d = loop_state->end_origin_d;
        //     req.enable_loop = true;
        // }

        req.type = ZDJ_DECODE_DISCON_REQUEST_SKIP_PLAY_TO_PLAY;
        req.skip_coord_subspace = ZDJ_ADDR_SUBSPACE_D;
        req.skip_depart_origin_d = depart_origin_d;
        req.skip_dest_origin_d = dest_origin_d;
        
        skip_state->dest_origin_d = dest_origin_d;
        skip_state->locked = true;

        decode_state->request_discon( deck_state->decode_node, &req );

        
    
    // Reset if deck isn't playing
    // The control will also have requested a platter start,
    // so this forms an instant reset-to-play sequence.
    } else if( !zdj_dj_deck_platter_is_playing( deck ) ) {
        deck->controls.cue_state.reset_pending = true;
        deck->controls.cue_state.dest_origin_d = perf->cuepoints[ num ]->sample;
    }
}
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

    // bug out if we're outside song's pcm coords
    if( decode_state->head.origin_d < 0.0 || 
        decode_state->head.origin_d > decode_state->song_pcm_duration 
    ) {
        deck->controls.loop_state.phase = ZDJ_DECK_LOOP_PHASE_EXIT;
        return;
    }

    double loop_start_origin_d = 0;
    double loop_end_origin_d = 0;
    double loop_pcm_len = controls->loop_state.pcm_len;
    double loop_bg_len = controls->loop_state.beatgrid_len;
    double head_origin_bg = decode_state->head.origin_bg;
    double head_origin_d = decode_state->head.origin_d;
    double song_pcm_len = decode_state->song_pcm_duration;


    ///////////////////////////////////////////////////
    // Phase 1 - determine/constrain new loop coords //
    ///////////////////////////////////////////////////

    if( controls->discon_quantize &&
        decode_state->song->performance &&
        decode_state->song->performance->has_beat_grid 
    ) {
        double bpm = deck_state->song->performance->bpm;
        int rate = deck_state->song->audio->av_sample_rate;
        double len = loop_bg_len;

        // Quantize the start to the most recent beatgrid w/current quant setting
        double head_quant = floor( head_origin_bg / controls->discon_quantize_val ) * controls->discon_quantize_val;
        double quant_offset = zdj_signal_pcm_count_for_beatgrid_count(
            (head_quant - head_origin_bg), bpm, rate
        );
        loop_start_origin_d = fmax( 0.0, head_origin_d + quant_offset );
        

        // Constrain the loop length to song end
        bool continue_search = true;
        // Find longest loop which can be enabled
        if( continue_search || (fabs( len - 64.0 ) < zdj_eps) ) {
            double p_len = zdj_signal_pcm_count_for_beatgrid_count( 64.0, bpm, rate );
            if( (head_origin_d + p_len) < song_pcm_len ) {
                loop_pcm_len = p_len; loop_bg_len = 64.0; continue_search = false;
            }
        }
        if( continue_search || (fabs( len - 32.0 ) < zdj_eps) ) {
            double p_len = zdj_signal_pcm_count_for_beatgrid_count( 32.0, bpm, rate );
            if( (head_origin_d + p_len) < song_pcm_len ) {
                loop_pcm_len = p_len; loop_bg_len = 32.0; continue_search = false;
            }
        }
        if( continue_search || (fabs( len - 16.0 ) < zdj_eps) ) {
            double p_len = zdj_signal_pcm_count_for_beatgrid_count( 16.0, bpm, rate );
            if( (head_origin_d + p_len) < song_pcm_len ) {
                loop_pcm_len = p_len; loop_bg_len = 16.0; continue_search = false;
            }
        }
        if( continue_search || (fabs( len - 8.0 ) < zdj_eps) ) {
            double p_len = zdj_signal_pcm_count_for_beatgrid_count( 8.0, bpm, rate );
            if( (head_origin_d + p_len) < song_pcm_len ) {
                loop_pcm_len = p_len; loop_bg_len = 8.0; continue_search = false;
            }
        }
        if( continue_search || (fabs( len - 4.0 ) < zdj_eps) ) {
            double p_len = zdj_signal_pcm_count_for_beatgrid_count( 4.0, bpm, rate );
            if( (head_origin_d + p_len) < song_pcm_len ) {
                loop_pcm_len = p_len; loop_bg_len = 4.0; continue_search = false;
            }
        }
        if( continue_search || (fabs( len - 1.0 ) < zdj_eps) ) {
            double p_len = zdj_signal_pcm_count_for_beatgrid_count( 1.0, bpm, rate );
            if( (head_origin_d + p_len) < song_pcm_len ) {
                loop_pcm_len = p_len; loop_bg_len = 1.0; continue_search = false;
            }
        }
        if( continue_search || (fabs( len - 0.5 ) < zdj_eps) ) {
            double p_len = zdj_signal_pcm_count_for_beatgrid_count( 0.5, bpm, rate );
            if( (head_origin_d + p_len) < song_pcm_len ) {
                loop_pcm_len = p_len; len = 0.5; continue_search = false;
            }
        }
        if( continue_search || (fabs( len - 0.25 ) < zdj_eps) ) {
            double p_len = zdj_signal_pcm_count_for_beatgrid_count( 0.25, bpm, rate );
            if( (head_origin_d + p_len) < song_pcm_len ) {
                loop_pcm_len = p_len; loop_bg_len = 0.25; continue_search = false;
            }
        }

        loop_end_origin_d = loop_start_origin_d + loop_pcm_len;
    } else {
        loop_start_origin_d = decode_state->head.origin_d;
        if( loop_start_origin_d + loop_pcm_len > song_pcm_len ) {
            loop_pcm_len = song_pcm_len - loop_start_origin_d - 1.0;
        }
        loop_end_origin_d = loop_start_origin_d + loop_pcm_len;
    }

    

    controls->loop_state.start_origin_d = loop_start_origin_d;
    controls->loop_state.end_origin_d = loop_end_origin_d;
    controls->loop_state.pcm_len = loop_pcm_len;
    controls->loop_state.beatgrid_len = loop_bg_len;
    controls->loop_state.is_enabled = true;
    controls->loop_state.phase = ZDJ_DECK_LOOP_PHASE_RUN;

    ////////////////////////////////////////////////////////////////////
    // Phase 2 - determine and set current layer's truncation details //
    ////////////////////////////////////////////////////////////////////

    // Use transport coords to grab the current layer under head to ensure uniqueness.
    zdj_decode_layer_t * layer_under_head = decode_state->get_layer_containing_core_addr( 
        deck_state->decode_node,
        &decode_state->head,
        ZDJ_ADDR_COORD_TRANSPORT
    );
    // Since we're truncating to a loop, we need to add a ref to the loop_state.
    layer_under_head->_loop_state = &controls->loop_state;

    // Truncate current layer to start/end based on above.
    layer_under_head->truncate_to_loop( 
        layer_under_head, 
        deck_state->decode_node,  
        loop_start_origin_d,
        loop_end_origin_d
    );
    decode_state->discon_is_active = true;

    // Zero move the window to re-fill
    deck_state->decode_node->move_window( deck_state->decode_node, 0 );
}

// Enable and jump to a pre-defined loop
// Not currently implemented
static void _enable_loop( zdj_deck_t * deck, zdj_library_cuepoint_t * cuepoint ) {
    // zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    // zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    // zdj_deck_control_state_t * controls = &deck->controls;
    // controls->loop_state.is_enabled = true;
    // controls->loop_state.phase = ZDJ_DECK_LOOP_PHASE_RUN;
    // decode_state->enable_loop_discon( deck_state->decode_node, controls );
    // decode_state->refresh_loop_discon_layers( deck_state->decode_node, controls );
    
    // truncate
    // move( 0 )
}

static void _disable_loop( zdj_deck_t * deck ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_state_t * controls = &deck->controls;
    deck->controls.loop_state.is_enabled = false;
    deck->controls.loop_state.phase = ZDJ_DECK_LOOP_PHASE_EXIT;

    decode_state->discon_is_active = false;

    zdj_decode_layer_t * layer_under_head = decode_state->get_layer_containing_core_addr( 
        deck_state->decode_node, &decode_state->head, ZDJ_ADDR_COORD_TRANSPORT 
    );
    layer_under_head->untruncate( layer_under_head, deck_state->decode_node );

    // Zero move to re-fill the window
    deck_state->decode_node->move_window( deck_state->decode_node, 0 );
}

static void _move_loop( zdj_deck_t * deck ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_pipeline_node_t * decode_node = deck_state->decode_node;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;  
    zdj_tsm_tempo_node_state_t * tsm_tempo_state = (zdj_tsm_tempo_node_state_t*)deck_state->tsm_tempo_node->state;
    zdj_tsm_pitch_node_state_t * tsm_pitch_state = (zdj_tsm_pitch_node_state_t*)deck_state->tsm_pitch_node->state;  
    
    double req_offset = deck->controls.loop_state.move_req_len;
    deck->controls.loop_state.move_req_len = 0;
    double loop_start_origin_d = deck->controls.loop_state.start_origin_d;
    double loop_end_origin_d = deck->controls.loop_state.end_origin_d;
    double loop_pcm_len = deck->controls.loop_state.pcm_len;

    //////////////////////////////////////////////////////////
    // Phase 1 - constrain loop move within song PCM coords //
    //////////////////////////////////////////////////////////
    // Constrain offset within song PCM coords
    if( loop_start_origin_d + req_offset < 0 ) {
        // Update the req offset to the start of the song
        req_offset = (loop_start_origin_d-1.0) * -1.0;
    } else if( loop_end_origin_d + req_offset > decode_state->song_pcm_duration ) {
        // Update the req offset so the loop ends at the end of the song
        req_offset = decode_state->song_pcm_duration - loop_pcm_len - loop_start_origin_d;
    }
    // Update the origin coords with constrained offset request
    loop_start_origin_d += req_offset;
    loop_end_origin_d += req_offset;

    //////////////////////////////////////////////////
    // Phase 2 - If we're not playing and loop move //
    //  needs to move the head, perform needledrop. //
    //////////////////////////////////////////////////
    if ( !deck->controls.platter.motor.enabled && 
         deck->controls.platter.motor.instant_rate < zdj_eps &&
         (decode_state->head.origin_d < loop_start_origin_d ||
          decode_state->head.origin_d > loop_end_origin_d)
    ) {
        zdj_decode_init_addr( &decode_state->head );
        // Hard-set the decode head.
        if( decode_state->first_layer ) {
            // If there's a layer present, use the current layers' transport coords to move the head
            if( req_offset > 0 ) {
                decode_state->addr_for_origin_d_coord_in_layer( decode_node, decode_state->first_layer, &decode_state->head, loop_start_origin_d + 1 );
            } else {
                decode_state->addr_for_origin_d_coord_in_layer( decode_node, decode_state->last_layer, &decode_state->head, loop_end_origin_d - 1000 );
            }
        } else {
            // If there's no layer present, create a new one based on loop state
            zdj_decode_addr_t * loop_start_addr = calloc( 1, sizeof( zdj_decode_addr_t ) );
            zdj_decode_init_addr( loop_start_addr );
            if( req_offset > 0 ) {
                decode_state->head.origin_d = loop_start_origin_d + 1;
                decode_state->head.origin_i = (int64_t)decode_state->head.origin_d;
            } else {
                decode_state->head.origin_d = loop_end_origin_d - 1000;
                decode_state->head.origin_i = (int64_t)decode_state->head.origin_d;
            }
            decode_state->head.transport_d = 0.0;
            decode_state->head.transport_i = 0;
            decode_state->head.has_valid_origin = true;

            loop_start_addr->origin_d = loop_start_origin_d;
            loop_start_addr->origin_i = (int64_t)loop_start_addr->origin_d;
            loop_start_addr->transport_d = decode_state->head.transport_d - loop_start_origin_d;
            loop_start_addr->transport_i = (int64_t)loop_start_addr->transport_d;

            zdj_decode_layer_t * new_layer = zdj_new_decode_loop_layer( decode_node, loop_start_addr, &deck->controls.loop_state );
            decode_state->append_layer( decode_node, new_layer );

            free( loop_start_addr );
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
    

    ///////////////////////////////////////////////////////////////////
    // Phase 3 - If we're playing quantized, and loop move will move //
    //  head, quantize current loop layer's end to nearest beatgrid. //
    ///////////////////////////////////////////////////////////////////
    } else if ( deck->controls.discon_quantize &&
        deck_state->song->performance &&
        deck_state->song->performance->has_beat_grid &&
        deck->controls.platter.motor.enabled && 
        deck->controls.platter.motor.instant_rate > zdj_eps &&
        (decode_state->head.origin_d < loop_start_origin_d || // <-- head will move
         decode_state->head.origin_d > loop_end_origin_d)
    ) {
        // Use transport coords to grab the current layer under head to ensure uniqueness.
        zdj_decode_layer_t * layer_under_head = decode_state->get_layer_containing_core_addr( 
            deck_state->decode_node,
            &decode_state->head,
            ZDJ_ADDR_COORD_TRANSPORT
        );
        // Blow away the rest of the layer stack to force re-fill w/new coords
        decode_state->remove_all_layers_except( decode_node, layer_under_head );
        // Cheat the head origin coord forward a little to give
        // the playback system a few buffers to get things ready
        double cheat_bg = zdj_signal_beatgrid_count_for_pcm_count(
            ZDJ_SOUNDCARD_BUF_LEN*2,
            decode_state->song->audio->av_sample_rate,
            decode_state->song->performance->bpm
        );
        // Get origin coord for next quantized tick
        double head_origin_bg = decode_state->head.origin_bg + cheat_bg;
        // Quantize head to previous BG coord
        double head_quant = ceil( head_origin_bg / deck->controls.discon_quantize_val ) * deck->controls.discon_quantize_val;
        double quant_offset = zdj_signal_pcm_count_for_beatgrid_count(
            head_quant - head_origin_bg,
            decode_state->song->performance->bpm,
            decode_state->song->audio->av_sample_rate
        );
        double depart_origin_d = decode_state->head.origin_d + quant_offset;
        // Re-truncate
        layer_under_head->retruncate_loop( 
            layer_under_head, 
            decode_node,
            layer_under_head->core_start.origin_d,
            depart_origin_d
        );


    /////////////////////////////////////////////////////////////////////
    // Phase 4 - If we're playing unquantized, and loop move will move //
    //  head, cut current loop layer's end to something soon-ish.      //
    /////////////////////////////////////////////////////////////////////
    } else if ( !deck->controls.discon_quantize &&
        deck->controls.platter.motor.enabled && 
        deck->controls.platter.motor.instant_rate > zdj_eps &&
        (decode_state->head.origin_d < loop_start_origin_d || // <-- head will move
         decode_state->head.origin_d > loop_end_origin_d)
    ) {
        // Use transport coords to grab the current layer under head to ensure uniqueness.
        zdj_decode_layer_t * layer_under_head = decode_state->get_layer_containing_core_addr( 
            deck_state->decode_node,
            &decode_state->head,
            ZDJ_ADDR_COORD_TRANSPORT
        );
        // Blow away the rest of the layer stack to force re-fill w/new coords
        decode_state->remove_all_layers_except( decode_node, layer_under_head );
        double depart_origin_d = decode_state->head.origin_d + 3000;
        // Re-truncate
        layer_under_head->retruncate_loop( 
            layer_under_head, 
            decode_node,
            layer_under_head->core_start.origin_d,
            depart_origin_d
        );

    //////////////////////////////////////////////////
    // Phase 5 - Loop remains outside current head, //     
    //  calculate params and re-truncate.           //
    //////////////////////////////////////////////////
    } else {
        // Use transport coords to grab the current layer under head to ensure uniqueness.
        zdj_decode_layer_t * layer_under_head = decode_state->get_layer_containing_core_addr( 
            deck_state->decode_node,
            &decode_state->head,
            ZDJ_ADDR_COORD_TRANSPORT
        );
        // Blow away the rest of the layer stack to force re-fill w/new coords
        decode_state->remove_all_layers_except( decode_node, layer_under_head );
        // Re-truncate
        layer_under_head->retruncate_loop( 
            layer_under_head, 
            decode_node, 
            loop_start_origin_d, 
            loop_end_origin_d
        );
    }

    ////////////////////////////////////////////////////
    // Phase 5 - Update loop_state and re-fill layers //
    ////////////////////////////////////////////////////
    deck->controls.loop_state.start_origin_d = loop_start_origin_d;
    deck->controls.loop_state.end_origin_d = loop_end_origin_d;
    // deck->controls.loop_state.is_enabled = true;
    deck->controls.loop_state.phase = ZDJ_DECK_LOOP_PHASE_RUN;
    deck_state->decode_node->move_window( deck_state->decode_node, 0 );
}

static void _resize_loop( zdj_deck_t * deck ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_pipeline_node_t * decode_node = deck_state->decode_node;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;

    double req_len = deck->controls.loop_state.length_change_req_len;
    deck->controls.loop_state.length_change_req_len = 0;
    double loop_start_origin_d = deck->controls.loop_state.start_origin_d;
    double loop_end_origin_d = deck->controls.loop_state.end_origin_d;
    double loop_pcm_len = deck->controls.loop_state.pcm_len;
    double loop_bg_len = deck->controls.loop_state.beatgrid_len;
    double song_pcm_len = decode_state->song_pcm_duration;

    ///////////////////////////////////////////////////////////////////////
    // Phase 1 - Constrain the loop length change within song PCM coords //
    ///////////////////////////////////////////////////////////////////////

    // Quantized:
    if( deck->controls.discon_quantize &&
        deck_state->song->performance &&
        deck_state->song->performance->has_beat_grid 
    ) {
        
        double bpm = deck_state->song->performance->bpm;
        double rate = deck_state->song->audio->av_sample_rate;

        if( req_len > 0 ) {
            // When growing a loop, make sure we don't grow outside song's PCM coords
            if( fabs( loop_bg_len - 0.25 ) < zdj_eps ) {
                double p_len = zdj_signal_pcm_count_for_beatgrid_count( 0.5, bpm, rate );
                if( (loop_start_origin_d + p_len) < song_pcm_len ) { loop_bg_len = 0.5; }
            } else if( fabs( loop_bg_len - 0.5 ) < zdj_eps ) {
                double p_len = zdj_signal_pcm_count_for_beatgrid_count( 1.0, bpm, rate );
                if( (loop_start_origin_d + p_len) < song_pcm_len ) { loop_bg_len = 1.0; }
            } else if( fabs( loop_bg_len - 1.0 ) < zdj_eps ) {
                double p_len = zdj_signal_pcm_count_for_beatgrid_count( 4.0, bpm, rate );
                if( (loop_start_origin_d + p_len) < song_pcm_len ) { loop_bg_len = 4.0; }
            } else if( fabs( loop_bg_len - 4.0 ) < zdj_eps ) {
                double p_len = zdj_signal_pcm_count_for_beatgrid_count( 8.0, bpm, rate );
                if( (loop_start_origin_d + p_len) < song_pcm_len ) { loop_bg_len = 8.0; }
            } else if( fabs( loop_bg_len - 8.0 ) < zdj_eps ) {
                double p_len = zdj_signal_pcm_count_for_beatgrid_count( 16.0, bpm, rate );
                if( (loop_start_origin_d + p_len) < song_pcm_len ) { loop_bg_len = 16.0; }
            } else if( fabs( loop_bg_len - 16.0 ) < zdj_eps ) {
                double p_len = zdj_signal_pcm_count_for_beatgrid_count( 32.0, bpm, rate );
                if( (loop_start_origin_d + p_len) < song_pcm_len ) { loop_bg_len = 32.0; }
            } else if( fabs( loop_bg_len - 32.0 ) < zdj_eps ) {
                double p_len = zdj_signal_pcm_count_for_beatgrid_count( 64.0, bpm, rate );
                if( (loop_start_origin_d + p_len) < song_pcm_len ) { loop_bg_len = 64.0; }
            }
        } else {
            // Shrink the loop
            if( fabs( loop_bg_len - 0.5 ) < zdj_eps ) { loop_bg_len = 0.25; }
            else if( fabs( loop_bg_len - 1.0 ) < zdj_eps ) { loop_bg_len = 0.5; }
            else if( fabs( loop_bg_len - 4.0 ) < zdj_eps ) { loop_bg_len = 1.0; }
            else if( fabs( loop_bg_len - 8.0 ) < zdj_eps ) { loop_bg_len = 4.0; }
            else if( fabs( loop_bg_len - 16.0 ) < zdj_eps ) { loop_bg_len = 8.0; }
            else if( fabs( loop_bg_len - 32.0 ) < zdj_eps ) { loop_bg_len = 16.0; } 
            else if( fabs( loop_bg_len - 64.0 ) < zdj_eps ) { loop_bg_len = 32.0; }

        }

        // Convert the constrained BG length to PCM
        loop_pcm_len = zdj_signal_pcm_count_for_beatgrid_count( loop_bg_len, bpm, rate );

        // printf( "quatized resize pcm:%1.0f bg: %1.0f\n", loop_pcm_len, loop_bg_len );

    // Unquantized:
    } else {
        if( req_len > 0 ) {
            // When growing loop: constrain to song end
            if( (loop_end_origin_d + req_len) > (song_pcm_len - 1.0) ) {
                req_len = song_pcm_len - (loop_end_origin_d + req_len);
            }

        } else if( loop_pcm_len + req_len < 1000 ) {
            // When shrinking, constrain to minimum loop length
            req_len = 1000 - loop_pcm_len;
        }

        // Commit the constrained loop len
        loop_pcm_len += req_len;
    }

    // Commit the new loop end coord
    // loop_end_origin_d += req_len;
    loop_end_origin_d = loop_start_origin_d + loop_pcm_len;

    ////////////////////////////////////////////////////
    // Phase 2 - If we're not playing and loop shrink //
    //  needs to move the head, perform needldrop.    //
    ////////////////////////////////////////////////////
    if ( !deck->controls.platter.motor.enabled && 
         deck->controls.platter.motor.instant_rate < zdj_eps && 
         loop_end_origin_d < decode_state->head.origin_d // <-- head is after new loop end
    ) {
        printf( "shrink move\n" );
        zdj_decode_init_addr( &decode_state->head );
        // Hard-set the decode head.
        if( decode_state->first_layer ) {
            // If there's a layer present, use the current layers' transport coords to move the head
            if( loop_state->length_change_req_len > 0 ) {
                decode_state->addr_for_origin_d_coord_in_layer( decode_node, decode_state->first_layer, &decode_state->head, loop_start_origin_d + 1 );
            } else {
                decode_state->addr_for_origin_d_coord_in_layer( decode_node, decode_state->last_layer, &decode_state->head, loop_end_origin_d - 1000 );
            }
        } else {
            // If there's no layer present, create a new one based on loop state
            zdj_decode_addr_t * loop_start_addr = calloc( 1, sizeof( zdj_decode_addr_t ) );
            zdj_decode_init_addr( loop_start_addr );
            if( loop_state->length_change_req_len > 0 ) {
                decode_state->head.origin_d = loop_start_origin_d + 1;
                decode_state->head.origin_i = (int64_t)decode_state->head.origin_d;
            } else {
                decode_state->head.origin_d = loop_end_origin_d - 1000;
                decode_state->head.origin_i = (int64_t)decode_state->head.origin_d;
            }
            decode_state->head.transport_d = 0.0;
            decode_state->head.transport_i = 0;
            decode_state->head.has_valid_origin = true;

            loop_start_addr->origin_d = loop_start_origin_d;
            loop_start_addr->origin_i = (int64_t)loop_start_addr->origin_d;
            loop_start_addr->transport_d = decode_state->head.transport_d - loop_start_origin_d;
            loop_start_addr->transport_i = (int64_t)loop_start_addr->transport_d;

            zdj_decode_layer_t * new_layer = zdj_new_decode_loop_layer( decode_node, loop_start_addr, &deck->controls.loop_state );
            decode_state->append_layer( decode_node, new_layer );

            free( loop_start_addr );
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

    ///////////////////////////////////////////////////////////////////////////
    // Phase 4 - If we're playing quantized, and loop shrinks before current //
    //  head, quantize current loop layer's end to nearest beatgrid.         //
    ///////////////////////////////////////////////////////////////////////////
    } else if ( deck->controls.discon_quantize &&
        deck_state->song->performance &&
        deck_state->song->performance->has_beat_grid &&
        deck->controls.platter.motor.enabled && 
        deck->controls.platter.motor.instant_rate > zdj_eps &&
        loop_end_origin_d < decode_state->head.origin_d // <-- head is after new loop end
    ) {
        // Use transport coords to grab the current layer under head to ensure uniqueness.
        zdj_decode_layer_t * layer_under_head = decode_state->get_layer_containing_core_addr( 
            deck_state->decode_node,
            &decode_state->head,
            ZDJ_ADDR_COORD_TRANSPORT
        );
        if( layer_under_head ) {
            // Blow away the rest of the layer stack to force re-fill w/new coords
            decode_state->remove_all_layers_except( decode_node, layer_under_head );
            // Cheat the head origin coord forward a little to give
            // the playback system a few buffers to get things ready
            double cheat_bg = zdj_signal_beatgrid_count_for_pcm_count(
                ZDJ_SOUNDCARD_BUF_LEN*2,
                decode_state->song->audio->av_sample_rate,
                decode_state->song->performance->bpm
            );
            // Get origin coord for next quantized tick
            double head_origin_bg = decode_state->head.origin_bg + cheat_bg;
            // Quantize head to previous BG coord
            double head_quant = ceil( head_origin_bg / deck->controls.discon_quantize_val ) * deck->controls.discon_quantize_val;
            double quant_offset = zdj_signal_pcm_count_for_beatgrid_count(
                head_quant - head_origin_bg,
                decode_state->song->performance->bpm,
                decode_state->song->audio->av_sample_rate
            );
            double depart_origin_d = decode_state->head.origin_d + quant_offset;
            // Re-truncate
            layer_under_head->retruncate_loop( 
                layer_under_head, 
                decode_node,
                layer_under_head->core_start.origin_d,
                depart_origin_d
            );
        }

    /////////////////////////////////////////////////////////////////////////////
    // Phase 5 - If we're playing unquantized, and loop shrinks before current //
    //  head, cut current loop layer's end to something soon-ish.              //
    /////////////////////////////////////////////////////////////////////////////
    } else if ( !deck->controls.discon_quantize &&
        deck->controls.platter.motor.enabled && 
        deck->controls.platter.motor.instant_rate > zdj_eps &&
        loop_end_origin_d < decode_state->head.origin_d // <-- head is after new loop end
    ) {
        // Use transport coords to grab the current layer under head to ensure uniqueness.
        zdj_decode_layer_t * layer_under_head = decode_state->get_layer_containing_core_addr( 
            deck_state->decode_node,
            &decode_state->head,
            ZDJ_ADDR_COORD_TRANSPORT
        );
        if( layer_under_head ) {
            // Blow away the rest of the layer stack to force re-fill w/new coords
            decode_state->remove_all_layers_except( decode_node, layer_under_head );
            double depart_origin_d = decode_state->head.origin_d + 3000;
            // Re-truncate
            layer_under_head->retruncate_loop( 
                layer_under_head, 
                decode_node,
                layer_under_head->core_start.origin_d,
                depart_origin_d
            );
        }

    /////////////////////////////////////////////////
    // Phase 6 - Loop remains beyond current head, //     
    //  calculate params and re-truncate.          //
    /////////////////////////////////////////////////
    } else {
        // Use transport coords to grab the current layer under head to ensure uniqueness.
        zdj_decode_layer_t * layer_under_head = decode_state->get_layer_containing_core_addr( 
            deck_state->decode_node,
            &decode_state->head,
            ZDJ_ADDR_COORD_TRANSPORT
        );
        printf( "luh: %p\n", layer_under_head );
        if( layer_under_head ) {
            // Blow away the rest of the layer stack to force re-fill w/new coords
            decode_state->remove_all_layers_except( decode_node, layer_under_head );
            // Re-truncate
            layer_under_head->retruncate_loop( 
                layer_under_head, 
                decode_node, 
                loop_start_origin_d, 
                loop_end_origin_d
            );
        }
    }

    ////////////////////////////////////////////////////
    // Phase 5 - Update loop_state and re-fill layers //
    ////////////////////////////////////////////////////
    deck->controls.loop_state.start_origin_d = loop_start_origin_d;
    deck->controls.loop_state.end_origin_d = loop_end_origin_d;
    deck->controls.loop_state.pcm_len = loop_pcm_len;
    deck->controls.loop_state.beatgrid_len = loop_bg_len;
    // deck->controls.loop_state.is_enabled = true;
    deck->controls.loop_state.phase = ZDJ_DECK_LOOP_PHASE_RUN;
    deck_state->decode_node->move_window( deck_state->decode_node, 0 );

    // printf( "resize: s:%1.0f e:%1.0f pcm:%1.0f bg:%1.0f\n",
    //     deck->controls.loop_state.start_origin_d,
    //     deck->controls.loop_state.end_origin_d,
    //     deck->controls.loop_state.pcm_len,
    //     deck->controls.loop_state.beatgrid_len
    // );

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
    
    // printf( "stage_needledrop: %1.3f -> %1.3f\n", decode_state->head.origin_d, needledrop_state->origin_coord );

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
    // printf( "update_needledrop\n" );
    zdj_deck_control_needledrop_state_t * needledrop_state = &deck->controls.needledrop_state;
    // if( needledrop_state->counter++ > 3 ) { // Debounce the needledrop for a few DAC cycles
        needledrop_state->phase = ZDJ_DECK_NEEDLEDROP_PHASE_INACTIVE;
        // // Do an inert decode window move to force layer refill
        // zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
        // zdj_pipeline_node_t * decode_node = deck_state->decode_node;
        // decode_node->move_window( decode_node, 0 ); 
    // }
}
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


// static void zdj_dj_deck_reset_tsm_nodes( zdj_deck_t * deck );

// bool zdj_dj_deck_loop_is_enabled( zdj_deck_t * deck ) {
//     return deck->controls.loop_state.is_enabled;
// }

//////////////////////////////////////////////
// Internal API                             //
// Only accessible on fast soundcard thread //
//////////////////////////////////////////////

void zdj_dj_deck_new_loop( zdj_deck_t * deck ) {
    printf( "_new_loop\n" );
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_state_t * controls = &deck->controls;

    // bug out if we're outside song's pcm coords
    if( decode_state->head.origin_d < 0.0 || 
        decode_state->head.origin_d > decode_state->song_pcm_duration 
    ) { return; }

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
        double head_quant_bg = floor( head_origin_bg / controls->discon_quantize_val ) * controls->discon_quantize_val;
        double head_targ_d = zdj_signal_pcm_count_for_beatgrid_count( head_quant_bg, bpm, rate );
        loop_start_origin_d = fmax( 0.0, head_targ_d );

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


    ////////////////////////////////////////////////////////
    // Phase 2 - Build + request new loop in decode node. //
    ////////////////////////////////////////////////////////
    zdj_decode_discon_request_t req;
    memset( &req, 0, sizeof( zdj_decode_discon_request_t ) );
    req.type = ZDJ_DECODE_DISCON_REQUEST_NEW_LOOP;
    req.loop_start_origin_d = loop_start_origin_d;
    req.loop_end_origin_d = loop_end_origin_d;

    decode_state->request_discon( deck_state->decode_node, &req );
}

// Enable and jump to a pre-defined loop
// Not currently implemented
void zdj_dj_deck_enable_loop( zdj_deck_t * deck, zdj_library_cuepoint_t * cuepoint ) {
    
}

void zdj_dj_deck_disable_loop( zdj_deck_t * deck ) {
    printf( "disable loop\n" );
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_state_t * controls = &deck->controls;
    deck->controls.loop_state.is_enabled = false;

    ////////////////////////////////////////////////////////
    // Ask decode node to diasble the current loop discon //
    ////////////////////////////////////////////////////////
    decode_state->remove_discon( deck_state->decode_node );
}

void zdj_dj_deck_move_loop( zdj_deck_t * deck, double val ) {
    // printf( "move loop: %1.1f\n", val );
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_pipeline_node_t * decode_node = deck_state->decode_node;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;  
    zdj_deck_control_skip_state_t * skip_state = &deck->controls.skip_state;  
    zdj_tsm_tempo_node_state_t * tsm_tempo_state = (zdj_tsm_tempo_node_state_t*)deck_state->tsm_tempo_node->state;
    zdj_tsm_pitch_node_state_t * tsm_pitch_state = (zdj_tsm_pitch_node_state_t*)deck_state->tsm_pitch_node->state;  

    double req_offset = 0;

    if( zdj_dj_deck_quantize_commands( deck ) && zdj_dj_deck_has_beatgrid( deck ) ){
        double bg_offset = deck->controls.discon_quantize_val * val;
        req_offset = decode_state->get_d_offset_for_beatgrid_dist( deck_state->decode_node, bg_offset );
    } else {
        req_offset = val * 7000;
    }

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

    deck->controls.loop_state.start_origin_d = loop_start_origin_d;
    deck->controls.loop_state.end_origin_d = loop_end_origin_d;
    deck->controls.loop_state.pcm_len = loop_end_origin_d - loop_start_origin_d;

    zdj_decode_discon_request_t req;
    memset( &req, 0, sizeof( zdj_decode_discon_request_t ) );

    //////////////////////////////////////////////////
    // Phase 2 - If we're not playing and loop move //
    //  needs to move the head, request reset.      //
    //////////////////////////////////////////////////
    if ( !zdj_dj_deck_platter_is_playing( deck ) &&
         !zdj_signal_bounds_check( 
            decode_state->head.origin_d, 
            loop_start_origin_d,
            loop_end_origin_d 
          )
    ) {
        // We're going to reset the decode node, 
        // so just figure out new head origin coords.
        req.type = ZDJ_DECODE_DISCON_REQUEST_RESET;
        req.reset_coord_subspace = ZDJ_ADDR_SUBSPACE_D;
        if( decode_state->head.origin_d < loop_start_origin_d ) {
            req.reset_head_origin_d = loop_start_origin_d + 10;
        } else if( decode_state->head.origin_d > loop_end_origin_d ) {
            req.reset_head_origin_d = loop_end_origin_d - 10;
        }
        req.loop_coord_subspace = ZDJ_ADDR_SUBSPACE_D;
        req.loop_start_origin_d = loop_start_origin_d;
        req.loop_end_origin_d = loop_end_origin_d;
        req.enable_loop = true;
        decode_state->request_discon( deck_state->decode_node, &req );

        // Reset TSM nodes
        zdj_dj_deck_reset_tsm_nodes( deck );


    ///////////////////////////////////////////////////////////////
    // Phase 3 - If we're playing, and loop move will move head, //
    // quantize current loop layer's end to nearest beatgrid.    //
    ///////////////////////////////////////////////////////////////
    } else if ( zdj_dj_deck_platter_is_playing( deck ) &&
                !zdj_signal_bounds_check( 
                    decode_state->head.origin_d, 
                    loop_start_origin_d,
                    loop_end_origin_d 
                )
    ) {
        req.type = ZDJ_DECODE_DISCON_REQUEST_SKIP_PLAY_TO_PLAY;
        req.loop_start_origin_d = loop_start_origin_d;
        req.loop_end_origin_d = loop_end_origin_d;
        req.enable_loop = true;
        req.loop_coord_subspace = ZDJ_ADDR_SUBSPACE_D;

        // First figure out the departure coord.
        // Since the platter is moving in this case, 
        // will need to be in the future.
        // Observe command quantize behavior.
        if( zdj_dj_deck_quantize_commands( deck ) && zdj_dj_deck_has_beatgrid( deck ) ){
            // Quantize departure to next BG tick after head
            req.skip_depart_origin_d = zdj_decode_guantize_origin_d_for_beatgrid(
                decode_state->head.origin_d + 300,
                deck->controls.discon_quantize_val,
                decode_state->song,
                ZDJ_DECODE_QUANTIZE_CEIL
            );
        } else {
            // Set departure to head + xxx
            req.skip_depart_origin_d = decode_state->head.origin_d + 500;
        }

        // Set destination to start of loop.
        req.skip_dest_origin_d = loop_start_origin_d;
        req.skip_coord_subspace = ZDJ_ADDR_SUBSPACE_D;

        // Update the skip_state so refresh_layers knows where to start the next layer.
        skip_state->dest_origin_d = req.skip_dest_origin_d;

        decode_state->request_discon( deck_state->decode_node, &req );


    //////////////////////////////////////////////////
    // Phase 4 - Loop remains outside current head, //     
    //  calculate params and re-truncate.           //
    //////////////////////////////////////////////////
    } else {
        // Request a simple loop edit
        req.type = ZDJ_DECODE_DISCON_REQUEST_EDIT_LOOP;
        req.loop_coord_subspace = ZDJ_ADDR_SUBSPACE_D;
        req.loop_start_origin_d = loop_start_origin_d;
        req.loop_end_origin_d = loop_end_origin_d;
        req.enable_loop = true;
        decode_state->request_discon( deck_state->decode_node, &req );
    }
}

void zdj_dj_deck_resize_loop( zdj_deck_t * deck, double req_len ) {
    // printf( "resize loop: %1.1f\n", req_len );
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_pipeline_node_t * decode_node = deck_state->decode_node;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_loop_state_t * loop_state = &deck->controls.loop_state;  
    zdj_deck_control_skip_state_t * skip_state = &deck->controls.skip_state;  

    double loop_start_origin_d = deck->controls.loop_state.start_origin_d;
    double loop_end_origin_d = deck->controls.loop_state.end_origin_d;
    double loop_pcm_len = deck->controls.loop_state.pcm_len;
    double loop_bg_len = deck->controls.loop_state.beatgrid_len;
    double song_pcm_len = decode_state->song_pcm_duration;


    ///////////////////////////////////////////////////////////////////////
    // Phase 1 - Constrain the loop length change within song PCM coords //
    ///////////////////////////////////////////////////////////////////////

    // Quantized:
    if( zdj_dj_deck_quantize_commands( deck ) && zdj_dj_deck_has_beatgrid( deck ) ) {
        
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
        req_len *= 500;
        
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
    loop_end_origin_d = loop_start_origin_d + loop_pcm_len;

    deck->controls.loop_state.start_origin_d = loop_start_origin_d;
    deck->controls.loop_state.end_origin_d = loop_end_origin_d;
    deck->controls.loop_state.pcm_len = loop_pcm_len;
    deck->controls.loop_state.beatgrid_len = loop_bg_len;

    // If loop is not enabled, we're done
    if( !zdj_dj_deck_loop_is_enabled( deck ) ) { return; }

    zdj_decode_discon_request_t req;
    memset( &req, 0, sizeof( zdj_decode_discon_request_t ) );

    ////////////////////////////////////////////////////
    // Phase 2 - If we're not playing and loop shrink //
    //  needs to move the head, reset to end of loop. //
    ////////////////////////////////////////////////////
    if ( !zdj_dj_deck_platter_is_playing( deck ) &&
         !zdj_signal_bounds_check( 
            decode_state->head.origin_d, 
            loop_start_origin_d,
            loop_end_origin_d 
          )
    ) {
        // printf( "shrink move\n" );
        req.type = ZDJ_DECODE_DISCON_REQUEST_RESET;
        req.reset_coord_subspace = ZDJ_ADDR_SUBSPACE_D;
        if( decode_state->head.origin_d < loop_start_origin_d ) {
            // This should never happen
            req.reset_head_origin_d = loop_start_origin_d + 10;
        } else if( decode_state->head.origin_d > loop_end_origin_d ) {
            req.reset_head_origin_d = loop_end_origin_d - 10;
        }
        req.loop_coord_subspace = ZDJ_ADDR_SUBSPACE_D;
        req.loop_start_origin_d = loop_start_origin_d;
        req.loop_end_origin_d = loop_end_origin_d;
        req.enable_loop = true;
        decode_state->request_discon( deck_state->decode_node, &req );


        // Reset TSM nodes
        zdj_dj_deck_reset_tsm_nodes( deck );

        // TODO: Reset anti-pop

    ///////////////////////////////////////////////////////////////////
    // Phase 3 - If we're playing, and loop shrinks before current   //
    //  head, quantize current loop layer's end to nearest beatgrid. //
    ///////////////////////////////////////////////////////////////////
    } else if ( zdj_dj_deck_platter_is_playing( deck ) &&
                zdj_dj_deck_quantize_commands( deck ) && 
                zdj_dj_deck_has_beatgrid( deck ) &&
                !zdj_signal_bounds_check( 
                    decode_state->head.origin_d, 
                    loop_start_origin_d,
                    loop_end_origin_d 
                )
    ) {
        req.type = ZDJ_DECODE_DISCON_REQUEST_SKIP_PLAY_TO_PLAY;
        req.loop_start_origin_d = loop_start_origin_d;
        req.loop_end_origin_d = loop_end_origin_d;
        req.loop_coord_subspace = ZDJ_ADDR_SUBSPACE_D;
        req.enable_loop = true;

        // First figure out the departure coord.
        // Since the platter is moving in this case, 
        // will need to be in the future.
        // Observe command quantize behavior.
        if( zdj_dj_deck_quantize_commands( deck ) && zdj_dj_deck_has_beatgrid( deck ) ){
            // Quantize departure to next BG tick after head
            req.skip_depart_origin_d = zdj_decode_guantize_origin_d_for_beatgrid(
                decode_state->head.origin_d,
                deck->controls.discon_quantize_val,
                decode_state->song,
                ZDJ_DECODE_QUANTIZE_CEIL
            );
        } else {
            // Set departure to head + xxx
            req.skip_depart_origin_d = decode_state->head.origin_d + 500;
        }

        // Set destination to start of loop.
        req.skip_dest_origin_d = loop_start_origin_d;
        req.skip_coord_subspace = ZDJ_ADDR_SUBSPACE_D;

        // Update the skip_state so refresh_layers knows where to start the next layer.
        skip_state->dest_origin_d = req.skip_dest_origin_d;


        decode_state->request_discon( deck_state->decode_node, &req );


    /////////////////////////////////////////////////
    // Phase 4 - Loop remains beyond current head, //     
    //  calculate params and re-truncate.          //
    /////////////////////////////////////////////////
    } else {
        // Request a simple loop edit
        req.type = ZDJ_DECODE_DISCON_REQUEST_EDIT_LOOP;
        req.loop_coord_subspace = ZDJ_ADDR_SUBSPACE_D;
        req.loop_start_origin_d = loop_start_origin_d;
        req.loop_end_origin_d = loop_end_origin_d;
        req.enable_loop = true;
        decode_state->request_discon( deck_state->decode_node, &req );
    }
}


void zdj_dj_deck_reset_to_loop_start( zdj_deck_t * deck ) {
    printf( "Loop Reset to Start\n" );
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_pipeline_node_t * decode_node = deck_state->decode_node;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
    zdj_deck_control_loop_state_t * loop_state = (zdj_deck_control_loop_state_t*)&deck->controls.loop_state;
    zdj_deck_control_skip_state_t * skip_state = (zdj_deck_control_skip_state_t*)&deck->controls.skip_state;

    zdj_decode_discon_request_t req;
    memset( &req, 0, sizeof( zdj_decode_discon_request_t ) );


    /////////////////////////////////////////////////////
    // If deck is not playing, just do a simple reset. //
    /////////////////////////////////////////////////////
    if( !zdj_dj_deck_platter_is_playing( deck ) ) {
        req.type = ZDJ_DECODE_DISCON_REQUEST_RESET;
        req.reset_coord_subspace = ZDJ_ADDR_SUBSPACE_D;
        req.reset_head_origin_d = deck->controls.loop_state.start_origin_d;

        req.loop_coord_subspace = ZDJ_ADDR_SUBSPACE_D;
        req.loop_start_origin_d = deck->controls.loop_state.start_origin_d;
        req.loop_end_origin_d = deck->controls.loop_state.end_origin_d;
        req.enable_loop = true;
        decode_state->request_discon( deck_state->decode_node, &req );

        // Reset TSM nodes
        zdj_dj_deck_reset_tsm_nodes( deck );

    /////////////////////////////////////////////
    // If deck is playing, do a play_to_reset. //
    /////////////////////////////////////////////
    } else if ( zdj_dj_deck_platter_is_playing( deck ) ) {
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
        if( depart_origin_d < 0.0 || depart_origin_d > decode_state->song_pcm_duration ) { return; }

        ///////////////////////////////////////////////////
        // Phase 2 - Build the destination origin coord. //
        ///////////////////////////////////////////////////
        double dest_origin_d = loop_state->start_origin_d;

        //////////////////////////////////
        // Phase 3 - Build the requesst //
        //////////////////////////////////
        zdj_decode_discon_request_t req;
        memset( &req, 0, sizeof( zdj_decode_discon_request_t ) );
        req.type = ZDJ_DECODE_DISCON_REQUEST_SKIP_PLAY_TO_PLAY;
        req.skip_depart_origin_d = depart_origin_d;
        req.skip_dest_origin_d = dest_origin_d;
        req.skip_coord_subspace = ZDJ_ADDR_SUBSPACE_D;
        req.enable_loop = true;

        // Update the skip_state so refresh_layers knows where to start the next layer.
        // Lock the skip_state until head has reached discon and processed new layers.
        skip_state->current_offset = 0.0;
        skip_state->dest_origin_d = dest_origin_d;
        skip_state->locked = true;

        decode_state->request_discon( deck_state->decode_node, &req );
    } 
}
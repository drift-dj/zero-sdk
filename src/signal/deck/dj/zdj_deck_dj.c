#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <math.h>
#include <sys/syscall.h>

#include <zerodj/controls/zdj_controls.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/zdj_deck_manager.h>
#include <zerodj/signal/deck/dj/zdj_deck_dj.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/signal/pipeline/node/audio/tsm/zdj_tsm_pitch_node.h>
#include <zerodj/signal/pipeline/node/audio/tsm/zdj_tsm_tempo_node.h>

// External data/event entry points
static void _update_state ( zdj_deck_t * deck );
static void _begin_teardown( zdj_deck_t * deck );
static void _deinit( zdj_deck_t * deck );
static void _get_edge_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo );

// Pipeline update thread
static void * _pipeline_thread_main( void * arg );

zdj_error_type_t zdj_new_dj_deck( zdj_deck_t * deck, void * resource, int win_buf_count ) {
    // printf( "zdj_new_dj_deck\n" );
    
    zdj_dj_deck_state_t * state = calloc( 1, sizeof( zdj_dj_deck_state_t ) );
    deck->state = state;
    state->song = (zdj_library_song_t *)resource;
    state->decode_win_buf_count = win_buf_count;
    state->tsm_source = ZDJ_DECK_TSM_SOURCE_PITCH;
    
    state->tempo_tsm_enabled = true;
    // state->tempo_tsm_enabled = false;

    // Lifecycle
    deck->update_state = &_update_state;
    deck->begin_teardown = &_begin_teardown;
    deck->deinit = &_deinit;
    
    zdj_deck_dj_init_soundcard( deck );
    zdj_deck_dj_init_platter( deck );
    zdj_deck_dj_init_command( deck );

    sem_init( &state->start_cycle, 0, 0 );

    return ZDJ_ERROR_OKAY;
}

static void _deinit( zdj_deck_t * deck ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    // Teardown decode + tsm pipeline.
    deck_state->decode_node->deinit( deck_state->decode_node );
    deck_state->tsm_pitch_node->deinit( deck_state->tsm_pitch_node );
}

static void _begin_teardown( zdj_deck_t * deck ) {
    deck->status = ZDJ_DECK_STATUS_STOP_TRANSPORT;
}

// Called from the slow deck update thread (~.25Hz).
static void _update_state ( zdj_deck_t * deck ) {
    // printf( "lib deck _update_state\n" );

    // Early exit if we're up and running.
    if( deck->status == ZDJ_DECK_STATUS_RUNNING ) { return; }
    
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;

    switch ( deck->status ) {

        case ZDJ_DECK_STATUS_NEW:
            // printf( "ZDJ_DECK_STATUS_NEW (%p)\n", deck );
            deck->status = ZDJ_DECK_STATUS_MAKE_PIPELINE;
            break;

        // Stand up pipeline nodes.
        case ZDJ_DECK_STATUS_MAKE_PIPELINE:
            // printf( "ZDJ_DECK_STATUS_MAKE_PIPELINE (%p)\n", deck );
            zdj_deck_dj_init_sync( deck );
            
            int decode_buf_count = deck_state->decode_win_buf_count; // number of decode buffers which fit in window.

            // Set audio fade-out constants - tune these to sound good
            deck->controls.platter.antipop.engage_thresh = 10.0f;
            deck->controls.platter.antipop.lowpass_val = 0.23;
            deck->controls.platter.antipop.slew_limit = 0.11;
            
            // >0.8 of full window - sound totally faded out
            deck->controls.platter.scrub_fade.fade_start_rate = (float)(ZDJ_SOUNDCARD_BUF_LEN*decode_buf_count) * 0.7f;
            deck->controls.platter.scrub_fade.fade_complete_rate = (float)(ZDJ_SOUNDCARD_BUF_LEN*decode_buf_count) * 0.9f;

            // Hyperscrub hysteresis constants:
            deck->controls.platter.hyperscrub.exit_thresh = (float)(ZDJ_SOUNDCARD_BUF_LEN*decode_buf_count) * 0.95f;
            deck->controls.platter.hyperscrub.reentry_thresh = (float)(ZDJ_SOUNDCARD_BUF_LEN*decode_buf_count) * 0.7f;

            deck_state->decode_node = zdj_new_decode_node( 
                deck_state->song, 0, ZDJ_SOUNDCARD_BUF_LEN*decode_buf_count, ZDJ_SOUNDCARD_BUF_LEN*decode_buf_count 
            );
            // Capture loop state in decode node
            zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
            decode_state->loop_state = (void*)&deck->controls.loop_state;
            decode_state->skip_state = (void*)&deck->controls.skip_state;
            
            deck_state->tsm_pitch_node = zdj_new_tsm_pitch_node( 
                deck_state->song->audio->av_channel_count,
                ZDJ_SOUNDCARD_BUF_LEN,
                deck_state->decode_node
            );
            deck_state->tsm_tempo_node = zdj_new_tsm_tempo_node( 
                deck_state->song->audio->av_channel_count,
                ZDJ_SOUNDCARD_BUF_LEN,
                deck_state->decode_node
            );

            // Add the RB instance to the tsm tempo node.
            // WARNING - this segfaults when called inside zdj_new_tsm_tempo_node().
            // This indicates some threading or memory un-good-ness.  Be fearful.
            zdj_tsm_tempo_node_state_t * tsm_tempo_state = (zdj_tsm_tempo_node_state_t*)deck_state->tsm_tempo_node->state;
            if( deck->station == ZDJ_DECK_STATION_1 ) {
                tsm_tempo_state->rb = rubberband_new(
                    44100,
                    2,
                    RubberBandOptionProcessRealTime | 
                    RubberBandOptionEngineFaster | 
                    RubberBandOptionChannelsTogether,
                    1.0f,
                    1.0f 
                );
            } else {
                tsm_tempo_state->rb = rubberband_new(
                    44100,
                    2,
                    RubberBandOptionProcessRealTime | 
                    RubberBandOptionEngineFaster | 
                    RubberBandOptionChannelsTogether,
                    1.0f,
                    1.0f 
                );
            }
            // rubberband_set_debug_level( tsm_tempo_state->rb, 3 );
            rubberband_set_max_process_size( tsm_tempo_state->rb, ZDJ_SOUNDCARD_BUF_LEN / 2 );

            zdj_decode_addr_t win_start;
            decode_state->get_win_start_addr( deck_state->decode_node, &win_start );

            deck->predelay_counter = 0;

            deck->status = ZDJ_DECK_STATUS_WAIT_THREAD_AVAIL;
            break;
            
        // Make sure station 1 doesn't already have a thread running.
        // If it does, assume it's in the process of exiting,
        // and keep polling here until it becomes available.
        case ZDJ_DECK_STATUS_WAIT_THREAD_AVAIL:
            // printf( "ZDJ_DECK_STATUS_WAIT_THREAD_AVAIL (%p)\n", deck );
            // if( deck->predelay_counter++ < 5 ) { break; }
            if( deck->station == ZDJ_DECK_STATION_1 &&
                zdj_thread_deck_1_station_available 
            ) {
                zdj_thread_launch_deck_station_1_cycle( _pipeline_thread_main, deck );
                deck->status = ZDJ_DECK_STATUS_WAIT_THREAD_READY;
            } else if( deck->station == ZDJ_DECK_STATION_2 &&
                zdj_thread_deck_2_station_available 
            ) {
                zdj_thread_launch_deck_station_2_cycle( _pipeline_thread_main, deck );
                deck->status = ZDJ_DECK_STATUS_WAIT_THREAD_READY;
            } else if( deck->station == ZDJ_DECK_STATION_EXT &&
                zdj_thread_deck_ext_station_available 
            ) {
                zdj_thread_launch_deck_station_ext_cycle( _pipeline_thread_main, deck );
                deck->status = ZDJ_DECK_STATUS_WAIT_THREAD_READY;
            }
            break;

        // Wait for new deck thread to finish filling its buffers.
        case ZDJ_DECK_STATUS_WAIT_THREAD_READY:
            // printf( "DJ ZDJ_DECK_STATUS_WAIT_THREAD_READY (%p)\n", deck );
            if( deck_state->thread_ready ) {              
                // Start serving deck samples to soundcard.
                zdj_soundcard_link_deck( zdj_soundcard, deck );
                // Mute the headphone cue channel
                if( deck->station == ZDJ_DECK_STATION_1 ) { 
                    zdj_soundcard_node_t * node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_CUE );
                    node->dsp_dto->mute = true;
                } else if( deck->station == ZDJ_DECK_STATION_2 ) {
                    zdj_soundcard_node_t * node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_CUE );
                    node->dsp_dto->mute = true;
                }
                // Accept control events when we're ready for playback
                zdj_deck_dj_init_controls( deck );

                // Advance state to running
                deck->status = ZDJ_DECK_STATUS_RUNNING; 

                zdj_decode_addr_t win_start;
                zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
                decode_state->get_win_start_addr( deck_state->decode_node, &win_start );

                // Update deck manager tempo
                deck->set_sync_bpm( deck, zdj_deck_manager( )->sync.set_bpm );


                // printf( "3\n" );
                // Prep loop/skip state
                if( deck_state->song->performance &&
                    deck_state->song->performance->has_beat_grid
                ) {
                    deck->controls.discon_quantize = true;
                    deck->controls.loop_state.beatgrid_len = 1.0f;
                    deck->controls.loop_state.pcm_len = decode_state->get_d_offset_for_beatgrid_dist( 
                        deck_state->decode_node, 
                        deck->controls.loop_state.beatgrid_len 
                    );
                } else {
                    deck->controls.discon_quantize = false;
                    deck->controls.loop_state.pcm_len = 44100.0;
                }
                deck->controls.discon_quantize_val = 0.125f;
                deck->controls.loop_state.fade_len = 300;
                
                deck->controls.skip_state.locked = false;
                
                if( deck_state->song->performance &&
                    deck_state->song->performance->cuepoint_count > 0
                ) {
                    deck_state->song->performance->current_cuepoint = 0;
                }

                // Call UI load CB
                if( deck->ui_load_cb ){ deck->ui_load_cb( deck ); }
            }
            // printf( "ZDJ_DECK_STATUS_WAIT_THREAD_READY DONE\n" );
            break;

        // Ignore new control events and start spooldown of deck drive/transport model.
        case ZDJ_DECK_STATUS_STOP_TRANSPORT:
            // printf( "ZDJ_DECK_STATUS_STOP_TRANSPORT (%p)\n", deck );
            // Immediately stop accepting control events.
            deck->handle_control_event = NULL;
            // Stop deck playback if running.
            if( zdj_dj_deck_platter_is_playing( deck ) ) {
                zdj_dj_deck_platter_stop_motor( deck, true );
            } else {
                zdj_dj_deck_platter_stop_motor( deck, false );
            }
            deck->status = ZDJ_DECK_STATUS_WAIT_SPOOLDOWN; 

            // Call UI unload CB
            if( deck->ui_unload_cb ){ deck->ui_unload_cb( deck ); }
            break;

        // Wait while deck transport fades out or slows to rate=0.
        // Allow this to take several audio buffer cycles.
        case ZDJ_DECK_STATUS_WAIT_SPOOLDOWN:
            // printf( "ZDJ_DECK_STATUS_WAIT_SPOOLDOWN (%p)\n", deck );
            if( deck->safe_to_deinit ) {
                // Stop sending deck samples to soundcard
                zdj_soundcard_unlink_deck( zdj_soundcard, deck );
                // Tell the deck's fast-audio pipeline thread to exit.
                // Manually post the sem since we have unlinked the deck.
                deck_state->exit_thread = true;
                sem_post( &deck_state->start_cycle );
                // Tell the deck_manager we are ready for deinit()
                deck->status = ZDJ_DECK_STATUS_IDLE;
            }
            break;
        default: break;
    }
    // printf( "lib deck _update_state done\n" );
}



///////////////////////////////////////////////
// Buffer Refill Thread                      //
// Waits for semaphore from fast audio cycle //
///////////////////////////////////////////////

// Thread for processing soundcard fast-cycle requests.
// get_edge_data posts the start_cycle semaphore below.
static void * _pipeline_thread_main( void * arg ) {
    // printf( "_pipeline_thread_main: %p\n", arg );
    zdj_deck_t * deck = (zdj_deck_t*)arg;
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;

    zdj_pipeline_node_t * decode_node = deck_state->decode_node;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;

    zdj_pipeline_node_t * tsm_pitch_node = deck_state->tsm_pitch_node;
    zdj_tsm_pitch_node_state_t * tsm_pitch_state = (zdj_tsm_pitch_node_state_t*)deck_state->tsm_pitch_node->state;
    
    zdj_pipeline_node_t * tsm_tempo_node = deck_state->tsm_tempo_node;
    zdj_tsm_tempo_node_state_t * tsm_tempo_state = (zdj_tsm_tempo_node_state_t*)deck_state->tsm_tempo_node->state;

    zdj_deck_platter_t * platter = &deck->controls.platter;
    zdj_deck_control_skip_state_t * skip_state = (zdj_deck_control_skip_state_t*)decode_state->skip_state;
    zdj_deck_control_loop_state_t * loop_state;

    // Set up scheduling
    int prio = sched_get_priority_max( SCHED_FIFO );
	struct sched_param param;
	param.sched_priority = prio;
	sched_setscheduler( syscall(SYS_gettid), SCHED_FIFO, &param );

    // Give realtime scheduler access to 100% of core time
	system( "echo -1 >/proc/sys/kernel/sched_rt_runtime_us" );

    // Set core affinity to Core #1;
    cpu_set_t cpuset;
	CPU_ZERO( &cpuset );

    // Claim the deck station thread
    switch ( deck->station ) {
        case ZDJ_DECK_STATION_1:
            CPU_SET( 2,&cpuset );
            zdj_thread_deck_1_station_available = false;
            break;
        case ZDJ_DECK_STATION_2:
            CPU_SET( 3,&cpuset );
            zdj_thread_deck_2_station_available = false;
            break;
        case ZDJ_DECK_STATION_EXT:
            CPU_SET( 2,&cpuset );
            zdj_thread_deck_ext_station_available = false;
            break;
        default: break;
    }

    int err = sched_setaffinity( syscall(SYS_gettid), sizeof(cpu_set_t), &cpuset );
    if( err != 0 ) {
        perror( "set affinity failed" );
    }


    //////////////////////////
    // DJ Deck update cycle //
    //////////////////////////

    while( !deck_state->exit_thread ) {
        // printf( "dj deck thread\n" );

        ///////////////////////////////////////////////////////
        // Phase 1 - Update Platter model with new requests  //
        // from control input thread                         //
        ///////////////////////////////////////////////////////
        if( deck->update_platter_model ){ deck->update_platter_model( deck ); }

        // if( head_move_val != 0 ) { printf( "head move: %1.1f\n", head_move_val ); }

        /////////////////////////////////////////////////////////////
        // Phase 2 - Handle any TSM state transition requests from //
        // the preceeding platter update.                          //
        // Do this before the decode move_window to capture the    //
        // current state of the decode head.                       //
        /////////////////////////////////////////////////////////////
        if( deck_state->tsm_tx_req == ZDJ_DECK_TSM_TX_TO_TEMPO ) {
            // set the tempo node's start coord to the pitch TSM node's end coord
            deck_state->tsm_source = ZDJ_DECK_TSM_SOURCE_TEMPO;
            tsm_tempo_state->decode_coord = tsm_pitch_state->decode_end_coord;
            tsm_tempo_state->rate = platter->head_move_rate;
            zdj_reset_tsm_tempo_node( tsm_tempo_node );
        } else if( deck_state->tsm_tx_req == ZDJ_DECK_TSM_TX_TO_PITCH ) {
            // set the pitch node's start coord to the pre-move head
            deck_state->tsm_source = ZDJ_DECK_TSM_SOURCE_PITCH;
            tsm_pitch_state->decode_start_coord = decode_state->head.transport_d;
        }
        deck_state->tsm_tx_req = ZDJ_DECK_TSM_TX_NONE;

        ////////////////////////////////////////////////////////////////
        // Phase 3a - Update decode state under hyperscrub conditions //
        ////////////////////////////////////////////////////////////////
        if( zdj_dj_deck_platter_is_hyperscrubbing( deck ) ) {
            // printf( "hyperscrub by: %1.1f\n", platter->head_move_samps );
            // Discard discon any request.
            if( deck->command_req.state != ZDJ_DECK_COMMAND_REQUEST_NONE ) {
                deck->command_req.state = ZDJ_DECK_COMMAND_REQUEST_RECEIVED;
            }
            // Hard-set the current head addr so other threads will 
            // still get a meaningful value for current playhead.
            decode_node->reset_window( 
                decode_node, 
                decode_state->head.origin_d + platter->head_move_samps 
            ); 
            // Disable the decode node's layer grooming to prevent decoding.
            // decode_state->refresh_mode = ZDJ_DECODE_REFRESH_NOOP_ON_EMPTY;
            decode_state->refresh_mode = ZDJ_DECODE_REFRESH_NOOP_HYPERSCRUB;
            // Update pitch node coords so they're valid on exit from hyperscrub
            tsm_pitch_state->decode_start_coord = decode_state->head.transport_d;
            tsm_pitch_state->decode_end_coord = decode_state->head.transport_d;
            zdj_tsm_pitch_node_clear_out_buf( tsm_pitch_node );
        
        
        ////////////////////////////////////////////////////////////////////
        // Phase 3b - Update decode state under normal decode conditions. //
        // Note the double refresh_layers call when a discon_model update //
        // happens. This is intentional - it allows us to place discons   //
        // very near the playhead, or even before it.                     //
        ////////////////////////////////////////////////////////////////////
        } else {
            bool needs_refresh = false;
            if( deck->update_command_models ){ 
                needs_refresh = deck->update_command_models( deck );
            }
            // If platter isn't hyperscrub, but node refresh mode IS still hyperscrub,
            // we've just exited hyperscrub.  Put the refresh mode into whatever
            // it should be to refill the node.
            if( decode_state->refresh_mode == ZDJ_DECODE_REFRESH_NOOP_HYPERSCRUB ) {
                if( deck->controls.loop_state.is_enabled ) {
                    printf( "exiting hyperscrub to loop\n" );
                    decode_state->refresh_mode = ZDJ_DECODE_REFRESH_DISCON_LOOP;
                } else {
                    printf( "exiting hyperscrub to contig.\n" );
                    decode_state->refresh_mode = ZDJ_DECODE_REFRESH_CONTIGUOUS;
                }
            }
            // Refresh to back-fill potential discon state change BEFORE moving the window.
            if( needs_refresh ){ decode_state->refresh_layers( decode_node ); }
            
            /////////////////////////////////////////////////////////////
            // Phase 4 - Recharge the decode node's buffers if head is //
            // moving, or if discon_model needs new decode pass.       //
            /////////////////////////////////////////////////////////////
            if( fabs( platter->head_move_samps ) > zdj_eps ) {
                decode_node->move_window( decode_node, platter->head_move_samps );
                decode_state->refresh_layers( decode_node );
                decode_node->update_wait( decode_node );
            }

            
            ///////////////////////////////////////////////////////////////
            // Phase 5 - Update the TSM Node based on the head movement. //
            ///////////////////////////////////////////////////////////////
            zdj_decode_addr_t win_start;
            zdj_decode_addr_t win_end;
            decode_state->get_win_start_addr( decode_node, &win_start );
            decode_state->get_win_end_addr( decode_node, &win_end );

            // Handle hot-reset request


            // Tempo TSM Mode - time stretching/slo-coder
            if ( zdj_dj_deck_is_in_tempo_tsm_mode( deck ) ) {
                // printf( "tsm tempo update\n" );
                tsm_tempo_state->rate = platter->head_move_rate;
                
                //////////////////////////////////////
                // Process normal Tempo TSM buffer. //
                //////////////////////////////////////
                if( !deck->controls.cue_state.reset_pending ) {
                    deck_state->tsm_tempo_node->update_wait( deck_state->tsm_tempo_node );
                
                ///////////////////////////////////////////
                // Process play_to_reset request.        //
                // Fade out from start to end of buffer. //
                // Reset deck to reqested coord after    //
                // buffer is rendered.                   //
                ///////////////////////////////////////////
                } else if( deck->controls.cue_state.reset_pending ) {
                    tsm_tempo_state->fade_out = true;
                    deck->controls.cue_state.reset_pending = false;
                    deck_state->tsm_tempo_node->update_wait( deck_state->tsm_tempo_node );
                    tsm_tempo_state->fade_out = false;
                    // TSM samples are rendered now, so we can stop/reset the deck
                    // Stop platter w/no spin-down
                    zdj_dj_deck_platter_stop_motor( deck, false );
                    // Reset decode win to requested coord
                    decode_node->reset_window( decode_node, deck->controls.cue_state.dest_origin_d );
                    // Reset anti-pop to new decode head
                    zdj_dj_deck_platter_reset_antipop( deck );
                    // Reset TSM nodes for immediate re-start
                    zdj_dj_deck_reset_tsm_nodes( deck );
                }
                
            
            // Pitch TSM Mode - scratching/scrubbing/spooldown
            } else if ( zdj_dj_deck_is_in_pitch_tsm_mode( deck ) ) {
                tsm_pitch_state->decode_end_coord = decode_state->head.transport_d;

                // Clip start/end to buffer bounds - for safety
                if( tsm_pitch_state->decode_start_coord < win_start.transport_d ) {                    
                    tsm_pitch_state->decode_start_coord = win_start.transport_d;
                } 
                if( tsm_pitch_state->decode_start_coord > win_end.transport_d ) {
                    tsm_pitch_state->decode_start_coord = win_end.transport_d;
                }

                // Update the Fast-Scrub Fade model
                zdj_dj_deck_platter_update_scrub_fade( deck );
                // Update the Anti-Pop model
                zdj_dj_deck_platter_update_antipop( deck );
                
                //////////////////////////////////////
                // Process normal Pitch TSM buffer. //
                //////////////////////////////////////
                if( !deck->controls.cue_state.reset_pending ) {
                    
                    
                    // Merge fade values
                    tsm_pitch_state->start_fade_val = platter->scrub_fade.start_fade_val * platter->antipop.start_fade_val;
                    tsm_pitch_state->end_fade_val = platter->scrub_fade.end_fade_val * platter->antipop.end_fade_val;

                    // Render the pitched output buffer
                    deck_state->tsm_pitch_node->update_wait( deck_state->tsm_pitch_node );
                    tsm_pitch_state->decode_start_coord = tsm_pitch_state->decode_end_coord;
                
                
                ///////////////////////////////////////////
                // Process play_to_reset request.        //
                // Fade out from start to end of buffer. //
                // Reset deck to reqested coord after    //
                // buffer is rendered.                   //
                ///////////////////////////////////////////
                } else if( deck->controls.cue_state.reset_pending ) {
                    deck->controls.cue_state.reset_pending = false;

                    // Merge fade values
                    tsm_pitch_state->start_fade_val = platter->scrub_fade.start_fade_val * platter->antipop.start_fade_val;
                    tsm_pitch_state->end_fade_val = 0.0;

                    // Render the pitched output buffer
                    deck_state->tsm_pitch_node->update_wait( deck_state->tsm_pitch_node );
                    
                    // TSM samples are rendered now, so we can stop/reset the deck
                    // Stop platter w/no spin-down
                    zdj_dj_deck_platter_stop_motor( deck, false );
                    // Reset decode win to requested coord
                    decode_node->reset_window( decode_node, deck->controls.cue_state.dest_origin_d );
                    // Reset anti-pop to new decode head
                    zdj_dj_deck_platter_reset_antipop( deck );
                    // Reset TSM nodes for immediate re-start
                    zdj_dj_deck_reset_tsm_nodes( deck );
                }
            }
        }


        // This doesn't mean anything after the first run thru the loop.
        // Can we clean that up a bit?
        // thread_ready tells the lib deck's init sequence to enter 'running' state.
        deck_state->thread_ready = true;

        ///////////////////////////////////////////////////////////////////////////
        // Phase 6 - Sleep thread pending a semaphore from the Fast Audio thread //
        ///////////////////////////////////////////////////////////////////////////
        // TESTING - if sval, then we missed a buffer - harden this implementation
        // int sval;
        // sem_getvalue( &deck_state->start_cycle, &sval );
        // if( sval > 0 ){ 
        //     printf( "deck sem miss con:%1.3f dcd:%1.3f tsm:%1.3f\n",
        //         ( con_end - con_start ) / 1000000.0,
        //         ( dcd_end - dcd_start ) / 1000000.0,
        //         ( tsm_end - tsm_start ) / 1000000.0
        //     ); 
        // }
        sem_wait( &deck_state->start_cycle );
    }

    // Thread cleanup before exit.

    printf( "DJ deck deinit: %p %d\n", deck, deck->station );

    // Give back the deck station thread
    switch ( deck->station ) {
        case ZDJ_DECK_STATION_1:
            zdj_thread_deck_1_station_available = true;
            break;
        case ZDJ_DECK_STATION_2:
            zdj_thread_deck_2_station_available = true;
            break;
        case ZDJ_DECK_STATION_EXT:
            zdj_thread_deck_ext_station_available = true;
            break;
        default: break;
    }

    // Detach so kernel cleans up mem
    pthread_detach( pthread_self( ) );
    return NULL;
}

bool zdj_dj_deck_is_in_tempo_tsm_mode( zdj_deck_t * deck ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    return deck_state->tsm_source == ZDJ_DECK_TSM_SOURCE_TEMPO;
}

bool zdj_dj_deck_is_in_pitch_tsm_mode( zdj_deck_t * deck ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    return deck_state->tsm_source == ZDJ_DECK_TSM_SOURCE_PITCH;
}

bool zdj_dj_deck_has_beatgrid( zdj_deck_t * deck ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    return ( deck_state->song->performance && deck_state->song->performance->has_beat_grid );
}

bool zdj_dj_deck_command_is_active( zdj_deck_t * deck ) {
    return false;
}

bool zdj_dj_deck_loop_is_enabled( zdj_deck_t * deck ) {
    return deck->controls.loop_state.is_enabled;
}

void zdj_dj_deck_reset_tsm_nodes( zdj_deck_t * deck ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_pipeline_node_t * decode_node = deck_state->decode_node;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;

    zdj_tsm_tempo_node_state_t * tsm_tempo_state = (zdj_tsm_tempo_node_state_t*)deck_state->tsm_tempo_node->state;
    // tsm_tempo_st\ate->decode_coord = decode_state->head.transport_d;
    zdj_reset_tsm_tempo_node( deck_state->tsm_tempo_node );
    zdj_tsm_pitch_node_state_t * tsm_pitch_state = (zdj_tsm_pitch_node_state_t*) deck_state->tsm_pitch_node->state;
    tsm_pitch_state->decode_start_coord = decode_state->head.transport_d;
    tsm_pitch_state->decode_end_coord = tsm_pitch_state->decode_start_coord;
}
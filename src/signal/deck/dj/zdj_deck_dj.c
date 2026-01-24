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
// #include <zerodj/signal/pipeline/node/audio/deck_dsp/zdj_deck_dsp_node.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/signal/pipeline/node/audio/tsm/zdj_tsm_pitch_node.h>
#include <zerodj/signal/pipeline/node/audio/tsm/zdj_tsm_tempo_node.h>

// External data/event entry points
static void _update_state ( zdj_deck_t * deck );
static void _begin_teardown( zdj_deck_t * deck );
static void _deinit( zdj_deck_t * deck );
static void _get_edge_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo );

// static void _reset_tsm_tempo_node_to_decode_addr( 
//     zdj_pipeline_node_t * node, 
//     double addr 
// );


static int64_t _get_resource_addr( struct zdj_deck_t * deck );
static void _set_resource_addr( struct zdj_deck_t * deck, int64_t addr );

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

    // Lifecycle
    deck->update_state = &_update_state;
    deck->begin_teardown = &_begin_teardown;
    deck->deinit = &_deinit;
    
    zdj_deck_dj_init_soundcard( deck );
    zdj_deck_dj_init_transport( deck );
    zdj_deck_dj_init_discon( deck );

    deck->get_resource_addr = &_get_resource_addr;
    deck->set_resource_addr = &_set_resource_addr;

    sem_init( &state->start_cycle, 0, 0 );

    // printf( "zdj_new_dj_deck done\n" );

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
            printf( "ZDJ_DECK_STATUS_NEW (%p)\n", deck );
            deck->status = ZDJ_DECK_STATUS_MAKE_PIPELINE;
            break;

        // Stand up pipeline nodes.
        case ZDJ_DECK_STATUS_MAKE_PIPELINE:
            printf( "ZDJ_DECK_STATUS_MAKE_PIPELINE (%p)\n", deck );
            // deck_state->dsp_node = zdj_new_deck_dsp_node( );

            zdj_deck_dj_init_sync( deck );
            
            int decode_buf_count = deck_state->decode_win_buf_count; // number of decode buffers which fit in window.

            deck_state->decode_node = zdj_new_decode_node( 
                deck_state->song, 0, ZDJ_SOUNDCARD_BUF_LEN*decode_buf_count, ZDJ_SOUNDCARD_BUF_LEN*decode_buf_count 
            );
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
            zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
            decode_state->get_win_start_addr( deck_state->decode_node, &win_start );

            deck->predelay_counter = 0;

            deck->status = ZDJ_DECK_STATUS_WAIT_THREAD_AVAIL;
            break;
            
        // Make sure station 1 doesn't already have a thread running.
        // If it does, assume it's in the process of exiting,
        // and keep polling here until it becomes available.
        case ZDJ_DECK_STATUS_WAIT_THREAD_AVAIL:
            printf( "ZDJ_DECK_STATUS_WAIT_THREAD_AVAIL (%p)\n", deck );
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
            printf( "DJ ZDJ_DECK_STATUS_WAIT_THREAD_READY (%p)\n", deck );
            if( deck_state->thread_ready ) {     
                // printf( "0\n" );           
                // Start serving deck samples to soundcard.
                zdj_soundcard_link_deck( zdj_soundcard, deck );
                // Accept control events when we're ready for playback
                // deck->handle_control_event = &zdj_dj_deck_handle_controls;
                // printf( "1\n" );
                zdj_deck_dj_init_controls( deck );
                // Advance state to running
                deck->status = ZDJ_DECK_STATUS_RUNNING; 

                zdj_decode_addr_t win_start;
                zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;
                decode_state->get_win_start_addr( deck_state->decode_node, &win_start );

                // printf( "2\n" );
                // Update deck manager tempo
                deck->set_sync_bpm( deck, zdj_deck_manager( )->sync.set_bpm );

                // printf( "3\n" );
                // Prep loop/skip state
                deck->controls.discon_quantize = true;
                deck->controls.discon_quantize_val = 0.0625f;
                deck->controls.loop_state.fade_len = 300;
                deck->controls.loop_state.beatgrid_len = 1.0f;
                deck->controls.loop_state.pcm_len = decode_state->get_d_offset_for_beatgrid_dist( 
                    deck_state->decode_node, deck->controls.loop_state.beatgrid_len 
                );
                deck->controls.skip_state.phase = ZDJ_DECK_SKIP_PHASE_INACTIVE;
                deck->controls.needledrop_state.phase = ZDJ_DECK_NEEDLEDROP_PHASE_INACTIVE;

                // printf( "4\n" );
                // Call UI load CB
                if( deck->ui_load_cb ){ deck->ui_load_cb( deck ); }
            }
            // printf( "ZDJ_DECK_STATUS_WAIT_THREAD_READY DONE\n" );
            break;

        // Ignore new control events and start spooldown of deck drive/transport model.
        case ZDJ_DECK_STATUS_STOP_TRANSPORT:
            printf( "ZDJ_DECK_STATUS_STOP_TRANSPORT (%p)\n", deck );
            // Immediately stop accepting control events.
            deck->handle_control_event = NULL;
            // Stop deck playback if running.
            if( deck->controls.platter.motor.enabled ) {
                deck->safe_to_deinit = false;
                if( deck->controls.platter.slip.state == ZDJ_PLATTER_SLIP_LAMINAR_TEMPO ) {
                    // If we're in tempo mode, we need to sync the needle head with tempo node.
                    zdj_dj_deck_reset_platter( &deck->controls.platter, tsm_tempo_state->decode_coord );
                    // We also need to set the pitch node's sample addresses to something reasonable
                    zdj_tsm_pitch_node_state_t * tsm_pitch_state = (zdj_tsm_pitch_node_state_t*)deck_state->tsm_pitch_node->state;
                    tsm_pitch_state->decode_start_coord = tsm_tempo_state->decode_coord - ZDJ_SOUNDCARD_BUF_LEN;
                }
                deck->controls.platter.motor.enabled = false;
                deck->controls.platter.motor.state = ZDJ_PLATTER_MOTOR_SPIN_DOWN;
                deck->controls.platter.motor.cur_spin_down_cycle = 0;
                // Ensure slip is in pitch mode so we hear spin down.
                deck->controls.platter.slip.state = ZDJ_PLATTER_SLIP_LAMINAR_PITCH;
            } else {
                deck->safe_to_deinit = true;
            }
            deck->status = ZDJ_DECK_STATUS_WAIT_SPOOLDOWN; 

            // Call UI unload CB
            if( deck->ui_unload_cb ){ deck->ui_unload_cb( deck ); }
            break;

        // Wait while deck transport fades out or slows to rate=0.
        // Allow this to take several audio buffer cycles.
        case ZDJ_DECK_STATUS_WAIT_SPOOLDOWN:
            printf( "ZDJ_DECK_STATUS_WAIT_SPOOLDOWN (%p)\n", deck );
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

    // Set up scheduling
    int prio = sched_get_priority_max( SCHED_RR );
	struct sched_param param;
	param.sched_priority = prio;
	sched_setscheduler( syscall(SYS_gettid), SCHED_RR, &param );

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
            // CPU_SET( 1,&cpuset );
            zdj_thread_deck_ext_station_available = false;
            break;
        default: break;
    }

    int err = sched_setaffinity( syscall(SYS_gettid), sizeof(cpu_set_t), &cpuset );
    if( err != 0 ) {
        perror( "set affinity failed" );
    }

    double p_start, p_start_prev, d_start, p_end, d_end;

    double head_move_val;
    double head_move_rate;
    while( !deck_state->exit_thread ) {
        // printf( "dj deck thread\n" );

        double tsm_start, tsm_end, con_start, con_end, dcd_start, dcd_end;

        con_start = zdj_perf_time( );
        // Process any new loop/skip requests
        if( deck->controls.loop_state.phase == ZDJ_DECK_LOOP_PHASE_ACTIVATE ) {
            // zdj_deck_new_loop( deck );
            // printf( "handle new loop req: %p\n", deck_state->handle_new_loop_req );
            deck_state->handle_new_loop_req( deck );
        } else if( deck->controls.loop_state.phase == ZDJ_DECK_LOOP_PHASE_DEACTIVATE ) {
            // zdj_deck_disable_loop( deck );
            deck_state->handle_disable_loop_req( deck );
        } else if( deck->controls.loop_state.phase == ZDJ_DECK_LOOP_PHASE_MOVE ) {
            deck_state->handle_move_loop_req( deck );
        } else if( deck->controls.loop_state.phase == ZDJ_DECK_LOOP_PHASE_RESIZE ) {
            deck_state->handle_resize_loop_req( deck );
        }

        // printf( "skip: %p->phase: %d\n", &deck->controls.skip_state, deck->controls.skip_state.phase );
        if( deck->controls.skip_state.phase == ZDJ_DECK_SKIP_PHASE_ACTIVATE ) {
            // printf( "skip state activate\n" );
            deck_state->stage_skip_req( deck );
        } else if( deck->controls.skip_state.phase == ZDJ_DECK_SKIP_PHASE_STAGED ) {
            deck_state->update_skip_req( deck );
        }

        if( deck->controls.needledrop_state.phase == ZDJ_DECK_NEEDLEDROP_PHASE_ACTIVATE ) {
            deck_state->stage_needledrop_req( deck );
            continue;
        } else if( deck->controls.needledrop_state.phase == ZDJ_DECK_NEEDLEDROP_PHASE_STAGED ) {
            deck_state->update_needledrop_req( deck );
            continue;
        }

        con_end = zdj_perf_time( );

        dcd_start = zdj_perf_time( );

        zdj_decode_addr_t win_start;
        zdj_decode_addr_t win_end;

        // Limit the head move val to prevent overrunning the decode buffer
        double max_head_move_val = (double)decode_state->win_fwd_sample_count;
        
        // Update needle head model based on most recent rate calculated within the control update thread
        if( deck->update_transport_outputs ){ deck->update_transport_outputs( deck ); }

        // UPGRADE:
        // Build head_move_val by integrating <SOUNDCARD_BUF_LEN> steps over transport coords starting at start_rate, interpolating up to end_rate.
        // CURRENT:
        // Just jump to latest rate value for entire soundcard buf cycle
        double head_move_val = deck->controls.platter.needle.head - decode_state->head.transport_d;

        if( head_move_val > max_head_move_val ) { 
            head_move_val = max_head_move_val; 
            // printf( "limit fwd head_mov_val: %1.1f\n", head_move_val ); 
        } else if( head_move_val < max_head_move_val*-1 ) { 
            head_move_val = max_head_move_val*-1; 
            // printf( "limit back head_mov_val: %1.1f\n", head_move_val );
        }

        // printf( "head_move: %1.0f\n", head_move_val );
        if( fabs( head_move_val ) > zdj_eps) {
            // If hyperscrub has been requested, this means we're scrubbing faster than
            // the decode window will allow.  Add a skip to catch up to the requested scrub rate,
            // but continue moving thru the decode buffer at the clamped scrub rate.
            // if( fabs( deck->controls.hyperscrub_state.req_offset ) > zdj_eps ) {
            //     deck->new_hyperscrub( deck, deck->controls.hyperscrub_state.req_offset );
            // }

            // if ( zdj_perf_enabled( ) ) {
                
                

            //     zdj_perf_tag_t * move_tag = zdj_new_perf_tag_for_thread( ZDJ_SYSTEM_THREAD_DECK_AUDIO_CYCLE );
            //     move_tag->name = ZDJ_PERF_TAG_DECK_MOVE;
            //     move_tag->start = zdj_perf_time( );
            //     decode_node->move_window( decode_node, head_move_val );
            //     decode_node->update_wait( decode_node );
            //     move_tag->end = zdj_perf_time( );
            
            // d_start = zdj_perf_time( );

            // } else {
                decode_node->move_window( decode_node, head_move_val );
                decode_node->update_wait( decode_node );
            // }

            // d_end = zdj_perf_time( );

            

            // decode_state->get_win_start_addr( decode_node, &win_start );
            // decode_state->get_win_end_addr( decode_node, &win_end );
            // printf( "1 %p ffwd tsm:%1.1f (%p)win:%1.1f\n", deck, tsm_pitch_state->decode_start_coord, decode_node, win_start.transport_d );
            // printf( "post-move win: %1.0f/%1.0f -> %1.0f/%1.0f\n", 
            //     win_start.transport_d, win_start.origin_d,
            //     win_end.transport_d, win_end.origin_d
            // );
        }

        decode_state->get_win_start_addr( decode_node, &win_start );
        decode_state->get_win_end_addr( decode_node, &win_end );

        dcd_end = zdj_perf_time( );

        switch( deck_state->tsm_source ) {
            case ZDJ_DECK_TSM_SOURCE_PITCH:
                // Update the pitch tsm node only during transitions/pitch playback
                tsm_pitch_state->decode_end_coord = decode_state->head.transport_d;

                // Clip start/end to buffer bounds
                if( tsm_pitch_state->decode_start_coord < win_start.transport_d ) {                    
                    tsm_pitch_state->decode_start_coord = win_start.transport_d;
                } 
                if( tsm_pitch_state->decode_start_coord > win_end.transport_d ) {
                    tsm_pitch_state->decode_start_coord = win_end.transport_d;
                }
                
                // printf( "tsm pitch update tsm:%1.0f dcd:%1.0f/%1.0f -> tsm:%1.0f dcd:%1.0f/%1.0f\n", 
                //     tsm_pitch_state->decode_start_coord, 
                //     win_start.transport_d, win_start.origin_d,
                //     tsm_pitch_state->decode_end_coord,
                //     win_end.transport_d, win_end.origin_d 
                // );

                deck_state->tsm_pitch_node->update_wait( deck_state->tsm_pitch_node );
                tsm_pitch_state->decode_start_coord = tsm_pitch_state->decode_end_coord;

                
                // printf( "tsm pitch update done\n" );
                break;
            
            case ZDJ_DECK_TSM_SOURCE_TEMPO:
                // printf( "tsm tempo update\n" );
                // Tempo TSM works differently than Pitch TSM.
                // Tempo takes a rate and decides how many whole samples it wants
                // to come close to that playback rate.  
                // So we use the platter model to calculate the rate, but we actually
                // ignore the calculated needle head coords.
                // When it's time to exit tempo mode, we re-sync the platter simulation
                // to the tempo node's last read coords.

                
                tsm_start = zdj_perf_time( );

                tsm_tempo_state->rate = deck->controls.platter.motor.set_rate + deck->controls.platter.slip.tempo_nudge_rate;
                deck_state->tsm_tempo_node->update_wait( deck_state->tsm_tempo_node );

                tsm_end = zdj_perf_time( );

                break;
        }

        
        // double p_interval = ( p_start - p_end ) / 1000000.0;
        // if( p_interval >  ) {
        //     printf( "int:%1.5f\n", p_interval );
        // }

        // This doesn't mean anything after the first run thru the loop.
        // Can we clean that up a bit?
        // thread_ready tells the lib deck's init sequence to enter 'running' state.
        deck_state->thread_ready = true;

        // printf( "dj deck thread done\n" );
        // Wait for signal from soundcard fast cycle to process another buffer
        // p_end = zdj_perf_time( );
        // printf( "d:%1.2f\n", ( p_end - p_start ) / 1000000.0 );
        int sval;
        sem_getvalue( &deck_state->start_cycle, &sval );
        if( sval > 0 ){ 
            printf( "deck sem miss con:%1.3f dcd:%1.3f tsm:%1.3f\n",
                ( con_end - con_start ) / 1000000.0,
                ( dcd_end - dcd_start ) / 1000000.0,
                ( tsm_end - tsm_start ) / 1000000.0
            ); 
        }
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

static int64_t _get_resource_addr( struct zdj_deck_t * deck ) {

}


static void _set_resource_addr( struct zdj_deck_t * deck, int64_t addr ) {

}
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <math.h>

#include <zerodj/controls/zdj_controls.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/dj/zdj_deck_dj.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/signal/pipeline/node/audio/tsm/zdj_tsm_pitch_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/system/thread/zdj_thread.h>

// External data/event entry points
static void _update_state ( zdj_deck_t * deck );
static void _deinit( zdj_deck_t * deck );
static void _get_edge_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo );
static void _handle_control_event( zdj_deck_t * deck, zdj_control_event_t * event );
static void _update_controls ( zdj_deck_t * deck );

// Pipeline update thread
static void * _pipeline_thread_main( void * arg );

zdj_error_type_t zdj_new_dj_deck( zdj_deck_t * deck, void * resource ) {
    deck->update_state = &_update_state;
    deck->deinit = &_deinit;
    deck->get_edge_data = &_get_edge_data;
    deck->update_controls = zdj_deck_update_controls;
    deck->handle_control_event = NULL; // Ignore control events until we're playable.
    zdj_dj_deck_state_t * state = calloc( 1, sizeof( zdj_dj_deck_state_t ) );
    deck->state = state;
    state->song = (zdj_library_song_t *)resource;
    sem_init( &state->start_cycle, 0, 0 );

    // Reset deck controls
    memset( &deck->controls, 0, sizeof( zdj_deck_control_state_t ) );

    return ZDJ_ERROR_OKAY;
}

static void _deinit( zdj_deck_t * deck ) {
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    // Teardown decode + tsm pipeline.
    deck_state->decode_node->deinit( deck_state->decode_node );
    deck_state->tsm_node->deinit( deck_state->tsm_node );
}

// Called from the slow deck update thread (~.25Hz).
static void _update_state ( zdj_deck_t * deck ) {
    // printf( "lib deck _update_state\n" );

    // Early exit if we're up and running.
    if( deck->status == ZDJ_DECK_STATUS_RUNNING ) { return; }
    
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;

    switch ( deck->status ) {

        case ZDJ_DECK_STATUS_NEW:
            printf( "ZDJ_DECK_STATUS_NEW\n" );
            deck->status = ZDJ_DECK_STATUS_MAKE_PIPELINE;
            break;

        // Statnd up pipeline nodes.
        case ZDJ_DECK_STATUS_MAKE_PIPELINE:
            printf( "ZDJ_DECK_STATUS_MAKE_PIPELINE\n" );
            deck_state->decode_node = zdj_new_decode_node( 
                deck_state->song, 0, 64, 64 
            );
            deck_state->tsm_node = zdj_new_tsm_pitch_node( 
                deck_state->song->audio->av_channel_count,
                ZDJ_SOUNDCARD_BUF_LEN,
                deck_state->decode_node
            );
            deck->status = ZDJ_DECK_STATUS_WAIT_THREAD_AVAIL;
            break;
            
        // Make sure station 1 doesn't already have a thread running.
        // If it does, assume it's in the process of exiting,
        // and keep polling here until it becomes available.
        case ZDJ_DECK_STATUS_WAIT_THREAD_AVAIL:
            printf( "ZDJ_DECK_STATUS_WAIT_THREAD_AVAIL\n" );
            if( deck->station == ZDJ_DECK_STATION_1 && zdj_thread_deck_1_station_available ) {
                zdj_thread_launch_deck_station_1_cycle( _pipeline_thread_main, deck );
                deck->status = ZDJ_DECK_STATUS_WAIT_THREAD_READY;
            } else if( deck->station == ZDJ_DECK_STATION_2 && zdj_thread_deck_2_station_available ) {
                zdj_thread_launch_deck_station_2_cycle( _pipeline_thread_main, deck );
                deck->status = ZDJ_DECK_STATUS_WAIT_THREAD_READY;
            } else if( deck->station == ZDJ_DECK_STATION_EXT && zdj_thread_deck_ext_station_available ) {
                zdj_thread_launch_deck_station_ext_cycle( _pipeline_thread_main, deck );
                deck->status = ZDJ_DECK_STATUS_WAIT_THREAD_READY;
            }
            break;

        // Wait for new deck thread to finish filling its buffers.
        case ZDJ_DECK_STATUS_WAIT_THREAD_READY:
            printf( "ZDJ_DECK_STATUS_WAIT_THREAD_READY\n" );
            if( deck_state->thread_ready ) {                
                // Start serving deck samples to soundcard.
                zdj_soundcard_link_deck( zdj_soundcard, deck );
                // Accept control events when we're ready for playback
                deck->handle_control_event = zdj_deck_handle_control;
                // Advance state to running
                deck->status = ZDJ_DECK_STATUS_RUNNING; 
            }
            break;

        // Ignore new control events and start spooldown of deck drive/transport model.
        case ZDJ_DECK_STATUS_STOP_TRANSPORT:
            printf( "ZDJ_DECK_STATUS_STOP_TRANSPORT\n" );
            // Immediately stop accepting control events.
            deck->handle_control_event = NULL;
            // Stop deck playback if running.
            deck->controls.platter.motor.set_rate = 0;
            deck->status = ZDJ_DECK_STATUS_WAIT_SPOOLDOWN; 
            break;

        // Wait while deck transport fades out or slows to rate=0.
        // Allow this to take several audio buffer cycles.
        case ZDJ_DECK_STATUS_WAIT_SPOOLDOWN:
            printf( "ZDJ_DECK_STATUS_WAIT_SPOOLDOWN\n" );
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

static void _get_edge_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo ) {
    // printf( "lib get edge data\n" );
    zdj_deck_t * deck = (zdj_deck_t*)_deck;
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_tsm_pitch_node_state_t * tsm_state = (zdj_tsm_pitch_node_state_t*)deck_state->tsm_node->state;

    float * soundcard_buf = data_pipe->get_data( data_pipe );
    
    // copy TSM out_buffer to data_pipe
    if( stereo && tsm_state->channel_count == 2 ) {
        // stereo->stereo
        memcpy( soundcard_buf, tsm_state->out_buffer, ZDJ_SOUNDCARD_BUF_LEN * 2 * sizeof( float ) );
    } else if( !stereo && tsm_state->channel_count == 1 ) {
        // mono->mono
        memcpy( soundcard_buf, tsm_state->out_buffer, ZDJ_SOUNDCARD_BUF_LEN * sizeof( float ) );
    }
    // stereo->mono + mono->stereo not handled yet.

    // Release another cycle of the pipeline_thread_main below.
    sem_post( &deck_state->start_cycle );
}

// Thread for processing soundcard fast-cycle requests.
// get_edge_data posts the start_cycle semaphore below.
static void * _pipeline_thread_main( void * arg ) {
    // printf( "_pipeline_thread_main: %p\n", arg );
    zdj_deck_t * deck = (zdj_deck_t*)arg;
    zdj_dj_deck_state_t * deck_state = (zdj_dj_deck_state_t*)deck->state;
    zdj_tsm_pitch_node_state_t * tsm_state = (zdj_tsm_pitch_node_state_t*)deck_state->tsm_node->state;
    zdj_decode_node_state_t * decode_state = (zdj_decode_node_state_t*)deck_state->decode_node->state;

    // Claim the deck station thread
    switch ( deck->station ) {
        case ZDJ_DECK_STATION_1: zdj_thread_deck_1_station_available = false; break;
        case ZDJ_DECK_STATION_2: zdj_thread_deck_2_station_available = false; break;
        case ZDJ_DECK_STATION_EXT: zdj_thread_deck_ext_station_available = false; break;
        default: break;
    }
    
    double head_move_val;
    double head_move_rate;
    double last_head_val = deck->controls.platter.needle.head;

    while( !deck_state->exit_thread ) {
        // printf( "lib deck thread\n" );
        // Gather transport control model values.
        // Build a window-move value for the decode_node.
        head_move_val = deck->controls.platter.needle.head - last_head_val;
        last_head_val = deck->controls.platter.needle.head;

        // // Move the decode window and keep the tsm head in sync.
        // deck_state->decode_node->move_window( deck_state->decode_node, head_move_val );
        // // Re-charge the decode node buffer after the move
        // deck_state->decode_node->update_wait( deck_state->decode_node );

        // // Update tsm rate from control model
        // tsm_state->rate = deck->controls.drive_state.instant_pitch;
        // // Pitch-interpolate samples from decode window into tsm buffer        
        // deck_state->tsm_node->update_wait( deck_state->tsm_node );

        // This doesn't mean anything after the first run thru the loop.
        // Can we clean that up a bit?
        // thread_ready tells the lib deck's init sequence to enter 'running' state.
        deck_state->thread_ready = true;

        // printf( "addr: %ld\n", decode_state->decode_mono_addr.i_val );

        // printf( "lib deck thread waiting\n" );
        // Wait for signal from soundcard fast cycle to process another buffer
        sem_wait( &deck_state->start_cycle );
    }

    // Thread cleanup before exit.

    // Give back the deck station thread
    switch ( deck->station ) {
        case ZDJ_DECK_STATION_1: zdj_thread_deck_1_station_available = true; break;
        case ZDJ_DECK_STATION_2: zdj_thread_deck_2_station_available = true; break;
        case ZDJ_DECK_STATION_EXT: zdj_thread_deck_ext_station_available = true; break;
        default: break;
    }

    // Detach so kernel cleans up mem
    pthread_detach( pthread_self( ) );
    return NULL;
}
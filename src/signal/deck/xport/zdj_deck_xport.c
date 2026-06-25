#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <math.h>
#include <sys/syscall.h>

#include <zerodj/controls/zdj_controls.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/xport/zdj_deck_xport.h>
#include <zerodj/signal/deck/zdj_deck_manager.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

static void _update_state ( zdj_deck_t * deck );
static void _deinit( zdj_deck_t * deck );
static void _push_edge_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo );
static void _get_edge_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo );
static void _handle_control( zdj_deck_t * deck, zdj_control_event_t * event );

// Pipeline update thread
static void * _pipeline_thread_main( void * arg );

zdj_error_type_t zdj_new_xport_deck( zdj_deck_t * deck ) {
    deck->update_state = &_update_state;
    deck->deinit = &_deinit;
    deck->push_edge_data = &_push_edge_data;
    deck->get_edge_data = &_get_edge_data;
    deck->can_sync = true;
    deck->handle_control_event = NULL; // handle_control is set later in the init flow
    
    zdj_xport_deck_state_t * state = calloc( 1, sizeof( zdj_xport_deck_state_t ) );
    state->set_bpm = 120.0;
    state->transport_bg = 0.0;
    state->meter_on = false;
    state->meter_counter = 0;
    deck->state = state;

    zdj_deck_xport_init_sync( deck );
    zdj_deck_xport_init_transport( deck );

    sem_init( &state->start_cycle, 0, 0 );

    return ZDJ_ERROR_OKAY;
}

static void _deinit( zdj_deck_t * deck ) {
    zdj_xport_deck_state_t * deck_state = (zdj_xport_deck_state_t*)deck->state;
}

// Called from the slow deck update thread (~.25Hz).
static void _update_state ( zdj_deck_t * deck ) {
    // printf( "clock deck update\n" );
    // Early exit if we're up and running.
    if( deck->status == ZDJ_DECK_STATUS_RUNNING ) { return; }
    
    zdj_xport_deck_state_t * deck_state = (zdj_xport_deck_state_t*)deck->state;

    switch ( deck->status ) {

        case ZDJ_DECK_STATUS_NEW:
            // printf( "ZDJ_CLOCK_DECK_STATUS_NEW\n" );
            deck->status = ZDJ_DECK_STATUS_MAKE_PIPELINE;
            break;

        // Stand up pipeline nodes.
        case ZDJ_DECK_STATUS_MAKE_PIPELINE:
            // printf( "ZDJ_CLOCK_DECK_STATUS_MAKE_PIPELINE\n" );
            deck->status = ZDJ_DECK_STATUS_WAIT_THREAD_AVAIL;
            break;
            
        // Make sure station 1 doesn't already have a thread running.
        // If it does, assume it's in the process of exiting,
        // and keep polling here until it becomes available.
        case ZDJ_DECK_STATUS_WAIT_THREAD_AVAIL:
            // printf( "ZDJ_CLOCK_DECK_STATUS_WAIT_THREAD_AVAIL\n" );
            deck->status = ZDJ_DECK_STATUS_WAIT_THREAD_READY;
            break;

        // Wait for new deck thread to finish filling its buffers.
        case ZDJ_DECK_STATUS_WAIT_THREAD_READY:
            // printf( "ZDJ_CLOCK_DECK_STATUS_WAIT_THREAD_READY\n" );             
            // Start serving deck samples to soundcard.
            zdj_soundcard_link_deck( zdj_soundcard, deck );
            // Accept control events when we're ready for playback
            deck->handle_control_event = &_handle_control;
            // Advance state to running
            deck->status = ZDJ_DECK_STATUS_RUNNING; 
            // Call UI unload CB
            // if( deck->ui_load_cb ){ deck->ui_load_cb( deck ); }
            break;

        // Ignore new control events and start spooldown of deck drive/transport model.
        case ZDJ_DECK_STATUS_STOP_TRANSPORT:
            // printf( "ZDJ_CLOCK_DECK_STATUS_STOP_TRANSPORT\n" );
            // Immediately stop accepting control events.
            deck->handle_control_event = NULL;
            
            // Stop clock output

            deck->status = ZDJ_DECK_STATUS_WAIT_SPOOLDOWN; 
            // Call UI unload CB
            // if( deck->ui_unload_cb ){ deck->ui_unload_cb( deck ); }
            break;

        // Wait while deck transport fades out or slows to rate=0.
        // Allow this to take several audio buffer cycles.
        case ZDJ_DECK_STATUS_WAIT_SPOOLDOWN:
            // printf( "ZDJ_DECK_STATUS_WAIT_SPOOLDOWN\n" );
            // Stop sending deck samples to soundcard
            zdj_soundcard_unlink_deck( zdj_soundcard, deck );
            // Tell the deck's fast-audio pipeline thread to exit.
            // Manually post the sem since we have unlinked the deck.
            deck_state->exit_thread = true;
            sem_post( &deck_state->start_cycle );
            // Tell the deck_manager we are ready for deinit()
            deck->status = ZDJ_DECK_STATUS_IDLE;
            break;
        default: break;
    }

    // printf( "lib deck _update_state done\n" );
}


//////////////////////////////////
// Fast-Cycle Soundcard Buffers
//////////////////////////////////
// We manage the analog clock buffers here.

// Push clock synth samples out to soundcard node
static void _get_edge_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo ) {
    zdj_deck_t * deck = (zdj_deck_t*)_deck;
    zdj_xport_deck_state_t * deck_state = (zdj_xport_deck_state_t*)deck->state;
    // printf( "clock get edge data\n" );

    // Get a reference to external deck input node's buffer
    zdj_soundcard_node_t * ext_input_node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_XPORT_0 );
    float * input_buf = ext_input_node->data_pipe->get_data( ext_input_node->data_pipe );

    // Get a reference to the soundcard deck edge buffer which needs filling.
    float * out_buf = data_pipe->get_data( data_pipe );

    if( deck->update_platter_model ) { deck->update_platter_model( deck ); }

    if( deck->controls.platter.motor.enabled ) {
        double ppqn = deck_state->ppqn;
        double quant_val = 0.250 / ppqn;

        double bars_per_minute = (deck_state->set_bpm / 4.0) * (120.0 / deck_state->set_bpm);
        double samples_per_minute = 44100.0 * 60.0;
        double samples_per_bar = samples_per_minute / bars_per_minute;

        // double d_offset = deck->controls.platter.motor.set_rate;
        double d_offset = deck->controls.platter.motor.instant_rate;
        double bg_offset = d_offset / samples_per_bar;
        double start_transport_d = deck_state->transport_d;
        double start_transport_bg = deck_state->transport_bg;
        double cur_bg = start_transport_bg;
        double quant_bg;

        for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
            quant_bg = cur_bg - ( floor( cur_bg / quant_val ) * quant_val );
            if( quant_bg < (quant_val * 0.3) ) {
                deck_state->meter_on = true;
                out_buf[ i ] = 0.0;
            } else {
                deck_state->meter_on = false;
                out_buf[ i ] = 1.0;
            }
            cur_bg += bg_offset;
        }

        deck_state->transport_d += (ZDJ_SOUNDCARD_BUF_LEN * deck->controls.platter.motor.instant_rate);
        deck_state->transport_bg += (ZDJ_SOUNDCARD_BUF_LEN * deck->controls.platter.motor.instant_rate) / samples_per_bar;
    }
}

// We get this call after soundcard accums everything into Deck Ext Input
static void _push_edge_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo ) {
    
    zdj_deck_t * deck = (zdj_deck_t*)_deck;
    zdj_xport_deck_state_t * deck_state = (zdj_xport_deck_state_t*)deck->state;
    // printf( "clock push edge data\n" );

    // Get a reference to the soundcard deck edge buffer with samples.
    float * in_buf = data_pipe->get_data( data_pipe );

    // printf( "data pipe in: %f\n", in_buf[ 4 ] );

    // Push the input samples into the DSP node to the output buffer
    // zdj_deck_dsp_node_push_buffer( deck_state->dsp_node, in_buf, stereo+1, ZDJ_SOUNDCARD_BUF_LEN );

    // printf( "dj get edge data done\n" );
    // Release another cycle of the pipeline_thread_main below.
    // sem_post( &deck_state->start_cycle );
}


static void _handle_control( zdj_deck_t * deck, zdj_control_event_t * event ) {
    printf( "xport _handle_control: %p %d\n", deck, event->id );
    // All external clock and transport signals flow through here.
    // They are then re-routed based on instantaneous system state

    zdj_xport_deck_state_t * deck_state = (zdj_xport_deck_state_t*)deck->state;
    zdj_deck_platter_t * platter = &deck->controls.platter;

    switch ( event->id ) {

//     //////////////////
//     // Play / Pause //
//     //////////////////  
     
    case ZDJ_DECK_XPORT_CONTROL_PLAY_PAUSE:
        if( !platter->motor.enabled ) {
            platter->motor.enabled = true;
            platter->motor.set_rate = platter->motor.pitch_setting;
            platter->motor.state = ZDJ_PLATTER_MOTOR_RUN;
        
        // Pause
        } else {
            platter->motor.enabled = false;
            platter->motor.state = ZDJ_PLATTER_MOTOR_IDLE;
        }
        printf( "xport deck toggle play/pause: %1.3f\n", platter->motor.set_rate );
        break;


//     ///////////
//     // Scrub //
//     ///////////  

    case ZDJ_DECK_XPORT_CONTROL_SCRUB:
        // Nudge clock synth
        if( platter->motor.enabled ) {
            // printf( "clock_nudge: %d\n", event->i_val );
            platter->slip.instant_val += (double)event->i_val * 0.010;       
        } 
        break;


//     ///////////
//     // Tempo //
//     ///////////  

    case ZDJ_DECK_XPORT_CONTROL_TEMPO:
    
        if( zdj_deck_manager( )->sync.preferred &&
            !zdj_deck_manager( )->sync.active &&
            zdj_deck_manager_can_activate_sync( )
        ) {
            // Sync is set to enabled, can be enabled, but hasn't been activated by any other deck
            zdj_deck_manager( )->sync.active = true;
            zdj_deck_manager( )->sync.locked = true;
            zdj_deck_manager( )->sync.set_bpm = deck_state->set_bpm;

        } else if( zdj_deck_manager( )->sync.active ) {
            zdj_deck_manager_update_sync_bpm( event->i_val * 1 );
        } else {
            deck->offset_sync_bpm( deck, event->i_val * 1 );
        }
        break;

    case ZDJ_DECK_XPORT_CONTROL_TEMPO_FINE:

        // if( clock_is_output( ) ) {
        if( zdj_deck_manager( )->sync.preferred &&
            !zdj_deck_manager( )->sync.active &&
            zdj_deck_manager_can_activate_sync( )
        ) {
            // Sync is set to enabled, can be enabled, but hasn't been activated by any other deck
            zdj_deck_manager( )->sync.active = true;
            zdj_deck_manager( )->sync.locked = true;
            zdj_deck_manager( )->sync.set_bpm = deck_state->set_bpm;
        } else if( zdj_deck_manager( )->sync.active ) {
            // Sync is active - adjust main sync tempo
            zdj_deck_manager_update_sync_bpm( event->i_val * 0.01 );
        } else {
            // Sync is disabled - adjust clock synth tempo only
            deck->offset_sync_bpm( deck, event->i_val * 0.01 );
        }
        // } else {
            // Clock is input - flash tempo warning
        // }
        break;


//     //////////
//     // Sync //
//     //////////  

    case ZDJ_DECK_CONTROL_SYNC_TOGGLE:
        break;

    case ZDJ_DECK_XPORT_CONTROL_SYNC_MULT:
        break;

    default:
        break;
    }
}
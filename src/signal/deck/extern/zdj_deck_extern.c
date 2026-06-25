#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <math.h>
#include <sys/syscall.h>

#include <zerodj/controls/zdj_controls.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/extern/zdj_deck_extern.h>
#include <zerodj/signal/deck/zdj_deck_manager.h>
// #include <zerodj/signal/pipeline/node/audio/deck_dsp/zdj_deck_dsp_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

static void _update_state ( zdj_deck_t * deck );
static void _deinit( zdj_deck_t * deck );
static void _push_edge_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo );
static void _get_edge_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo );
static void _handle_control( zdj_deck_t * deck, zdj_control_event_t * event );

// Pipeline update thread
static void * _pipeline_thread_main( void * arg );

zdj_error_type_t zdj_new_extern_deck( zdj_deck_t * deck ) {
    deck->update_state = &_update_state;
    deck->deinit = &_deinit;
    deck->push_edge_data = &_push_edge_data;
    deck->get_edge_data = &_get_edge_data;
    deck->can_sync = false;
    deck->handle_control_event = NULL; // handle_control is set later in the init flow
    
    zdj_ext_deck_state_t * state = calloc( 1, sizeof( zdj_ext_deck_state_t ) );
    deck->state = state;

    sem_init( &state->start_cycle, 0, 0 );

    return ZDJ_ERROR_OKAY;
}

static void _deinit( zdj_deck_t * deck ) {
    zdj_ext_deck_state_t * deck_state = (zdj_ext_deck_state_t*)deck->state;
    // deck_state->dsp_node->deinit( deck_state->dsp_node );
}

// Called from the slow deck update thread (~.25Hz).
static void _update_state ( zdj_deck_t * deck ) {
    // Early exit if we're up and running.
    if( deck->status == ZDJ_DECK_STATUS_RUNNING ) { return; }
    
    zdj_ext_deck_state_t * deck_state = (zdj_ext_deck_state_t*)deck->state;

    switch ( deck->status ) {

        case ZDJ_DECK_STATUS_NEW:
            // printf( "ZDJ_EXT_DECK_STATUS_NEW\n" );
            deck->status = ZDJ_DECK_STATUS_MAKE_PIPELINE;
            break;

        // Stand up pipeline nodes.
        case ZDJ_DECK_STATUS_MAKE_PIPELINE:
            // printf( "ZDJ_EXT_DECK_STATUS_MAKE_PIPELINE\n" );
            deck->status = ZDJ_DECK_STATUS_WAIT_THREAD_AVAIL;
            break;
            
        // Make sure station 1 doesn't already have a thread running.
        // If it does, assume it's in the process of exiting,
        // and keep polling here until it becomes available.
        case ZDJ_DECK_STATUS_WAIT_THREAD_AVAIL:
            // printf( "ZDJ_EXT_DECK_STATUS_WAIT_THREAD_AVAIL\n" );
            deck->status = ZDJ_DECK_STATUS_WAIT_THREAD_READY;
            break;

        // Wait for new deck thread to finish filling its buffers.
        case ZDJ_DECK_STATUS_WAIT_THREAD_READY:
            // printf( "ZDJ_EXT_DECK_STATUS_WAIT_THREAD_READY\n" );             
            // Start serving deck samples to soundcard.
            zdj_soundcard_link_deck( zdj_soundcard, deck );
            // Mute the headphone cue channel
            zdj_soundcard_node_t * node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_CUE );
            node->dsp_dto->mute = true;
            // Accept control events when we're ready for playback
            deck->handle_control_event = &_handle_control;
            // Advance state to running
            deck->status = ZDJ_DECK_STATUS_RUNNING; 
            // Call UI unload CB
            if( deck->ui_load_cb ){ deck->ui_load_cb( deck ); }
            break;

        // Ignore new control events and start spooldown of deck drive/transport model.
        case ZDJ_DECK_STATUS_STOP_TRANSPORT:
            // printf( "ZDJ_DECK_STATUS_STOP_TRANSPORT\n" );
            // Immediately stop accepting control events.
            deck->handle_control_event = NULL;
            
            // Stop clock output

            deck->status = ZDJ_DECK_STATUS_WAIT_SPOOLDOWN; 
            // Call UI unload CB
            if( deck->ui_unload_cb ){ deck->ui_unload_cb( deck ); }
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

static void _get_edge_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo ) {
    zdj_deck_t * deck = (zdj_deck_t*)_deck;
    zdj_ext_deck_state_t * deck_state = (zdj_ext_deck_state_t*)deck->state;
    // printf( "ext get edge data: %p->%p\n", deck_state, deck_state->dsp_node );

    // Get a reference to external deck input node's buffer
    zdj_soundcard_node_t * ext_input_node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT );
    float * input_buf = ext_input_node->data_pipe->get_data( ext_input_node->data_pipe );
    // printf( "deck in: %p %p\n", input_buf, deck_state->dsp_node );

    // Get a reference to the soundcard deck edge buffer which needs filling.
    float * out_buf = data_pipe->get_data( data_pipe );

    // Push the input samples through the DSP node to the output buffer
    // zdj_deck_dsp_node_pull_buffer( deck_state->dsp_node, out_buf, stereo+1, ZDJ_SOUNDCARD_BUF_LEN );

    // printf( "data pipe out: %f\n", out_buf[ 4 ] );

    // printf( "dj get edge data done\n" );
    // Release another cycle of the pipeline_thread_main below.
    // sem_post( &deck_state->start_cycle );
}

// We get this call after soundcard accums everything into Deck Ext Input
static void _push_edge_data( void * _deck, zdj_pipeline_node_t * data_pipe, bool stereo ) {
    
    zdj_deck_t * deck = (zdj_deck_t*)_deck;
    zdj_ext_deck_state_t * deck_state = (zdj_ext_deck_state_t*)deck->state;
    // printf( "ext push edge data: %p->%p\n", deck_state, deck_state->dsp_node );
    // Exit early if we haven't stood up the dsp node yet.
    // if( !deck_state->dsp_node ) { return; }

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
    // printf( "extern _handle_control\n" );

    // Make sure soundcard isn't in reset before processing inputs
    if( zdj_soundcard->state != ZDJ_SOUNDCARD_STATE_RUNNING ){ return; }

    zdj_ext_deck_state_t * deck_state = (zdj_ext_deck_state_t*)deck->state;
    zdj_soundcard_node_t * node;
    zdj_soundcard_dsp_dto_t * dsp_dto;
    zdj_soundcard_dsp_stage_dto_t * dsp_stage;

    switch ( event->id ) {

    //////////////////
    // Play / Pause //
    //////////////////  
     
    case ZDJ_DECK_EXT_CONTROL_PLAY_PAUSE:
        // Play - start clock synth
       
        
        // Pause - stop clock synth
        
        printf( "ext deck toggle play/pause\n" );
        break;


    ////////////
    // Hotcue //
    ////////////  

    ///////////
    // Scrub //
    ///////////  

    case ZDJ_DECK_EXT_CONTROL_SCRUB:
        // Nudge clock synth

        break;


    ///////////
    // Tempo //
    ///////////  

    case ZDJ_DECK_EXT_CONTROL_TEMPO:
        // Tempo will ultimately control ext. deck buffer playback
        // if( zdj_deck_manager( )->sync.active ) {
        //     // If we're synced, update the overall tempo
        //     zdj_deck_manager_update_sync_bpm( event->i_val * 1 );
        // } else {
        //     deck->offset_sync_bpm( deck, event->i_val * 1 );
        // }
        break;

    case ZDJ_DECK_EXT_CONTROL_TEMPO_FINE:
        // Tempo will ultimately control ext. deck buffer playback
        // if( zdj_deck_manager( )->sync.active ) {
        //     // If we're synced, update the overall tempo
        //     zdj_deck_manager_update_sync_bpm( event->i_val * 0.01 );
        // } else {
        //     deck->offset_sync_bpm( deck, event->i_val * 0.01 );
        // }
        break;


    /////////////////
    // EQ + Filter //
    /////////////////

    case ZDJ_DECK_EXT_CONTROL_EQ_LO:
        node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE );
        dsp_dto = node->dsp_dto;
        dsp_stage = dsp_dto->get_stage_for_type( node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ );
        if( dsp_stage ) { dsp_stage->adjust_knob( dsp_stage, 0, event->i_val ); }
        // printf( "dj deck ext eq lo\n" );
        break;
    
    case ZDJ_DECK_EXT_CONTROL_EQ_MID:
        node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE );
        dsp_dto = node->dsp_dto;
        dsp_stage = dsp_dto->get_stage_for_type( node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ );
        if( dsp_stage ) { dsp_stage->adjust_knob( dsp_stage, 1, event->i_val ); }
        // printf( "ext deck eq mid\n" );
        break;

    case ZDJ_DECK_EXT_CONTROL_EQ_HI:
        node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE );
        dsp_dto = node->dsp_dto;
        dsp_stage = dsp_dto->get_stage_for_type( node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ );
        if( dsp_stage ) { dsp_stage->adjust_knob( dsp_stage, 2, event->i_val ); }
        // printf( "ext deck eq hi\n" );
        break;

    case ZDJ_DECK_EXT_CONTROL_FILTER_0:
        // printf( "deck filter 0\n" );
        node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE );
        dsp_dto = node->dsp_dto;
        dsp_stage = dsp_dto->get_stage_for_type( node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_FILT );
        if( dsp_stage ) { 
            dsp_stage->adjust_knob( dsp_stage, 0, event->i_val ); 
        }
        break;
    
    case ZDJ_DECK_EXT_CONTROL_FILTER_2:
        // printf( "dj deck filter 2\n" );
        node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE );
        dsp_dto = node->dsp_dto;
        dsp_stage = dsp_dto->get_stage_for_type( node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_FILT );
        if( dsp_stage ) { dsp_stage->adjust_knob( dsp_stage, 2, event->i_val ); }
        break;

    case ZDJ_DECK_EXT_CONTROL_FILTER_RESET:
        // printf( "dj deck filter reset\n" );
        node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE );
        dsp_dto = node->dsp_dto;
        dsp_stage = dsp_dto->get_stage_for_type( node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_FILT );
        if( dsp_stage ) { dsp_stage->set_knob( dsp_stage, 0, 0.0 ); }
        break;

    ///////////////////////
    // Fade / Trim / Cue //
    ///////////////////////  

    case ZDJ_DECK_CONTROL_XFADE:
        // printf( "ext deck xfad\n" );
        break;
    
    case ZDJ_DECK_EXT_CONTROL_TRIM:
        // printf( "ext deck trim\n" );
        node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE );
        node->dsp_dto->adjust_gain( node, event->i_val );
        event->blocked = true;
        break;

    case ZDJ_DECK_EXT_CONTROL_PFL_TRIM:
        // printf( "ext deck pfl trim\n" );
        node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_CUE );
        node->dsp_dto->adjust_gain( node, event->i_val );
        event->blocked = true;
        break;
    
    case ZDJ_DECK_EXT_CONTROL_PFL_TOGGLE_MUTE:
        // printf( "ext deck pfl mute\n" );
        node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_CUE );
        node->dsp_dto->toggle_mute( node );
        event->blocked = true;
        break;


    //////////
    // Sync //
    //////////  

    case ZDJ_DECK_CONTROL_SYNC_TOGGLE:
        if( zdj_deck_manager( )->sync.preferred ) {
            zdj_deck_manager_set_prefer_sync( false );
        } else {
            zdj_deck_manager_set_prefer_sync( true );
        }
        break;

    case ZDJ_DECK_EXT_CONTROL_SYNC_MULT:
        // if( deck->can_sync ) { request_sync_mult( event->i_val ); }
        break;


    ////////
    // FX //
    ////////  

    case ZDJ_DECK_EXT_CONTROL_FX_SELECT:
        break;

    case ZDJ_DECK_EXT_CONTROL_FX_0:
        break;
    
    case ZDJ_DECK_EXT_CONTROL_FX_1:
        break;

    case ZDJ_DECK_EXT_CONTROL_FX_2:
        break;

    case ZDJ_DECK_EXT_CONTROL_FX_3:
        break;

    case ZDJ_DECK_EXT_CONTROL_FX_4:
        break;

    case ZDJ_DECK_EXT_CONTROL_FX_5:
        break;

    default:
        break;
    }
}
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <zerodj/signal/deck/zdj_deck_manager.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/analysis/waveform/zdj_waveform.h>
#include <zerodj/signal/pipeline/node/audio/buffer/zdj_audio_buffer_node.h>
#include <zerodj/signal/pipeline/node/audio/io/zdj_io_node.h>
#include <zerodj/signal/pipeline/node/audio/record/zdj_audio_record_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/signal/soundcard/zdj_soundcard_dto.h>
#include <zerodj/system/sql/zdj_sql.h>

zdj_soundcard_t * zdj_soundcard;

void _zdj_soundcard_io_fast_cycle_cb( zdj_pipeline_node_t * node );

zdj_error_type_t zdj_soundcard_init( char * entity_id ) {
    // printf( "zdj_soundcard_init: %s\n", entity_id );
    zdj_soundcard_t * soundcard = calloc( 1, sizeof( zdj_soundcard_t ) );
    soundcard->has_edits = false;
    zdj_soundcard = soundcard;

    // Bring up the M7's soundcard + shared buffers.
    soundcard->analog_io_node = zdj_new_io_analog_node( );
    zdj_io_analog_node_state_t * io_node_state = (zdj_io_analog_node_state_t*)
    soundcard->analog_io_node->state;
    soundcard->analog_io_node->update_cb = &_zdj_soundcard_io_fast_cycle_cb;
    zdj_io_analog_configure( soundcard->analog_io_node );

    if( entity_id ) {
        // If explicitly asked, bring up a specific record from the soundcard db.
        zdj_soundcard_fetch_dto( entity_id, &soundcard->dto );
    } else {
        // Else bring up the sound card with the last saved state of the __temp__ record.
        zdj_soundcard_fetch_dto( "__temp__", &soundcard->dto );
    }

    // Create nodes for everything
    for( int i=ZDJ_SOUNDCARD_NODE_NAME_NONE; i<ZDJ_SOUNDCARD_NODE_NAME_COUNT; i++ ) {
        // printf( "making node: %s\n", zdj_soundcard_node_name[ i ] );
        zdj_soundcard_node_t * node = zdj_soundcard_create_node( i );
        zdj_soundcard_install_node( soundcard, node );
    }

    // Link analog_io pipeline node to io soundcard nodes
    io_node_state->out_1_buffer = zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0 )->data_pipe;
    io_node_state->out_2_buffer = zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2 )->data_pipe;
    io_node_state->in_1_buffer = zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0 )->data_pipe;
    zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1 )->data_pipe = io_node_state->in_1_buffer;
    io_node_state->in_2_buffer = zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2 )->data_pipe;
    zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3 )->data_pipe = io_node_state->in_2_buffer;

    // Create a waveform pipeline node to process o-scope data.
    soundcard->scope_waveform = zdj_new_live_waveform( );
    soundcard->scope_node_name = ZDJ_SOUNDCARD_NODE_NAME_NONE;

    // Bring up/link an audio recording pipeline node.
    soundcard->recording_node = zdj_new_audio_record_node( zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS ) );

    // Start the transport pipeline
    zdj_soundcard_start( soundcard );

    return ZDJ_ERROR_OKAY;
}

// Re-init the soundcard with a saved record in the db.
zdj_error_type_t zdj_soundcard_load( zdj_soundcard_t * soundcard, char * entity_id ) {

}

// Write the current soundcard state to a record in the db.
zdj_error_type_t zdj_soundcard_save( zdj_soundcard_t * soundcard, char * entity_id ) {
    // Push all node params down to the DTO, then store the DTO in the temp record.
    for( int i=ZDJ_SOUNDCARD_NODE_NAME_NONE; i<ZDJ_SOUNDCARD_NODE_NAME_COUNT; i++ ) {
        zdj_soundcard_node_t * node = zdj_soundcard_get_node_for_name( soundcard, i );
        zdj_soundcard_dto_set_sigtype_for_node_name( &soundcard->dto, i, node->signal_type );
        zdj_soundcard_dto_set_gain_for_node_name( &soundcard->dto, i, node->gain );
        zdj_soundcard_dto_set_mute_for_node_name( &soundcard->dto, i, node->mute );
        zdj_soundcard_dto_set_pan_for_node_name( &soundcard->dto, i, node->pan );
        zdj_soundcard_dto_set_source_for_node_name( &soundcard->dto, i, node->source );
        zdj_soundcard_dto_set_stereo_for_node_name( &soundcard->dto, i, node->stereo );
        zdj_soundcard_dto_set_val_for_node_name( &soundcard->dto, i, node->val );
        zdj_soundcard_dto_set_invert_for_node_name( &soundcard->dto, i, node->invert );
    }
    zdj_soundcard_store_dto( entity_id, &soundcard->dto );
}

zdj_error_type_t zdj_soundcard_save_temp( zdj_soundcard_t * soundcard ) {
    zdj_soundcard_save( soundcard, "__temp__" );
}

// Signal M7 soundcard to start DAC/ADC transport cycle.
// Begin polling for 'cycle_ready' messages from M7 soundcard.
zdj_error_type_t zdj_soundcard_start( zdj_soundcard_t * soundcard ) {
    zdj_io_analog_run( soundcard->analog_io_node );
}

// Signal M7 soundcard to stop DAC/ADC transport cycle.
zdj_error_type_t zdj_soundcard_stop( zdj_soundcard_t * soundcard ) {
    zdj_io_analog_stop( soundcard->analog_io_node );
}

// M7 soundcard has signaled that there is a new cycle of buffers available for processing.
// This CB sets the cadence for Soundcard's fast-cycle - finish before next cycle or we stutter.
void _zdj_soundcard_io_fast_cycle_cb( zdj_pipeline_node_t * node ) {


    // // Transform output samples from io_node's output float buffers to shared M7 buffers
    // zdj_analog_io_push_samples( zdj_soundcard->analog_io_node );

    // Note this is currently running as a 'single-buffered' op.
    // New cycle's data isn't available until after full soundcard has been mixed + DSP'd.
    // If we get into underrun trouble, go to 'double-buffered' model, immediately
    // copying last cycle's outut to shared M7 buffer before processing next cycle.

    // Run the deck control update cycles.
    // These are directly coupled to the buffer so they must be run here.
    zdj_deck_manager_control_update_cycle( );

    // Reset all node buffers and mix_complete flags for this cycle.
    for( int i=ZDJ_SOUNDCARD_NODE_NAME_NONE; i<ZDJ_SOUNDCARD_NODE_NAME_COUNT; i++ ) {
        zdj_soundcard_clear_buffer( 
            zdj_soundcard, 
            zdj_soundcard_get_node_for_name( zdj_soundcard, i )
        );
    }

    // Transform input samples from shared M7 buffer to io_node's input float buffers
    zdj_analog_io_pull_samples( zdj_soundcard->analog_io_node );

    // mix_inputs will recursively walk the graph of input nodes for a single node.
    // How to deal with stereo output channels? - do we need to?
    zdj_soundcard_node_t * ana_out_0 = zdj_soundcard_get_node_for_name( zdj_soundcard,ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0 );
    zdj_soundcard_node_t * ana_out_1 = zdj_soundcard_get_node_for_name( zdj_soundcard,ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1 );
    zdj_soundcard_mix_input( zdj_soundcard, ana_out_0 );
    if( !ana_out_0->stereo ) { zdj_soundcard_mix_input( zdj_soundcard, ana_out_1 ); }

    zdj_soundcard_node_t * ana_out_2 = zdj_soundcard_get_node_for_name( zdj_soundcard,ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2 );
    zdj_soundcard_node_t * ana_out_3 = zdj_soundcard_get_node_for_name( zdj_soundcard,ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3 );
    zdj_soundcard_mix_input( zdj_soundcard, ana_out_2 );
    if( !ana_out_3->stereo ) { zdj_soundcard_mix_input( zdj_soundcard, ana_out_3 ); }

    // Explicitly mix inputs to persistent nodes - Main bus, Cue, Decks, etc.
    for( int i=ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS; i<ZDJ_SOUNDCARD_NODE_NAME_COUNT; i++ ) {
        zdj_soundcard_node_t * bus = zdj_soundcard_get_node_for_name( zdj_soundcard, i );
        zdj_soundcard_mix_input( zdj_soundcard, bus );
    }

    // zdj_soundcard_mix_inputs( zdj_soundcard_get_node_for_name( 
    //     zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_USB_OUT
    // ) );

    // Transform output samples from io_node's output float buffers to shared M7 buffers
    zdj_analog_io_push_samples( zdj_soundcard->analog_io_node );

    // Update the audio recording node - periodically flush samples to file
    zdj_soundcard->recording_node->update_wait( zdj_soundcard->recording_node );
}


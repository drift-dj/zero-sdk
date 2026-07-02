#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <zerodj/signal/deck/zdj_deck_manager.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/analysis/waveform/zdj_waveform.h>
#include <zerodj/signal/pipeline/node/audio/buffer/zdj_audio_buffer_node.h>
#include <zerodj/signal/pipeline/node/audio/io/zdj_io_node.h>
#include <zerodj/signal/pipeline/node/audio/record/zdj_audio_record_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/signal/soundcard/db/zdj_soundcard_dto.h>
#include <zerodj/signal/soundcard/usb/zdj_soundcard_usb.h>
#include <zerodj/system/sql/zdj_sql.h>
#include <zerodj/system/usb/zdj_usb.h>
#include <zerodj/ui/widget/zdj_ui_widget.h>

zdj_soundcard_t * zdj_soundcard;

void _zdj_soundcard_io_fast_cycle_cb( zdj_pipeline_node_t * node );
void _zdj_soundcard_io_reset_cycle_cb( zdj_pipeline_node_t * node );

zdj_error_type_t zdj_soundcard_init( char * entity_id ) {
    // printf( "zdj_soundcard_init 0: %s\n", entity_id );
    
    // Create DB if it doesn't exist
    if( access( ZDJ_SOUNDCARD_DB_PATH, F_OK ) != 0 ) { zdj_drop_soundcard( ); }
    
    zdj_soundcard_t * soundcard = calloc( 1, sizeof( zdj_soundcard_t ) );
    soundcard->has_edits = false;
    zdj_soundcard = soundcard;

    soundcard->state = ZDJ_SOUNDCARD_STATE_INIT;

    // Bring up the M7's soundcard + shared buffers.
    soundcard->analog_io_node = zdj_new_io_analog_node( );
    zdj_io_analog_node_state_t * io_node_state = (zdj_io_analog_node_state_t*)soundcard->analog_io_node->state;
    soundcard->analog_io_node->update_cb = &_zdj_soundcard_io_fast_cycle_cb;
    zdj_io_analog_configure( soundcard->analog_io_node );

    // Bring up the USB I/O node
    soundcard->usb_io_node = zdj_new_io_usb_node( );
    zdj_io_usb_node_state_t * io_usb_node_state = (zdj_io_usb_node_state_t*)soundcard->usb_io_node->state;

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
    io_node_state->in_2_buffer = zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2 )->data_pipe;

    // Link usb_io pipeline node to io soundcard nodes
    io_usb_node_state->soundcard_out_buffer = zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_0 )->data_pipe;
    io_usb_node_state->soundcard_in_buffer = zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_USB_IN_0 )->data_pipe;
    zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_USB_IN_1 )->data_pipe = io_usb_node_state->soundcard_in_buffer;

    // Bring up/link an audio recording pipeline node.
    soundcard->recording_node = zdj_new_audio_record_node( zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS ) );

    // Bring up a clock deck and obey any linkage in current soundcard
    soundcard->clock_deck = zdj_deck_manager_add_deck( ZDJ_DECK_TYPE_XPORT, ZDJ_DECK_STATION_XPORT, NULL, 0 );
    zdj_soundcard_init_clock_deck( soundcard );

    // Bring up USB admin thread
    zdj_soundcard_launch_usb_admin_thread( soundcard );


    // Start the transport pipeline
    zdj_soundcard_start( soundcard );

    soundcard->state = ZDJ_SOUNDCARD_STATE_RUNNING;

    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t zdj_soundcard_deinit( zdj_soundcard_t * soundcard ) {
    if( !soundcard ) { return ZDJ_ERROR_OKAY; }
    // printf( "zdj_soundcard_deinit\n" );
    zdj_soundcard->state = ZDJ_SOUNDCARD_STATE_TEARDOWN;
    // Silence DAC buffers
    zdj_io_analog_stop( soundcard->analog_io_node );
    zdj_io_analog_silence( soundcard->analog_io_node );
    // Release DTO

    // Null out zdj_soundcard
    zdj_soundcard = NULL;
}

// THREAD-SAFE - May be called from UI thread
// Re-init the soundcard with a saved record in the db.
zdj_error_type_t zdj_soundcard_load_mixer( zdj_soundcard_t * soundcard, char * entity_id ) {
    // Bug out early if entity_id doesn't exist
    if( !zdj_soundcard_check_dto_exists_in_db( entity_id ) ) { 
        printf( "entity not found: %s\n", entity_id );
        return ZDJ_ERROR_SOUNDCARD_DB_NO_RECORD; 
    }

    // Save the entity_id for the reset cycle
    strcpy( zdj_soundcard->load_entity_id, entity_id );
    
    // Set soundcard state to reset so UI doesn't try to draw meters.
    zdj_soundcard->state = ZDJ_SOUNDCARD_STATE_TEARDOWN;

    // Redirect analog io node CB to reset
    soundcard->analog_io_node->update_cb = &_zdj_soundcard_io_reset_cycle_cb;
}

// Write the current soundcard state to a record in the db.
zdj_error_type_t zdj_soundcard_save( zdj_soundcard_t * soundcard, char * entity_id ) {
    // Push all node params down to the DTO, then store the DTO in the temp record.
    for( int i=ZDJ_SOUNDCARD_NODE_NAME_NONE; i<ZDJ_SOUNDCARD_NODE_NAME_COUNT; i++ ) {
        zdj_soundcard_node_t * node = zdj_soundcard_get_node_for_name( soundcard, i );
        zdj_soundcard_dto_set_sigtype_for_node_name( &soundcard->dto, i, node->signal_type );
        zdj_soundcard_dto_set_mute_for_node_name( &soundcard->dto, i, node->mute );
        zdj_soundcard_dto_set_source_for_node_name( &soundcard->dto, i, node->direction );
        zdj_soundcard_dto_set_stereo_for_node_name( &soundcard->dto, i, node->stereo );
        zdj_soundcard_dto_set_val_for_node_name( &soundcard->dto, i, node->val );
        zdj_soundcard_dto_update_dsp_for_node_name( &soundcard->dto, i );
    }
    zdj_soundcard_store_dto( &soundcard->dto );
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
    // Transform output samples from io_node's output float buffers to shared M7 buffers
    zdj_analog_io_push_samples( zdj_soundcard->analog_io_node );
    if( zdj_usb_state &&
        zdj_usb_state->mode_state.mode == ZDJ_USB_MODE_HOST &&
        zdj_usb_state->host_state.attached.count > 0
    ) {
        zdj_usb_io_push_samples( zdj_soundcard->usb_io_node );
    }
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
    
    if( zdj_usb_state &&
        zdj_usb_state->mode_state.mode == ZDJ_USB_MODE_HOST &&
        zdj_usb_state->host_state.attached.count > 0
    ) {
        zdj_usb_io_pull_samples( zdj_soundcard->usb_io_node );
    }

    // mix_inputs will recursively walk the graph of input nodes for a single node.
    // How to deal with stereo output channels? - do we need to?
    zdj_soundcard_node_t * ana_out_0 = zdj_soundcard_get_node_for_name( zdj_soundcard,ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0 );
    zdj_soundcard_node_t * ana_out_1 = zdj_soundcard_get_node_for_name( zdj_soundcard,ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1 );
    zdj_soundcard_mix_input( zdj_soundcard, ana_out_0 );
    if( !ana_out_0->stereo ) { 
        zdj_soundcard_mix_input( zdj_soundcard, ana_out_1 ); 
    }

    zdj_soundcard_node_t * ana_out_2 = zdj_soundcard_get_node_for_name( zdj_soundcard,ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2 );
    zdj_soundcard_node_t * ana_out_3 = zdj_soundcard_get_node_for_name( zdj_soundcard,ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3 );
    zdj_soundcard_mix_input( zdj_soundcard, ana_out_2 );
    if( !ana_out_3->stereo ) { zdj_soundcard_mix_input( zdj_soundcard, ana_out_3 ); }

    // Explicitly mix inputs to persistent nodes - Main bus, Cue, Decks, etc.
    for( int i=ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0; i<ZDJ_SOUNDCARD_NODE_NAME_COUNT; i++ ) {
        zdj_soundcard_node_t * bus = zdj_soundcard_get_node_for_name( zdj_soundcard, i );
        zdj_soundcard_mix_input( zdj_soundcard, bus );
    }

    // Update the audio recording node - periodically flush samples to file
    zdj_soundcard->recording_node->update_wait( zdj_soundcard->recording_node );

    // If there's a new attached USB device, and it has completed ALSA bringup,
    // start the ALSA loop on this thread
    if( zdj_usb_state &&
        zdj_usb_state->mode_state.mode == ZDJ_USB_MODE_HOST &&
        zdj_soundcard->usb_io_node 
    ) {
        zdj_io_usb_node_state_t * usb_node_state = (zdj_io_usb_node_state_t*)zdj_soundcard->usb_io_node ->state;
        if( usb_node_state->phase == ZDJ_IO_USB_PHASE_READY ) {
            zdj_io_usb_alsa_start( zdj_soundcard->usb_io_node );
        }
    }
}


// Perform a Hot Reset of the entire soundcard linkage
// Silence all I/O buffers then rebuild Soundcard linkage
void _zdj_soundcard_io_reset_cycle_cb( zdj_pipeline_node_t * node ) {
    // Unlink the CB so we don't get re-invoked while we're working
    zdj_soundcard->analog_io_node->update_cb = NULL;

    // Mute the analog IO while we're doing this - it may take a couple cycles
    zdj_io_analog_silence( zdj_soundcard->analog_io_node );

    // Unlink and free all current nodes
    zdj_soundcard_remove_all_nodes( zdj_soundcard );

    zdj_soundcard->state = ZDJ_SOUNDCARD_STATE_INIT;

    // Fetch the new DTO, or default to standard mixer
    if( zdj_soundcard_check_dto_exists_in_db( zdj_soundcard->load_entity_id ) ) {
        // If explicitly asked, bring up a specific record from the soundcard db.
        zdj_soundcard_fetch_dto( zdj_soundcard->load_entity_id, &zdj_soundcard->dto );
    } else {
        // Else bring up the Default soundcard
        char entity_id[ 128 ];
        zdj_error_type_t res = zdj_soundcard_put_default_entity_id( entity_id, &zdj_soundcard->dto );
        if( res == ZDJ_ERROR_OKAY ) {
            zdj_soundcard_fetch_dto( entity_id, &zdj_soundcard->dto );
        } else {
            printf( "Failed to find Default Soundcard, loading __temp__\n" );
            zdj_soundcard_fetch_dto( "__temp__", &zdj_soundcard->dto );
        }
    }

    // Link up new nodes
    for( int i=ZDJ_SOUNDCARD_NODE_NAME_NONE; i<ZDJ_SOUNDCARD_NODE_NAME_COUNT; i++ ) {
        zdj_soundcard_node_t * node = zdj_soundcard_create_node( i );
        zdj_soundcard_install_node( zdj_soundcard, node );
        zdj_soundcard_pull_node_links_from_dto( zdj_soundcard, node );
    }

    // Link analog_io pipeline node to io soundcard nodes
    zdj_io_analog_node_state_t * io_node_state = (zdj_io_analog_node_state_t*)zdj_soundcard->analog_io_node->state;
    io_node_state->out_1_buffer = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0 )->data_pipe;
    io_node_state->out_2_buffer = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2 )->data_pipe;
    io_node_state->in_1_buffer = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0 )->data_pipe;
    io_node_state->in_2_buffer = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2 )->data_pipe;

    // Link usb_io pipeline node to io soundcard nodes
    zdj_io_usb_node_state_t * io_usb_node_state = (zdj_io_usb_node_state_t*)zdj_soundcard->usb_io_node->state;
    io_usb_node_state->soundcard_out_buffer = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_0 )->data_pipe;
    io_usb_node_state->soundcard_in_buffer = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_USB_IN_0 )->data_pipe;
    zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_USB_IN_1 )->data_pipe = io_usb_node_state->soundcard_in_buffer;

    // Re-link the audio recording pipeline node.
    zdj_audio_record_node_state_t * record_state = (zdj_audio_record_node_state_t*)zdj_soundcard->recording_node->state;
    record_state->soundcard_node = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS );

    zdj_soundcard_init_clock_deck( zdj_soundcard );

    // Re-link any existing Decks
    zdj_deck_t * deck_1 = zdj_deck_manager_get_deck_for_station( ZDJ_DECK_STATION_1 );
    if( deck_1 ) { zdj_soundcard_link_deck( zdj_soundcard, deck_1 ); }
    zdj_deck_t * deck_2 = zdj_deck_manager_get_deck_for_station( ZDJ_DECK_STATION_2 );
    if( deck_2 ) { zdj_soundcard_link_deck( zdj_soundcard, deck_2 ); }
    zdj_deck_t * deck_xport = zdj_deck_manager_get_deck_for_station( ZDJ_DECK_STATION_XPORT );
    if( deck_xport ) { zdj_soundcard_link_deck( zdj_soundcard, deck_xport ); }
    zdj_deck_t * deck_ext = zdj_deck_manager_get_deck_for_station( ZDJ_DECK_STATION_EXT );
    if( deck_ext ) { 
        zdj_soundcard_link_deck( zdj_soundcard, deck_ext ); 
        deck_ext->needs_ui_reset = true;
    }

    // Update any widgets
    zdj_ui_widget_update_soundcard( );

    // Get Fader/Crossfade Vals
    int fader_val;
    zdj_soundcard_node_t * deck_1_fade = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_POSTFADE );
    fader_val = zdj_hmi_input_get_fader_val( ZDJ_HMI_POT_0_CH_1 );
    deck_1_fade->dsp_dto->set_gain( deck_1_fade, 255 - fader_val );
    zdj_soundcard_node_t * deck_2_fade = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_POSTFADE );
    fader_val = zdj_hmi_input_get_fader_val( ZDJ_HMI_POT_1_CH_2 );
    deck_2_fade->dsp_dto->set_gain( deck_2_fade, 255 - fader_val );

    fader_val = zdj_hmi_input_get_fader_val( ZDJ_HMI_POT_2_XFADE );
    zdj_soundcard_node_t * node_a = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_XFADE_A );
    double a_curve_pow = node_a->dsp_dto->stages[ 0 ].knob_0 * 5.5;
    double a_input_val = (255.0 - (double)fader_val) / 255.0;
    double a_coeff = 1.0 - pow( a_input_val, a_curve_pow );

    zdj_soundcard_node_t * node_b = zdj_soundcard_get_node_for_name( zdj_soundcard, ZDJ_SOUNDCARD_NODE_NAME_XFADE_B );
    double b_curve_pow = node_b->dsp_dto->stages[ 0 ].knob_0 * 5.5;
    double b_input_val = (double)fader_val / 255.0;
    double b_coeff = 1.0 - pow( b_input_val, b_curve_pow );

    // FIXME - This is backwards
    node_a->dsp_dto->set_gain( node_a, round(b_coeff*255.0) );
    node_b->dsp_dto->set_gain( node_b, round(a_coeff*255.0) );

    // Push the xfade value back into the deck manager.
    // Used for deck lock status.
    zdj_deck_manager_update_xfade( (float)fader_val / 255.0 );

    zdj_soundcard->state = ZDJ_SOUNDCARD_STATE_RUNNING;

    // Store the new DTO as the Current DTO
    strcpy( zdj_soundcard->dto.entity_id, "__temp__" );
    strcpy( zdj_soundcard->dto.name, "Current" );
    zdj_soundcard_store_dto( &zdj_soundcard->dto );

    // Relink the CB
    zdj_soundcard->analog_io_node->update_cb = &_zdj_soundcard_io_fast_cycle_cb;
}
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/buffer/zdj_audio_buffer_node.h>
#include <zerodj/signal/pipeline/node/audio/io/zdj_io_node.h>
#include <zerodj/soundcard/zdj_soundcard.h>
#include <zerodj/soundcard/zdj_soundcard_dto.h>
#include <zerodj/sql/zdj_sql.h>

zdj_soundcard_t * zdj_soundcard;

zdj_error_type_t zdj_soundcard_init( char * entity_id ) {
    // printf( "zdj_soundcard_init: %s\n", entity_id );
    zdj_soundcard_t * soundcard = calloc( 1, sizeof( zdj_soundcard_t ) );

    if( entity_id ) {
        // If explicitly asked, bring up a specific record from the soundcard db.
        zdj_soundcard_fetch_dto( entity_id, &soundcard->dto );
    } else {
        // Else bring up the sound card with the last saved state of the __temp__ record.
        zdj_soundcard_fetch_dto( "__temp__", &soundcard->dto );
    }
    
    soundcard->has_edits = false;

    // Bring up the M7's soundcard + shared buffers.
    soundcard->analog_io_node = zdj_new_io_analog_node( );
    zdj_io_analog_node_state_t * io_node_state = (zdj_io_analog_node_state_t*)soundcard->analog_io_node->state;

    zdj_soundcard = soundcard;
    
    // Create nodes for everything
    for( int i=ZDJ_SOUNDCARD_NODE_NAME_NONE; i<ZDJ_SOUNDCARD_NODE_NAME_COUNT; i++ ) {
        zdj_soundcard_node_t * node = zdj_soundcard_create_node( i );
        zdj_soundcard_install_node( soundcard, node );
    }

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

void zdj_soundcard_set_stereo_for_node( zdj_soundcard_node_t * node, bool stereo ) {
    zdj_soundcard_node_t * stereo_partner_node = zdj_soundcard_node_get_stereo_partner_node( node );
    node->stereo = stereo;
    // zdj_soundcard_dto_set_stereo_for_node_name( node )
    if( stereo_partner_node ) { 
        // Link stereo
        stereo_partner_node->stereo = stereo; 
        // zdj_soundcard_dto_set_stereo_for_node_name( partner )

        // If stereo, join up the signals
        if( stereo ) { 
            stereo_partner_node->signal_type = node->signal_type;
            // zdj_soundcard_dto_set_sig_type_for_node_name( partner )
            stereo_partner_node->mute = node->mute;
            // zdj_soundcard_dto_set_mute_for_node_name( partner )
        }
    }
}

void zdj_soundcard_set_mute_for_node( zdj_soundcard_node_t * node, bool mute ) {
    node->mute = mute;
    // zdj_soundcard_dto_set_mute_for_node_name( node )
    if( node->stereo ) {
        zdj_soundcard_node_t * stereo_partner_node = zdj_soundcard_node_get_stereo_partner_node( node );
        if( stereo_partner_node ) { 
            stereo_partner_node->mute = mute;
            // zdj_soundcard_dto_set_mute_for_node_name( partner )
        }
    }
}


bool zdj_soundcard_can_add_aux_bus( zdj_soundcard_t * soundcard ) {
    if( !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0 )->link_count ||
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1 )->link_count ||
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2 )->link_count ||
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3 )->link_count 
    ) {
        return true;
    } else {
        return false;
    }
}

bool zdj_soundcard_can_add_clock_bus( zdj_soundcard_t * soundcard ) {
    if( !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_0 )->link_count ||
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_1 )->link_count ||
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_2 )->link_count ||
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_3 )->link_count 
    ) {
        return true;
    } else {
        return false;
    }
}

bool zdj_soundcard_can_add_cv_bus( zdj_soundcard_t * soundcard ) {
    if( !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_0 )->link_count ||
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_1 )->link_count ||
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_2 )->link_count ||
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_3 )->link_count 
    ) {
        return true;
    } else {
        return false;
    }
}

bool zdj_soundcard_can_add_midi_bus( zdj_soundcard_t * soundcard ) {
    // if( !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0 )->link_count ||
    //     !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1 )->link_count ||
    //     !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2 )->link_count ||
    //     !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3 )->link_count 
    // ) {
    //     return true;
    // } else {
        return false;
    // }
}

zdj_soundcard_node_t * zdj_soundcard_get_available_aux_bus_node( zdj_soundcard_t * soundcard ) {
    if( !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0 )->link_count 
    ) {
        return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0 );
    } else if( 
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1 )->link_count 
    ) {
        return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1 );
    } else if( 
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2 )->link_count 
    ) {
        return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2 );
    } else if( 
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3 )->link_count 
    ) {
        return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3 );
    }
}

zdj_soundcard_node_t * zdj_soundcard_get_available_clock_bus_node( zdj_soundcard_t * soundcard ) {
    if( !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_0 )->link_count 
    ) {
        return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_0 );
    } else if( 
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_1 )->link_count 
    ) {
        return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_1 );
    } else if( 
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_2 )->link_count 
    ) {
        return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_2 );
    } else if( 
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_3 )->link_count 
    ) {
        return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_3 );
    }
}

zdj_soundcard_node_t * zdj_soundcard_get_available_cv_bus_node( zdj_soundcard_t * soundcard ) {
    if( !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_0 )->link_count 
    ) {
        return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_0 );
    } else if( 
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_1 )->link_count 
    ) {
        return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_1 );
    } else if( 
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_2 )->link_count 
    ) {
        return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_2 );
    } else if( 
        !zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_3 )->link_count 
    ) {
        return zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_3 );
    }
}

zdj_soundcard_node_t * zdj_soundcard_get_available_midi_bus_node( zdj_soundcard_t * soundcard ) {
    return NULL;
}



// Step to next sig type for node.
zdj_error_type_t zdj_soundcard_cycle_pad_for_io_node(
    zdj_soundcard_t * soundcard,
    zdj_soundcard_node_t * node
) {
    if( zdj_soundcard_node_name_is_io( node->name ) ) {
        zdj_soundcard_signal_type_t sig_type = zdj_soundcard_dto_get_sigtype_for_node_name( 
            &soundcard->dto, node->name 
        );
        switch ( sig_type ){
            case ZDJ_SOUNDCARD_SIGNAL_CON_0_DBV:
                node->signal_type = ZDJ_SOUNDCARD_SIGNAL_PRO_PLUS_4_DBU;
                break;
            case ZDJ_SOUNDCARD_SIGNAL_PRO_PLUS_4_DBU:
                node->signal_type = ZDJ_SOUNDCARD_SIGNAL_EURO_AUDIO;
                break;
            case ZDJ_SOUNDCARD_SIGNAL_EURO_AUDIO:
                node->signal_type = ZDJ_SOUNDCARD_SIGNAL_CON_0_DBV;
                break;
        } 
        zdj_soundcard_dto_set_sigtype_for_node_name( 
            &soundcard->dto, node->name, node->signal_type 
        );
    }
}

bool zdj_soundcard_node_name_show_in_mixer( zdj_soundcard_node_name_t name ) {
    return true;
}

bool zdj_soundcard_node_name_has_buffer( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3: return false;
        default: return true;
    }
}

bool zdj_soundcard_node_name_is_audio( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS:
        case ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS:
        case ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS:
        case ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3:
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0:
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1:
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2:
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_BUS:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_BUS:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_BUS:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_io( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_physical_port( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_input( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_output( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS:
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2:
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_aux_bus( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0:
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1:
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2:
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_clock( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_0:
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_1:
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_2:
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_3: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_cv( zdj_soundcard_node_name_t name ) {
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_CV_0:
        case ZDJ_SOUNDCARD_NODE_NAME_CV_1:
        case ZDJ_SOUNDCARD_NODE_NAME_CV_2:
        case ZDJ_SOUNDCARD_NODE_NAME_CV_3: return true;
        default: return false;
    }
}

bool zdj_soundcard_node_name_is_muted( zdj_soundcard_node_name_t name ) {
    return false;
}

bool zdj_soundcard_node_name_is_midi( zdj_soundcard_node_name_t name ) {
    return false;
}

bool zdj_soundcard_node_name_is_usb( zdj_soundcard_node_name_t name ) {
    return false;
}


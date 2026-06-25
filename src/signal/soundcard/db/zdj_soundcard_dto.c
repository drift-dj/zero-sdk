#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <sqlite3.h>

#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/signal/soundcard/db/zdj_soundcard_dto.h>
#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/sql/zdj_sql.h>
#include <zerodj/system/uuid/zdj_uuid.h>

static void _populate_dsp_dto( zdj_soundcard_dsp_dto_t * dto, char * eid );

zdj_soundcard_dto_t * zdj_soundcard_create_dto( void ) {
    zdj_soundcard_dto_t * dto = calloc( 1, sizeof( zdj_soundcard_dto_t ) );
    zdj_put_uuid_no_dash( dto->entity_id );

    // Set DSPs to unity gain, no pan, no stages
    _populate_dsp_dto( &dto->ana_in_0_dsp, dto->ana_in_0_dsp_eid );
    _populate_dsp_dto( &dto->ana_in_1_dsp, dto->ana_in_1_dsp_eid );
    _populate_dsp_dto( &dto->ana_in_2_dsp, dto->ana_in_2_dsp_eid );
    _populate_dsp_dto( &dto->ana_in_3_dsp, dto->ana_in_3_dsp_eid );

    _populate_dsp_dto( &dto->usb_in_0_dsp, dto->usb_in_0_dsp_eid );
    _populate_dsp_dto( &dto->usb_in_1_dsp, dto->usb_in_1_dsp_eid );

    _populate_dsp_dto( &dto->main_bus_dsp, dto->main_bus_dsp_eid );
    _populate_dsp_dto( &dto->cue_bus_dsp, dto->cue_bus_dsp_eid );
    _populate_dsp_dto( &dto->annot_bus_dsp, dto->annot_bus_dsp_eid );
    _populate_dsp_dto( &dto->record_bus_dsp, dto->record_bus_dsp_eid );

    _populate_dsp_dto( &dto->deck_1_input_dsp, dto->deck_1_input_dsp_eid );
    _populate_dsp_dto( &dto->deck_1_prefade_dsp, dto->deck_1_prefade_dsp_eid );
    _populate_dsp_dto( &dto->deck_1_postfade_dsp, dto->deck_1_postfade_dsp_eid );
    _populate_dsp_dto( &dto->deck_1_cue_dsp, dto->deck_1_cue_dsp_eid );

    _populate_dsp_dto( &dto->deck_2_input_dsp, dto->deck_2_input_dsp_eid );
    _populate_dsp_dto( &dto->deck_2_prefade_dsp, dto->deck_2_prefade_dsp_eid );
    _populate_dsp_dto( &dto->deck_2_postfade_dsp, dto->deck_2_postfade_dsp_eid );
    _populate_dsp_dto( &dto->deck_2_cue_dsp, dto->deck_2_cue_dsp_eid );

    _populate_dsp_dto( &dto->deck_ext_input_dsp, dto->deck_ext_input_dsp_eid );
    _populate_dsp_dto( &dto->deck_ext_prefade_dsp, dto->deck_ext_prefade_dsp_eid );
    _populate_dsp_dto( &dto->deck_ext_postfade_dsp, dto->deck_ext_postfade_dsp_eid );
    _populate_dsp_dto( &dto->deck_ext_cue_dsp, dto->deck_ext_cue_dsp_eid );

    _populate_dsp_dto( &dto->aux_bus_0_dsp, dto->aux_bus_0_dsp_eid );
    _populate_dsp_dto( &dto->aux_bus_1_dsp, dto->aux_bus_1_dsp_eid );
    _populate_dsp_dto( &dto->aux_bus_2_dsp, dto->aux_bus_2_dsp_eid );
    _populate_dsp_dto( &dto->aux_bus_3_dsp, dto->aux_bus_3_dsp_eid );

    _populate_dsp_dto( &dto->cv_0_dsp, dto->cv_0_dsp_eid );
    _populate_dsp_dto( &dto->cv_1_dsp, dto->cv_1_dsp_eid );
    _populate_dsp_dto( &dto->cv_2_dsp, dto->cv_2_dsp_eid );
    _populate_dsp_dto( &dto->cv_3_dsp, dto->cv_3_dsp_eid );

    _populate_dsp_dto( &dto->xfade_a_dsp, dto->xfade_a_dsp_eid );
    _populate_dsp_dto( &dto->xfade_b_dsp, dto->xfade_b_dsp_eid );
    return dto;
}

zdj_error_type_t zdj_soundcard_copy_dto( 
    zdj_soundcard_dto_t * src_dto, 
    zdj_soundcard_dto_t * dst_dto 
) {
    memcpy( dst_dto, src_dto, sizeof( zdj_soundcard_dto_t ) );
}

static void _populate_dsp_dto( zdj_soundcard_dsp_dto_t * dto, char * eid ) {
    zdj_put_uuid_no_dash( dto->entity_id );
    strcpy( eid, dto->entity_id );
    dto->gain = 1.0;
    dto->pan = 1.0;
    dto->mute = 0.0;
    dto->has_stages = false;
}


uint64_t zdj_soundcard_dto_get_linkmap_for_node_name( 
    zdj_soundcard_dto_t * dto,
    int name
) {
    if( !dto ) { return 0; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0: return dto->ana_in_0_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1: return dto->ana_in_1_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2: return dto->ana_in_2_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3: return dto->ana_in_3_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_0: return dto->usb_in_0_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_1: return dto->usb_in_1_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS: return dto->main_bus_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS: return dto->cue_bus_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS: return dto->annot_bus_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0: return dto->aux_bus_0_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1: return dto->aux_bus_1_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2: return dto->aux_bus_2_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3: return dto->aux_bus_3_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_0: return dto->clock_0_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_1: return dto->clock_1_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_2: return dto->clock_2_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_3: return dto->clock_3_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_0: return dto->cv_0_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_1: return dto->cv_1_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_2: return dto->cv_2_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_3: return dto->cv_3_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_INPUT: return dto->deck_1_input_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_EDGE: return dto->deck_1_edge_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE: return dto->deck_1_prefade_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_POSTFADE: return dto->deck_1_postfade_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_CUE: return dto->deck_1_cue_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_INPUT: return dto->deck_2_input_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_EDGE: return dto->deck_2_edge_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE: return dto->deck_2_prefade_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_POSTFADE: return dto->deck_2_postfade_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_CUE: return dto->deck_2_cue_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT: return dto->deck_ext_input_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_EDGE: return dto->deck_ext_edge_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE: return dto->deck_ext_prefade_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_POSTFADE: return dto->deck_ext_postfade_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_CUE: return dto->deck_ext_cue_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_XFADE_A: return dto->xfade_a_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_XFADE_B: return dto->xfade_b_link_map;
        default: return 0;
    }
}

int zdj_soundcard_dto_set_linkmap_for_node_name( 
	zdj_soundcard_dto_t * dto, int name, uint64_t link_map 
) {
    if( !dto ) { return 0; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0: dto->ana_in_0_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1: dto->ana_in_1_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2: dto->ana_in_2_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3: dto->ana_in_3_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_0: dto->usb_in_0_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_1: dto->usb_in_1_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS: dto->main_bus_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS: dto->cue_bus_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS: dto->annot_bus_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0: dto->aux_bus_0_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1: dto->aux_bus_1_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2: dto->aux_bus_2_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3: dto->aux_bus_3_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_0: dto->clock_0_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_1: dto->clock_1_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_2: dto->clock_2_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_3: dto->clock_3_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_0: dto->cv_0_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_1: dto->cv_1_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_2: dto->cv_2_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_3: dto->cv_3_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_INPUT: dto->deck_1_input_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_EDGE: dto->deck_1_edge_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE: dto->deck_1_prefade_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_POSTFADE: dto->deck_1_postfade_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_CUE: dto->deck_1_cue_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_INPUT: dto->deck_2_input_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_EDGE: dto->deck_2_edge_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE: dto->deck_2_prefade_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_POSTFADE: dto->deck_2_postfade_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_CUE: dto->deck_2_cue_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT: dto->deck_ext_input_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_EDGE: dto->deck_ext_edge_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE: dto->deck_ext_prefade_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_POSTFADE: dto->deck_ext_postfade_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_CUE: dto->deck_ext_cue_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_XFADE_A: dto->xfade_a_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_XFADE_B: dto->xfade_b_link_map = link_map; break;
        default: break;
    }
}

int zdj_soundcard_dto_get_sigtype_for_node_name( 
    zdj_soundcard_dto_t * dto,
    int name
) {
    if( !dto ) { return 0; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0: return dto->ana_in_0_sig;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1: return dto->ana_in_1_sig;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2: return dto->ana_in_2_sig;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3: return dto->ana_in_3_sig;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0: return dto->ana_out_0_sig;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1: return dto->ana_out_1_sig;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2: return dto->ana_out_2_sig;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3: return dto->ana_out_3_sig;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_0: return dto->clock_0_sig;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_1: return dto->clock_1_sig;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_2: return dto->clock_2_sig;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_3: return dto->clock_3_sig;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_0: return dto->cv_0_sig;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_1: return dto->cv_1_sig;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_2: return dto->cv_2_sig;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_3: return dto->cv_3_sig;
        default: return ZDJ_SOUNDCARD_SIGNAL_INTERNAL_CANONICAL;
    }
}

int zdj_soundcard_dto_set_sigtype_for_node_name( 
    zdj_soundcard_dto_t * dto, 
    int name, 
    int val 
) {
    if( !dto ) { return 0; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0: dto->ana_in_0_sig = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1: dto->ana_in_1_sig = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2: dto->ana_in_2_sig = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3: dto->ana_in_3_sig = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0: dto->ana_out_0_sig = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1: dto->ana_out_1_sig = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2: dto->ana_out_2_sig = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3: dto->ana_out_3_sig = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_0: dto->clock_0_sig = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_1: dto->clock_1_sig = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_2: dto->clock_2_sig = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_3: dto->clock_3_sig = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_0: dto->cv_0_sig = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_1: dto->cv_1_sig = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_2: dto->cv_2_sig = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_3: dto->cv_3_sig = val; break;
        default: break;
    }
}

int zdj_soundcard_dto_get_stereo_for_node_name( 
    zdj_soundcard_dto_t * dto,
    int name
) {
    if( !dto ) { return 0; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_INPUT:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_EDGE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_POSTFADE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_CUE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_INPUT:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_EDGE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_POSTFADE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_CUE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_EDGE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE: 
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_POSTFADE: 
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_CUE:
        case ZDJ_SOUNDCARD_NODE_NAME_XFADE_A:
        case ZDJ_SOUNDCARD_NODE_NAME_XFADE_B: return 1;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0: return dto->ana_out_0_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1: return dto->ana_out_1_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2: return dto->ana_out_2_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3: return dto->ana_out_3_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0: return dto->ana_in_0_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1: return dto->ana_in_1_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2: return dto->ana_in_2_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3: return dto->ana_in_3_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_0: return dto->usb_out_0_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_1: return dto->usb_out_1_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_0: return dto->usb_in_0_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_1: return dto->usb_in_1_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS: return dto->main_bus_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS: return dto->cue_bus_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS: return dto->annot_bus_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS: return dto->record_bus_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0: return dto->aux_bus_0_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1: return dto->aux_bus_1_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2: return dto->aux_bus_2_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3: return dto->aux_bus_3_stereo;
        default: return 0;
    }
}

int zdj_soundcard_dto_set_stereo_for_node_name( 
    zdj_soundcard_dto_t * dto, 
    int name, 
    int val 
) {
    if( !dto ) { return 0; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0: dto->ana_out_0_stereo = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1: dto->ana_out_1_stereo = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2: dto->ana_out_2_stereo = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3: dto->ana_out_3_stereo = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0: dto->ana_in_0_stereo = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1: dto->ana_in_1_stereo = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2: dto->ana_in_2_stereo = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3: dto->ana_in_3_stereo = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_0: dto->usb_out_0_stereo = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_1: dto->usb_out_1_stereo = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_0: dto->usb_in_0_stereo = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_1: dto->usb_in_1_stereo = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS: dto->main_bus_stereo = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS: dto->cue_bus_stereo = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS: dto->annot_bus_stereo = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS: dto->record_bus_stereo = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0: dto->aux_bus_0_stereo = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1: dto->aux_bus_1_stereo = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2: dto->aux_bus_2_stereo = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3: dto->aux_bus_3_stereo = val; break;
        default: return 0;
    }
}

int zdj_soundcard_dto_get_mute_for_node_name( 
    zdj_soundcard_dto_t * dto,
    int name
) {
    if( !dto ) { return 0; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0: return dto->ana_in_0_mute;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1: return dto->ana_in_1_mute;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2: return dto->ana_in_2_mute;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3: return dto->ana_in_3_mute;
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_0: return dto->usb_in_0_mute;
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_1: return dto->usb_in_1_mute;
        case ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS: return dto->main_bus_mute;
        case ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS: return dto->cue_bus_mute;
        case ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS: return dto->annot_bus_mute;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0: return dto->aux_bus_0_mute;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1: return dto->aux_bus_1_mute;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2: return dto->aux_bus_2_mute;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3: return dto->aux_bus_3_mute;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_0: return dto->cv_0_mute;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_1: return dto->cv_1_mute;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_2: return dto->cv_2_mute;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_3: return dto->cv_3_mute;
        default: return 0;
    }
}

int zdj_soundcard_dto_set_mute_for_node_name( 
    zdj_soundcard_dto_t * dto, 
    int name, 
    int val 
) {
    if( !dto ) { return 0; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0: dto->ana_in_0_mute = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1: dto->ana_in_1_mute = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2: dto->ana_in_2_mute = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3: dto->ana_in_3_mute = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_0: dto->usb_in_0_mute = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_1: dto->usb_in_1_mute = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS: dto->main_bus_mute = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS: dto->cue_bus_mute = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS: dto->annot_bus_mute = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0: dto->aux_bus_0_mute = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1: dto->aux_bus_1_mute = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2: dto->aux_bus_2_mute = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3: dto->aux_bus_3_mute = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_0: dto->cv_0_mute = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_1: dto->cv_1_mute = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_2: dto->cv_2_mute = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_3: dto->cv_3_mute = val; break;
        default: return 0;
    }
}

int zdj_soundcard_dto_get_source_for_node_name( 
    zdj_soundcard_dto_t * dto, 
    int name 
) {
    if( !dto ) { return 0; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_0: return dto->clock_0_source;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_1: return dto->clock_1_source;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_2: return dto->clock_2_source;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_3: return dto->clock_3_source;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_0: return dto->cv_0_source;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_1: return dto->cv_1_source;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_2: return dto->cv_2_source;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_3: return dto->cv_3_source;
        default: return 0;
    }
}

int zdj_soundcard_dto_set_source_for_node_name( 
    zdj_soundcard_dto_t * dto, 
    int name, 
    int val 
) {
    if( !dto ) { return 0; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_0: dto->clock_0_source = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_1: dto->clock_1_source = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_2: dto->clock_2_source = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_3: dto->clock_3_source = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_0: dto->cv_0_source = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_1: dto->cv_1_source = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_2: dto->cv_2_source = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_3: dto->cv_3_source = val; break;
        default: return 0;
    }
}

double zdj_soundcard_dto_get_val_for_node_name( 
    zdj_soundcard_dto_t * dto, 
    int name 
) {
    if( !dto ) { return 0; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_0: return dto->clock_0_val;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_1: return dto->clock_1_val;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_2: return dto->clock_2_val;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_3: return dto->clock_3_val;
        default: return 0;
    }
}

void zdj_soundcard_dto_set_val_for_node_name( 
    zdj_soundcard_dto_t * dto, 
    int name, 
    double val 
) {
    if( !dto ) { return; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_0: dto->clock_0_val = (float)val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_1: dto->clock_1_val = (float)val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_2: dto->clock_2_val = (float)val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_3: dto->clock_3_val = (float)val; break;
        default: return;
    }
}

int zdj_soundcard_dto_get_sync_for_node_name( 
    zdj_soundcard_dto_t * dto, 
    int name 
) {
    if( !dto ) { return 0; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_0: return dto->clock_0_sync;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_1: return dto->clock_1_sync;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_2: return dto->clock_2_sync;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_3: return dto->clock_3_sync;
        default: return 0;
    }
}

int zdj_soundcard_dto_set_sync_for_node_name( 
    zdj_soundcard_dto_t * dto, 
    int name, 
    int val 
) {
    if( !dto ) { return 0; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_0: dto->clock_0_sync = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_1: dto->clock_1_sync = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_2: dto->clock_2_sync = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_XPORT_3: dto->clock_3_sync = val; break;
        default: return 0;
    }
}

void zdj_soundcard_dto_update_dsp_for_node_name( zdj_soundcard_dto_t * dto, int name ){ 

}

zdj_soundcard_dsp_dto_t * zdj_soundcard_dto_get_dsp_for_node_name( 
    zdj_soundcard_dto_t * dto, 
    int name 
) {
    if( !dto ) { return 0; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_INPUT: return &dto->deck_1_input_dsp; 
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE: return &dto->deck_1_prefade_dsp; 
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_POSTFADE: return &dto->deck_1_postfade_dsp; 
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_CUE: return &dto->deck_1_cue_dsp; 
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_INPUT: return &dto->deck_2_input_dsp; 
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE: return &dto->deck_2_prefade_dsp; 
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_POSTFADE: return &dto->deck_2_postfade_dsp; 
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_CUE: return &dto->deck_2_cue_dsp; 
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT: return &dto->deck_ext_input_dsp; 
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE: return &dto->deck_ext_prefade_dsp; 
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_POSTFADE: return &dto->deck_ext_postfade_dsp; 
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_CUE: return &dto->deck_ext_cue_dsp; 
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0: return &dto->ana_in_0_dsp;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1: return &dto->ana_in_1_dsp;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2: return &dto->ana_in_2_dsp;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3: return &dto->ana_in_3_dsp;
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_0: return &dto->usb_in_0_dsp;
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN_1: return &dto->usb_in_1_dsp;
        case ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS: return &dto->annot_bus_dsp;
        case ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS: return &dto->record_bus_dsp;
        case ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS: return &dto->main_bus_dsp;
        case ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS: return &dto->cue_bus_dsp;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0: return &dto->aux_bus_0_dsp;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1: return &dto->aux_bus_1_dsp;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2: return &dto->aux_bus_2_dsp;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3: return &dto->aux_bus_3_dsp;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_0: return &dto->cv_0_dsp;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_1: return &dto->cv_1_dsp;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_2: return &dto->cv_2_dsp;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_3: return &dto->cv_3_dsp;
        case ZDJ_SOUNDCARD_NODE_NAME_XFADE_A: return &dto->xfade_a_dsp;
        case ZDJ_SOUNDCARD_NODE_NAME_XFADE_B: return &dto->xfade_b_dsp;
        default: return NULL;
    }
}

void zdj_soundcard_put_dsp_state_for_dto( zdj_soundcard_dsp_dto_t * dto ) {
    // Add getters
    dto->get_stage_for_type = &zdj_soundcard_dto_get_dsp_stage_for_type;
    dto->get_stage_for_id = &zdj_soundcard_dto_get_dsp_stage_for_id;
    
    // Add gain/pan knob handlers  
    dto->set_gain = &zdj_soundcard_dsp_gain_set_knob;
    dto->adjust_gain = &zdj_soundcard_dsp_gain_adjust_knob;
    dto->get_gain_display_val = &zdj_soundcard_dsp_get_knob_display_val;
    dto->adjust_pan = &zdj_soundcard_dsp_pan_adjust_knob;
    dto->get_pan_display_val = &zdj_soundcard_dsp_get_knob_display_val;
    dto->toggle_mute = &zdj_soundcard_dsp_mute_toggle;

    // Fill in state for all DSP stages
    for( int i=0; i<8; i++ ) {
        // dto->stages[ i ].adjust_knob = zdj_soundcard_dsp_eq_adjust_knob;
        dto->stages[ i ].get_knob_display_val = zdj_soundcard_dsp_get_knob_display_val;

        switch ( dto->stages[ i ].id ) {
            case ZDJ_SOUNDCARD_DSP_ID_EQ_3_1P:
                zdj_soundcard_dsp_eq_3_1p_init( &dto->stages[ i ] );
                dto->stages[ i ].adjust_knob = zdj_soundcard_dsp_eq_adjust_knob;
                dto->stages[ i ].fn = zdj_soundcard_dsp_eq_3_1p_update;
                dto->stages[ i ].knob_0_model = ZDJ_SOUNDCARD_DSP_KNOB_12DB_GAIN_FAST; // Lo
                dto->stages[ i ].knob_1_model = ZDJ_SOUNDCARD_DSP_KNOB_12DB_GAIN_FAST; // Mid
                dto->stages[ i ].knob_2_model = ZDJ_SOUNDCARD_DSP_KNOB_12DB_GAIN_FAST; // Hi
                dto->stages[ i ].knob_3_model = ZDJ_SOUNDCARD_DSP_KNOB_0_1_SLOW; // Xover Lo
                dto->stages[ i ].knob_4_model = ZDJ_SOUNDCARD_DSP_KNOB_0_1_SLOW; // Xover Hi
                dto->has_stages = true;
                break;
            case ZDJ_SOUNDCARD_DSP_ID_EQ_3_2P:
                zdj_soundcard_dsp_eq_3_2p_init( &dto->stages[ i ] );
                dto->stages[ i ].adjust_knob = zdj_soundcard_dsp_eq_adjust_knob;
                dto->stages[ i ].fn = zdj_soundcard_dsp_eq_3_2p_update;
                dto->stages[ i ].knob_0_model = ZDJ_SOUNDCARD_DSP_KNOB_12DB_GAIN_FAST; // Lo
                dto->stages[ i ].knob_1_model = ZDJ_SOUNDCARD_DSP_KNOB_12DB_GAIN_FAST; // Mid
                dto->stages[ i ].knob_2_model = ZDJ_SOUNDCARD_DSP_KNOB_12DB_GAIN_FAST; // Hi
                dto->stages[ i ].knob_3_model = ZDJ_SOUNDCARD_DSP_KNOB_0_1_SLOW; // Xover Lo
                dto->stages[ i ].knob_4_model = ZDJ_SOUNDCARD_DSP_KNOB_0_1_SLOW; // Xover Hi
                dto->has_stages = true;
                break;
            case ZDJ_SOUNDCARD_DSP_ID_EQ_3_4P:
                zdj_soundcard_dsp_eq_3_4p_init( &dto->stages[ i ] );
                dto->stages[ i ].adjust_knob = zdj_soundcard_dsp_eq_adjust_knob;
                dto->stages[ i ].fn = zdj_soundcard_dsp_eq_3_4p_update;
                dto->stages[ i ].knob_0_model = ZDJ_SOUNDCARD_DSP_KNOB_12DB_GAIN_FAST; // Lo
                dto->stages[ i ].knob_1_model = ZDJ_SOUNDCARD_DSP_KNOB_12DB_GAIN_FAST; // Mid
                dto->stages[ i ].knob_2_model = ZDJ_SOUNDCARD_DSP_KNOB_12DB_GAIN_FAST; // Hi
                dto->stages[ i ].knob_3_model = ZDJ_SOUNDCARD_DSP_KNOB_0_1_SLOW; // Xover Lo
                dto->stages[ i ].knob_4_model = ZDJ_SOUNDCARD_DSP_KNOB_0_1_SLOW; // Xover Hi
                dto->has_stages = true;
                break;
            case ZDJ_SOUNDCARD_DSP_ID_XFADE_CURVE:
                // Curve just holds contour setting -- all processing is done via gain in parent dsp_dto
                dto->stages[ i ].knob_0_model = ZDJ_SOUNDCARD_DSP_KNOB_0_1_FAST; // Curve
                break;
            case ZDJ_SOUNDCARD_DSP_ID_FILT_BI:
                zdj_soundcard_dsp_filt_bi_init( &dto->stages[ i ] );
                dto->stages[ i ].adjust_knob = zdj_soundcard_dsp_filt_bi_adjust_knob;
                dto->stages[ i ].set_knob = zdj_soundcard_dsp_filt_bi_set_knob;
                dto->stages[ i ].fn = zdj_soundcard_dsp_filt_bi_update;
                dto->stages[ i ].knob_0_model = ZDJ_SOUNDCARD_DSP_KNOB_NEG_POS_1_FAST;
                dto->stages[ i ].knob_2_model = ZDJ_SOUNDCARD_DSP_KNOB_0_1_FAST;
                break;
            case ZDJ_SOUNDCARD_DSP_ID_DYN_COMP:
            case ZDJ_SOUNDCARD_DSP_ID_DYN_TAPE_SAT:
            case ZDJ_SOUNDCARD_DSP_ID_DYN_XIST:
            case ZDJ_SOUNDCARD_DSP_ID_DYN_XFORM:
            case ZDJ_SOUNDCARD_DSP_ID_DYN_VOX:
            case ZDJ_SOUNDCARD_DSP_ID_REVERB:
            case ZDJ_SOUNDCARD_DSP_ID_DELAY:
            case ZDJ_SOUNDCARD_DSP_ID_MOD:
            case ZDJ_SOUNDCARD_DSP_ID_MOD_WIDEN: break;
            default: break;
        }
    }
}

// Crossfade DSP is a special case
void zdj_soundcard_put_dsp_state_for_xfade_dto( zdj_soundcard_dsp_dto_t * dto ) {
    dto->set_gain = &zdj_soundcard_dsp_set_xfade;
}

void * zdj_soundcard_dto_get_dsp_stage_for_type( void * _node, int type ) {
    // printf( "zdj_soundcard_dto_get_dsp_stage_for_type\n" );
    zdj_soundcard_node_t * node = (zdj_soundcard_node_t*)_node;
    if( node->dsp_dto->has_stages ) {
        for( int i=0; i<8; i++ ) {
            if( node->dsp_dto->stages[ i ].type == type ) {
                return &node->dsp_dto->stages[ i ];
            }
        }
    }
    return NULL;
}
void * zdj_soundcard_dto_get_dsp_stage_for_id( void * _node, int id ) {
    zdj_soundcard_node_t * node = (zdj_soundcard_node_t*)_node;
    if( node->dsp_dto->has_stages ) {
        for( int i=0; i<8; i++ ) {
            if( node->dsp_dto->stages[ i ].id == id ) {
                return &node->dsp_dto->stages[ i ];
            }
        }
    }
    return NULL;
}
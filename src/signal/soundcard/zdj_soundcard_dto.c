#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <sqlite3.h>

#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/signal/soundcard/zdj_soundcard_dto.h>
#include <zerodj/system/sql/zdj_sql.h>

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
        case ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS: return dto->main_bus_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS: return dto->cue_bus_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS: return dto->annot_bus_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0: return dto->aux_bus_0_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1: return dto->aux_bus_1_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2: return dto->aux_bus_2_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3: return dto->aux_bus_3_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_0: return dto->clock_0_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_1: return dto->clock_1_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_2: return dto->clock_2_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_3: return dto->clock_3_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_0: return dto->cv_0_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_1: return dto->cv_1_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_2: return dto->cv_2_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_3: return dto->cv_3_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_INPUT: return dto->deck_1_input_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_EDGE: return dto->deck_1_edge_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_BUS: return dto->deck_1_bus_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE: return dto->deck_1_prefade_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_INPUT: return dto->deck_2_input_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_EDGE: return dto->deck_2_edge_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_BUS: return dto->deck_2_bus_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE: return dto->deck_2_prefade_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT: return dto->deck_ext_input_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_EDGE: return dto->deck_ext_edge_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_BUS: return dto->deck_ext_bus_link_map;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE: return dto->deck_ext_prefade_link_map;
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
        case ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS: dto->main_bus_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS: dto->cue_bus_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS: dto->annot_bus_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0: dto->aux_bus_0_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1: dto->aux_bus_1_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2: dto->aux_bus_2_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3: dto->aux_bus_3_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_0: dto->clock_0_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_1: dto->clock_1_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_2: dto->clock_2_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_3: dto->clock_3_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_0: dto->cv_0_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_1: dto->cv_1_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_2: dto->cv_2_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_3: dto->cv_3_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_INPUT: dto->deck_1_input_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_EDGE: dto->deck_1_edge_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_BUS: dto->deck_1_bus_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE: dto->deck_1_prefade_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_INPUT: dto->deck_2_input_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_EDGE: dto->deck_2_edge_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_BUS: dto->deck_2_bus_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE: dto->deck_2_prefade_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT: dto->deck_ext_input_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_EDGE: dto->deck_ext_edge_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_BUS: dto->deck_ext_bus_link_map = link_map; break;
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE: dto->deck_ext_prefade_link_map = link_map; break;
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
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_0: return dto->clock_0_sig;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_1: return dto->clock_1_sig;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_2: return dto->clock_2_sig;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_3: return dto->clock_3_sig;
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
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_0: dto->clock_0_sig = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_1: dto->clock_1_sig = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_2: dto->clock_2_sig = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_3: dto->clock_3_sig = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_0: dto->cv_0_sig = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_1: dto->cv_1_sig = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_2: dto->cv_2_sig = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_3: dto->cv_3_sig = val; break;
        default: break;
    }
}

int zdj_soundcard_dto_get_gain_for_node_name( 
    zdj_soundcard_dto_t * dto,
    int name
) {
    if( !dto ) { return 0; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0: return dto->ana_in_0_trim;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1: return dto->ana_in_1_trim;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2: return dto->ana_in_2_trim;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3: return dto->ana_in_3_trim;
        case ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS: return dto->annot_bus_trim;
        case ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS: return dto->record_bus_trim;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0: return dto->aux_bus_0_trim;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1: return dto->aux_bus_1_trim;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2: return dto->aux_bus_2_trim;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3: return dto->aux_bus_3_trim;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_0: return dto->cv_0_gain;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_1: return dto->cv_1_gain;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_2: return dto->cv_2_gain;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_3: return dto->cv_3_gain;
        default: return 0;
    }
}

int zdj_soundcard_dto_set_gain_for_node_name( 
    zdj_soundcard_dto_t * dto, 
    int name, 
    int val 
) {
    if( !dto ) { return 0; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0: dto->ana_in_0_trim = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1: dto->ana_in_1_trim = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2: dto->ana_in_2_trim = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3: dto->ana_in_3_trim = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS: dto->annot_bus_trim = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS: dto->record_bus_trim = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0: dto->aux_bus_0_trim = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1: dto->aux_bus_1_trim = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2: dto->aux_bus_2_trim = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3: dto->aux_bus_3_trim = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_0: dto->cv_0_gain = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_1: dto->cv_1_gain = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_2: dto->cv_2_gain = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_3: dto->cv_3_gain = val; break;
        default: return 0;
    }
}

int zdj_soundcard_dto_get_pan_for_node_name( 
    zdj_soundcard_dto_t * dto,
    int name
) {
    if( !dto ) { return 0; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0: return dto->ana_in_0_pan;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1: return dto->ana_in_1_pan;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2: return dto->ana_in_2_pan;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3: return dto->ana_in_3_pan;
        case ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS: return dto->main_bus_pan;
        case ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS: return dto->cue_bus_pan;
        case ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS: return dto->annot_bus_pan;
        case ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS: return dto->record_bus_pan;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0: return dto->aux_bus_0_pan;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1: return dto->aux_bus_1_pan;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2: return dto->aux_bus_2_pan;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3: return dto->aux_bus_3_pan;
        default: return 0;
    }
}

int zdj_soundcard_dto_set_pan_for_node_name( 
    zdj_soundcard_dto_t * dto, 
    int name, 
    int val 
) {
    if( !dto ) { return 0; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0: dto->ana_in_0_pan = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1: dto->ana_in_1_pan = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2: dto->ana_in_2_pan = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3: dto->ana_in_3_pan = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS: dto->main_bus_pan = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS: dto->cue_bus_pan = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS: dto->annot_bus_pan = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS: dto->record_bus_pan = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0: dto->aux_bus_0_pan = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1: dto->aux_bus_1_pan = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2: dto->aux_bus_2_pan = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3: dto->aux_bus_3_pan = val; break;
        default: return 0;
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
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_BUS:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_INPUT:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_EDGE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_BUS:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_EDGE:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_BUS:
        case ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE: return 1;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0: return dto->ana_out_0_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1: return dto->ana_out_1_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2: return dto->ana_out_2_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3: return dto->ana_out_3_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0: return dto->ana_in_0_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1: return dto->ana_in_1_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2: return dto->ana_in_2_stereo;
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3: return dto->ana_in_3_stereo;
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
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_0: return dto->clock_0_source;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_1: return dto->clock_1_source;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_2: return dto->clock_2_source;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_3: return dto->clock_3_source;
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
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_0: dto->clock_0_source = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_1: dto->clock_1_source = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_2: dto->clock_2_source = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_3: dto->clock_3_source = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_0: dto->cv_0_source = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_1: dto->cv_1_source = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_2: dto->cv_2_source = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_3: dto->cv_3_source = val; break;
        default: return 0;
    }
}

int zdj_soundcard_dto_get_val_for_node_name( 
    zdj_soundcard_dto_t * dto, 
    int name 
) {
    if( !dto ) { return 0; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_0: return dto->clock_0_val;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_1: return dto->clock_1_val;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_2: return dto->clock_2_val;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_3: return dto->clock_3_val;
        default: return 0;
    }
}

int zdj_soundcard_dto_set_val_for_node_name( 
    zdj_soundcard_dto_t * dto, 
    int name, 
    int val 
) {
    if( !dto ) { return 0; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_0: dto->clock_0_val = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_1: dto->clock_1_val = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_2: dto->clock_2_val = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_3: dto->clock_3_val = val; break;
        default: return 0;
    }
}

int zdj_soundcard_dto_get_invert_for_node_name( 
    zdj_soundcard_dto_t * dto, 
    int name 
) {
    if( !dto ) { return 0; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_CV_0: return dto->cv_0_invert;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_1: return dto->cv_1_invert;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_2: return dto->cv_2_invert;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_3: return dto->cv_3_invert;
        default: return 0;
    }
}

int zdj_soundcard_dto_set_invert_for_node_name( 
    zdj_soundcard_dto_t * dto, 
    int name, 
    int val 
) {
    if( !dto ) { return 0; }
    switch ( name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_CV_0: dto->cv_0_invert = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_1: dto->cv_1_invert = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_2: dto->cv_2_invert = val; break;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_3: dto->cv_3_invert = val; break;
        default: return 0;
    }
}

zdj_error_type_t zdj_soundcard_fetch_dto( char * entity_id, zdj_soundcard_dto_t * dto ) {
    if( !entity_id ) { return ZDJ_ERROR_OKAY; }
    
    sqlite3 * zdj_soundcard_db = zdj_sql_open( ZDJ_SOUNDCARD_DB_PATH );

    if( !zdj_soundcard_db ) { 
        printf( "failed to open zero db\n" );
        return ZDJ_ERROR_LIBRARY_DB_ERROR; 
    }

    // Grab all the values from db
    int sql_res;
    char _sql[ 256 ];
    sprintf( _sql, "select * from Linkage where entity_id like \'%s\'", entity_id );
    sqlite3_stmt * stmt = zdj_sql_prep_row_stepper( _sql, zdj_soundcard_db );
    if( stmt ) {
        while ( ( sql_res = sqlite3_step( stmt ) ) == SQLITE_ROW ) {
            strcpy( dto->entity_id, (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_ENTITY_ID ) );
            strcpy( dto->name, (char*)sqlite3_column_text ( stmt, ZDJ_SOUNDCARD_COL_NAME ) );

            dto->ana_out_0_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_OUT_0_SIG );
            dto->ana_out_0_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_OUT_0_STEREO );
            dto->ana_out_1_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_OUT_1_SIG );
            dto->ana_out_1_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_OUT_1_STEREO );
            dto->ana_out_2_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_OUT_2_SIG );
            dto->ana_out_2_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_OUT_2_STEREO );
            dto->ana_out_3_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_OUT_3_SIG );
            dto->ana_out_3_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_OUT_3_STEREO );

            dto->ana_in_0_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_0_LINK );
            dto->ana_in_0_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_0_SIG );
            dto->ana_in_0_trim = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_0_TRIM );
            dto->ana_in_0_pan = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_0_PAN );
            dto->ana_in_0_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_0_STEREO );
            dto->ana_in_0_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_0_MUTE );
            dto->ana_in_1_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_1_LINK );
            dto->ana_in_1_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_1_SIG );
            dto->ana_in_1_trim = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_1_TRIM );
            dto->ana_in_1_pan = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_1_PAN );
            dto->ana_in_1_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_1_STEREO );
            dto->ana_in_1_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_1_MUTE );
            dto->ana_in_2_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_2_LINK );
            dto->ana_in_2_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_2_SIG );
            dto->ana_in_2_trim = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_2_TRIM );
            dto->ana_in_2_pan = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_2_PAN );
            dto->ana_in_2_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_2_STEREO );
            dto->ana_in_2_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_2_MUTE );
            dto->ana_in_3_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_3_LINK );
            dto->ana_in_3_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_3_SIG );
            dto->ana_in_3_trim = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_3_TRIM );
            dto->ana_in_3_pan = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_3_PAN );
            dto->ana_in_3_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_3_STEREO );
            dto->ana_in_3_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANA_IN_3_MUTE );

            dto->main_bus_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_MAIN_BUS_LINK );
            dto->main_bus_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_MAIN_BUS_STEREO );
            dto->main_bus_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_MAIN_BUS_MUTE );

            dto->cue_bus_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_CUE_BUS_LINK );
            dto->cue_bus_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CUE_BUS_STEREO );
            dto->cue_bus_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CUE_BUS_MUTE );

            dto->annot_bus_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_ANNOT_BUS_LINK );
            dto->annot_bus_trim = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANNOT_BUS_TRIM );
            dto->annot_bus_pan = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANNOT_BUS_PAN );
            dto->annot_bus_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANNOT_BUS_STEREO );
            dto->annot_bus_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_ANNOT_BUS_MUTE );

            dto->record_bus_trim = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_RECORD_BUS_TRIM );
            dto->record_bus_pan = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_RECORD_BUS_PAN );
            dto->record_bus_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_RECORD_BUS_STEREO );

            dto->deck_1_input_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_1_INPUT_LINK );
            dto->deck_1_edge_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_1_EDGE_LINK );
            dto->deck_1_bus_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_1_BUS_LINK );
            dto->deck_1_prefade_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_1_PREFADE_LINK );

            dto->deck_2_input_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_2_INPUT_LINK );
            dto->deck_2_edge_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_2_EDGE_LINK );
            dto->deck_2_bus_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_2_BUS_LINK );
            dto->deck_2_prefade_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_2_PREFADE_LINK );

            dto->deck_ext_input_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_EXT_INPUT_LINK );
            dto->deck_ext_edge_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_EXT_EDGE_LINK );
            dto->deck_ext_bus_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_EXT_BUS_LINK );
            dto->deck_ext_prefade_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_DECK_EXT_PREFADE_LINK );

            dto->aux_bus_0_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_0_LINK );
            dto->aux_bus_0_trim = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_0_TRIM );
            dto->aux_bus_0_pan = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_0_PAN );
            dto->aux_bus_0_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_0_STEREO );
            dto->aux_bus_0_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_0_MUTE );
            dto->aux_bus_1_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_1_LINK );
            dto->aux_bus_1_trim = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_1_TRIM );
            dto->aux_bus_1_pan = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_1_PAN );
            dto->aux_bus_1_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_1_STEREO );
            dto->aux_bus_1_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_1_MUTE );
            dto->aux_bus_2_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_2_LINK );
            dto->aux_bus_2_trim = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_2_TRIM );
            dto->aux_bus_2_pan = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_2_PAN );
            dto->aux_bus_2_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_2_STEREO );
            dto->aux_bus_2_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_2_MUTE );
            dto->aux_bus_3_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_3_LINK );
            dto->aux_bus_3_trim = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_3_TRIM );
            dto->aux_bus_3_pan = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_3_PAN );
            dto->aux_bus_3_stereo = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_3_STEREO );
            dto->aux_bus_3_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_AUX_BUS_3_MUTE );

            dto->clock_0_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_0_SIG );
            dto->clock_0_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_0_LINK );
            dto->clock_0_source = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_0_SOURCE );
            dto->clock_0_val = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_0_VAL );
            dto->clock_1_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_1_SIG );
            dto->clock_1_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_1_LINK );
            dto->clock_1_source = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_1_SOURCE );
            dto->clock_1_val = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_1_VAL );
            dto->clock_2_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_2_SIG );
            dto->clock_2_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_2_LINK );
            dto->clock_2_source = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_2_SOURCE );
            dto->clock_2_val = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_2_VAL );
            dto->clock_3_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_3_SIG );
            dto->clock_3_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_3_LINK );
            dto->clock_3_source = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_3_SOURCE );
            dto->clock_3_val = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CLOCK_3_VAL );

            dto->cv_0_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_0_SIG );
            dto->cv_0_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_CV_0_LINK );
            dto->cv_0_source = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_0_SOURCE );
            dto->cv_0_invert = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_0_INVERT );
            dto->cv_0_gain = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_0_GAIN );
            dto->cv_0_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_0_MUTE );
            dto->cv_1_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_1_SIG );
            dto->cv_1_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_CV_1_LINK );
            dto->cv_1_source = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_1_SOURCE );
            dto->cv_1_invert = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_1_INVERT );
            dto->cv_1_gain = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_1_GAIN );
            dto->cv_1_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_1_MUTE );
            dto->cv_2_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_2_SIG );
            dto->cv_2_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_CV_2_LINK );
            dto->cv_2_source = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_2_SOURCE );
            dto->cv_2_invert = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_2_INVERT );
            dto->cv_2_gain = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_2_GAIN );
            dto->cv_2_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_2_MUTE );
            dto->cv_3_sig = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_3_SIG );
            dto->cv_3_link_map = sqlite3_column_int64 ( stmt, ZDJ_SOUNDCARD_COL_CV_3_LINK );
            dto->cv_3_source = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_3_SOURCE );
            dto->cv_3_invert = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_3_INVERT );
            dto->cv_3_gain = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_3_GAIN );
            dto->cv_2_mute = sqlite3_column_int ( stmt, ZDJ_SOUNDCARD_COL_CV_3_MUTE );
        }
        sqlite3_finalize( stmt );
    }

    int res = zdj_sql_close( zdj_soundcard_db );
    if( res ) {
        return ZDJ_ERROR_LIBRARY_DB_ERROR;
    } else {
        return ZDJ_ERROR_OKAY;
    }

    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t zdj_soundcard_store_dto( char * entity_id, zdj_soundcard_dto_t * dto ) {    
    
    sqlite3 * zdj_soundcard_db = zdj_sql_open( ZDJ_SOUNDCARD_DB_PATH );

    if( !zdj_soundcard_db ) { 
        printf( "failed to open zero db\n" );
        return ZDJ_ERROR_LIBRARY_DB_ERROR; 
    }

    int count = 0;
    int res;
    char sql[ 4096 ];
    // Set up for prepared stmt w/binds to use built-in string escaping.
    snprintf( sql, sizeof( sql ), 
        // Insert new record
        "INSERT INTO %s VALUES(\'%s\',\'%s\',%d,%d,%d,%d,%d,%d,%d,%d,%lu,%d,%d,%d,%d,%d,%lu,%d,%d,%d,%d,%d,%lu,%d,%d,%d,%d,%d,%lu,%d,%d,%d,%d,%d,%lu,%d,%d,%lu,%d,%d,%lu,%d,%d,%d,%d,%d,%d,%d,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%d,%d,%d,%d,%lu,%d,%d,%d,%d,%lu,%d,%d,%d,%d,%lu,%d,%d,%d,%d,%d,%lu,%d,%d,%d,%lu,%d,%d,%d,%lu,%d,%d,%d,%lu,%d,%d,%d,%lu,%d,%d,%d,%d,%d,%lu,%d,%d,%d,%d,%d,%lu,%d,%d,%d,%d,%d,%lu,%d,%d,%d,%d)\n"

        // Or update existing record
        "ON CONFLICT(entity_id) DO UPDATE SET entity_id=\'%s\',name=\'%s\',ana_out_0_sig=%d,ana_out_0_stereo=%d,ana_out_1_sig=%d,ana_out_1_stereo=%d,ana_out_2_sig=%d,ana_out_2_stereo=%d,ana_out_3_sig=%d,ana_out_3_stereo=%d,ana_in_0_link=%lu,ana_in_0_sig=%d,ana_in_0_trim=%d,ana_in_0_pan=%d,ana_in_0_stereo=%d,ana_in_0_mute=%d,ana_in_1_link=%lu,ana_in_1_sig=%d,ana_in_1_trim=%d,ana_in_1_pan=%d,ana_in_1_stereo=%d,ana_in_1_mute=%d,ana_in_2_link=%lu,ana_in_2_sig=%d,ana_in_2_trim=%d,ana_in_2_pan=%d,ana_in_2_stereo=%d,ana_in_2_mute=%d,ana_in_3_link=%lu,ana_in_3_sig=%d,ana_in_3_trim=%d,ana_in_3_pan=%d,ana_in_3_stereo=%d,ana_in_3_mute=%d,main_bus_link=%lu,main_bus_stereo=%d,main_bus_mute=%d,cue_bus_link=%lu,cue_bus_stereo=%d,cue_bus_mute=%d,annot_bus_link=%lu,annot_bus_trim=%d,annot_bus_pan=%d,annot_bus_stereo=%d,annot_bus_mute=%d,record_bus_trim=%d,record_bus_pan=%d,record_bus_stereo=%d,deck_1_input_link=%lu,deck_1_edge_link=%lu,deck_1_bus_link=%lu,deck_1_prefade_link=%lu,deck_2_input_link=%lu,deck_2_edge_link=%lu,deck_2_bus_link=%lu,deck_2_prefade_link=%lu,deck_ext_input_link=%lu,deck_ext_edge_link=%lu,deck_ext_bus_link=%lu,deck_ext_prefade_link=%lu,aux_bus_0_link=%lu,aux_bus_0_trim=%d,aux_bus_0_pan=%d,aux_bus_0_stereo=%d,aux_bus_0_mute=%d,aux_bus_1_link=%lu,aux_bus_1_trim=%d,aux_bus_1_pan=%d,aux_bus_1_stereo=%d,aux_bus_1_mute=%d,aux_bus_2_link=%lu,aux_bus_2_trim=%d,aux_bus_2_pan=%d,aux_bus_2_stereo=%d,aux_bus_2_mute=%d,aux_bus_3_link=%lu,aux_bus_3_trim=%d,aux_bus_3_pan=%d,aux_bus_3_stereo=%d,aux_bus_3_mute=%d,clock_0_sig=%d,clock_0_link=%lu,clock_0_source=%d,clock_0_val=%d,clock_1_sig=%d,clock_1_link=%lu,clock_1_source=%d,clock_1_val=%d,clock_2_sig=%d,clock_2_link=%lu,clock_2_source=%d,clock_2_val=%d,clock_3_sig=%d,clock_3_link=%lu,clock_3_source=%d,clock_3_val=%d,cv_0_sig=%d,cv_0_link=%lu,cv_0_source=%d,cv_0_invert=%d,cv_0_gain=%d,cv_0_mute=%d,cv_1_sig=%d,cv_1_link=%lu,cv_1_source=%d,cv_1_invert=%d,cv_1_gain=%d,cv_1_mute=%d,cv_2_sig=%d,cv_2_link=%lu,cv_2_source=%d,cv_2_invert=%d,cv_2_gain=%d,cv_2_mute=%d,cv_3_sig=%d,cv_3_link=%lu,cv_3_source=%d,cv_3_invert=%d,cv_3_gain=%d,cv_3_mute=%d",

        // Table Name
        "Linkage",
        dto->entity_id,
        dto->name,
        dto->ana_out_0_sig,
        dto->ana_out_0_stereo,
        dto->ana_out_1_sig,
        dto->ana_out_1_stereo,
        dto->ana_out_2_sig,
        dto->ana_out_2_stereo,
        dto->ana_out_3_sig,
        dto->ana_out_3_stereo,
        dto->ana_in_0_link_map,
        dto->ana_in_0_sig,
        dto->ana_in_0_trim,
        dto->ana_in_0_pan,
        dto->ana_in_0_stereo,
        dto->ana_in_0_mute,
        dto->ana_in_1_link_map,
        dto->ana_in_1_sig,
        dto->ana_in_1_trim,
        dto->ana_in_1_pan,
        dto->ana_in_1_stereo,
        dto->ana_in_1_mute,
        dto->ana_in_2_link_map,
        dto->ana_in_2_sig,
        dto->ana_in_2_trim,
        dto->ana_in_2_pan,
        dto->ana_in_2_stereo,
        dto->ana_in_2_mute,
        dto->ana_in_3_link_map,
        dto->ana_in_3_sig,
        dto->ana_in_3_trim,
        dto->ana_in_3_pan,
        dto->ana_in_3_stereo,
        dto->ana_in_3_mute,
        dto->main_bus_link_map,
        dto->main_bus_stereo,
        dto->main_bus_mute,
        dto->cue_bus_link_map,
        dto->cue_bus_stereo,
        dto->cue_bus_mute,
        dto->annot_bus_link_map,
        dto->annot_bus_trim,
        dto->annot_bus_pan,
        dto->annot_bus_stereo,
        dto->annot_bus_mute,
        dto->record_bus_trim,
        dto->record_bus_pan,
        dto->record_bus_stereo,
        dto->deck_1_input_link_map,
        dto->deck_1_edge_link_map,
        dto->deck_1_bus_link_map,
        dto->deck_1_prefade_link_map,
        dto->deck_2_input_link_map,
        dto->deck_2_edge_link_map,
        dto->deck_2_bus_link_map,
        dto->deck_2_prefade_link_map,
        dto->deck_ext_input_link_map,
        dto->deck_ext_edge_link_map,
        dto->deck_ext_bus_link_map,
        dto->deck_ext_prefade_link_map,
        dto->aux_bus_0_link_map,
        dto->aux_bus_0_trim,
        dto->aux_bus_0_pan,
        dto->aux_bus_0_stereo,
        dto->aux_bus_0_mute,
        dto->aux_bus_1_link_map,
        dto->aux_bus_1_trim,
        dto->aux_bus_1_pan,
        dto->aux_bus_1_stereo,
        dto->aux_bus_1_mute,
        dto->aux_bus_2_link_map,
        dto->aux_bus_2_trim,
        dto->aux_bus_2_pan,
        dto->aux_bus_2_stereo,
        dto->aux_bus_2_mute,
        dto->aux_bus_3_link_map,
        dto->aux_bus_3_trim,
        dto->aux_bus_3_pan,
        dto->aux_bus_3_stereo,
        dto->aux_bus_3_mute,
        dto->clock_0_sig,
        dto->clock_0_link_map,
        dto->clock_0_source,
        dto->clock_0_val,
        dto->clock_1_sig,
        dto->clock_1_link_map,
        dto->clock_1_source,
        dto->clock_1_val,
        dto->clock_2_sig,
        dto->clock_2_link_map,
        dto->clock_2_source,
        dto->clock_2_val,
        dto->clock_3_sig,
        dto->clock_3_link_map,
        dto->clock_3_source,
        dto->clock_3_val,
        dto->cv_0_sig,
        dto->cv_0_link_map,
        dto->cv_0_source,
        dto->cv_0_invert,
        dto->cv_0_gain,
        dto->cv_0_mute,
        dto->cv_1_sig,
        dto->cv_1_link_map,
        dto->cv_1_source,
        dto->cv_1_invert,
        dto->cv_1_gain,
        dto->cv_1_mute,
        dto->cv_2_sig,
        dto->cv_2_link_map,
        dto->cv_2_source,
        dto->cv_2_invert,
        dto->cv_2_gain,
        dto->cv_2_mute,
        dto->cv_3_sig,
        dto->cv_3_link_map,
        dto->cv_3_source,
        dto->cv_3_invert,
        dto->cv_3_gain,
        dto->cv_3_mute,

        dto->entity_id,
        dto->name,
        dto->ana_out_0_sig,
        dto->ana_out_0_stereo,
        dto->ana_out_1_sig,
        dto->ana_out_1_stereo,
        dto->ana_out_2_sig,
        dto->ana_out_2_stereo,
        dto->ana_out_3_sig,
        dto->ana_out_3_stereo,
        dto->ana_in_0_link_map,
        dto->ana_in_0_sig,
        dto->ana_in_0_trim,
        dto->ana_in_0_pan,
        dto->ana_in_0_stereo,
        dto->ana_in_0_mute,
        dto->ana_in_1_link_map,
        dto->ana_in_1_sig,
        dto->ana_in_1_trim,
        dto->ana_in_1_pan,
        dto->ana_in_1_stereo,
        dto->ana_in_1_mute,
        dto->ana_in_2_link_map,
        dto->ana_in_2_sig,
        dto->ana_in_2_trim,
        dto->ana_in_2_pan,
        dto->ana_in_2_stereo,
        dto->ana_in_2_mute,
        dto->ana_in_3_link_map,
        dto->ana_in_3_sig,
        dto->ana_in_3_trim,
        dto->ana_in_3_pan,
        dto->ana_in_3_stereo,
        dto->ana_in_3_mute,
        dto->main_bus_link_map,
        dto->main_bus_stereo,
        dto->main_bus_mute,
        dto->cue_bus_link_map,
        dto->cue_bus_stereo,
        dto->cue_bus_mute,
        dto->annot_bus_link_map,
        dto->annot_bus_trim,
        dto->annot_bus_pan,
        dto->annot_bus_stereo,
        dto->annot_bus_mute,
        dto->record_bus_trim,
        dto->record_bus_pan,
        dto->record_bus_stereo,
        dto->deck_1_input_link_map,
        dto->deck_1_edge_link_map,
        dto->deck_1_bus_link_map,
        dto->deck_1_prefade_link_map,
        dto->deck_2_input_link_map,
        dto->deck_2_edge_link_map,
        dto->deck_2_bus_link_map,
        dto->deck_2_prefade_link_map,
        dto->deck_ext_input_link_map,
        dto->deck_ext_edge_link_map,
        dto->deck_ext_bus_link_map,
        dto->deck_ext_prefade_link_map,
        dto->aux_bus_0_link_map,
        dto->aux_bus_0_trim,
        dto->aux_bus_0_pan,
        dto->aux_bus_0_stereo,
        dto->aux_bus_0_mute,
        dto->aux_bus_1_link_map,
        dto->aux_bus_1_trim,
        dto->aux_bus_1_pan,
        dto->aux_bus_1_stereo,
        dto->aux_bus_1_mute,
        dto->aux_bus_2_link_map,
        dto->aux_bus_2_trim,
        dto->aux_bus_2_pan,
        dto->aux_bus_2_stereo,
        dto->aux_bus_2_mute,
        dto->aux_bus_3_link_map,
        dto->aux_bus_3_trim,
        dto->aux_bus_3_pan,
        dto->aux_bus_3_stereo,
        dto->aux_bus_3_mute,
        dto->clock_0_sig,
        dto->clock_0_link_map,
        dto->clock_0_source,
        dto->clock_0_val,
        dto->clock_1_sig,
        dto->clock_1_link_map,
        dto->clock_1_source,
        dto->clock_1_val,
        dto->clock_2_sig,
        dto->clock_2_link_map,
        dto->clock_2_source,
        dto->clock_2_val,
        dto->clock_3_sig,
        dto->clock_3_link_map,
        dto->clock_3_source,
        dto->clock_3_val,
        dto->cv_0_sig,
        dto->cv_0_link_map,
        dto->cv_0_source,
        dto->cv_0_invert,
        dto->cv_0_gain,
        dto->cv_0_mute,
        dto->cv_1_sig,
        dto->cv_1_link_map,
        dto->cv_1_source,
        dto->cv_1_invert,
        dto->cv_1_gain,
        dto->cv_1_mute,
        dto->cv_2_sig,
        dto->cv_2_link_map,
        dto->cv_2_source,
        dto->cv_2_invert,
        dto->cv_2_gain,
        dto->cv_2_mute,
        dto->cv_3_sig,
        dto->cv_3_link_map,
        dto->cv_3_source,
        dto->cv_3_invert,
        dto->cv_3_gain,
        dto->cv_3_mute 
    );

    zdj_sql_exec( sql, zdj_soundcard_db );

    res = zdj_sql_close( zdj_soundcard_db );
    if( res ) {
        return ZDJ_ERROR_LIBRARY_DB_ERROR;
    } else {
        return ZDJ_ERROR_OKAY;
    }

    return ZDJ_ERROR_OKAY;
}
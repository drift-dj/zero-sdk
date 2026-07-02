#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <sqlite3.h>

#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/signal/soundcard/db/zdj_soundcard_dto.h>
#include <zerodj/system/sql/zdj_sql.h>

void zdj_soundcard_reset_db_defaults( void ) {
    ///////////////////////////////////////////
    // Headphone DJ Mixer                    //
    // - Main L/R + Cue are routed to Port 1 //
    // - Mail L/R is routed to Port 2        //
    ///////////////////////////////////////////
    zdj_soundcard_dto_t * dto = zdj_soundcard_create_dto( );
    strcpy( dto->name, "Headphone DJ" );

    // Add default linkages

    // Analog input outs
    dto->ana_in_0_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT;
    dto->ana_in_0_stereo = true;
    dto->ana_in_1_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT;
    dto->ana_in_1_stereo = true;
    dto->ana_in_2_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT;
    dto->ana_in_2_stereo = true;
    dto->ana_in_3_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT;
    dto->ana_in_3_stereo = true;

    dto->ana_out_0_stereo = true;
    dto->ana_out_0_sig = ZDJ_SOUNDCARD_SIGNAL_PRO_PLUS_4_DBU;
    dto->ana_out_1_stereo = true;
    dto->ana_out_1_sig = ZDJ_SOUNDCARD_SIGNAL_PRO_PLUS_4_DBU;
    dto->ana_out_2_stereo = true;
    dto->ana_out_2_sig = ZDJ_SOUNDCARD_SIGNAL_HEADPHONE_HI;
    dto->ana_out_3_stereo = true;
    dto->ana_out_3_sig = ZDJ_SOUNDCARD_SIGNAL_HEADPHONE_HI;

    // USB I/O
    dto->usb_in_0_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT;
    dto->usb_in_0_stereo = true;
    dto->usb_in_1_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT;
    dto->usb_in_1_stereo = true;

    dto->usb_out_0_stereo = true;
    dto->usb_out_1_stereo = true;

    // Admin bus outs
    dto->annot_bus_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS;
    dto->annot_bus_stereo = false;
    dto->cue_bus_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2;
    dto->cue_bus_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3;
    dto->cue_bus_stereo = true;
    dto->main_bus_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2;
    dto->main_bus_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3;
    dto->main_bus_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0;
    dto->main_bus_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1;
    dto->main_bus_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS;
    dto->main_bus_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_0;
    dto->main_bus_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_1;
    dto->main_bus_stereo = true;
    dto->record_bus_stereo = true;
    
    // Deck 1
    dto->deck_1_input_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE;
    dto->deck_1_edge_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE;
    dto->deck_1_prefade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_1_POSTFADE;
    dto->deck_1_prefade_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_1_CUE;
    dto->deck_1_cue_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS;
    dto->deck_1_postfade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_XFADE_A;
    dto->deck_1_prefade_dsp.stages[ 0 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ;
    dto->deck_1_prefade_dsp.stages[ 0 ].id = ZDJ_SOUNDCARD_DSP_ID_EQ_3_4P;
    dto->deck_1_prefade_dsp.stages[ 0 ].knob_0 = 1.0; // Lo
    dto->deck_1_prefade_dsp.stages[ 0 ].knob_1 = 1.0; // Mid
    dto->deck_1_prefade_dsp.stages[ 0 ].knob_2 = 1.0; // Hi
    dto->deck_1_prefade_dsp.stages[ 0 ].knob_3 = 0.250; // Xover Hi
    dto->deck_1_prefade_dsp.stages[ 0 ].knob_4 = 1.0; // Xover Lo
    dto->deck_1_prefade_dsp.stages[ 1 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_FILT;
    dto->deck_1_prefade_dsp.stages[ 1 ].id = ZDJ_SOUNDCARD_DSP_ID_FILT_BI;
    dto->deck_1_prefade_dsp.stages[ 1 ].knob_0 = 0.0; // Freq
    dto->deck_1_prefade_dsp.stages[ 1 ].knob_2 = 0.0; // Res

    // Deck 2
    dto->deck_2_input_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE;
    dto->deck_2_edge_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE;
    dto->deck_2_prefade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_2_POSTFADE;
    dto->deck_2_prefade_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_2_CUE;
    dto->deck_2_cue_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS;
    dto->deck_2_postfade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_XFADE_B;
    dto->deck_2_prefade_dsp.stages[ 0 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ;
    dto->deck_2_prefade_dsp.stages[ 0 ].id = ZDJ_SOUNDCARD_DSP_ID_EQ_3_4P;
    dto->deck_2_prefade_dsp.stages[ 0 ].knob_0 = 1.0; // Lo
    dto->deck_2_prefade_dsp.stages[ 0 ].knob_1 = 1.0; // Mid
    dto->deck_2_prefade_dsp.stages[ 0 ].knob_2 = 1.0; // Hi
    dto->deck_2_prefade_dsp.stages[ 0 ].knob_3 = 0.250; // Xover Hi
    dto->deck_2_prefade_dsp.stages[ 0 ].knob_4 = 1.0; // Xover Lo
    dto->deck_2_prefade_dsp.stages[ 1 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_FILT;
    dto->deck_2_prefade_dsp.stages[ 1 ].id = ZDJ_SOUNDCARD_DSP_ID_FILT_BI;
    dto->deck_2_prefade_dsp.stages[ 1 ].knob_0 = 0.0; // Freq
    dto->deck_2_prefade_dsp.stages[ 1 ].knob_2 = 0.0; // Res

    // Ext Deck
    dto->deck_ext_input_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE;
    dto->deck_ext_edge_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE;
    dto->deck_ext_prefade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_POSTFADE;
    dto->deck_ext_prefade_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_CUE;
    dto->deck_ext_cue_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS;
    dto->deck_ext_postfade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS;
    dto->deck_ext_prefade_dsp.stages[ 0 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ;
    dto->deck_ext_prefade_dsp.stages[ 0 ].id = ZDJ_SOUNDCARD_DSP_ID_EQ_3_4P;
    dto->deck_ext_prefade_dsp.stages[ 0 ].knob_0 = 1.0; // Lo
    dto->deck_ext_prefade_dsp.stages[ 0 ].knob_1 = 1.0; // Mid
    dto->deck_ext_prefade_dsp.stages[ 0 ].knob_2 = 1.0; // Hi
    dto->deck_ext_prefade_dsp.stages[ 0 ].knob_3 = 0.250; // Xover Hi
    dto->deck_ext_prefade_dsp.stages[ 0 ].knob_4 = 1.0; // Xover Lo
    dto->deck_ext_prefade_dsp.stages[ 1 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_FILT;
    dto->deck_ext_prefade_dsp.stages[ 1 ].id = ZDJ_SOUNDCARD_DSP_ID_FILT_BI;
    dto->deck_ext_prefade_dsp.stages[ 1 ].knob_0 = 0.0; // Freq
    dto->deck_ext_prefade_dsp.stages[ 1 ].knob_2 = 0.0; // Res

    // Main Clock
    dto->clock_0_sig = ZDJ_SOUNDCARD_SIGNAL_XPORT_ANALOG_PPQN_4; // Clock output 4 PPQN
    dto->clock_0_source = ZDJ_SOUNDCARD_CLOCK_DIRECTION_OUTPUT; // Clock source Output
    dto->clock_0_sync = ZDJ_SOUNDCARD_CLOCK_SYNC_NORMAL;
    dto->clock_0_val = 120.0; // Clock BPM setting

    // Crossfader
    dto->xfade_a_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS;
    dto->xfade_a_dsp.stages[ 0 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_XFADE;
    dto->xfade_a_dsp.stages[ 0 ].id = ZDJ_SOUNDCARD_DSP_ID_XFADE_CURVE;
    dto->xfade_a_dsp.stages[ 0 ].knob_0 = 0.8; // Crossfader curve
    dto->xfade_b_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS;
    dto->xfade_b_dsp.stages[ 0 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_XFADE;
    dto->xfade_b_dsp.stages[ 0 ].id = ZDJ_SOUNDCARD_DSP_ID_XFADE_CURVE;
    dto->xfade_b_dsp.stages[ 0 ].knob_0 = 0.8; // Crossfader curve

    zdj_soundcard_store_dto( dto );


    // Make __temp__ from DJ default
    strcpy( dto->entity_id, "__temp__" );
    strcpy( dto->name, "Current" );
    zdj_soundcard_store_dto( dto );

    free( dto );


    ////////////////////////////////////
    // Booth DJ Mixer                 //
    // - Main L/R is routed to Port 1 //
    // - Cue is routed to Port 2      //
    ////////////////////////////////////
    dto = zdj_soundcard_create_dto( );
    strcpy( dto->name, "Booth DJ" );

    // Add default linkages

    // Analog input outs
    dto->ana_in_0_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT;
    dto->ana_in_0_stereo = true;
    dto->ana_in_1_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT;
    dto->ana_in_1_stereo = true;
    dto->ana_in_2_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT;
    dto->ana_in_2_stereo = true;
    dto->ana_in_3_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT;
    dto->ana_in_3_stereo = true;

    dto->ana_out_0_stereo = true;
    dto->ana_out_0_sig = ZDJ_SOUNDCARD_SIGNAL_PRO_PLUS_4_DBU;
    dto->ana_out_1_stereo = true;
    dto->ana_out_1_sig = ZDJ_SOUNDCARD_SIGNAL_PRO_PLUS_4_DBU;
    dto->ana_out_2_stereo = true;
    dto->ana_out_2_sig = ZDJ_SOUNDCARD_SIGNAL_HEADPHONE_HI;
    dto->ana_out_3_stereo = true;
    dto->ana_out_3_sig = ZDJ_SOUNDCARD_SIGNAL_HEADPHONE_HI;

    // USB I/O
    dto->usb_in_0_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT;
    dto->usb_in_0_stereo = true;
    dto->usb_in_1_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT;
    dto->usb_in_1_stereo = true;

    dto->usb_out_0_stereo = true;
    dto->usb_out_1_stereo = true;

    // Admin bus outs
    dto->annot_bus_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS;
    dto->annot_bus_stereo = false;
    dto->cue_bus_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0;
    dto->cue_bus_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1;
    dto->cue_bus_stereo = true;
    dto->main_bus_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2;
    dto->main_bus_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3;
    dto->main_bus_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS;
    dto->main_bus_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_0;
    dto->main_bus_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_1;
    dto->main_bus_stereo = true;
    dto->record_bus_stereo = true;
    
    // Deck 1
    dto->deck_1_input_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE;
    dto->deck_1_edge_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE;
    dto->deck_1_prefade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_1_POSTFADE;
    dto->deck_1_prefade_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_1_CUE;
    dto->deck_1_cue_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS;
    dto->deck_1_postfade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_XFADE_A;
    dto->deck_1_prefade_dsp.stages[ 0 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ;
    dto->deck_1_prefade_dsp.stages[ 0 ].id = ZDJ_SOUNDCARD_DSP_ID_EQ_3_4P;
    dto->deck_1_prefade_dsp.stages[ 0 ].knob_0 = 1.0; // Lo
    dto->deck_1_prefade_dsp.stages[ 0 ].knob_1 = 1.0; // Mid
    dto->deck_1_prefade_dsp.stages[ 0 ].knob_2 = 1.0; // Hi
    dto->deck_1_prefade_dsp.stages[ 0 ].knob_3 = 0.250; // Xover Hi
    dto->deck_1_prefade_dsp.stages[ 0 ].knob_4 = 1.0; // Xover Lo
    dto->deck_1_prefade_dsp.stages[ 1 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_FILT;
    dto->deck_1_prefade_dsp.stages[ 1 ].id = ZDJ_SOUNDCARD_DSP_ID_FILT_BI;
    dto->deck_1_prefade_dsp.stages[ 1 ].knob_0 = 0.0; // Freq
    dto->deck_1_prefade_dsp.stages[ 1 ].knob_2 = 0.0; // Res

    // Deck 2
    dto->deck_2_input_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE;
    dto->deck_2_edge_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE;
    dto->deck_2_prefade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_2_POSTFADE;
    dto->deck_2_prefade_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_2_CUE;
    dto->deck_2_cue_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS;
    dto->deck_2_postfade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_XFADE_B;
    dto->deck_2_prefade_dsp.stages[ 0 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ;
    dto->deck_2_prefade_dsp.stages[ 0 ].id = ZDJ_SOUNDCARD_DSP_ID_EQ_3_4P;
    dto->deck_2_prefade_dsp.stages[ 0 ].knob_0 = 1.0; // Lo
    dto->deck_2_prefade_dsp.stages[ 0 ].knob_1 = 1.0; // Mid
    dto->deck_2_prefade_dsp.stages[ 0 ].knob_2 = 1.0; // Hi
    dto->deck_2_prefade_dsp.stages[ 0 ].knob_3 = 0.250; // Xover Hi
    dto->deck_2_prefade_dsp.stages[ 0 ].knob_4 = 1.0; // Xover Lo
    dto->deck_2_prefade_dsp.stages[ 1 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_FILT;
    dto->deck_2_prefade_dsp.stages[ 1 ].id = ZDJ_SOUNDCARD_DSP_ID_FILT_BI;
    dto->deck_2_prefade_dsp.stages[ 1 ].knob_0 = 0.0; // Freq
    dto->deck_2_prefade_dsp.stages[ 1 ].knob_2 = 0.0; // Res

    // Ext Deck
    dto->deck_ext_input_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE;
    dto->deck_ext_edge_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE;
    dto->deck_ext_prefade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_POSTFADE;
    dto->deck_ext_prefade_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_CUE;
    dto->deck_ext_cue_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS;
    dto->deck_ext_postfade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS;
    dto->deck_ext_prefade_dsp.stages[ 0 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ;
    dto->deck_ext_prefade_dsp.stages[ 0 ].id = ZDJ_SOUNDCARD_DSP_ID_EQ_3_4P;
    dto->deck_ext_prefade_dsp.stages[ 0 ].knob_0 = 1.0; // Lo
    dto->deck_ext_prefade_dsp.stages[ 0 ].knob_1 = 1.0; // Mid
    dto->deck_ext_prefade_dsp.stages[ 0 ].knob_2 = 1.0; // Hi
    dto->deck_ext_prefade_dsp.stages[ 0 ].knob_3 = 0.250; // Xover Hi
    dto->deck_ext_prefade_dsp.stages[ 0 ].knob_4 = 1.0; // Xover Lo
    dto->deck_ext_prefade_dsp.stages[ 1 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_FILT;
    dto->deck_ext_prefade_dsp.stages[ 1 ].id = ZDJ_SOUNDCARD_DSP_ID_FILT_BI;
    dto->deck_ext_prefade_dsp.stages[ 1 ].knob_0 = 0.0; // Freq
    dto->deck_ext_prefade_dsp.stages[ 1 ].knob_2 = 0.0; // Res

    // Main Clock
    dto->clock_0_sig = ZDJ_SOUNDCARD_SIGNAL_XPORT_ANALOG_PPQN_4; // Clock output 4 PPQN
    dto->clock_0_source = ZDJ_SOUNDCARD_CLOCK_DIRECTION_OUTPUT; // Clock source Output
    dto->clock_0_sync = ZDJ_SOUNDCARD_CLOCK_SYNC_NORMAL;
    dto->clock_0_val = 120.0; // Clock BPM setting

    // Crossfader
    dto->xfade_a_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS;
    dto->xfade_a_dsp.stages[ 0 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_XFADE;
    dto->xfade_a_dsp.stages[ 0 ].id = ZDJ_SOUNDCARD_DSP_ID_XFADE_CURVE;
    dto->xfade_a_dsp.stages[ 0 ].knob_0 = 0.8; // Crossfader curve
    dto->xfade_b_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS;
    dto->xfade_b_dsp.stages[ 0 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_XFADE;
    dto->xfade_b_dsp.stages[ 0 ].id = ZDJ_SOUNDCARD_DSP_ID_XFADE_CURVE;
    dto->xfade_b_dsp.stages[ 0 ].knob_0 = 0.8; // Crossfader curve

    zdj_soundcard_store_dto( dto );

    free( dto );


    ////////////////////////////////////
    // DAWless 2x Stereo Mixer        //
    // - Main L/R is routed to Port 1 //
    // - Cue is routed to Port 2      //
    ////////////////////////////////////
    dto = zdj_soundcard_create_dto( );
    strcpy( dto->name, "DAWless 2x Stereo" );

    // Add default linkages

    // Analog input outs
    dto->ana_in_0_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT;
    dto->ana_in_0_stereo = true;
    dto->ana_in_1_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT;
    dto->ana_in_1_stereo = true;
    dto->ana_in_2_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT;
    dto->ana_in_2_stereo = true;
    dto->ana_in_3_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT;
    dto->ana_in_3_stereo = true;

    dto->ana_out_0_stereo = false;
    dto->ana_out_0_sig = ZDJ_SOUNDCARD_SIGNAL_PRO_PLUS_4_DBU;
    dto->ana_out_1_stereo = false;
    dto->ana_out_1_sig = ZDJ_SOUNDCARD_SIGNAL_XPORT_ANALOG_PPQN_4;
    dto->ana_out_2_stereo = true;
    dto->ana_out_2_sig = ZDJ_SOUNDCARD_SIGNAL_HEADPHONE_HI;
    dto->ana_out_3_stereo = true;
    dto->ana_out_3_sig = ZDJ_SOUNDCARD_SIGNAL_HEADPHONE_HI;

    // USB I/O
    dto->usb_in_0_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT;
    dto->usb_in_0_stereo = true;
    dto->usb_in_1_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT;
    dto->usb_in_1_stereo = true;

    dto->usb_out_0_stereo = true;
    dto->usb_out_1_stereo = true;

    // Admin bus outs
    dto->annot_bus_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS;
    dto->annot_bus_stereo = false;
    dto->cue_bus_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0;
    dto->cue_bus_stereo = true;
    dto->main_bus_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2;
    dto->main_bus_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3;
    dto->main_bus_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS;
    dto->main_bus_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_0;
    dto->main_bus_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_1;
    dto->main_bus_stereo = true;
    dto->record_bus_stereo = true;
    
    // Deck 1
    dto->deck_1_input_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE;
    dto->deck_1_edge_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE;
    dto->deck_1_prefade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_1_POSTFADE;
    dto->deck_1_prefade_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_1_CUE;
    dto->deck_1_cue_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS;
    dto->deck_1_postfade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_XFADE_A;
    dto->deck_1_prefade_dsp.stages[ 0 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ;
    dto->deck_1_prefade_dsp.stages[ 0 ].id = ZDJ_SOUNDCARD_DSP_ID_EQ_3_4P;
    dto->deck_1_prefade_dsp.stages[ 0 ].knob_0 = 1.0; // Lo
    dto->deck_1_prefade_dsp.stages[ 0 ].knob_1 = 1.0; // Mid
    dto->deck_1_prefade_dsp.stages[ 0 ].knob_2 = 1.0; // Hi
    dto->deck_1_prefade_dsp.stages[ 0 ].knob_3 = 0.250; // Xover Hi
    dto->deck_1_prefade_dsp.stages[ 0 ].knob_4 = 1.0; // Xover Lo
    dto->deck_1_prefade_dsp.stages[ 1 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_FILT;
    dto->deck_1_prefade_dsp.stages[ 1 ].id = ZDJ_SOUNDCARD_DSP_ID_FILT_BI;
    dto->deck_1_prefade_dsp.stages[ 1 ].knob_0 = 0.0; // Freq
    dto->deck_1_prefade_dsp.stages[ 1 ].knob_2 = 0.0; // Res

    // Deck 2
    dto->deck_2_input_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE;
    dto->deck_2_edge_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE;
    dto->deck_2_prefade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0;
    dto->deck_2_prefade_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_2_CUE;
    dto->deck_2_cue_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS;
    dto->deck_2_postfade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_XFADE_B;
    dto->deck_2_prefade_dsp.stages[ 0 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ;
    dto->deck_2_prefade_dsp.stages[ 0 ].id = ZDJ_SOUNDCARD_DSP_ID_EQ_3_4P;
    dto->deck_2_prefade_dsp.stages[ 0 ].knob_0 = 1.0; // Lo
    dto->deck_2_prefade_dsp.stages[ 0 ].knob_1 = 1.0; // Mid
    dto->deck_2_prefade_dsp.stages[ 0 ].knob_2 = 1.0; // Hi
    dto->deck_2_prefade_dsp.stages[ 0 ].knob_3 = 0.250; // Xover Hi
    dto->deck_2_prefade_dsp.stages[ 0 ].knob_4 = 1.0; // Xover Lo
    dto->deck_2_prefade_dsp.stages[ 1 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_FILT;
    dto->deck_2_prefade_dsp.stages[ 1 ].id = ZDJ_SOUNDCARD_DSP_ID_FILT_BI;
    dto->deck_2_prefade_dsp.stages[ 1 ].knob_0 = 0.0; // Freq
    dto->deck_2_prefade_dsp.stages[ 1 ].knob_2 = 0.0; // Res

    // Ext Deck
    dto->deck_ext_input_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE;
    dto->deck_ext_edge_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE;
    dto->deck_ext_prefade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_2_POSTFADE;
    dto->deck_ext_prefade_link_map |= 1ULL << ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_CUE;
    dto->deck_ext_cue_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS;
    dto->deck_ext_postfade_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS;
    dto->deck_ext_prefade_dsp.stages[ 0 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ;
    dto->deck_ext_prefade_dsp.stages[ 0 ].id = ZDJ_SOUNDCARD_DSP_ID_EQ_3_4P;
    dto->deck_ext_prefade_dsp.stages[ 0 ].knob_0 = 1.0; // Lo
    dto->deck_ext_prefade_dsp.stages[ 0 ].knob_1 = 1.0; // Mid
    dto->deck_ext_prefade_dsp.stages[ 0 ].knob_2 = 1.0; // Hi
    dto->deck_ext_prefade_dsp.stages[ 0 ].knob_3 = 0.250; // Xover Hi
    dto->deck_ext_prefade_dsp.stages[ 0 ].knob_4 = 1.0; // Xover Lo
    dto->deck_ext_prefade_dsp.stages[ 1 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_FILT;
    dto->deck_ext_prefade_dsp.stages[ 1 ].id = ZDJ_SOUNDCARD_DSP_ID_FILT_BI;
    dto->deck_ext_prefade_dsp.stages[ 1 ].knob_0 = 0.0; // Freq
    dto->deck_ext_prefade_dsp.stages[ 1 ].knob_2 = 0.0; // Res

    // Main Clock
    dto->clock_1_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1;
    dto->clock_1_sig = ZDJ_SOUNDCARD_SIGNAL_XPORT_ANALOG_PPQN_4; // Clock output 4 PPQN
    dto->clock_1_source = ZDJ_SOUNDCARD_CLOCK_DIRECTION_OUTPUT; // Clock source Input
    dto->clock_1_sync = ZDJ_SOUNDCARD_CLOCK_SYNC_NORMAL;
    dto->clock_1_val = 120.0; // Clock BPM setting

    dto->clock_0_sig = ZDJ_SOUNDCARD_SIGNAL_XPORT_ANALOG_PPQN_4; // Clock output 4 PPQN
    dto->clock_0_source = ZDJ_SOUNDCARD_CLOCK_DIRECTION_INPUT; // Clock source Input
    dto->clock_0_sync = ZDJ_SOUNDCARD_CLOCK_SYNC_NORMAL;
    dto->clock_0_val = 120.0; // Clock BPM setting

    // Crossfader
    dto->xfade_a_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS;
    dto->xfade_a_dsp.stages[ 0 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_XFADE;
    dto->xfade_a_dsp.stages[ 0 ].id = ZDJ_SOUNDCARD_DSP_ID_XFADE_CURVE;
    dto->xfade_a_dsp.stages[ 0 ].knob_0 = 0.8; // Crossfader curve
    dto->xfade_b_link_map = 1ULL << ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS;
    dto->xfade_b_dsp.stages[ 0 ].type = ZDJ_SOUNDCARD_DSP_STAGE_TYPE_XFADE;
    dto->xfade_b_dsp.stages[ 0 ].id = ZDJ_SOUNDCARD_DSP_ID_XFADE_CURVE;
    dto->xfade_b_dsp.stages[ 0 ].knob_0 = 0.8; // Crossfader curve

    zdj_soundcard_store_dto( dto );
}
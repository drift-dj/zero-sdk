#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>


#include <zerodj/signal/pipeline/node/analysis/meter/zdj_meter_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/soundcard_view/zdj_soundcard_view.h>
#include <zerodj/ui/view/soundcard_view/meters/zdj_soundcard_meter.h>
#include <zerodj/ui/view/zdj_view_stack.h>

zdj_error_type_t zdj_meter_view_set_label( 
    zdj_view_t * meter, 
    zdj_soundcard_meter_label_t label 
) {
    zdj_soundcard_meter_state_t * state = (zdj_soundcard_meter_state_t*)meter->state;
    state->label = label;
}

zdj_soundcard_meter_label_t zdj_meter_label_for_node( zdj_soundcard_node_t * node ) {
    switch ( node->name ) {
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0: 
            if( node->stereo ) { return ZDJ_SOUNDCARD_LABEL_ANALOG_OUT_12; } else { return ZDJ_SOUNDCARD_LABEL_ANALOG_OUT_1; }
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1:
            if( node->stereo ) { return ZDJ_SOUNDCARD_LABEL_ANALOG_OUT_12; } else { return ZDJ_SOUNDCARD_LABEL_ANALOG_OUT_2; }
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2:
            if( node->stereo ) { return ZDJ_SOUNDCARD_LABEL_ANALOG_OUT_34; } else { return ZDJ_SOUNDCARD_LABEL_ANALOG_OUT_3; }
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3: return ZDJ_SOUNDCARD_LABEL_ANALOG_OUT_4;
            if( node->stereo ) { return ZDJ_SOUNDCARD_LABEL_ANALOG_OUT_34; } else { return ZDJ_SOUNDCARD_LABEL_ANALOG_OUT_4; }
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0:
            if( node->stereo ) { return ZDJ_SOUNDCARD_LABEL_ANALOG_OUT_12; } else { return ZDJ_SOUNDCARD_LABEL_ANALOG_IN_1; }
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1:
            if( node->stereo ) { return ZDJ_SOUNDCARD_LABEL_ANALOG_IN_12; } else { return ZDJ_SOUNDCARD_LABEL_ANALOG_IN_2; }
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2:
            if( node->stereo ) { return ZDJ_SOUNDCARD_LABEL_ANALOG_IN_34; } else { return ZDJ_SOUNDCARD_LABEL_ANALOG_IN_3; }
        case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3:
            if( node->stereo ) { return ZDJ_SOUNDCARD_LABEL_ANALOG_IN_34; } else { return ZDJ_SOUNDCARD_LABEL_ANALOG_IN_4; }
        case ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS: return ZDJ_SOUNDCARD_LABEL_MAIN_BUS;
        case ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS: return ZDJ_SOUNDCARD_LABEL_CUE_BUS;
        case ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS: return ZDJ_SOUNDCARD_LABEL_ANNOT_BUS;
        case ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS: return ZDJ_SOUNDCARD_LABEL_RECORD_BUS;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0: return ZDJ_SOUNDCARD_LABEL_AUX_1;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1: return ZDJ_SOUNDCARD_LABEL_AUX_2;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2: return ZDJ_SOUNDCARD_LABEL_AUX_3;
        case ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3: return ZDJ_SOUNDCARD_LABEL_AUX_4;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_0: return ZDJ_SOUNDCARD_LABEL_CLOCK_1;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_1: return ZDJ_SOUNDCARD_LABEL_CLOCK_2;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_2: return ZDJ_SOUNDCARD_LABEL_CLOCK_3;
        case ZDJ_SOUNDCARD_NODE_NAME_CLOCK_3: return ZDJ_SOUNDCARD_LABEL_CLOCK_4;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_0: return ZDJ_SOUNDCARD_LABEL_CV_1;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_1: return ZDJ_SOUNDCARD_LABEL_CV_2;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_2: return ZDJ_SOUNDCARD_LABEL_CV_3;
        case ZDJ_SOUNDCARD_NODE_NAME_CV_3: return ZDJ_SOUNDCARD_LABEL_CV_4;
        case ZDJ_SOUNDCARD_NODE_NAME_USB_OUT: return ZDJ_SOUNDCARD_LABEL_USB_12;
        case ZDJ_SOUNDCARD_NODE_NAME_USB_IN: return ZDJ_SOUNDCARD_LABEL_USB_12;
        default: ZDJ_SOUNDCARD_LABEL_MAIN_BUS;
    }
}

zdj_ui_asset_t zdj_meter_asset_for_label( zdj_soundcard_meter_label_t label ) {
    switch ( label ) {
        case ZDJ_SOUNDCARD_LABEL_MAIN_BUS: return ZDJ_UI_ASSET_MIXER_LR_BUS;
        case ZDJ_SOUNDCARD_LABEL_CUE_BUS: return ZDJ_UI_ASSET_MIXER_CUE_BUS;
        case ZDJ_SOUNDCARD_LABEL_ANNOT_BUS: return ZDJ_UI_ASSET_MIXER_ANNOT_BUS;
        case ZDJ_SOUNDCARD_LABEL_RECORD_BUS: return ZDJ_UI_ASSET_MIXER_RECORD_BUS;
        case ZDJ_SOUNDCARD_LABEL_ANALOG_IN_12: return ZDJ_UI_ASSET_MIXER_ANA_IN_12;
        case ZDJ_SOUNDCARD_LABEL_ANALOG_IN_34: return ZDJ_UI_ASSET_MIXER_ANA_IN_34;
        case ZDJ_SOUNDCARD_LABEL_ANALOG_IN_1: return ZDJ_UI_ASSET_MIXER_ANA_IN_1;
        case ZDJ_SOUNDCARD_LABEL_ANALOG_IN_2: return ZDJ_UI_ASSET_MIXER_ANA_IN_2;
        case ZDJ_SOUNDCARD_LABEL_ANALOG_IN_3: return ZDJ_UI_ASSET_MIXER_ANA_IN_3;
        case ZDJ_SOUNDCARD_LABEL_ANALOG_IN_4: return ZDJ_UI_ASSET_MIXER_ANA_IN_4;
        case ZDJ_SOUNDCARD_LABEL_ANALOG_OUT_12: return ZDJ_UI_ASSET_MIXER_ANA_OUT_12;
        case ZDJ_SOUNDCARD_LABEL_ANALOG_OUT_34: return ZDJ_UI_ASSET_MIXER_ANA_OUT_34;
        case ZDJ_SOUNDCARD_LABEL_ANALOG_OUT_1: return ZDJ_UI_ASSET_MIXER_ANA_OUT_1;
        case ZDJ_SOUNDCARD_LABEL_ANALOG_OUT_2: return ZDJ_UI_ASSET_MIXER_ANA_OUT_2;
        case ZDJ_SOUNDCARD_LABEL_ANALOG_OUT_3: return ZDJ_UI_ASSET_MIXER_ANA_OUT_3;
        case ZDJ_SOUNDCARD_LABEL_ANALOG_OUT_4: return ZDJ_UI_ASSET_MIXER_ANA_OUT_4;
        case ZDJ_SOUNDCARD_LABEL_USB_12: return ZDJ_UI_ASSET_MIXER_USB_IO_12;
        case ZDJ_SOUNDCARD_LABEL_USB_34: return ZDJ_UI_ASSET_MIXER_USB_IO_34;
        case ZDJ_SOUNDCARD_LABEL_USB_1: return ZDJ_UI_ASSET_MIXER_USB_IO_1;
        case ZDJ_SOUNDCARD_LABEL_USB_2: return ZDJ_UI_ASSET_MIXER_USB_IO_2;
        case ZDJ_SOUNDCARD_LABEL_USB_3: return ZDJ_UI_ASSET_MIXER_USB_IO_3;
        case ZDJ_SOUNDCARD_LABEL_USB_4: return ZDJ_UI_ASSET_MIXER_USB_IO_4;
        case ZDJ_SOUNDCARD_LABEL_AUX_12: return ZDJ_UI_ASSET_MIXER_AUX_12;
        case ZDJ_SOUNDCARD_LABEL_AUX_34: return ZDJ_UI_ASSET_MIXER_AUX_34;
        case ZDJ_SOUNDCARD_LABEL_AUX_1: return ZDJ_UI_ASSET_MIXER_AUX_1;
        case ZDJ_SOUNDCARD_LABEL_AUX_2: return ZDJ_UI_ASSET_MIXER_AUX_2;
        case ZDJ_SOUNDCARD_LABEL_AUX_3: return ZDJ_UI_ASSET_MIXER_AUX_3;
        case ZDJ_SOUNDCARD_LABEL_AUX_4: return ZDJ_UI_ASSET_MIXER_AUX_4;
        case ZDJ_SOUNDCARD_LABEL_CLOCK_1: return ZDJ_UI_ASSET_MIXER_CLOCK_1;
        case ZDJ_SOUNDCARD_LABEL_CLOCK_2: return ZDJ_UI_ASSET_MIXER_CLOCK_2;
        case ZDJ_SOUNDCARD_LABEL_CLOCK_3: return ZDJ_UI_ASSET_MIXER_CLOCK_3;
        case ZDJ_SOUNDCARD_LABEL_CLOCK_4: return ZDJ_UI_ASSET_MIXER_CLOCK_4;
        case ZDJ_SOUNDCARD_LABEL_CV_1: return ZDJ_UI_ASSET_MIXER_CV_1;
        case ZDJ_SOUNDCARD_LABEL_CV_2: return ZDJ_UI_ASSET_MIXER_CV_2;
        case ZDJ_SOUNDCARD_LABEL_CV_3: return ZDJ_UI_ASSET_MIXER_CV_3;
        case ZDJ_SOUNDCARD_LABEL_CV_4: return ZDJ_UI_ASSET_MIXER_CV_4;
        case ZDJ_SOUNDCARD_LABEL_MIDI_1: return ZDJ_UI_ASSET_MIXER_MIDI_1;
        case ZDJ_SOUNDCARD_LABEL_MIDI_2: return ZDJ_UI_ASSET_MIXER_MIDI_2;
        case ZDJ_SOUNDCARD_LABEL_MIDI_3: return ZDJ_UI_ASSET_MIXER_MIDI_3;
        case ZDJ_SOUNDCARD_LABEL_MIDI_4: return ZDJ_UI_ASSET_MIXER_MIDI_4;
        default: return ZDJ_UI_ASSET_ERROR_TEX;
    }
}
// Copyright (c) 2025 Drift DJ Industries

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef SOUNDCARD_H
#define SOUNDCARD_H

#include <stdbool.h>

#include <sqlite3.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/soundcard/zdj_soundcard_dto.h>

#define ZDJ_SOUNDCARD_BUF_LEN 64

#define ZDJ_SOUNDCARD_DB_PATH "/etc/zero_data/soundcard.db"
#define ZDJ_SOUNDCARD_DEFAULT_DJ "DJ_Default"
#define ZDJ_SOUNDCARD_DEFAULT_LIB "Library_Default"

typedef enum {
    ZDJ_SOUNDCARD_SIGNAL_CON_0_DBV,
    ZDJ_SOUNDCARD_SIGNAL_PRO_PLUS_4_DBU,
    ZDJ_SOUNDCARD_SIGNAL_HEADPHONE_LOW,
    ZDJ_SOUNDCARD_SIGNAL_HEADPHONE_HI,
    ZDJ_SOUNDCARD_SIGNAL_EURO_AUDIO,
    ZDJ_SOUNDCARD_SIGNAL_CLOCK_PPQN,
    ZDJ_SOUNDCARD_SIGNAL_CLOCK_QNPP,
    ZDJ_SOUNDCARD_SIGNAL_CLOCK_TRIG,
    ZDJ_SOUNDCARD_SIGNAL_CLOCK_START_HI,
    ZDJ_SOUNDCARD_SIGNAL_CLOCK_START_LO,
    ZDJ_SOUNDCARD_SIGNAL_CLOCK_STARTSTOP_PULSE,
    ZDJ_SOUNDCARD_SIGNAL_CV_UNIPOLAR,
    ZDJ_SOUNDCARD_SIGNAL_CV_BIPOLAR,
    ZDJ_SOUNDCARD_SIGNAL_CV_1V_OCT,
    ZDJ_SOUNDCARD_SIGNAL_CV_TRIGGER,
    ZDJ_SOUNDCARD_SIGNAL_USB_MIDI,
    ZDJ_SOUNDCARD_SIGNAL_USB_AUDIO,
    ZDJ_SOUNDCARD_SIGNAL_INTERNAL_CANONICAL,
    ZDJ_SOUNDCARD_SIGNAL_COUNT
} zdj_soundcard_signal_type_t;

static char * zdj_soundcard_signal_name[ ZDJ_SOUNDCARD_SIGNAL_COUNT ] = { 
    "Line 0 dBV",// SOUNDCARD_PAD_CON_0_DBV,
    "Line Pro +4 dBu",// SOUNDCARD_PAD_PRO_PLUS_4_DBU,
    "Headphone Low",// SOUNDCARD_PAD_PRO_PLUS_4_DBU,
    "Headphone Hi",// SOUNDCARD_PAD_PRO_PLUS_4_DBU,
    "Euro. Audio",// SOUNDCARD_PAD_EURO_AUDIO,
    "PPQN Clock",// ZDJ_SOUNDCARD_SIGNAL_EURO_CLOCK_PPQN,
    "QNPP Clock",// ZDJ_SOUNDCARD_SIGNAL_EURO_CLOCK_QNPP,
    "Trigger Clock",// ZDJ_SOUNDCARD_SIGNAL_EURO_TRIG,
    "Clock Start Hi",// ZDJ_SOUNDCARD_SIGNAL_CLOCK_START_HI,
    "Clock Start Lo",// ZDJ_SOUNDCARD_SIGNAL_CLOCK_START_LO,
    "Clock Start Pulse",// ZDJ_SOUNDCARD_SIGNAL_CLOCK_STARTSTOP_PULSE,
    "CV Unipolar", // ZDJ_SOUNDCARD_SIGNAL_CV_UNIPOLAR,
    "CV Bipolar", // ZDJ_SOUNDCARD_SIGNAL_CV_BIPOLAR,
    "CV 1V/Oct.", // ZDJ_SOUNDCARD_SIGNAL_CV_1V_OCT,
    "CV Trigger", // ZDJ_SOUNDCARD_SIGNAL_CV_TRIGGER,
    "USB MIDI",// ZDJ_SOUNDCARD_SIGNAL_USB_MIDI,
    "USB Audio"// ZDJ_SOUNDCARD_SIGNAL_USB_AUDIO,
};

// NEVER EVER Re-order these //
// This enum sources the bit position of each node in the dto linkmap.
// Ex. a link to ANALOG_OUT_0 is found at (link_map & 1U << ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0)
typedef enum {
    ZDJ_SOUNDCARD_NODE_NAME_UNKNOWN, // 0
    ZDJ_SOUNDCARD_NODE_NAME_NONE,
    ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0,
    ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1,
    ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2,
    ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3,
    ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0,
    ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1,
    ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2,
    ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3,
    
    ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS, // 10
    ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS,
    ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS,
    ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS,
    ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0,
    ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1,
    ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2,
    ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3,
    ZDJ_SOUNDCARD_NODE_NAME_CLOCK_0,
    ZDJ_SOUNDCARD_NODE_NAME_CLOCK_1,

    ZDJ_SOUNDCARD_NODE_NAME_CLOCK_2, // 20
    ZDJ_SOUNDCARD_NODE_NAME_CLOCK_3,
    ZDJ_SOUNDCARD_NODE_NAME_CV_0,
    ZDJ_SOUNDCARD_NODE_NAME_CV_1,
    ZDJ_SOUNDCARD_NODE_NAME_CV_2,
    ZDJ_SOUNDCARD_NODE_NAME_CV_3,
    ZDJ_SOUNDCARD_NODE_NAME_USB_OUT,
    ZDJ_SOUNDCARD_NODE_NAME_USB_IN,
    ZDJ_SOUNDCARD_NODE_NAME_DECK_1_INPUT,
    ZDJ_SOUNDCARD_NODE_NAME_DECK_1_BUS,

    ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE, // 30
    ZDJ_SOUNDCARD_NODE_NAME_DECK_2_INPUT,
    ZDJ_SOUNDCARD_NODE_NAME_DECK_2_BUS,
    ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE,
    ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT,
    ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_BUS,
    ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE,

    ZDJ_SOUNDCARD_NODE_NAME_COUNT // 37
} zdj_soundcard_node_name_t;

static char * zdj_soundcard_node_name[ ZDJ_SOUNDCARD_NODE_NAME_COUNT ] = {
    "Unknown",// ZDJ_SOUNDCARD_NODE_NAME_UNKNOWN,
    "None",// ZDJ_SOUNDCARD_NODE_NAME_NONE,
    "Analog Out 1",// ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0,
    "Analog Out 2",// ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1,
    "Analog Out 3",// ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2,
    "Analog Out 4",// ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3,
    "Analog In 1",// ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0,
    "Analog In 2",// ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1,
    "Analog In 3",// ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2,
    "Analog In 4",// ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3,

    "Main L/R Bus",// ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS,
    "Cue Bus",// ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS,
    "Annotation Bus",// ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS,
    "Recording Bus",// ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS,
    "Aux Bus 1",// ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0,
    "Aux Bus 2",// ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1,
    "Aux Bus 3",// ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2,
    "Aux Bus 4",// ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3,
    "Clock 1",// ZDJ_SOUNDCARD_NODE_NAME_CLOCK_0,
    "Clock 2",// ZDJ_SOUNDCARD_NODE_NAME_CLOCK_1,

    "Clock 3",// ZDJ_SOUNDCARD_NODE_NAME_CLOCK_2,
    "Clock 4",// ZDJ_SOUNDCARD_NODE_NAME_CLOCK_3,
    "CV 1",// ZDJ_SOUNDCARD_NODE_NAME_CV_0,
    "CV 2",// ZDJ_SOUNDCARD_NODE_NAME_CV_1,
    "CV 3",// ZDJ_SOUNDCARD_NODE_NAME_CV_2,
    "CV 4",// ZDJ_SOUNDCARD_NODE_NAME_CV_3,
    "USB Out",// ZDJ_SOUNDCARD_NODE_NAME_USB_OUT,
    "USB In",// ZDJ_SOUNDCARD_NODE_NAME_USB_IN,
    "Deck 1 Input",// ZDJ_SOUNDCARD_NODE_NAME_DECK_1_INPUT,
    "Deck 1 Bus",// ZDJ_SOUNDCARD_NODE_NAME_DECK_1_BUS,

    "Deck 1 Prefade",// ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE, // 30
    "Deck 2 Input",// ZDJ_SOUNDCARD_NODE_NAME_DECK_2_INPUT,
    "Deck 2 Bus",// ZDJ_SOUNDCARD_NODE_NAME_DECK_2_BUS,
    "Deck 2 Prefade",// ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE,
    "Ext. Deck Input",// ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT,
    "Ext. Deck Bus",// ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_BUS,
    "Ext. Deck Prefade"// ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE,
};

typedef uint64_t zdj_soundcard_link_bitmap_t;

typedef struct {
    zdj_soundcard_node_name_t source_node;
    zdj_soundcard_node_name_t dest_node;
} zdj_soundcard_link_t;

typedef enum {
    ZDJ_SOUNDCARD_CLOCK_SOURCE_INPUT,
    ZDJ_SOUNDCARD_CLOCK_SOURCE_BEAT_GRID,
    ZDJ_SOUNDCARD_CLOCK_SOURCE_SYNTH
} zdj_soundcard_clock_source_t;

typedef enum {
    ZDJ_SOUNDCARD_CV_SOURCE_INPUT,
    ZDJ_SOUNDCARD_CV_SOURCE_SYNTH
} zdj_soundcard_cv_source_t;

typedef struct {
    int source_channel_count;
    int source_channel_stride;
    int source_channel_offset;
    int source_data_size;
    int source_pan;
    int dest_channel_count;
    int dest_channel_stride;
    int dest_channel_offset;
    int dest_data_size;
} zdj_soundcard_accumulate_map_t;

typedef struct  {
    zdj_soundcard_node_name_t name;
    struct zdj_soundcard_node_t * next;
    struct zdj_soundcard_node_t * prev;
    bool show_meter; // Show this node's meter in routing mixer UI
    zdj_soundcard_signal_type_t signal_type;
    int gain;
    int pan;
    int stereo;
    int mute;
    int source;
    int val;
    int invert;
    zdj_pipeline_node_t * data_pipe;
    zdj_pipeline_node_t * meter_pipe;
    void ( *get_edge_data )( zdj_pipeline_node_t * );
    zdj_soundcard_link_bitmap_t link_map;
    int input_link_count;
    zdj_soundcard_link_t input_links[ 8 ];
    int output_link_count;
    zdj_soundcard_link_t output_links[ 8 ];
    bool mix_complete;
    void ( *dsp )( struct zdj_soundcard_node_t * );
} zdj_soundcard_node_t;

typedef struct {
    char name[ 128 ];
    zdj_soundcard_dto_t dto;
    zdj_soundcard_node_t * nodes;
    zdj_pipeline_node_t * analog_io_node;
    zdj_pipeline_node_t * usb_io_node;
    bool has_edits;
} zdj_soundcard_t;

extern zdj_soundcard_t * zdj_soundcard;

zdj_error_type_t zdj_soundcard_init( char * entity_id );
zdj_error_type_t zdj_soundcard_load( zdj_soundcard_t * soundcard, char * entity_id );
zdj_error_type_t zdj_soundcard_save( zdj_soundcard_t * soundcard, char * entity_id );
zdj_error_type_t zdj_soundcard_save_temp( zdj_soundcard_t * soundcard );

// Transport
zdj_error_type_t zdj_soundcard_start( zdj_soundcard_t * soundcard );
zdj_error_type_t zdj_soundcard_stop( zdj_soundcard_t * soundcard );

// Perf
zdj_error_type_t zdj_soundcard_enable_perf( zdj_soundcard_t * soundcard, uint32_t tag_count );
zdj_error_type_t zdj_soundcard_disable_perf( zdj_soundcard_t * soundcard );
zdj_error_type_t zdj_soundcard_reset_perf( zdj_soundcard_t * soundcard );
zdj_pipeline_perf_report_t * zdj_soundcard_make_fast_cycle_perf_report(zdj_soundcard_t * soundcard );

// Mixing/DSP
zdj_error_type_t zdj_soundcard_mix_input( 
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_t * node 
);
zdj_error_type_t zdj_soundcard_accumulate_node(
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_t * input_node,
    zdj_soundcard_node_t * node
);

// Decks
zdj_error_type_t zdj_soundcard_link_deck( 
    zdj_soundcard_t * soundcard, 
    zdj_deck_t * deck 
);

// Node
zdj_soundcard_node_t * zdj_soundcard_create_node( zdj_soundcard_node_name_t name );
zdj_error_type_t zdj_soundcard_install_node( 
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_t * node 
);
zdj_error_type_t zdj_soundcard_remove_node( 
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_t * node 
);
zdj_error_type_t zdj_soundcard_remove_all_nodes( zdj_soundcard_t * soundcard );
zdj_soundcard_node_t * zdj_soundcard_get_node_for_name( 
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_name_t name 
);

// Node Linkage
zdj_error_type_t zdj_soundcard_unlink_all_nodes_from_node( 
    zdj_soundcard_t * soundcard,
    zdj_soundcard_node_t * node 
);
zdj_error_type_t zdj_soundcard_link_source_node_to_dest_node( 
    zdj_soundcard_t * soundcard,
    zdj_soundcard_node_t * source_node,
    zdj_soundcard_node_t * dest_node
);
zdj_error_type_t zdj_soundcard_unlink_source_node_from_dest_node( 
    zdj_soundcard_t * soundcard,
    zdj_soundcard_node_t * source_node,
    zdj_soundcard_node_t * dest_node
);
zdj_error_type_t zdj_soundcard_pull_node_links_from_dto( 
    zdj_soundcard_t * soundcard,
    zdj_soundcard_node_t * node
);
int zdj_soundcard_count_input_nodes_to_node_name( 
    zdj_soundcard_dto_t * dto,
    zdj_soundcard_node_name_t name 
);
int zdj_soundcard_count_output_nodes_from_node( 
    zdj_soundcard_dto_t * dto,
    zdj_soundcard_node_t * node 
);
int zdj_soundcard_count_output_nodes_from_link_map( 
    zdj_soundcard_link_bitmap_t link_map
);

// Node Getters/Setters
void zdj_soundcard_set_mute_for_node( zdj_soundcard_node_t * node, bool stereo );
void zdj_soundcard_set_stereo_for_node( zdj_soundcard_node_t * node, bool stereo );
zdj_soundcard_node_t * zdj_soundcard_node_get_stereo_partner_node( zdj_soundcard_node_t * node );
zdj_error_type_t zdj_soundcard_get_port_title_with_stereo( 
    zdj_soundcard_t * soundcard,
    zdj_soundcard_node_name_t name, 
    char * str 
);
zdj_error_type_t zdj_soundcard_cycle_pad_for_io_node(
    zdj_soundcard_t * soundcard, zdj_soundcard_node_t * node
);
zdj_soundcard_node_t * zdj_soundcard_get_available_aux_bus_node( zdj_soundcard_t * soundcard );
zdj_soundcard_node_t * zdj_soundcard_get_available_clock_bus_node( zdj_soundcard_t * soundcard );
zdj_soundcard_node_t * zdj_soundcard_get_available_cv_bus_node( zdj_soundcard_t * soundcard );
zdj_soundcard_node_t * zdj_soundcard_get_available_midi_bus_node( zdj_soundcard_t * soundcard );

// Node Calculated Properties
bool zdj_soundcard_can_add_aux_bus( zdj_soundcard_t * soundcard );
bool zdj_soundcard_can_add_clock_bus( zdj_soundcard_t * soundcard );
bool zdj_soundcard_can_add_cv_bus( zdj_soundcard_t * soundcard );
bool zdj_soundcard_can_add_midi_bus( zdj_soundcard_t * soundcard );
bool zdj_soundcard_node_name_show_in_mixer( zdj_soundcard_node_name_t name );
bool zdj_soundcard_node_name_is_audio( zdj_soundcard_node_name_t name );
bool zdj_soundcard_node_name_is_io( zdj_soundcard_node_name_t name );
bool zdj_soundcard_node_name_is_input( zdj_soundcard_node_name_t name );
bool zdj_soundcard_node_name_is_analog_input( zdj_soundcard_node_name_t name );
bool zdj_soundcard_node_name_is_output( zdj_soundcard_node_name_t name );
bool zdj_soundcard_node_name_is_analog_output( zdj_soundcard_node_name_t name );
bool zdj_soundcard_node_name_is_internal_bus( zdj_soundcard_node_name_t name );
bool zdj_soundcard_node_name_is_aux_bus( zdj_soundcard_node_name_t name );
bool zdj_soundcard_node_name_is_physical_port( zdj_soundcard_node_name_t name );
bool zdj_soundcard_node_name_is_clock( zdj_soundcard_node_name_t name );
bool zdj_soundcard_node_name_is_cv( zdj_soundcard_node_name_t name );
bool zdj_soundcard_node_name_is_muted( zdj_soundcard_node_name_t name );
bool zdj_soundcard_node_name_is_midi( zdj_soundcard_node_name_t name );
bool zdj_soundcard_node_name_is_usb( zdj_soundcard_node_name_t name );

#endif
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

#ifndef SOUNDCARD_DTO_H
#define SOUNDCARD_DTO_H

#include <stdint.h>
#include <stdbool.h>

#include <sqlite3.h>


typedef enum {
	ZDJ_SOUNDCARD_COL_ENTITY_ID,
	ZDJ_SOUNDCARD_COL_NAME,

	ZDJ_SOUNDCARD_COL_ANA_OUT_0_SIG,
	ZDJ_SOUNDCARD_COL_ANA_OUT_0_STEREO,
	ZDJ_SOUNDCARD_COL_ANA_OUT_1_SIG,
	ZDJ_SOUNDCARD_COL_ANA_OUT_1_STEREO,
	ZDJ_SOUNDCARD_COL_ANA_OUT_2_SIG,
	ZDJ_SOUNDCARD_COL_ANA_OUT_2_STEREO,
	ZDJ_SOUNDCARD_COL_ANA_OUT_3_SIG,
	ZDJ_SOUNDCARD_COL_ANA_OUT_3_STEREO,

	ZDJ_SOUNDCARD_COL_ANA_IN_0_LINK,
	ZDJ_SOUNDCARD_COL_ANA_IN_0_SIG,
	ZDJ_SOUNDCARD_COL_ANA_IN_0_TRIM,
	ZDJ_SOUNDCARD_COL_ANA_IN_0_PAN,
	ZDJ_SOUNDCARD_COL_ANA_IN_0_STEREO,
	ZDJ_SOUNDCARD_COL_ANA_IN_0_MUTE,
	ZDJ_SOUNDCARD_COL_ANA_IN_1_LINK,
	ZDJ_SOUNDCARD_COL_ANA_IN_1_SIG,
	ZDJ_SOUNDCARD_COL_ANA_IN_1_TRIM,
	ZDJ_SOUNDCARD_COL_ANA_IN_1_PAN,
	ZDJ_SOUNDCARD_COL_ANA_IN_1_STEREO,
	ZDJ_SOUNDCARD_COL_ANA_IN_1_MUTE,
	ZDJ_SOUNDCARD_COL_ANA_IN_2_LINK,
	ZDJ_SOUNDCARD_COL_ANA_IN_2_SIG,
	ZDJ_SOUNDCARD_COL_ANA_IN_2_TRIM,
	ZDJ_SOUNDCARD_COL_ANA_IN_2_PAN,
	ZDJ_SOUNDCARD_COL_ANA_IN_2_STEREO,
	ZDJ_SOUNDCARD_COL_ANA_IN_2_MUTE,
	ZDJ_SOUNDCARD_COL_ANA_IN_3_LINK,
	ZDJ_SOUNDCARD_COL_ANA_IN_3_SIG,
	ZDJ_SOUNDCARD_COL_ANA_IN_3_TRIM,
	ZDJ_SOUNDCARD_COL_ANA_IN_3_PAN,
	ZDJ_SOUNDCARD_COL_ANA_IN_3_STEREO,
	ZDJ_SOUNDCARD_COL_ANA_IN_3_MUTE,

	ZDJ_SOUNDCARD_COL_MAIN_BUS_LINK,
	ZDJ_SOUNDCARD_COL_MAIN_BUS_STEREO,
	ZDJ_SOUNDCARD_COL_MAIN_BUS_MUTE,

	ZDJ_SOUNDCARD_COL_CUE_BUS_LINK,
	ZDJ_SOUNDCARD_COL_CUE_BUS_STEREO,
	ZDJ_SOUNDCARD_COL_CUE_BUS_MUTE,

	ZDJ_SOUNDCARD_COL_ANNOT_BUS_LINK,
	ZDJ_SOUNDCARD_COL_ANNOT_BUS_TRIM,
	ZDJ_SOUNDCARD_COL_ANNOT_BUS_PAN,
	ZDJ_SOUNDCARD_COL_ANNOT_BUS_STEREO,
	ZDJ_SOUNDCARD_COL_ANNOT_BUS_MUTE,

	ZDJ_SOUNDCARD_COL_RECORD_BUS_TRIM,
	ZDJ_SOUNDCARD_COL_RECORD_BUS_PAN,
	ZDJ_SOUNDCARD_COL_RECORD_BUS_STEREO,

	ZDJ_SOUNDCARD_COL_DECK_1_BUS_LINK,
	ZDJ_SOUNDCARD_COL_DECK_1_PREFADE_LINK,

	ZDJ_SOUNDCARD_COL_DECK_2_BUS_LINK,
	ZDJ_SOUNDCARD_COL_DECK_2_PREFADE_LINK,

	ZDJ_SOUNDCARD_COL_DECK_EXT_BUS_LINK,
	ZDJ_SOUNDCARD_COL_DECK_EXT_PREFADE_LINK,

	ZDJ_SOUNDCARD_COL_AUX_BUS_0_LINK,
	ZDJ_SOUNDCARD_COL_AUX_BUS_0_TRIM,
	ZDJ_SOUNDCARD_COL_AUX_BUS_0_PAN,
	ZDJ_SOUNDCARD_COL_AUX_BUS_0_STEREO,
	ZDJ_SOUNDCARD_COL_AUX_BUS_0_MUTE,
	ZDJ_SOUNDCARD_COL_AUX_BUS_1_LINK,
	ZDJ_SOUNDCARD_COL_AUX_BUS_1_TRIM,
	ZDJ_SOUNDCARD_COL_AUX_BUS_1_PAN,
	ZDJ_SOUNDCARD_COL_AUX_BUS_1_STEREO,
	ZDJ_SOUNDCARD_COL_AUX_BUS_1_MUTE,
	ZDJ_SOUNDCARD_COL_AUX_BUS_2_LINK,
	ZDJ_SOUNDCARD_COL_AUX_BUS_2_TRIM,
	ZDJ_SOUNDCARD_COL_AUX_BUS_2_PAN,
	ZDJ_SOUNDCARD_COL_AUX_BUS_2_STEREO,
	ZDJ_SOUNDCARD_COL_AUX_BUS_2_MUTE,
	ZDJ_SOUNDCARD_COL_AUX_BUS_3_LINK,
	ZDJ_SOUNDCARD_COL_AUX_BUS_3_TRIM,
	ZDJ_SOUNDCARD_COL_AUX_BUS_3_PAN,
	ZDJ_SOUNDCARD_COL_AUX_BUS_3_STEREO,
	ZDJ_SOUNDCARD_COL_AUX_BUS_3_MUTE,

	ZDJ_SOUNDCARD_COL_CLOCK_0_SIG,
	ZDJ_SOUNDCARD_COL_CLOCK_0_LINK,
	ZDJ_SOUNDCARD_COL_CLOCK_0_SOURCE,
	ZDJ_SOUNDCARD_COL_CLOCK_0_VAL,
	ZDJ_SOUNDCARD_COL_CLOCK_1_SIG,
	ZDJ_SOUNDCARD_COL_CLOCK_1_LINK,
	ZDJ_SOUNDCARD_COL_CLOCK_1_SOURCE,
	ZDJ_SOUNDCARD_COL_CLOCK_1_VAL,
	ZDJ_SOUNDCARD_COL_CLOCK_2_SIG,
	ZDJ_SOUNDCARD_COL_CLOCK_2_LINK,
	ZDJ_SOUNDCARD_COL_CLOCK_2_SOURCE,
	ZDJ_SOUNDCARD_COL_CLOCK_2_VAL,
	ZDJ_SOUNDCARD_COL_CLOCK_3_SIG,
	ZDJ_SOUNDCARD_COL_CLOCK_3_LINK,
	ZDJ_SOUNDCARD_COL_CLOCK_3_SOURCE,
	ZDJ_SOUNDCARD_COL_CLOCK_3_VAL,

	ZDJ_SOUNDCARD_COL_CV_0_SIG,
	ZDJ_SOUNDCARD_COL_CV_0_LINK,
	ZDJ_SOUNDCARD_COL_CV_0_SOURCE,
	ZDJ_SOUNDCARD_COL_CV_0_INVERT,
	ZDJ_SOUNDCARD_COL_CV_0_GAIN,
	ZDJ_SOUNDCARD_COL_CV_0_MUTE,
	ZDJ_SOUNDCARD_COL_CV_1_SIG,
	ZDJ_SOUNDCARD_COL_CV_1_LINK,
	ZDJ_SOUNDCARD_COL_CV_1_SOURCE,
	ZDJ_SOUNDCARD_COL_CV_1_INVERT,
	ZDJ_SOUNDCARD_COL_CV_1_GAIN,
	ZDJ_SOUNDCARD_COL_CV_1_MUTE,
	ZDJ_SOUNDCARD_COL_CV_2_SIG,
	ZDJ_SOUNDCARD_COL_CV_2_LINK,
	ZDJ_SOUNDCARD_COL_CV_2_SOURCE,
	ZDJ_SOUNDCARD_COL_CV_2_INVERT,
	ZDJ_SOUNDCARD_COL_CV_2_GAIN,
	ZDJ_SOUNDCARD_COL_CV_2_MUTE,
	ZDJ_SOUNDCARD_COL_CV_3_SIG,
	ZDJ_SOUNDCARD_COL_CV_3_LINK,
	ZDJ_SOUNDCARD_COL_CV_3_SOURCE,
	ZDJ_SOUNDCARD_COL_CV_3_INVERT,
	ZDJ_SOUNDCARD_COL_CV_3_GAIN,
	ZDJ_SOUNDCARD_COL_CV_3_MUTE
} zdj_soundcard_db_col_t;

typedef struct {
    sqlite3_stmt * store_stmt;
	sqlite3_stmt * fetch_stmt;

    char entity_id[ 64 ];
	char name[ 256 ];

    // Analog Out Ports //
    // Analog Out Port links are derived from upstream
    // Analog Out 0
	int ana_out_0_sig;
    int ana_out_0_stereo;
    // Analog Out 1
	int ana_out_1_sig;
    int ana_out_1_stereo;
    // Analog Out 2
	int ana_out_2_sig;
    int ana_out_2_stereo;
    // Analog Out 3
	int ana_out_3_sig;
    int ana_out_3_stereo;

    // Analog In Ports //
    // Analog In 0
	uint64_t ana_in_0_link_map;
	int ana_in_0_sig;
	int ana_in_0_trim;
	int ana_in_0_pan;
    int ana_in_0_stereo;
	int ana_in_0_mute;
    // Analog In 1
	uint64_t ana_in_1_link_map;
	int ana_in_1_sig;
	int ana_in_1_trim;
	int ana_in_1_pan;
    int ana_in_1_stereo;
	int ana_in_1_mute;
    // Analog In 2
	uint64_t ana_in_2_link_map;
	int ana_in_2_sig;
	int ana_in_2_trim;
	int ana_in_2_pan;
    int ana_in_2_stereo;
	int ana_in_2_mute;
    // Analog In 3
	uint64_t ana_in_3_link_map;
	int ana_in_3_sig;
	int ana_in_3_trim;
	int ana_in_3_pan;
    int ana_in_3_stereo;
	int ana_in_3_mute;

    // Main LR Bus //
	uint64_t main_bus_link_map;
    int main_bus_pan;
	int main_bus_stereo;
	int main_bus_mute;

    // Cue Bus //
	uint64_t cue_bus_link_map;
    int cue_bus_pan;
	int cue_bus_stereo;
	int cue_bus_mute;

    // Annotation Bus //
    uint64_t annot_bus_link_map;
    int annot_bus_trim;
    int annot_bus_pan;
    int annot_bus_stereo;
	int annot_bus_mute;

    // Recording Bus //
    // Recording Bus links are derived from upstream
    int record_bus_trim;
    int record_bus_pan;
    int record_bus_stereo;

    // Deck 1 Busses //
    uint64_t deck_1_bus_link_map;
	uint64_t deck_1_prefade_link_map;

    // Deck 2 Busses //
	uint64_t deck_2_bus_link_map;
	uint64_t deck_2_prefade_link_map;

    // External Deck Busses //
	uint64_t deck_ext_bus_link_map;
	uint64_t deck_ext_prefade_link_map;

    // Aux Busses //
    // Aux 0
	uint64_t aux_bus_0_link_map;
	int aux_bus_0_trim;
	int aux_bus_0_pan;
    int aux_bus_0_stereo;
	int aux_bus_0_mute;
    // Aux 1
	uint64_t aux_bus_1_link_map;
	int aux_bus_1_trim;
	int aux_bus_1_pan;
    int aux_bus_1_stereo;
	int aux_bus_1_mute;
    // Aux 2
	uint64_t aux_bus_2_link_map;
	int aux_bus_2_trim;
	int aux_bus_2_pan;
    int aux_bus_2_stereo;
	int aux_bus_2_mute;
    // Aux 3
	uint64_t aux_bus_3_link_map;
	int aux_bus_3_trim;
	int aux_bus_3_pan;
    int aux_bus_3_stereo;
	int aux_bus_3_mute;

    // Clock Busses //
    // Clock 0
	int clock_0_sig;
	uint64_t clock_0_link_map;
    int clock_0_source;
    int clock_0_val;
    // Clock 1
	int clock_1_sig;
	uint64_t clock_1_link_map;
    int clock_1_source;
    int clock_1_val;
    // Clock 2
	int clock_2_sig;
	uint64_t clock_2_link_map;
    int clock_2_source;
    int clock_2_val;
    // Clock 3
	int clock_3_sig;
	uint64_t clock_3_link_map;
    int clock_3_source;
    int clock_3_val;

    // CV Busses //
    // CV 0
	int cv_0_sig;
	uint64_t cv_0_link_map;
    int cv_0_source;
    int cv_0_invert;
    int cv_0_gain;
	int cv_0_mute;
    // CV 1
	int cv_1_sig;
	uint64_t cv_1_link_map;
    int cv_1_source;
    int cv_1_invert;
    int cv_1_gain;
	int cv_1_mute;
    // CV 2
	int cv_2_sig;
	uint64_t cv_2_link_map;
    int cv_2_source;
    int cv_2_invert;
    int cv_2_gain;
	int cv_2_mute;
    // CV 3
	int cv_3_sig;
	uint64_t cv_3_link_map;
    int cv_3_source;
    int cv_3_invert;
    int cv_3_gain;
	int cv_3_mute;
} zdj_soundcard_dto_t;

zdj_error_type_t zdj_soundcard_fetch_dto( char * entity_id, zdj_soundcard_dto_t * dto );
zdj_error_type_t zdj_soundcard_store_dto( char * entity_id, zdj_soundcard_dto_t * dto );
uint64_t zdj_soundcard_dto_get_linkmap_for_node_name( zdj_soundcard_dto_t * dto, int name );
int zdj_soundcard_dto_set_linkmap_for_node_name( 
	zdj_soundcard_dto_t * dto, int name, uint64_t link_map 
);
int zdj_soundcard_dto_get_sigtype_for_node_name( zdj_soundcard_dto_t * dto, int name );
int zdj_soundcard_dto_set_sigtype_for_node_name( zdj_soundcard_dto_t * dto, int name, int val );
int zdj_soundcard_dto_get_gain_for_node_name( zdj_soundcard_dto_t * dto, int name );
int zdj_soundcard_dto_set_gain_for_node_name( zdj_soundcard_dto_t * dto, int name, int val );
int zdj_soundcard_dto_get_pan_for_node_name( zdj_soundcard_dto_t * dto, int name );
int zdj_soundcard_dto_set_pan_for_node_name( zdj_soundcard_dto_t * dto, int name, int val );
int zdj_soundcard_dto_get_stereo_for_node_name( zdj_soundcard_dto_t * dto, int name );
int zdj_soundcard_dto_set_stereo_for_node_name( zdj_soundcard_dto_t * dto, int name, int val );
int zdj_soundcard_dto_get_mute_for_node_name( zdj_soundcard_dto_t * dto, int name );
int zdj_soundcard_dto_set_mute_for_node_name( zdj_soundcard_dto_t * dto, int name, int val );
int zdj_soundcard_dto_get_source_for_node_name( zdj_soundcard_dto_t * dto, int name );
int zdj_soundcard_dto_set_source_for_node_name( zdj_soundcard_dto_t * dto, int name, int val );
int zdj_soundcard_dto_get_val_for_node_name( zdj_soundcard_dto_t * dto, int name );
int zdj_soundcard_dto_set_val_for_node_name( zdj_soundcard_dto_t * dto, int name, int val );
int zdj_soundcard_dto_get_invert_for_node_name( zdj_soundcard_dto_t * dto, int name );
int zdj_soundcard_dto_set_invert_for_node_name( zdj_soundcard_dto_t * dto, int name, int val );

#endif
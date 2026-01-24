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

#include <zerodj/signal/soundcard/dsp/zdj_soundcard_dsp.h>

#define ZDJ_SOUNDCARD_ENTITY_ID_LEN 37

typedef enum {
	ZDJ_SOUNDCARD_DSP_COL_ENTITY_ID,
	ZDJ_SOUNDCARD_DSP_COL_GAIN,
	ZDJ_SOUNDCARD_DSP_COL_PAN,

	ZDJ_SOUNDCARD_DSP_COL_STAGE_0_TYPE,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_0_ID,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_0_KNOB_0,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_0_KNOB_1,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_0_KNOB_2,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_0_KNOB_3,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_0_KNOB_4,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_0_KNOB_5,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_0_KNOB_6,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_0_KNOB_7,

	ZDJ_SOUNDCARD_DSP_COL_STAGE_1_TYPE,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_1_ID,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_1_KNOB_0,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_1_KNOB_1,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_1_KNOB_2,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_1_KNOB_3,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_1_KNOB_4,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_1_KNOB_5,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_1_KNOB_6,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_1_KNOB_7,

	ZDJ_SOUNDCARD_DSP_COL_STAGE_2_TYPE,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_2_ID,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_2_KNOB_0,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_2_KNOB_1,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_2_KNOB_2,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_2_KNOB_3,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_2_KNOB_4,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_2_KNOB_5,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_2_KNOB_6,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_2_KNOB_7,

	ZDJ_SOUNDCARD_DSP_COL_STAGE_3_TYPE,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_3_ID,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_3_KNOB_0,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_3_KNOB_1,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_3_KNOB_2,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_3_KNOB_3,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_3_KNOB_4,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_3_KNOB_5,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_3_KNOB_6,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_3_KNOB_7,

	ZDJ_SOUNDCARD_DSP_COL_STAGE_4_TYPE,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_4_ID,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_4_KNOB_0,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_4_KNOB_1,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_4_KNOB_2,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_4_KNOB_3,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_4_KNOB_4,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_4_KNOB_5,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_4_KNOB_6,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_4_KNOB_7,

	ZDJ_SOUNDCARD_DSP_COL_STAGE_5_TYPE,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_5_ID,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_5_KNOB_0,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_5_KNOB_1,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_5_KNOB_2,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_5_KNOB_3,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_5_KNOB_4,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_5_KNOB_5,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_5_KNOB_6,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_5_KNOB_7,

	ZDJ_SOUNDCARD_DSP_COL_STAGE_6_TYPE,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_6_ID,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_6_KNOB_0,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_6_KNOB_1,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_6_KNOB_2,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_6_KNOB_3,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_6_KNOB_4,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_6_KNOB_5,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_6_KNOB_6,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_6_KNOB_7,

	ZDJ_SOUNDCARD_DSP_COL_STAGE_7_TYPE,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_7_ID,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_7_KNOB_0,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_7_KNOB_1,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_7_KNOB_2,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_7_KNOB_3,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_7_KNOB_4,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_7_KNOB_5,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_7_KNOB_6,
	ZDJ_SOUNDCARD_DSP_COL_STAGE_7_KNOB_7
} zdj_soundcard_dsp_dto_db_col_t;

typedef enum {
	ZDJ_SOUNDCARD_DSP_STAGE_TYPE_NONE,
	ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ,
	ZDJ_SOUNDCARD_DSP_STAGE_TYPE_DYN,
	ZDJ_SOUNDCARD_DSP_STAGE_TYPE_FILT,
	ZDJ_SOUNDCARD_DSP_STAGE_TYPE_REVERB,
	ZDJ_SOUNDCARD_DSP_STAGE_TYPE_DELAY,
	ZDJ_SOUNDCARD_DSP_STAGE_TYPE_MOD,
	ZDJ_SOUNDCARD_DSP_STAGE_TYPE_XFADE,
	ZDJ_SOUNDCARD_DSP_STAGE_TYPE_COUNT
} zdj_soundcard_dsp_stage_type_t;

typedef enum {
	ZDJ_SOUNDCARD_DSP_ID_NONE,
	ZDJ_SOUNDCARD_DSP_ID_EQ_3_1P,
	ZDJ_SOUNDCARD_DSP_ID_EQ_3_2P,
	ZDJ_SOUNDCARD_DSP_ID_EQ_3_4P,
	ZDJ_SOUNDCARD_DSP_ID_DYN_COMP,
	ZDJ_SOUNDCARD_DSP_ID_DYN_TAPE_SAT,
	ZDJ_SOUNDCARD_DSP_ID_DYN_XIST,
	ZDJ_SOUNDCARD_DSP_ID_DYN_XFORM,
	ZDJ_SOUNDCARD_DSP_ID_DYN_VOX,
	ZDJ_SOUNDCARD_DSP_ID_REVERB,
	ZDJ_SOUNDCARD_DSP_ID_DELAY,
	ZDJ_SOUNDCARD_DSP_ID_MOD,
	ZDJ_SOUNDCARD_DSP_ID_MOD_WIDEN,
	ZDJ_SOUNDCARD_DSP_ID_XFADE_CURVE,
	ZDJ_SOUNDCARD_DSP_ID_COUNT
} zdj_soundcard_dsp_stage_id_t;

typedef void ( *zdj_dsp_func_t )( void *, float *, int );

typedef enum{  
	ZDJ_SOUNDCARD_DSP_KNOB_12DB_GAIN_FAST,
	ZDJ_SOUNDCARD_DSP_KNOB_12DB_GAIN_SLOW,
	ZDJ_SOUNDCARD_DSP_KNOB_PAN,
	ZDJ_SOUNDCARD_DSP_KNOB_INF_FAST,
	ZDJ_SOUNDCARD_DSP_KNOB_INF_SLOW,
	ZDJ_SOUNDCARD_DSP_KNOB_0_1_FAST,
	ZDJ_SOUNDCARD_DSP_KNOB_0_1_SLOW,
	ZDJ_SOUNDCARD_DSP_KNOB_NEG_POS_1_FAST,
	ZDJ_SOUNDCARD_DSP_KNOB_NEG_POS_1_SLOW
} zdj_soundcard_dsp_knob_model_t;

typedef struct {
    int type;
    int id;
    float knob_0;
	zdj_soundcard_dsp_knob_model_t knob_0_model;
    float knob_1;
	zdj_soundcard_dsp_knob_model_t knob_1_model;
    float knob_2;
	zdj_soundcard_dsp_knob_model_t knob_2_model;
    float knob_3;
	zdj_soundcard_dsp_knob_model_t knob_3_model;
    float knob_4;
	zdj_soundcard_dsp_knob_model_t knob_4_model;
    float knob_5;
	zdj_soundcard_dsp_knob_model_t knob_5_model;
	float knob_6;
	zdj_soundcard_dsp_knob_model_t knob_6_model;
	float knob_7;
	zdj_soundcard_dsp_knob_model_t knob_7_model;
    void * data;
	void ( *adjust_knob )( void *, int, int );
	double ( *get_knob_display_val )( zdj_soundcard_dsp_knob_model_t, float );
	zdj_dsp_func_t fn;
} zdj_soundcard_dsp_stage_dto_t;

typedef struct {
	char entity_id[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
    float gain;
	zdj_soundcard_dsp_knob_model_t gain_model;
	double ( *get_gain_display_val )( zdj_soundcard_dsp_knob_model_t, float );
	float pan;
	zdj_soundcard_dsp_knob_model_t pan_model;
	double ( *get_pan_display_val )( zdj_soundcard_dsp_knob_model_t, float );
	bool mute;
	void ( *set_gain )( void *, int );
	void ( *adjust_gain )( void *, int );
	void ( *adjust_pan )( void *, int );
	void ( *toggle_mute )( void * );
	bool has_stages;
    zdj_soundcard_dsp_stage_dto_t stages[ 8 ];
	void * ( *get_stage_for_type )( void *, int );
	void * ( *get_stage_for_id )( void *, int );
} zdj_soundcard_dsp_dto_t;


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
	ZDJ_SOUNDCARD_COL_ANA_IN_0_DSP,
	ZDJ_SOUNDCARD_COL_ANA_IN_0_SIG,
	ZDJ_SOUNDCARD_COL_ANA_IN_0_STEREO,
	ZDJ_SOUNDCARD_COL_ANA_IN_0_MUTE,
	ZDJ_SOUNDCARD_COL_ANA_IN_1_LINK,
	ZDJ_SOUNDCARD_COL_ANA_IN_1_DSP,
	ZDJ_SOUNDCARD_COL_ANA_IN_1_SIG,
	ZDJ_SOUNDCARD_COL_ANA_IN_1_STEREO,
	ZDJ_SOUNDCARD_COL_ANA_IN_1_MUTE,
	ZDJ_SOUNDCARD_COL_ANA_IN_2_LINK,
	ZDJ_SOUNDCARD_COL_ANA_IN_2_DSP,
	ZDJ_SOUNDCARD_COL_ANA_IN_2_SIG,
	ZDJ_SOUNDCARD_COL_ANA_IN_2_STEREO,
	ZDJ_SOUNDCARD_COL_ANA_IN_2_MUTE,
	ZDJ_SOUNDCARD_COL_ANA_IN_3_LINK,
	ZDJ_SOUNDCARD_COL_ANA_IN_3_DSP,
	ZDJ_SOUNDCARD_COL_ANA_IN_3_SIG,
	ZDJ_SOUNDCARD_COL_ANA_IN_3_STEREO,
	ZDJ_SOUNDCARD_COL_ANA_IN_3_MUTE,

	ZDJ_SOUNDCARD_COL_MAIN_BUS_LINK,
	ZDJ_SOUNDCARD_COL_MAIN_BUS_DSP,
	ZDJ_SOUNDCARD_COL_MAIN_BUS_STEREO,
	ZDJ_SOUNDCARD_COL_MAIN_BUS_MUTE,

	ZDJ_SOUNDCARD_COL_CUE_BUS_LINK,
	ZDJ_SOUNDCARD_COL_CUE_BUS_DSP,
	ZDJ_SOUNDCARD_COL_CUE_BUS_STEREO,
	ZDJ_SOUNDCARD_COL_CUE_BUS_MUTE,

	ZDJ_SOUNDCARD_COL_ANNOT_BUS_LINK,
	ZDJ_SOUNDCARD_COL_ANNOT_BUS_DSP,
	ZDJ_SOUNDCARD_COL_ANNOT_BUS_STEREO,
	ZDJ_SOUNDCARD_COL_ANNOT_BUS_MUTE,

	ZDJ_SOUNDCARD_COL_RECORD_BUS_DSP,
	ZDJ_SOUNDCARD_COL_RECORD_BUS_STEREO,

	ZDJ_SOUNDCARD_COL_DECK_1_INPUT_LINK,
	ZDJ_SOUNDCARD_COL_DECK_1_INPUT_DSP,
	ZDJ_SOUNDCARD_COL_DECK_1_EDGE_LINK,
	ZDJ_SOUNDCARD_COL_DECK_1_PREFADE_LINK,
	ZDJ_SOUNDCARD_COL_DECK_1_PREFADE_DSP,
	ZDJ_SOUNDCARD_COL_DECK_1_POSTFADE_LINK,
	ZDJ_SOUNDCARD_COL_DECK_1_POSTFADE_DSP,
	ZDJ_SOUNDCARD_COL_DECK_1_CUE_LINK,
	ZDJ_SOUNDCARD_COL_DECK_1_CUE_DSP,

	ZDJ_SOUNDCARD_COL_DECK_2_INPUT_LINK,
	ZDJ_SOUNDCARD_COL_DECK_2_INPUT_DSP,
	ZDJ_SOUNDCARD_COL_DECK_2_EDGE_LINK,
	ZDJ_SOUNDCARD_COL_DECK_2_PREFADE_LINK,
	ZDJ_SOUNDCARD_COL_DECK_2_PREFADE_DSP,
	ZDJ_SOUNDCARD_COL_DECK_2_POSTFADE_LINK,
	ZDJ_SOUNDCARD_COL_DECK_2_POSTFADE_DSP,
	ZDJ_SOUNDCARD_COL_DECK_2_CUE_LINK,
	ZDJ_SOUNDCARD_COL_DECK_2_CUE_DSP,

	ZDJ_SOUNDCARD_COL_DECK_EXT_INPUT_LINK,
	ZDJ_SOUNDCARD_COL_DECK_EXT_INPUT_DSP,
	ZDJ_SOUNDCARD_COL_DECK_EXT_EDGE_LINK,
	ZDJ_SOUNDCARD_COL_DECK_EXT_PREFADE_LINK,
	ZDJ_SOUNDCARD_COL_DECK_EXT_PREFADE_DSP,
	ZDJ_SOUNDCARD_COL_DECK_EXT_POSTFADE_LINK,
	ZDJ_SOUNDCARD_COL_DECK_EXT_POSTFADE_DSP,
	ZDJ_SOUNDCARD_COL_DECK_EXT_CUE_LINK,
	ZDJ_SOUNDCARD_COL_DECK_EXT_CUE_DSP,	

	ZDJ_SOUNDCARD_COL_AUX_BUS_0_LINK,
	ZDJ_SOUNDCARD_COL_AUX_BUS_0_DSP,
	ZDJ_SOUNDCARD_COL_AUX_BUS_0_STEREO,
	ZDJ_SOUNDCARD_COL_AUX_BUS_0_MUTE,
	ZDJ_SOUNDCARD_COL_AUX_BUS_1_LINK,
	ZDJ_SOUNDCARD_COL_AUX_BUS_1_DSP,
	ZDJ_SOUNDCARD_COL_AUX_BUS_1_STEREO,
	ZDJ_SOUNDCARD_COL_AUX_BUS_1_MUTE,
	ZDJ_SOUNDCARD_COL_AUX_BUS_2_LINK,
	ZDJ_SOUNDCARD_COL_AUX_BUS_2_DSP,
	ZDJ_SOUNDCARD_COL_AUX_BUS_2_STEREO,
	ZDJ_SOUNDCARD_COL_AUX_BUS_2_MUTE,
	ZDJ_SOUNDCARD_COL_AUX_BUS_3_LINK,
	ZDJ_SOUNDCARD_COL_AUX_BUS_3_DSP,
	ZDJ_SOUNDCARD_COL_AUX_BUS_3_STEREO,
	ZDJ_SOUNDCARD_COL_AUX_BUS_3_MUTE,

	ZDJ_SOUNDCARD_COL_CLOCK_0_SIG, // ZDJ_SOUNDCARD_COL_CLOCK_0_SIG, // PPQN
	ZDJ_SOUNDCARD_COL_CLOCK_0_LINK, // ZDJ_SOUNDCARD_COL_CLOCK_0_LINK, // Output Linkage
	ZDJ_SOUNDCARD_COL_CLOCK_0_DIRECTION, // ZDJ_SOUNDCARD_COL_CLOCK_0_DIRECTION, // Input/Output
	ZDJ_SOUNDCARD_COL_CLOCK_0_VAL, // ZDJ_SOUNDCARD_COL_CLOCK_0_VAL, // BPM
	ZDJ_SOUNDCARD_COL_CLOCK_0_SYNC, // Normal/half/double/decouple
	ZDJ_SOUNDCARD_COL_CLOCK_1_SIG,
	ZDJ_SOUNDCARD_COL_CLOCK_1_LINK,
	ZDJ_SOUNDCARD_COL_CLOCK_1_DIRECTION,
	ZDJ_SOUNDCARD_COL_CLOCK_1_VAL,
	ZDJ_SOUNDCARD_COL_CLOCK_1_SYNC,
	ZDJ_SOUNDCARD_COL_CLOCK_2_SIG,
	ZDJ_SOUNDCARD_COL_CLOCK_2_LINK,
	ZDJ_SOUNDCARD_COL_CLOCK_2_DIRECTION,
	ZDJ_SOUNDCARD_COL_CLOCK_2_VAL,
	ZDJ_SOUNDCARD_COL_CLOCK_2_SYNC,
	ZDJ_SOUNDCARD_COL_CLOCK_3_SIG,
	ZDJ_SOUNDCARD_COL_CLOCK_3_LINK,
	ZDJ_SOUNDCARD_COL_CLOCK_3_DIRECTION,
	ZDJ_SOUNDCARD_COL_CLOCK_3_VAL,
	ZDJ_SOUNDCARD_COL_CLOCK_3_SYNC,

	ZDJ_SOUNDCARD_COL_CV_0_SIG,
	ZDJ_SOUNDCARD_COL_CV_0_LINK,
	ZDJ_SOUNDCARD_COL_CV_0_DSP,
	ZDJ_SOUNDCARD_COL_CV_0_DIRECTION,
	ZDJ_SOUNDCARD_COL_CV_0_MUTE,
	ZDJ_SOUNDCARD_COL_CV_1_SIG,
	ZDJ_SOUNDCARD_COL_CV_1_LINK,
	ZDJ_SOUNDCARD_COL_CV_1_DSP,
	ZDJ_SOUNDCARD_COL_CV_1_DIRECTION,
	ZDJ_SOUNDCARD_COL_CV_1_MUTE,
	ZDJ_SOUNDCARD_COL_CV_2_SIG,
	ZDJ_SOUNDCARD_COL_CV_2_LINK,
	ZDJ_SOUNDCARD_COL_CV_2_DSP,
	ZDJ_SOUNDCARD_COL_CV_2_DIRECTION,
	ZDJ_SOUNDCARD_COL_CV_2_MUTE,
	ZDJ_SOUNDCARD_COL_CV_3_SIG,
	ZDJ_SOUNDCARD_COL_CV_3_LINK,
	ZDJ_SOUNDCARD_COL_CV_3_DSP,
	ZDJ_SOUNDCARD_COL_CV_3_DIRECTION,
	ZDJ_SOUNDCARD_COL_CV_3_MUTE,

	ZDJ_SOUNDCARD_COL_XFADE_A_LINK,
	ZDJ_SOUNDCARD_COL_XFADE_A_DSP,
	ZDJ_SOUNDCARD_COL_XFADE_B_LINK,
	ZDJ_SOUNDCARD_COL_XFADE_B_DSP
} zdj_soundcard_dto_db_col_t;

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
	char ana_in_0_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t ana_in_0_dsp;
	int ana_in_0_sig;
    int ana_in_0_stereo;
	int ana_in_0_mute;
    // Analog In 1
	uint64_t ana_in_1_link_map;
	char ana_in_1_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t ana_in_1_dsp;
	int ana_in_1_sig;
    int ana_in_1_stereo;
	int ana_in_1_mute;
    // Analog In 2
	uint64_t ana_in_2_link_map;
	char ana_in_2_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t ana_in_2_dsp;
	int ana_in_2_sig;
    int ana_in_2_stereo;
	int ana_in_2_mute;
    // Analog In 3
	uint64_t ana_in_3_link_map;
	char ana_in_3_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t ana_in_3_dsp;
	int ana_in_3_sig;
    int ana_in_3_stereo;
	int ana_in_3_mute;

    // Main LR Bus //
	uint64_t main_bus_link_map;
	char main_bus_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t main_bus_dsp;
	int main_bus_stereo;
	int main_bus_mute;

    // Cue Bus //
	uint64_t cue_bus_link_map;
	char cue_bus_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t cue_bus_dsp;
	int cue_bus_stereo;
	int cue_bus_mute;

    // Annotation Bus //
    uint64_t annot_bus_link_map;
	char annot_bus_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t annot_bus_dsp;
    int annot_bus_stereo;
	int annot_bus_mute;

    // Recording Bus //
    // Recording Bus links are derived from upstream
    char record_bus_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t record_bus_dsp;
    int record_bus_stereo;

    // Deck 1 Busses //
    uint64_t deck_1_input_link_map;
	char deck_1_input_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t deck_1_input_dsp;
	uint64_t deck_1_edge_link_map;
	uint64_t deck_1_prefade_link_map;
	char deck_1_prefade_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t deck_1_prefade_dsp;
	uint64_t deck_1_postfade_link_map;
	char deck_1_postfade_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t deck_1_postfade_dsp;
	uint64_t deck_1_cue_link_map;
	char deck_1_cue_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t deck_1_cue_dsp;

    // Deck 2 Busses //
	uint64_t deck_2_input_link_map;
	char deck_2_input_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t deck_2_input_dsp;
	uint64_t deck_2_edge_link_map;
	uint64_t deck_2_prefade_link_map;
	char deck_2_prefade_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t deck_2_prefade_dsp;
	uint64_t deck_2_postfade_link_map;
	char deck_2_postfade_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t deck_2_postfade_dsp;
	uint64_t deck_2_cue_link_map;
	char deck_2_cue_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t deck_2_cue_dsp;

    // External Deck Busses //
	uint64_t deck_ext_input_link_map;
	char deck_ext_input_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t deck_ext_input_dsp;
	uint64_t deck_ext_edge_link_map;
	uint64_t deck_ext_prefade_link_map;
	char deck_ext_prefade_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t deck_ext_prefade_dsp;
	uint64_t deck_ext_postfade_link_map;
	char deck_ext_postfade_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t deck_ext_postfade_dsp;
	uint64_t deck_ext_cue_link_map;
	char deck_ext_cue_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t deck_ext_cue_dsp;

    // Aux Busses //
    // Aux 0
	uint64_t aux_bus_0_link_map;
	char aux_bus_0_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t aux_bus_0_dsp;
    int aux_bus_0_stereo;
	int aux_bus_0_mute;
    // Aux 1
	uint64_t aux_bus_1_link_map;
	char aux_bus_1_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t aux_bus_1_dsp;

    int aux_bus_1_stereo;
	int aux_bus_1_mute;
    // Aux 2
	uint64_t aux_bus_2_link_map;
	char aux_bus_2_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t aux_bus_2_dsp;
    int aux_bus_2_stereo;
	int aux_bus_2_mute;
    // Aux 3
	uint64_t aux_bus_3_link_map;
	char aux_bus_3_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t aux_bus_3_dsp;
    int aux_bus_3_stereo;
	int aux_bus_3_mute;

    // Clock Busses //
    // Clock 0
	int clock_0_sig;
	uint64_t clock_0_link_map;
    int clock_0_source;
    double clock_0_val;
	int clock_0_sync;
    // Clock 1
	int clock_1_sig;
	uint64_t clock_1_link_map;
    int clock_1_source;
    double clock_1_val;
	int clock_1_sync;
    // Clock 2
	int clock_2_sig;
	uint64_t clock_2_link_map;
    int clock_2_source;
    double clock_2_val;
	int clock_2_sync;
    // Clock 3
	int clock_3_sig;
	uint64_t clock_3_link_map;
    int clock_3_source;
    double clock_3_val;
	int clock_3_sync;

    // CV Busses //
    // CV 0
	int cv_0_sig;
	uint64_t cv_0_link_map;
	char cv_0_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t cv_0_dsp;
    int cv_0_source;
	int cv_0_mute;
    // CV 1
	int cv_1_sig;
	uint64_t cv_1_link_map;
	char cv_1_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t cv_1_dsp;
    int cv_1_source;
	int cv_1_mute;
    // CV 2
	int cv_2_sig;
	uint64_t cv_2_link_map;
	char cv_2_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t cv_2_dsp;
    int cv_2_source;
	int cv_2_mute;
    // CV 3
	int cv_3_sig;
	uint64_t cv_3_link_map;
	char cv_3_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t cv_3_dsp;
    int cv_3_source;
	int cv_3_mute;

	// Crossfader
	uint64_t xfade_a_link_map;
	char xfade_a_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t xfade_a_dsp;
	uint64_t xfade_b_link_map;
	char xfade_b_dsp_eid[ ZDJ_SOUNDCARD_ENTITY_ID_LEN ];
	zdj_soundcard_dsp_dto_t xfade_b_dsp;
} zdj_soundcard_dto_t;

uint64_t zdj_soundcard_dto_get_linkmap_for_node_name( zdj_soundcard_dto_t * dto, int name );
int zdj_soundcard_dto_set_linkmap_for_node_name( 
	zdj_soundcard_dto_t * dto, int name, uint64_t link_map 
);
int zdj_soundcard_dto_get_sigtype_for_node_name( zdj_soundcard_dto_t * dto, int name );
int zdj_soundcard_dto_set_sigtype_for_node_name( zdj_soundcard_dto_t * dto, int name, int val );
int zdj_soundcard_dto_get_stereo_for_node_name( zdj_soundcard_dto_t * dto, int name );
int zdj_soundcard_dto_set_stereo_for_node_name( zdj_soundcard_dto_t * dto, int name, int val );
int zdj_soundcard_dto_get_mute_for_node_name( zdj_soundcard_dto_t * dto, int name );
int zdj_soundcard_dto_set_mute_for_node_name( zdj_soundcard_dto_t * dto, int name, int val );
int zdj_soundcard_dto_get_source_for_node_name( zdj_soundcard_dto_t * dto, int name );
int zdj_soundcard_dto_set_source_for_node_name( zdj_soundcard_dto_t * dto, int name, int val );
double zdj_soundcard_dto_get_val_for_node_name( zdj_soundcard_dto_t * dto, int name );
void zdj_soundcard_dto_set_val_for_node_name( zdj_soundcard_dto_t * dto, int name, double val );
int zdj_soundcard_dto_get_sync_for_node_name( zdj_soundcard_dto_t * dto, int name );
int zdj_soundcard_dto_set_sync_for_node_name( zdj_soundcard_dto_t * dto, int name, int val );
void zdj_soundcard_dto_update_dsp_for_node_name( zdj_soundcard_dto_t * dto, int name );
zdj_soundcard_dsp_dto_t * zdj_soundcard_dto_get_dsp_for_node_name( zdj_soundcard_dto_t * dto, int name );

void zdj_soundcard_put_dsp_state_for_dto( zdj_soundcard_dsp_dto_t * dto );
void zdj_soundcard_put_dsp_state_for_xfade_dto( zdj_soundcard_dsp_dto_t * dto );

void * zdj_soundcard_dto_get_dsp_stage_for_type( void * _node, int type );
void * zdj_soundcard_dto_get_dsp_stage_for_id( void * _node, int id );
void zdj_soundcard_dsp_process_knob_input( 
    float * knob, 
    zdj_soundcard_dsp_knob_model_t model, 
    int input_val 
);
double zdj_soundcard_dsp_get_knob_display_val( 
    zdj_soundcard_dsp_knob_model_t model, 
    float val
);
#endif
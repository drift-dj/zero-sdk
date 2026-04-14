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

#ifndef SOUNDCARD_DSP_H
#define SOUNDCARD_DSP_H

#include <stdint.h>
#include <stdbool.h>

#define ZDJ_SOUNDCARD_12DB_GAIN 4

// Gain/Pan/Fader stuff
void zdj_soundcard_dsp_gain_set_knob( void * _node, int input_val );
void zdj_soundcard_dsp_gain_adjust_knob( void * _node, int input_val );
void zdj_soundcard_dsp_pan_adjust_knob( void * _node, int input_val );
void zdj_soundcard_dsp_mute_toggle( void * _node );
void zdj_soundcard_dsp_set_xfade( void * _node, int input_val );


typedef struct {
    double  hf;
    double  lf;
    // Left Channel lo-pass poles
    double  f1p0_l;
    double  f1p1_l;
    double  f1p2_l;
    double  f1p3_l;
    // Right Channel lo-pass poles
    double  f1p0_r;
    double  f1p1_r;
    double  f1p2_r;
    double  f1p3_r;

    // Left Channel hi-pass poles
    double  f2p0_l;
    double  f2p1_l;
    double  f2p2_l;
    double  f2p3_l;
    // Right Channel hi-pass poles
    double  f2p0_r;
    double  f2p1_r;
    double  f2p2_r;
    double  f2p3_r;

    // Left Channel sample history buffer
    double  sdm1_l;
    double  sdm2_l;
    double  sdm3_l;
    // Right Channel sample history buffer
    double  sdm1_r;
    double  sdm2_r;
    double  sdm3_r;
} zdj_soundcard_dsp_eq_state_t;

typedef enum {
    ZDJ_SOUNDCARD_DSP_FILT_BI_MODE_OFF,
    ZDJ_SOUNDCARD_DSP_FILT_BI_MODE_HPF,
    ZDJ_SOUNDCARD_DSP_FILT_BI_MODE_LPF,
} zdj_soundcard_dsp_filt_bi_mode_t;

typedef struct {
    zdj_soundcard_dsp_filt_bi_mode_t mode;
    float r;
    float f;

    bool needs_recalc;

    // Lo-pass poles
    float c_lo;
    float a1_lo;
    float a2_lo;
    float a3_lo;
    float b1_lo;
    float b2_lo;

    // Lo-pass history
    float in_n1_l_lo;
    float in_n2_l_lo;
    float out_n1_l_lo;
    float out_n2_l_lo;

    float in_n1_r_lo;
    float in_n2_r_lo;
    float out_n1_r_lo;
    float out_n2_r_lo;

    // Hi-pass poles
    float c_hi;
    float a1_hi;
    float a2_hi;
    float a3_hi;
    float b1_hi;
    float b2_hi;

    // Hi-pass history
    float n1_l_hi;
    float n2_l_hi;
    float n1_r_hi;
    float n2_r_hi;
} zdj_soundcard_dsp_filt_bi_state_t;

typedef struct {

} zdj_soundcard_dsp_delay_state_t;

// EQ
void zdj_soundcard_dsp_eq_adjust_knob( void * _stage, int knob, int input_val );

void zdj_soundcard_dsp_eq_3_1p_init( void * _stage );
void zdj_soundcard_dsp_eq_3_1p_update( void * _stage, float * buf, int channel_count );

void zdj_soundcard_dsp_eq_3_2p_init( void * _stage );
void zdj_soundcard_dsp_eq_3_2p_update( void * _stage, float * buf, int channel_count );

void zdj_soundcard_dsp_eq_3_4p_init( void * _stage );
void zdj_soundcard_dsp_eq_3_4p_update( void * _stage, float * buf, int channel_count );

// Bi-modal filter
void zdj_soundcard_dsp_filt_bi_init( void * _stage );
void zdj_soundcard_dsp_filt_bi_update( void * _stage, float * buf, int channel_count );
void zdj_soundcard_dsp_filt_bi_adjust_knob( void * _stage, int knob, int input_val );
void zdj_soundcard_dsp_filt_bi_set_knob( void * _stage, int knob, int input_val );

// Delay
void zdj_soundcard_dsp_delay_init( void * _stage );
void zdj_soundcard_dsp_delay_update( void * _stage, float * buf, int channel_count );
void zdj_soundcard_dsp_delay_adjust_knob( void * _stage, int knob, int input_val );
void zdj_soundcard_dsp_delay_set_knob( void * _stage, int knob, int input_val );

// Mod

// Widen

#endif
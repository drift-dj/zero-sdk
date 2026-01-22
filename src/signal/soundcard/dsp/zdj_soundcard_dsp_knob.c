#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/signal/soundcard/dsp/zdj_soundcard_dsp.h>
#include <zerodj/signal/soundcard/db/zdj_soundcard_dto.h>

// ZDJ_SOUNDCARD_DSP_KNOB_12DB_GAIN_FAST,
// ZDJ_SOUNDCARD_DSP_KNOB_12DB_GAIN_SLOW,
// ZDJ_SOUNDCARD_DSP_KNOB_PAN,
// ZDJ_SOUNDCARD_DSP_KNOB_INF_FAST,
// ZDJ_SOUNDCARD_DSP_KNOB_INF_SLOW,
// ZDJ_SOUNDCARD_DSP_KNOB_0_1_FAST,
// ZDJ_SOUNDCARD_DSP_KNOB_0_1_SLOW,
// ZDJ_SOUNDCARD_DSP_KNOB_NEG_POS_1_FAST,
// ZDJ_SOUNDCARD_DSP_KNOB_NEG_POS_1_SLOW

static double _display_val_for_12db_gain_val( float val );

static void _process_12db_gain_fast( float * knob, zdj_soundcard_dsp_knob_model_t model, int input_val );
static void _process_0_1_fast( float * knob, zdj_soundcard_dsp_knob_model_t model, int input_val );


void zdj_soundcard_dsp_process_knob_input( 
    float * knob, 
    zdj_soundcard_dsp_knob_model_t model, 
    int input_val 
) {
    switch ( model ) {
    case ZDJ_SOUNDCARD_DSP_KNOB_12DB_GAIN_FAST:
    case ZDJ_SOUNDCARD_DSP_KNOB_12DB_GAIN_SLOW:
        _process_12db_gain_fast( knob, model, input_val );
        break;
    case ZDJ_SOUNDCARD_DSP_KNOB_PAN:
    case ZDJ_SOUNDCARD_DSP_KNOB_INF_FAST:
    case ZDJ_SOUNDCARD_DSP_KNOB_INF_SLOW:
    case ZDJ_SOUNDCARD_DSP_KNOB_0_1_FAST:
    case ZDJ_SOUNDCARD_DSP_KNOB_0_1_SLOW:
        _process_0_1_fast( knob, model, input_val );
        break;
    case ZDJ_SOUNDCARD_DSP_KNOB_NEG_POS_1_FAST:
    case ZDJ_SOUNDCARD_DSP_KNOB_NEG_POS_1_SLOW:
        _process_12db_gain_fast( knob, model, input_val );
        break;
    }
}

// Return a unit val 0-1 representing the knob's position in its overall travel
double zdj_soundcard_dsp_get_knob_display_val( 
    zdj_soundcard_dsp_knob_model_t model, 
    float val
) {
    switch ( model ) {
        case ZDJ_SOUNDCARD_DSP_KNOB_12DB_GAIN_FAST:
        case ZDJ_SOUNDCARD_DSP_KNOB_12DB_GAIN_SLOW:
            return _display_val_for_12db_gain_val( val );
            break;
        case ZDJ_SOUNDCARD_DSP_KNOB_PAN:
        case ZDJ_SOUNDCARD_DSP_KNOB_INF_FAST:
        case ZDJ_SOUNDCARD_DSP_KNOB_INF_SLOW:
        case ZDJ_SOUNDCARD_DSP_KNOB_0_1_FAST:
        case ZDJ_SOUNDCARD_DSP_KNOB_0_1_SLOW:
        case ZDJ_SOUNDCARD_DSP_KNOB_NEG_POS_1_FAST:
        case ZDJ_SOUNDCARD_DSP_KNOB_NEG_POS_1_SLOW: return 0; break;
            
    }
}

static double _display_val_for_12db_gain_val( float val ) {
    double coeff = val / (float)ZDJ_SOUNDCARD_12DB_GAIN;
    double res = 1.0 - pow( 1.0 - coeff, 3 );
    return res;
}

static void _process_12db_gain_fast( float * knob, zdj_soundcard_dsp_knob_model_t model, int input_val ) {
    float knob_val = *knob;
    knob_val += ((float)input_val * 0.069) * (1+(knob_val * 0.41));
    if( knob_val > (float)ZDJ_SOUNDCARD_12DB_GAIN ) { knob_val = (float)ZDJ_SOUNDCARD_12DB_GAIN; }
    else if( knob_val < 0 ) { knob_val = 0; }
    *knob = knob_val;
}

static void _process_0_1_fast( float * knob, zdj_soundcard_dsp_knob_model_t model, int input_val ) {
    float knob_val = *knob;
    knob_val += ((float)input_val * 0.043) * (0.04+(knob_val * 0.071));
    if( knob_val > 1.0 ) { knob_val = 1.0; }
    else if( knob_val < 0.0 ) { knob_val = 0.0; }
    *knob = knob_val;
}
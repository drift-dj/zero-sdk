#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/signal/soundcard/dsp/zdj_soundcard_dsp.h>
#include <zerodj/signal/soundcard/db/zdj_soundcard_dto.h>

#define FILT_SQRT2 1.414213562373095
#define FILT_R_LO 0.1f
#define FILT_R_TRAV FILT_SQRT2 - FILT_R_LO
#define FILT_SAMP_RATE 44100.0f
#define FILT_HALF_RATE 22050.0f

static void _recalc_coeffs( zdj_soundcard_dsp_stage_dto_t * stage );

void zdj_soundcard_dsp_filt_bi_adjust_knob( void * _stage, int knob, int input_val ) {
    zdj_soundcard_dsp_stage_dto_t * stage = (zdj_soundcard_dsp_stage_dto_t*)_stage;
    zdj_soundcard_dsp_filt_bi_state_t * data = (zdj_soundcard_dsp_filt_bi_state_t*)stage->data;
    
    float new_val;
    float cur_val;
    switch ( knob ) {
        case 0: cur_val = stage->knob_0; break;
        case 1: cur_val = stage->knob_1; break;
        case 2: cur_val = stage->knob_2; break;
        case 3: cur_val = stage->knob_3; break;
        case 4: cur_val = stage->knob_4; break;
        case 5: cur_val = stage->knob_5; break;
        case 6: cur_val = stage->knob_6; break;
        case 7: cur_val = stage->knob_7; break;
    }
    
    switch ( knob ) {
        case 0: 
            new_val = cur_val + ((float)input_val * 0.011);
            if( new_val > 0.95 ) { new_val = 0.95; }
            else if( new_val < -0.95 ) { new_val = -0.95; }
            stage->knob_0 = new_val; 
            break;
        case 1: stage->knob_1 = new_val; break;
        case 2: 
            new_val = cur_val + ((float)input_val * 0.019);
            if( new_val > 1.0 ) { new_val = 1.0; }
            else if( new_val < 0.0 ) { new_val = 0.0; }
            stage->knob_2 = new_val; 
            break;
        case 3: stage->knob_3 = new_val; break;
        case 4: stage->knob_4 = new_val; break;
        case 5: stage->knob_5 = new_val; break;
        case 6: stage->knob_6 = new_val; break;
        case 7: stage->knob_7 = new_val; break;
    }

    data->needs_recalc = true;
}

void zdj_soundcard_dsp_filt_bi_set_knob( void * _stage, int knob, int input_val ) {
    zdj_soundcard_dsp_stage_dto_t * stage = (zdj_soundcard_dsp_stage_dto_t*)_stage;
    zdj_soundcard_dsp_filt_bi_state_t * data = (zdj_soundcard_dsp_filt_bi_state_t*)stage->data;
    
    switch ( knob ) {
        case 0: stage->knob_0 = input_val; break;
        case 1: stage->knob_1 = input_val; break;
        case 2: stage->knob_2 = input_val; break;
        case 3: stage->knob_3 = input_val; break;
        case 4: stage->knob_4 = input_val; break;
        case 5: stage->knob_5 = input_val; break;
        case 6: stage->knob_6 = input_val; break;
        case 7: stage->knob_7 = input_val; break;
    }

    data->needs_recalc = true;
}

void zdj_soundcard_dsp_filt_bi_init( void * _stage ) {
    zdj_soundcard_dsp_stage_dto_t * stage = (zdj_soundcard_dsp_stage_dto_t*)_stage;
    zdj_soundcard_dsp_filt_bi_state_t * data = calloc( 1, sizeof( zdj_soundcard_dsp_filt_bi_state_t ) );
    stage->data = data;

    stage->knob_0 = 0.0;
    stage->knob_2 = 0.0;

    data->mode = ZDJ_SOUNDCARD_DSP_FILT_BI_MODE_OFF;

    _recalc_coeffs( stage );
}

void zdj_soundcard_dsp_filt_bi_update( void * _stage, float * buf, int channel_count ) {
    zdj_soundcard_dsp_stage_dto_t * stage = (zdj_soundcard_dsp_stage_dto_t*)_stage;
    zdj_soundcard_dsp_filt_bi_state_t * data = (zdj_soundcard_dsp_filt_bi_state_t*)stage->data;

    if( data->needs_recalc ) { _recalc_coeffs( stage ); }

    float sample_l_in;
    float sample_l_out;
    float sample_r_in;
    float sample_r_out;

    float a1 = 0;
    float a2 = 0;
    float a3 = 0;
    float b1 = 0;
    float b2 = 0;

    if( data->mode == ZDJ_SOUNDCARD_DSP_FILT_BI_MODE_LPF || 
        data->mode == ZDJ_SOUNDCARD_DSP_FILT_BI_MODE_HPF 
    ) {
        // Loop thru buffer
        for ( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
            sample_l_in = buf[ i*channel_count ];
            
            // Run algo
            a1 = data->a1_lo * sample_l_in;
            a2 = data->a2_lo * data->in_n1_l_lo;
            a3 = data->a3_lo * data->in_n2_l_lo;
            b1 = data->b1_lo * data->out_n1_l_lo;
            b2 = data->b2_lo * data->out_n2_l_lo;
            sample_l_out = a1+a2+a3-b1-b2;

            // printf( "\nvals -- \na1:%f\na2:%f\na3:%f\nb1:%f\nb2:%f\nin:%f\nout:%f\n", 
            //     a1, a2, a3, b1, b2, sample_l_in, sample_l_out 
            // );
            
            // Step history
            data->in_n2_l_lo = data->in_n1_l_lo;
            data->in_n1_l_lo = sample_l_in;
            data->out_n2_l_lo = data->out_n1_l_lo;
            data->out_n1_l_lo = sample_l_out;

            buf[ i*channel_count ] = sample_l_out;


            if( channel_count > 1 ) {
                sample_r_in = buf[ (i*channel_count)+1 ];
                
                // Run algo
                a1 = data->a1_lo * sample_r_in;
                a2 = data->a2_lo * data->in_n1_r_lo;
                a3 = data->a3_lo * data->in_n2_r_lo;
                b1 = data->b1_lo * data->out_n1_r_lo;
                b2 = data->b2_lo * data->out_n2_r_lo;
                sample_r_out = a1+a2+a3-b1-b2;

                // Step history
                data->in_n2_r_lo = data->in_n1_r_lo;
                data->in_n1_r_lo = sample_r_in;
                data->out_n2_r_lo = data->out_n1_r_lo;
                data->out_n1_r_lo = sample_r_out;

                buf[ (i*channel_count)+1 ] = sample_r_out;
            }
        }
  
    } else if( data->mode == ZDJ_SOUNDCARD_DSP_FILT_BI_MODE_OFF ) {
        data->in_n2_l_lo = 0;
        data->in_n1_l_lo = 0;
        data->out_n2_l_lo = 0;
        data->out_n1_l_lo = 0;
        data->in_n2_r_lo = 0;
        data->in_n1_r_lo = 0;
        data->out_n2_r_lo = 0;
        data->out_n1_r_lo = 0;
    }
}

static void _recalc_coeffs( zdj_soundcard_dsp_stage_dto_t * stage ) {
    zdj_soundcard_dsp_filt_bi_state_t * data = (zdj_soundcard_dsp_filt_bi_state_t*)stage->data;
    data->needs_recalc = false;

    if( stage->knob_0 > 0.1 ) {
        data->mode = ZDJ_SOUNDCARD_DSP_FILT_BI_MODE_HPF;
    } else if( stage->knob_0 < -0.02 ) {
        data->mode = ZDJ_SOUNDCARD_DSP_FILT_BI_MODE_LPF;
    } else {
        data->mode = ZDJ_SOUNDCARD_DSP_FILT_BI_MODE_OFF;
    }

    double knob_coeff;

    if( data->mode == ZDJ_SOUNDCARD_DSP_FILT_BI_MODE_HPF ) {
        knob_coeff = stage->knob_0 * stage->knob_0 * stage->knob_0 * stage->knob_0;
    } else if( data->mode == ZDJ_SOUNDCARD_DSP_FILT_BI_MODE_LPF ) {
        knob_coeff = (stage->knob_0+1.0) * (stage->knob_0+1.0) * (stage->knob_0+1.0);
    }
    data->f = knob_coeff * FILT_HALF_RATE;

    knob_coeff = (1.0 - stage->knob_2) * (1.0 - stage->knob_2) * (1.0 - stage->knob_2);
    data->r = (knob_coeff * FILT_R_TRAV) + FILT_R_LO;

    // printf( "freq: %f res: %f kn(%d): %f/%f\n", data->f, data->r, data->mode, stage->knob_0, knob_coeff );

    if( data->mode == ZDJ_SOUNDCARD_DSP_FILT_BI_MODE_HPF ) {
        data->c_lo = tan( SIG_M_PI * data->f / FILT_SAMP_RATE );
        data->a1_lo = 1.0 / ( 1.0 + data->r * data->c_lo + data->c_lo * data->c_lo );
        data->a2_lo = -2.0 * data->a1_lo;
        data->a3_lo = data->a1_lo;
        data->b1_lo = 2.0 * ( data->c_lo*data->c_lo - 1.0 ) * data->a1_lo;
        data->b2_lo = ( 1.0 - data->r * data->c_lo + data->c_lo * data->c_lo ) * data->a1_lo;
    } else if( data->mode == ZDJ_SOUNDCARD_DSP_FILT_BI_MODE_LPF ) {
        data->c_lo = 1.0 / tan( SIG_M_PI * data->f / FILT_SAMP_RATE );
        data->a1_lo = 1.0 / ( 1.0 + data->r * data->c_lo + data->c_lo * data->c_lo );
        data->a2_lo = 2.0 * data->a1_lo;
        data->a3_lo = data->a1_lo;
        data->b1_lo = 2.0 * ( 1.0 - data->c_lo*data->c_lo) * data->a1_lo;
        data->b2_lo = ( 1.0 - data->r * data->c_lo + data->c_lo * data->c_lo ) * data->a1_lo;
    }
    
    // printf( "\ncoeffs -- \nc:%f\na1:%f\na2:%f\na3:%f\nb1:%f\nb2:%f\n\n",
    //     data->c_lo,
    //     data->a1_lo,
    //     data->a2_lo,
    //     data->a3_lo,
    //     data->b1_lo,
    //     data->b2_lo 
    // );
}
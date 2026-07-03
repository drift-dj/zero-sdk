#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/signal/soundcard/dsp/zdj_soundcard_dsp.h>
#include <zerodj/signal/soundcard/db/zdj_soundcard_dto.h>

# define LOCAL_M_PI		3.14159265358979323846

static double vsa = (1.0 / 4294967295.0);

void zdj_soundcard_dsp_eq_adjust_knob( void * _stage, int knob, int input_val ) {
    zdj_soundcard_dsp_stage_dto_t * stage = (zdj_soundcard_dsp_stage_dto_t*)_stage;
    // printf( "adjust eq knob: %d\n", knob );
    float new_val;
    float cur_val;
    switch ( knob ) {
        case 0: cur_val = stage->knob_0; stage->knob_0_n1 = stage->knob_0; break;
        case 1: cur_val = stage->knob_1; stage->knob_1_n1 = stage->knob_1; break;
        case 2: cur_val = stage->knob_2; stage->knob_2_n1 = stage->knob_2; break;
        case 3: cur_val = stage->knob_3; stage->knob_3_n1 = stage->knob_3; break;
        case 4: cur_val = stage->knob_4; stage->knob_4_n1 = stage->knob_4; break;
        case 5: cur_val = stage->knob_5; stage->knob_5_n1 = stage->knob_5; break;
        case 6: cur_val = stage->knob_6; stage->knob_6_n1 = stage->knob_6; break;
        case 7: cur_val = stage->knob_7; stage->knob_7_n1 = stage->knob_7; break;
    }
    new_val = cur_val + ((float)input_val * 0.069) * (1+(cur_val * 0.41));
    if( new_val > (float)ZDJ_SOUNDCARD_12DB_GAIN ) { new_val = (float)ZDJ_SOUNDCARD_12DB_GAIN; }
    else if( new_val < 0 ) { new_val = 0; }

    switch ( knob ) {
        case 0: stage->knob_0 = new_val; break;
        case 1: stage->knob_1 = new_val; break;
        case 2: stage->knob_2 = new_val; break;
        case 3: stage->knob_3 = new_val; break;
        case 4: stage->knob_4 = new_val; break;
        case 5: stage->knob_5 = new_val; break;
        case 6: stage->knob_6 = new_val; break;
        case 7: stage->knob_7 = new_val; break;
    }
}

void zdj_soundcard_dsp_eq_3_1p_init( void * _stage ) {
    
}

// Process buffer using deck's trim, EQ, FX, etc. settings
void zdj_soundcard_dsp_eq_3_1p_update( void * _stage, float * buf, int channel_count ) {
    // zdj_soundcard_node_t * node = (zdj_soundcard_node_t*)_node;
    // // printf( "%s: %1.3f\n", zdj_soundcard_node_name[ node->name ], (double)node->gain / 140.0 );
    // float * out_buf = node->data_pipe->get_data( node->data_pipe );
    // int channel_count = zdj_soundcard_dto_get_stereo_for_node_name( &zdj_soundcard->dto, node->name ) + 1;
    // for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
    //     out_buf[ i*channel_count ] *= (double)node->gain / 140.0;
    //     if( channel_count > 1 ) {
    //         out_buf[ (i*channel_count)+1 ] *= (double)node->gain / 140.0;
    //     }
    // }
}

void zdj_soundcard_dsp_eq_3_2p_init( void * _stage ) {
    
}

void zdj_soundcard_dsp_eq_3_2p_update( void * _stage, float * buf, int channel_count ) {

}


void zdj_soundcard_dsp_eq_3_4p_init( void * _stage ) {
    zdj_soundcard_dsp_stage_dto_t * stage = (zdj_soundcard_dsp_stage_dto_t*)_stage;
    zdj_soundcard_dsp_eq_state_t * data = calloc( 1, sizeof( zdj_soundcard_dsp_eq_state_t ) );

    // Calculate filter cutoff frequencies
    // Lo - 40 <-> 500
    double f_lo = 500;
    // Hi - 1000 <-> 9000
    double f_hi = 4500;

    data->lf = 2 * sin( LOCAL_M_PI * ( f_lo / 44100.0 ) );
    data->hf = 2 * sin( LOCAL_M_PI * ( f_hi / 44100.0 ) );
    
    stage->data = data;
}

void zdj_soundcard_dsp_eq_3_4p_update( void * _stage, float * buf, int channel_count ) {
    zdj_soundcard_dsp_stage_dto_t * stage = (zdj_soundcard_dsp_stage_dto_t*)_stage;
    zdj_soundcard_dsp_eq_state_t * data = (zdj_soundcard_dsp_eq_state_t*)stage->data;

    double l_l,m_l,h_l;
    double l_r,m_r,h_r;
    float sample_l;
    float sample_r;

    // Update XOver from Knob Vals
    // Calculate filter cutoff frequencies
    // Lo - 40 <-> 1000
    float lo_span = 1000.0 - 40.0;
    double f_lo = stage->knob_4 * lo_span + 40.0;
    data->lf = 2 * sin( LOCAL_M_PI * ( f_lo / 44100.0 ) );
    // Hi - 1000 <-> 11500
    float hi_span = 11500 - 1000;
    double f_hi = stage->knob_3 * hi_span + 1000;
    data->hf = 2 * sin( LOCAL_M_PI * ( f_hi / 44100.0 ) );

    // Prep iteration vals
    double l_start_val = stage->knob_0_n1;
    double l_inc = (stage->knob_0 - l_start_val) / ZDJ_SOUNDCARD_BUF_LEN;
    double l_val = l_start_val;

    double m_start_val = stage->knob_1_n1;
    double m_inc = (stage->knob_1 - m_start_val) / ZDJ_SOUNDCARD_BUF_LEN;
    double m_val = m_start_val;

    double h_start_val = stage->knob_2_n1;
    double h_inc = (stage->knob_2 - h_start_val) / ZDJ_SOUNDCARD_BUF_LEN;
    double h_val = h_start_val;

    printf( "%1.3f->%1.3f: %f\n", l_start_val, stage->knob_0, l_inc );

    // Loop thru buffer
    for ( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
        sample_l = buf[ i*channel_count ];

        // Filter #1 (lowpass)
        data->f1p0_l += (data->lf * (sample_l - data->f1p0_l)) + vsa;
        data->f1p1_l += (data->lf * (data->f1p0_l - data->f1p1_l));
        data->f1p2_l += (data->lf * (data->f1p1_l - data->f1p2_l));
        data->f1p3_l += (data->lf * (data->f1p2_l - data->f1p3_l));        

        // Filter #2 (highpass)
        data->f2p0_l += (data->hf * (sample_l - data->f2p0_l)) + vsa;
        data->f2p1_l += (data->hf * (data->f2p0_l - data->f2p1_l));
        data->f2p2_l += (data->hf * (data->f2p1_l - data->f2p2_l));
        data->f2p3_l += (data->hf * (data->f2p2_l - data->f2p3_l));

        l_l = data->f1p3_l;
        h_l = data->sdm3_l - data->f2p3_l;
        m_l = data->sdm3_l - (h_l + l_l);

        // Refactor to read from knobs 
        l_l *= l_val;
        m_l *= m_val;
        h_l *= h_val;

        // Shuffle history buffer
        data->sdm3_l = data->sdm2_l;
        data->sdm2_l = data->sdm1_l;
        data->sdm1_l = sample_l;

        buf[ i*channel_count ] = l_l + m_l + h_l;

        if( channel_count > 1 ) {
            sample_r = buf[ (i*channel_count)+1 ];

            // Filter #1 (lowpass)
            data->f1p0_r += (data->lf * (sample_r - data->f1p0_r)) + vsa;
            data->f1p1_r += (data->lf * (data->f1p0_r - data->f1p1_r));
            data->f1p2_r += (data->lf * (data->f1p1_r - data->f1p2_r));
            data->f1p3_r += (data->lf * (data->f1p2_r - data->f1p3_r));        

            // Filter #2 (highpass)
            data->f2p0_r += (data->hf * (sample_r - data->f2p0_r)) + vsa;
            data->f2p1_r += (data->hf * (data->f2p0_r - data->f2p1_r));
            data->f2p2_r += (data->hf * (data->f2p1_r - data->f2p2_r));
            data->f2p3_r += (data->hf * (data->f2p2_r - data->f2p3_r));

            l_r = data->f1p3_r;
            h_r = data->sdm3_r - data->f2p3_r;
            m_r = data->sdm3_r - (h_r + l_r);

            // Refactor to read from knobs 
            l_r *= l_val;
            m_r *= m_val;
            h_r *= h_val;

            // Shuffle history buffer
            data->sdm3_r = data->sdm2_r;
            data->sdm2_r = data->sdm1_r;
            data->sdm1_r = sample_r;

            buf[ (i*channel_count)+1 ] = l_r + m_r + h_r;
        }

        l_val += l_inc;
        m_val += m_inc;
        h_val += h_inc;
    }

    stage->knob_0_n1 = stage->knob_0;
    stage->knob_1_n1 = stage->knob_1;
    stage->knob_2_n1 = stage->knob_2;
}
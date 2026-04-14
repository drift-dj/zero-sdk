#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/signal/soundcard/dsp/zdj_soundcard_dsp.h>
#include <zerodj/signal/soundcard/db/zdj_soundcard_dto.h>

void zdj_soundcard_dsp_delay_adjust_knob( void * _stage, int knob, int input_val ) {
    zdj_soundcard_dsp_stage_dto_t * stage = (zdj_soundcard_dsp_stage_dto_t*)_stage;
    // printf( "adjust eq knob: %d\n", knob );
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
    // new_val = cur_val + ((float)input_val * 0.069) * (1+(cur_val * 0.41));
    // if( new_val > (float)ZDJ_SOUNDCARD_12DB_GAIN ) { new_val = (float)ZDJ_SOUNDCARD_12DB_GAIN; }
    // else if( new_val < 0 ) { new_val = 0; }

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

void zdj_soundcard_dsp_delay_set_knob( void * _stage, int knob, int input_val ) {
    
}

void zdj_soundcard_dsp_delay_init( void * _stage ) {
    zdj_soundcard_dsp_stage_dto_t * stage = (zdj_soundcard_dsp_stage_dto_t*)_stage;
    zdj_soundcard_dsp_delay_state_t * data = calloc( 1, sizeof( zdj_soundcard_dsp_delay_state_t ) );
}

void zdj_soundcard_dsp_delay_update( void * _stage, float * buf, int channel_count ) {
    zdj_soundcard_dsp_stage_dto_t * stage = (zdj_soundcard_dsp_stage_dto_t*)_stage;
    zdj_soundcard_dsp_delay_state_t * data = calloc( 1, sizeof( zdj_soundcard_dsp_delay_state_t ) );

    
}


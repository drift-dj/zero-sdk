#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/signal/soundcard/dsp/zdj_soundcard_dsp.h>
#include <zerodj/signal/soundcard/db/zdj_soundcard_dto.h>

void zdj_soundcard_dsp_gain_set_knob( void * _node, int input_val ) {
    zdj_soundcard_node_t * node = (zdj_soundcard_node_t*)_node;
    node->dsp_dto->gain = (float)input_val / 255.0;
    // printf( "zdj_soundcard_dsp_gain_set_knob: %d / %f\n", input_val, node->dsp_dto->gain );
}

void zdj_soundcard_dsp_gain_adjust_knob( void * _node, int input_val ) {
    zdj_soundcard_node_t * node = (zdj_soundcard_node_t*)_node;
    // printf( "adjust gain: %s\n", zdj_soundcard_node_name[ node->name ] );
    // node->dsp_dto->gain += ((float)input_val * 0.069) * (1+(node->dsp_dto->gain * 0.41));
    // if( node->dsp_dto->gain > (float)ZDJ_SOUNDCARD_12DB_GAIN ) { 
    //     node->dsp_dto->gain = (float)ZDJ_SOUNDCARD_12DB_GAIN; 
    // }
    // else if( node->dsp_dto->gain < 0 ) { node->dsp_dto->gain = 0; }
    zdj_soundcard_dsp_process_knob_input( &node->dsp_dto->gain, node->dsp_dto->gain_model, input_val );
    // printf( "gain: %1.1f\n", node->dsp_dto->gain );
}

void zdj_soundcard_dsp_pan_adjust_knob( void * _node, int input_val ) {
    zdj_soundcard_node_t * node = (zdj_soundcard_node_t*)_node;
    // printf( "adjust pan: %s\n", zdj_soundcard_node_name[ node->name ] );
    node->dsp_dto->pan += input_val * 0.021;
    if( node->dsp_dto->pan > 1.0 ) { node->dsp_dto->pan = 1.0; }
    else if( node->dsp_dto->pan < -1.0 ) { node->dsp_dto->pan = -1.0; }
}

void zdj_soundcard_dsp_mute_toggle( void * _node ) {
    zdj_soundcard_node_t * node = (zdj_soundcard_node_t*)_node;
    node->dsp_dto->mute = !node->dsp_dto->mute;
}

void zdj_soundcard_dsp_set_xfade( void * _node, int input_val ) {
    zdj_soundcard_node_t * node = (zdj_soundcard_node_t*)_node;
    // Apply crossfade contour to input val
    node->dsp_dto->gain = (float)input_val / 255.0;
    // printf( "zdj_soundcard_dsp_gain_set_knob: %d / %f\n", input_val, node->dsp_dto->gain );
}
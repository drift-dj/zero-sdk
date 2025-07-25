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

#ifndef ZDJ_IO_NODE_H
#define ZDJ_IO_NODE_H

#include <zerodj/library/zdj_library.h>
#include <zerodj/system/m7/zdj_m7.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>

typedef struct {
    zdj_shared_audio_state_t * shared_audio_state;
    int32_t * shared_dac_buffer;
    volatile int32_t * shared_adc_buffer;
} zdj_io_analog_node_state_t;

zdj_pipeline_node_t * zdj_new_io_analog_node( void );
zdj_error_type_t zdj_io_analog_configure( zdj_pipeline_node_t * node );
zdj_error_type_t zdj_io_analog_run( zdj_pipeline_node_t * node );
zdj_error_type_t zdj_io_analog_stop( zdj_pipeline_node_t * node );

zdj_pipeline_node_t * zdj_new_io_usb_node( );
zdj_error_type_t zdj_io_update_usb_config( zdj_pipeline_node_t * node );

#endif
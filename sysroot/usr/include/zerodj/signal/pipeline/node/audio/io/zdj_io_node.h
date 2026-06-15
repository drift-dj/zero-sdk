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

#include <alsa/asoundlib.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/system/perf/zdj_perf.h>
#include <zerodj/system/m7/zdj_m7.h>
#include <zerodj/system/usb/zdj_usb.h>

typedef struct {
    volatile zdj_shared_audio_state_t * shared_audio_state;
    volatile int32_t * shared_dac_buffer;
    volatile int32_t * shared_adc_buffer;
    volatile zdj_pipeline_node_t * out_1_buffer;
    volatile zdj_pipeline_node_t * out_2_buffer;
    volatile zdj_pipeline_node_t * in_1_buffer;
    volatile zdj_pipeline_node_t * in_2_buffer;
    bool running;
} zdj_io_analog_node_state_t;

typedef enum {
    ZDJ_IO_USB_PHASE_UNKNOWN,
    ZDJ_IO_USB_PHASE_INIT,
    ZDJ_IO_USB_PHASE_READY,
    ZDJ_IO_USB_PHASE_RUNNING,
    ZDJ_IO_USB_PHASE_TEARDOWN,
    ZDJ_IO_USB_PHASE_DONE
} zdj_io_usb_node_phase_t;

typedef struct {
    zdj_io_usb_node_phase_t phase;
    zdj_pipeline_node_t * soundcard_out_buffer;
    zdj_pipeline_node_t * soundcard_in_buffer;

    bool needs_out_samples;
    bool has_in_samples;

    // ALSA storage/state
    snd_async_handler_t * pcm_callback;

    int buffer_len;
    int period_len;

    int write_counter;
    int period_count;
    int cur_period;

    bool capture_available;
    snd_pcm_t *pcm_in_handle;
    int capture_rate;
    snd_pcm_format_t capture_fmt;
    int capture_chan_count;
    // int capture_buf_len;
    void * capture_buf;
    void * capture_buf_0;
    void * capture_buf_1;
    int capture_buf_index;

    bool playback_available;
    snd_pcm_t *pcm_out_handle;
    int playback_rate;
    snd_pcm_format_t playback_fmt;
    int playback_chan_count;
    // int playback_buf_len;
    void * playback_buf;
    void * playback_buf_0;
    void * playback_buf_1;
    int playback_buf_index;
} zdj_io_usb_node_state_t;


zdj_pipeline_node_t * zdj_new_io_analog_node( void );
zdj_error_type_t zdj_io_analog_configure( zdj_pipeline_node_t * node );
zdj_error_type_t zdj_io_analog_run( zdj_pipeline_node_t * node );
zdj_error_type_t zdj_io_analog_stop( zdj_pipeline_node_t * node );
zdj_error_type_t zdj_io_analog_silence( zdj_pipeline_node_t * node );

zdj_error_type_t zdj_analog_io_push_samples( zdj_pipeline_node_t * node );
zdj_error_type_t zdj_analog_io_pull_samples( zdj_pipeline_node_t * node );

zdj_pipeline_node_t * zdj_new_io_usb_node( void );
zdj_error_type_t zdj_io_usb_discover_hwparams( zdj_pipeline_node_t * node, zdj_usb_device_t * device );
zdj_error_type_t zdj_io_usb_alsa_bringup( zdj_pipeline_node_t * node );
zdj_error_type_t zdj_io_usb_alsa_teardown( zdj_pipeline_node_t * node );
zdj_error_type_t zdj_io_usb_alsa_start( zdj_pipeline_node_t * node );

zdj_error_type_t zdj_usb_io_push_samples( zdj_pipeline_node_t * node );
zdj_error_type_t zdj_usb_io_pull_samples( zdj_pipeline_node_t * node );

#endif
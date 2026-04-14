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

#ifndef ZDJ_M7_H
#define ZDJ_M7_H

#define ZDJ_SHARED_MSG_BUF_ADDR 0x55a72000

#include <stdint.h>
#include <zerodj/system/error/zdj_error.h>

// Defined as shared memory region in drift-a106.dtsi device-tree.
// Can be modified (at os-build, not at runtime), but Linux device-tree, 
// the M7 firmware, and zero-sdk must agree on sizes.

// shared (A53<->M7) a/v buffer memory layout:
#define ZDJ_SHARED_VIDEO_BUF_ADDR 0x55a31000
// 0...packed pixels...8192

// audio_buf_state_t
// #define ZDJ_SHARED_AUDIO_STATE_ADDR ZDJ_SHARED_VIDEO_BUF_ADDR + 0x2000
#define ZDJ_SHARED_AUDIO_STATE_ADDR 0x55a33000

// Each buffer holds 16384 bytes total
// Up to 2048 frames of 4 stereo 32-bit samples.
#define ZDJ_SHARED_FRAME_COUNT 2048
#define ZDJ_SHARED_BUFFER_SAMPLE_COUNT ZDJ_SHARED_FRAME_COUNT * 4 * 2
#define ZDJ_SHARED_BUFFER_SIZE ZDJ_SHARED_BUFFER_SAMPLE_COUNT * 4
#define ZDJ_SHARED_DAC_BUF ZDJ_SHARED_AUDIO_STATE_ADDR + 0x1000
#define ZDJ_SHARED_ADC_BUF ZDJ_SHARED_DAC_BUF + 0x8000

// Struct in shared A/V buf for comms between M7+A53 cores.
typedef struct {
    uint8_t running;
    int16_t buffer_len; // sample count in each (DAC/ADC) buffer - 32 >= value <= 2048 
    uint8_t cycle_ready; // bool telling A53 that a new DAC buf is needed and ADC buf is available
    uint64_t cycle_count; // running tally of times M7 buffer was ready
    uint32_t miss_count; // running tally of times M7 buffer was ready but not read from A53
    uint32_t adc_error_count;
    uint32_t dac_error_count;
    uint8_t dac_needs_samples;
    uint8_t dac_back_buf;
    uint8_t adc_has_samples;
    uint8_t adc_back_buf;
    uint8_t dac_cfg_gain_boost_0; // Analog .8dB gain boost for LR DAC L
    uint8_t dac_cfg_gain_boost_1; // Analog .8dB gain boost for LR DAC R
    uint8_t dac_cfg_gain_boost_2; // Analog .8dB gain boost for HP DAC L
    uint8_t dac_cfg_gain_boost_3; // Analog .8dB gain boost for HP DAC R
    uint8_t dac_cfg_volume_0; // Pad (0=+24db/255=-∞db) for LR DAC L
    uint8_t dac_cfg_volume_1; // Pad (0=+24db/255=-∞db) for LR DAC R
    uint8_t dac_cfg_volume_2; // Pad (0=+24db/255=-∞db) for HP DAC L
    uint8_t dac_cfg_volume_3; // Pad (0=+24db/255=-∞db) for HP DAC R
    uint8_t dac_cfg_standby_0; // Power down LR DAC
    uint8_t dac_cfg_standby_1; // Pover down HP DAC
    uint8_t adc_cfg_gain_0; // PGA value for adc 0
    uint8_t adc_cfg_gain_1; // PGA value for adc 0
    uint8_t adc_cfg_gain_2; // PGA value for adc 0
    uint8_t adc_cfg_gain_3; // PGA value for adc 0
    uint8_t adc_cfg_standby; // Power down ADC
    uint16_t audio_watchdog; // Sentinel for shutting off audio output in event of a front-end crash
} zdj_shared_audio_state_t;
// volatile zdj_shared_audio_state_t * zdj_m7_shared_audio_state( void );

typedef struct {
    int32_t update_display_req;
    int32_t show_error_screen_req;
    int32_t activate_audio_req;
    int32_t deactivate_audio_req;
    int32_t update_audio_cfg_req;
    int32_t update_audio_cycle_ready_msg;
    int32_t display_cfg_req;
    char msg_buf[ 256 ];
} zdj_shared_msg_buffer_t;
volatile zdj_shared_msg_buffer_t * zdj_m7_shared_msg_buffer( void );

#endif
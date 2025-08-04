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

#ifndef ZDJ_THREAD_H
#define ZDJ_THREAD_H

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

typedef enum {
    ZDJ_SYSTEM_THREAD_CONTROL,
    ZDJ_SYSTEM_THREAD_UI,
    ZDJ_SYSTEM_THREAD_AUDIO_BUF
} zdj_system_thread_t;

typedef void * ( *zdj_thread_main )( void * );

// Control cycle runs ~1kHz
//  - It runs the GPIO bit-bang cycle to read encoders/pushbutton values.
//  - It then converts those signals into HMI Input events.
//  - It also collects external control inputs (MIDI, etc.) and 
//    puts them into the event buffers for the audio_buf and ui cycles.
zdj_error_type_t zdj_thread_launch_control_cycle( zdj_thread_main main, void * arg );

// UI cycle runs ~125Hz, and determines display frame rate.  UI stack is drawn, 
// pixels are copied into the shared A53/M7 pixel buffer, and M7 core is signalled 
// to transmit a frame of data to the display.
zdj_error_type_t zdj_thread_launch_ui_cycle( zdj_thread_main main, void * arg );

// Audio Buffer cycle runs for every audio buffer transaction with M7 core's codec.
// Cycle requires near-real-time thread servicing, to avoid buffer misses when
// requested by M7 core.  Though servicing should be fast, execution time
// of a single cycle is typicaly short.  
// Ex. @64 samps, cycle runs every 1.5 mSec, but only takes 0.3 mSec to complete.
// M7 core codec transfer length is set in soundcard.h. (ZDJ_SOUNDCARD_BUF_LEN)
// 64 samps: ~1.5 mSec
// 128 samps: ~3 mSec
// NOTE: Audio buf cycle is pinned to A53 Core 1 for performance
zdj_error_type_t zdj_thread_launch_audio_buf_cycle( zdj_thread_main main, void * arg );


#endif

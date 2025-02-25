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

#ifndef ZDJ_HMI_INPUT_H
#define ZDJ_HMI_INPUT_H

// Bitmaps used to build modifier key status for each control
extern uint16_t zdj_hmi_mod_bitmap;

void zdj_hmi_init_input( void );
void zdj_hmi_process_input( void );

void zdj_hmi_input_push_event( zdj_hmi_event_t * event );
zdj_hmi_event_t * zdj_hmi_input_new_event( zdj_hmi_control_id_t id, zdj_hmi_event_type_t type, bool is_modified );
void zdj_hmi_input_clear_events( void );

void zdj_hmi_process_digital_control( zdj_hmi_control_state_t * control_state, int32_t enco_val, int32_t pb_state );
void zdj_hmi_process_analog_control( zdj_hmi_control_state_t * control_state, int32_t pot_val );

#endif
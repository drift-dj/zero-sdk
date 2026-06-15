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

#ifndef ZDJ_SOUNDCARD_OPTIONS_VIEW_H
#define ZDJ_SOUNDCARD_OPTIONS_VIEW_H

typedef void ( *soundcard_options_update_layout_t )( zdj_view_t* );

typedef struct {
    zdj_view_t * menu;
    zdj_view_t * meter;
    bool needs_layout_update;
    soundcard_options_update_layout_t update_layout;
    void ( *handle_menu_hmi_input )( zdj_view_t*, void* );
    zdj_soundcard_node_config_context_t * config_context;

    // DEPRECATING
    int menu_index_gain;
    int menu_index_pad;
    int menu_index_pan;
    int menu_index_stereo;
    int menu_index_mute;
    int menu_index_scope;
} zdj_soundcard_options_state_t;

zdj_view_t * zdj_new_soundcard_options( zdj_soundcard_node_config_context_t * context );

soundcard_options_update_layout_t zdj_soundcard_options_get_update_layout_for_node( 
    zdj_soundcard_node_t * node 
);

void zdj_soundcard_options_update_annot_bus_layout( zdj_view_t * view );
void zdj_soundcard_options_update_record_bus_layout( zdj_view_t * view );
void zdj_soundcard_options_update_port_output_layout( zdj_view_t * view );
void zdj_soundcard_options_update_port_input_layout( zdj_view_t * view );
void zdj_soundcard_options_update_audio_bus_layout( zdj_view_t * view );
void zdj_soundcard_options_update_dj_deck_layout( zdj_view_t * view );
void zdj_soundcard_options_update_ext_deck_layout( zdj_view_t * view );
void zdj_soundcard_options_update_cv_layout( zdj_view_t * view );
void zdj_soundcard_options_update_clock_layout( zdj_view_t * view );
void zdj_soundcard_options_update_usb_output_layout( zdj_view_t * view );
void zdj_soundcard_options_update_usb_input_layout( zdj_view_t * view );
void zdj_soundcard_options_annot_bus_event( zdj_view_t * view, zdj_control_event_t * event );
void zdj_soundcard_options_record_bus_event( zdj_view_t * view, zdj_control_event_t * event );
void zdj_soundcard_options_port_output_hmi( zdj_view_t * view, zdj_control_event_t * _event );
void zdj_soundcard_options_port_input_hmi( zdj_view_t * view, zdj_control_event_t * _event );
void zdj_soundcard_options_audio_bus_hmi( zdj_view_t * view, zdj_control_event_t * _event );
void zdj_soundcard_options_dj_deck_hmi( zdj_view_t * view, zdj_control_event_t * _event );
void zdj_soundcard_options_ext_deck_hmi( zdj_view_t * view, zdj_control_event_t * _event );
void zdj_soundcard_options_cv_hmi( zdj_view_t * view, zdj_control_event_t * _event );
void zdj_soundcard_options_clock_hmi( zdj_view_t * view, zdj_control_event_t * _event );

#endif
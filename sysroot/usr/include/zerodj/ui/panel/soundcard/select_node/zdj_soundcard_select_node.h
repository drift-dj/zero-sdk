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

#ifndef ZDJ_SOUNDCARD_SELECT_OPTIONS_H
#define ZDJ_SOUNDCARD_SELECT_OPTIONS_H

// typedef void ( *soundcard_select_node_cb )( void );

// typedef struct {
//     zdj_soundcard_node_config_context_t * config_context;
// } zdj_soundcard_select_node_state_t;

zdj_view_t * zdj_new_soundcard_select_node( 
    zdj_soundcard_node_config_context_t * context,
    zdj_soundcard_node_name_t edit_node_name
);

zdj_error_type_t zdj_soundcard_build_select_node_output_menu( 
    zdj_view_t * menu, zdj_soundcard_node_config_context_t * context 
);

zdj_error_type_t zdj_soundcard_build_select_node_input_menu( 
    zdj_view_t * menu, zdj_soundcard_node_config_context_t * context 
);

zdj_error_type_t zdj_soundcard_build_select_node_internal_bus_menu( 
    zdj_view_t * menu,
    zdj_soundcard_node_config_context_t * context 
);

zdj_error_type_t zdj_soundcard_build_select_node_aux_bus_menu( 
    zdj_view_t * menu,
    zdj_soundcard_node_config_context_t * context 
);

zdj_error_type_t zdj_soundcard_build_select_node_deck_menu( 
    zdj_view_t * menu,
    zdj_soundcard_node_config_context_t * context 
);

zdj_error_type_t zdj_soundcard_build_select_node_clock_menu( 
    zdj_view_t * menu, zdj_soundcard_node_config_context_t * context 
);

zdj_error_type_t zdj_soundcard_build_select_node_cv_menu( 
    zdj_view_t * menu, zdj_soundcard_node_config_context_t * context 
);

zdj_error_type_t zdj_soundcard_build_select_node_midi_menu( 
    zdj_view_t * menu, zdj_soundcard_node_config_context_t * context 
);

#endif
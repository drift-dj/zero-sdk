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

#ifndef ZDJ_SOUNDCARD_PANEL_H
#define ZDJ_SOUNDCARD_PANEL_H

#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/ui/panel/soundcard/meters/zdj_soundcard_meter.h>

typedef struct {
    zdj_soundcard_t * soundcard;
    zdj_soundcard_node_t * node;
    void ( *main_view_cb )( void* );
    void * main_view_state;
    void ( *options_view_cb )( void* );
    void * options_view_state;
    bool node_selection_is_edit;
    zdj_soundcard_node_name_t edit_name;
    zdj_soundcard_node_t * new_node_selection; // Will be set during node selection page
    zdj_soundcard_node_t * remove_node_selection; // Will be set during node selection page
    int meter_status_counter;
} zdj_soundcard_node_config_context_t;

typedef enum {
    ZDJ_SOUNDCARD_LABEL_MAIN_BUS,
    ZDJ_SOUNDCARD_LABEL_CUE_BUS,
    ZDJ_SOUNDCARD_LABEL_ANNOT_BUS,
    ZDJ_SOUNDCARD_LABEL_RECORD_BUS,
    ZDJ_SOUNDCARD_LABEL_DECK_1_BUS,
    ZDJ_SOUNDCARD_LABEL_DECK_2_BUS,
    ZDJ_SOUNDCARD_LABEL_DECK_EXT_BUS,
    ZDJ_SOUNDCARD_LABEL_ANALOG_IN_12,
    ZDJ_SOUNDCARD_LABEL_ANALOG_IN_34,
    ZDJ_SOUNDCARD_LABEL_ANALOG_IN_1,
    ZDJ_SOUNDCARD_LABEL_ANALOG_IN_2,
    ZDJ_SOUNDCARD_LABEL_ANALOG_IN_3,
    ZDJ_SOUNDCARD_LABEL_ANALOG_IN_4,
    ZDJ_SOUNDCARD_LABEL_ANALOG_OUT_12,
    ZDJ_SOUNDCARD_LABEL_ANALOG_OUT_34,
    ZDJ_SOUNDCARD_LABEL_ANALOG_OUT_1,
    ZDJ_SOUNDCARD_LABEL_ANALOG_OUT_2,
    ZDJ_SOUNDCARD_LABEL_ANALOG_OUT_3,
    ZDJ_SOUNDCARD_LABEL_ANALOG_OUT_4,
    ZDJ_SOUNDCARD_LABEL_USB_IN,
    ZDJ_SOUNDCARD_LABEL_USB_OUT,
    ZDJ_SOUNDCARD_LABEL_USB_IN_L,
    ZDJ_SOUNDCARD_LABEL_USB_IN_R,
    ZDJ_SOUNDCARD_LABEL_USB_OUT_L,
    ZDJ_SOUNDCARD_LABEL_USB_OUT_R,
    ZDJ_SOUNDCARD_LABEL_AUX_12,
    ZDJ_SOUNDCARD_LABEL_AUX_34,
    ZDJ_SOUNDCARD_LABEL_AUX_1,
    ZDJ_SOUNDCARD_LABEL_AUX_2,
    ZDJ_SOUNDCARD_LABEL_AUX_3,
    ZDJ_SOUNDCARD_LABEL_AUX_4,
    ZDJ_SOUNDCARD_LABEL_CLOCK_1,
    ZDJ_SOUNDCARD_LABEL_CLOCK_2,
    ZDJ_SOUNDCARD_LABEL_CLOCK_3,
    ZDJ_SOUNDCARD_LABEL_CLOCK_4,
    ZDJ_SOUNDCARD_LABEL_CV_1,
    ZDJ_SOUNDCARD_LABEL_CV_2,
    ZDJ_SOUNDCARD_LABEL_CV_3,
    ZDJ_SOUNDCARD_LABEL_CV_4,
    ZDJ_SOUNDCARD_LABEL_MIDI_1,
    ZDJ_SOUNDCARD_LABEL_MIDI_2,
    ZDJ_SOUNDCARD_LABEL_MIDI_3,
    ZDJ_SOUNDCARD_LABEL_MIDI_4
} zdj_soundcard_meter_label_t;


typedef struct {
    // menu_item_view_state extension
    char title[ 256 ];
    char subtitle[ 256 ];
    zdj_ui_data_t data;
    void * owned_ptr;
    void * unowned_ptr;
    zdj_menu_item_data_display_type_t data_type;
    char data_prefix[ 32 ];
    char data_suffix[ 32 ];
    int scroll_index;
    zdj_menu_item_view_layout_t layout;
    bool needs_layout_init;
    init_layout_t init_layout;
    bool needs_layout_update;
    update_layout_t update_layout;
    zdj_ui_asset_t icon;
    zdj_ui_asset_t icon_hi;
    zdj_menu_item_view_action_t action; // DEPERECATED
    char link[ 256 ];
    bool is_hilite;
    bool is_blinking;
    int blink_timer;
    int blink_length;
    int blink_period;
    int blink_duty;
    bool handles_hmi;
    bool captures_all_events;
    bool edit_enabled;
    bool edit_active;
    double edit_option_index;
    zdj_menu_item_options_type_t edit_options_type;
    zdj_menu_item_action_t edit_action;
    enter_edit_mode_t enter_edit_mode;
    exit_edit_mode_t exit_edit_mode;
    bool scroll_to_exit_edit_mode;
    zdj_view_t * normal_view;
    zdj_view_t * hilite_view;
    zdj_view_t * title_view;
    zdj_view_t * data_view;
    zdj_view_t * div_view;
    
    // end menu_item_view_state extensions
    zdj_soundcard_meter_label_t label;
    zdj_soundcard_node_config_context_t * config_context;
    zdj_view_t * meter_cover_l;
    zdj_view_t * meter_cover_r;
    zdj_view_t * clock_bpm;
    zdj_view_t * clock_dir;
    zdj_view_t * clock_pulse;
    zdj_view_t * cv_baseline;
    zdj_view_t * cv_value;
    zdj_view_t * fader;
    zdj_view_t * mute_cover;
    zdj_view_t * detail;
    bool show_detail;
    bool has_adjust;
    int adjust_timer;
} zdj_soundcard_meter_state_t;

typedef struct {
    // PANEL_VIEW_BASE EXTENSION - do not edit
    zdj_view_t * menu;
    zdj_view_t * overlay;
    int overlay_counter;
    bool needs_layout_update;
    zdj_view_t * event_target;
    void (*exit_cb) ( void* );
    // PANEL_VIEW_BASE EXTENSION - do not edit

    zdj_soundcard_t * soundcard;
    bool needs_layout_teardown;
} zdj_soundcard_panel_state_t;

// zdj_view_t * zdj_new_soundcard_view( zdj_soundcard_t * soundcard );
zdj_view_t * zdj_new_soundcard_view( void );
int zdj_soundcard_view_add_meter_for_node( 
    zdj_view_t * menu, 
    zdj_soundcard_node_name_t meter_name,
    bool show_detail,
    bool mono,
    int x
);
zdj_view_t * zdj_soundcard_view_new_meter_for_node( 
    zdj_soundcard_node_t * node,
    zdj_soundcard_meter_label_t label,
    bool show_detail,
    bool mono
);
zdj_soundcard_node_config_context_t * zdj_soundcard_new_node_config_context( void );


zdj_view_t * zdj_new_audio_stereo_meter_view( 
    zdj_soundcard_node_t * node, 
    zdj_soundcard_meter_label_t label,
    bool show_detail 
);
zdj_view_t * zdj_new_audio_mono_meter_view( 
    zdj_soundcard_node_t * node, 
    zdj_soundcard_meter_label_t label,
    bool show_detail 
);
zdj_view_t * zdj_new_usb_stereo_meter_view( 
    zdj_soundcard_node_t * node, 
    zdj_soundcard_meter_label_t label,
    bool show_detail 
);
zdj_view_t * zdj_new_usb_mono_meter_view( 
    zdj_soundcard_node_t * node, 
    zdj_soundcard_meter_label_t label,
    bool show_detail 
);
zdj_view_t * zdj_new_clock_meter_view( 
    zdj_soundcard_node_t * node, 
    zdj_soundcard_meter_label_t label,
    bool show_detail 
);
zdj_view_t * zdj_new_cv_meter_view( 
    zdj_soundcard_node_t * node, 
    zdj_soundcard_meter_label_t label,
    bool show_detail 
);
zdj_view_t * zdj_new_midi_meter_view( 
    zdj_soundcard_node_t * node, 
    zdj_soundcard_meter_label_t label,
    bool show_detail 
);
zdj_view_t * zdj_new_discon_stereo_meter_view( 
    zdj_soundcard_node_t * node, 
    zdj_soundcard_meter_label_t label,
    bool show_detail 
);
zdj_view_t * zdj_new_discon_mono_meter_view( 
    zdj_soundcard_node_t * node, 
    zdj_soundcard_meter_label_t label,
    bool show_detail 
);
zdj_view_t * zdj_new_record_meter_view( void );
zdj_view_t * zdj_new_record_mini_meter_view( void );

zdj_soundcard_meter_label_t zdj_meter_label_for_node( zdj_soundcard_node_t * node );
zdj_ui_asset_t zdj_meter_asset_for_label( zdj_soundcard_meter_label_t label );
zdj_error_type_t zdj_meter_view_set_label( zdj_view_t * meter, zdj_soundcard_meter_label_t label );
#endif
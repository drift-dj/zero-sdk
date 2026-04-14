#include <stdio.h>
#include <dirent.h>
#include <unistd.h>
#include <math.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/modal_view/zdj_modal_view.h>
#include <zerodj/ui/panel/zdj_ui_panel.h>
#include <zerodj/ui/panel/soundcard/zdj_soundcard_panel.h>
#include <zerodj/ui/panel/soundcard/meters/zdj_soundcard_meter.h>
#include <zerodj/ui/panel/soundcard/options/zdj_soundcard_options.h>
#include <zerodj/ui/view/text_input_view/zdj_text_input_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _update_layout( zdj_view_t * view, zdj_view_clip_t * clip );
static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event );
static void _deinit_state( zdj_view_t * view );

static void _cb( void * _context );

static void _handle_back( zdj_view_t * menu_view );
static void _handle_aux_bus_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_clock_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_cv_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_save_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_load_btn( zdj_view_t * view, zdj_control_event_t * _event );

static bool _single_meter_for_port( zdj_soundcard_node_name_t name );

static void _text_input_cb( zdj_text_input_view_action_t action, char * result );

static zdj_soundcard_panel_state_t * _zdj_soundcard_view_state;

zdj_view_t * zdj_new_soundcard_view( void ) {
    // zdj_view_t * soundcard_view = zdj_new_modal_view( zdj_modal_rect( ) );
    zdj_view_t * soundcard_view = zdj_new_modal_view( zdj_screen_rect( ) );
    soundcard_view->draw = &_draw;
    soundcard_view->handle_control_event = &_handle_control;
    soundcard_view->deinit_state = &_deinit_state;
    soundcard_view->map = ZDJ_CONTROL_MAP_SOUNDCARD_PANEL;

    // Add a state instance
    zdj_soundcard_panel_state_t * state = calloc( 1, sizeof( zdj_soundcard_panel_state_t ) );
    _zdj_soundcard_view_state = state; // store for access during options cb func
    state->needs_layout_update = true;
    soundcard_view->state = state;

    // Make menu
    // zdj_view_t * menu = zdj_new_menu_view( ZDJ_HORIZONTAL, zdj_modal_rect( ) );
    zdj_view_t * menu = zdj_new_menu_view( ZDJ_HORIZONTAL, zdj_screen_rect( ) );
    zdj_add_subview( soundcard_view, menu );
    menu->frame.x = 0;
    menu->frame.y = 0;
    menu->frame.w = ZDJ_SCREEN_W;
    menu->frame.h = ZDJ_SCREEN_H;
    state->menu = menu;
    
    // Set up header
    zdj_view_t * menu_header = zdj_new_menu_header( 
        "Soundcard",
        " ",
        ZDJ_MENU_HEADER_STYLE_NORMAL,
        ZDJ_MENU_HEADER_BACK_STYLE_NONE
    );
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_header->state;
    // header_state->handle_back = &_handle_back;
    zdj_menu_view_add_header( menu, menu_header );

    state->overlay = zdj_ui_panel_new_overlay( "Mixer" );
    zdj_add_subview( soundcard_view, state->overlay );

    return soundcard_view;
}

static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {
    // printf( "soundcard _handle_control\n" );
    zdj_control_event_t * e = (zdj_control_event_t *)_event;
    zdj_soundcard_panel_state_t * state = (zdj_soundcard_panel_state_t*)view->state;
    
    // Ignore events which have been blocked by layers above this one.
    if( e->blocked ) { return; }

    // Grab Tone 1,2,3 + Jog push turn to send controls into channels.
    if( e->id == ZDJ_UI_CONTROL_JOG_ADJUST_1 ||
        e->id == ZDJ_UI_CONTROL_TONE_1_ADJUST_0 ||
        e->id == ZDJ_UI_CONTROL_TONE_1_RELEASE_0 ||
        e->id == ZDJ_UI_CONTROL_TONE_2_ADJUST_0
    ) {
        // printf( "soundcard view grabbing scroll\n" );
        // Get current menu scroll index
        zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu->state;
        if( menu_state->scroll_index > -1 ) {
            zdj_view_t * meter = zdj_menu_view_item_at_current_scroll_index( state->menu );
            
            // Send event into meter
            meter->handle_control_event( meter, _event );
        }
    } else if( e->id == ZDJ_UI_CONTROL_NAV_RELEASE_0 ) {
        _handle_back( state->menu );
    } else {
        // printf( "soundcard menu grabbing event\n" );
        // Send remaining events down into the menu
        state->menu->handle_control_event( state->menu, _event );
    }   
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_soundcard_panel_state_t * state = (zdj_soundcard_panel_state_t*)view->state;

    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );

    if( state->overlay_counter > 0 ) { state->overlay->frame.x = 0; state->overlay_counter--; }
    else { state->overlay->frame.x = 129; }

    if( state->needs_layout_update ) {
        _update_layout( view, clip );
    }
}

// This should be invoked anytime there's a bus or port linkage change in the soundcard
static void _update_layout( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // printf( "_update_layout\n" );

    printf( "frames: %d\n", zdj_ui_msec_to_frames( 400 ) );

    zdj_soundcard_panel_state_t * state = (zdj_soundcard_panel_state_t*)view->state;
    zdj_view_t * menu_view = state->menu;
    // zdj_soundcard_t * soundcard = state->soundcard;
    zdj_soundcard_t * soundcard = zdj_soundcard;

    state->needs_layout_update = false;

    zdj_menu_view_remove_all_items( menu_view );

    //////////////////////////////////////////////////////////
    // Analyze port linkages and add approprate meter views //
    //////////////////////////////////////////////////////////

    zdj_view_t * meter;
    zdj_view_t * meter_detail;
    int stereo_w = 18;
    int mono_w = 14;
    int meter_x = 4;

    // Add Deck 1
    meter_x += zdj_soundcard_view_add_meter_for_node( 
        menu_view, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE, true, false, meter_x 
    );
    // Add Ext Deck
    meter_x += zdj_soundcard_view_add_meter_for_node( 
        menu_view, ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE, true, false, meter_x 
    );
    // Add Deck 2
    meter_x += zdj_soundcard_view_add_meter_for_node( 
        menu_view, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE, true, false, meter_x 
    );

    // Add deck divider
    // zdj_view_t * decks = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_DECKS_DIV ], NULL );
    // decks->frame.x = meter_x;
    // decks->frame.y = 37;
    // zdj_menu_view_add_item( menu_view, decks );
    zdj_view_t * decks_div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_DIV ], NULL );
    decks_div->frame.x = meter_x+3;
    decks_div->frame.y = 5;
    zdj_menu_view_add_item( menu_view, decks_div );
    meter_x += 11;


    // // Add any connected USB devices
    // zdj_soundcard_node_t * usb_out_0 = zdj_soundcard_get_node_for_name( 
    //     soundcard, ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_0 
    // );
    // if( usb_out_0 ) {

    // }

    // User Aux busses
    if( zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0 )->output_link_count ||
        zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0 )->input_link_count ) {
        meter_x += zdj_soundcard_view_add_meter_for_node( menu_view, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0, true, false, meter_x );
    }
    if( zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1 )->output_link_count ||
        zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1 )->input_link_count ) {
        meter_x += zdj_soundcard_view_add_meter_for_node( menu_view, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1, true, false, meter_x );
    }
    if( zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2 )->output_link_count ||
        zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2 )->input_link_count ) {
        meter_x += zdj_soundcard_view_add_meter_for_node( menu_view, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2, true, false, meter_x );
    }
    if( zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3 )->output_link_count ||
        zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3 )->input_link_count ) {
        meter_x += zdj_soundcard_view_add_meter_for_node( menu_view, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3, true, false, meter_x );
    }

    // User CV busses
    if( zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_0 )->output_link_count ||
        zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_0 )->input_link_count ) {
        meter_x += zdj_soundcard_view_add_meter_for_node( menu_view, ZDJ_SOUNDCARD_NODE_NAME_CV_0, true, false, meter_x );
    }
    if( zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_1 )->output_link_count ||
        zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_1 )->input_link_count ) {
        meter_x += zdj_soundcard_view_add_meter_for_node( menu_view, ZDJ_SOUNDCARD_NODE_NAME_CV_1, true, false, meter_x );
    }
    if( zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_2 )->output_link_count ||
        zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_2 )->input_link_count ) {
        meter_x += zdj_soundcard_view_add_meter_for_node( menu_view, ZDJ_SOUNDCARD_NODE_NAME_CV_2, true, false, meter_x );
    }
    if( zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_3 )->output_link_count ||
        zdj_soundcard_get_node_for_name( soundcard, ZDJ_SOUNDCARD_NODE_NAME_CV_3 )->input_link_count ) {
        meter_x += zdj_soundcard_view_add_meter_for_node( menu_view, ZDJ_SOUNDCARD_NODE_NAME_CV_3, true, false, meter_x );
    }

    // Add LR Bus
    meter_x += zdj_soundcard_view_add_meter_for_node( 
        menu_view, ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS, true, false, meter_x 
    );
    // Add Cue Bus
    meter_x += zdj_soundcard_view_add_meter_for_node( 
        menu_view, ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS, true, false, meter_x 
    );
    // Add Annotation Bus
    meter_x += zdj_soundcard_view_add_meter_for_node( 
        menu_view, ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS, true, false, meter_x 
    );
    // Add Recording Bus
    meter_x += zdj_soundcard_view_add_meter_for_node( 
        menu_view, ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS, true, false, meter_x 
    );
    // Add Clock Bus
    meter_x += zdj_soundcard_view_add_meter_for_node( 
        menu_view, ZDJ_SOUNDCARD_NODE_NAME_XPORT_1, true, false, meter_x 
    );

    // Add Button Stack
    int btn_x = meter_x;
    int bus_btn_y = 2;
    int bus_btn_h = 7;
    // Add Bus btn
    if( zdj_soundcard_can_add_aux_bus( soundcard ) ) {
            zdj_view_t * bus_btn = zdj_new_asset_menu_item( 
            ZDJ_UI_ASSET_MIXER_ADD_BUS,
            ZDJ_UI_ASSET_MIXER_ADD_BUS_HI,
            false
        );
        bus_btn->handle_control_event = &_handle_aux_bus_btn;
        bus_btn->frame.x = btn_x;
        bus_btn->frame.y = bus_btn_y;
        bus_btn->frame.w = 16;
        bus_btn->frame.h = 5;
        zdj_menu_view_add_item( menu_view, bus_btn );
        bus_btn_y += bus_btn_h;
    }

    // Add Midi btn
    if( zdj_soundcard_can_add_midi_bus( soundcard ) ) {
        zdj_view_t * midi_btn = zdj_new_asset_menu_item( 
            ZDJ_UI_ASSET_MIXER_ADD_MIDI,
            ZDJ_UI_ASSET_MIXER_ADD_MIDI_HI,
            false
        );
        // midi_btn->handle_control_event = &_zdj_bus_mixer_handle_bus_btn;
        midi_btn->frame.x = btn_x;
        midi_btn->frame.y = 23;
        midi_btn->frame.w = 19;
        midi_btn->frame.h = 5;
        zdj_menu_view_add_item( menu_view, midi_btn );
        bus_btn_y += bus_btn_h;
    }

    meter_x += 12;

    // Add bus divider
    // zdj_view_t * bus = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_BUS_DIV ], NULL );
    // bus->frame.x = meter_x;
    // bus->frame.y = 39;
    // zdj_menu_view_add_item( menu_view, bus );
    zdj_view_t * bus_div = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_DIV ], NULL );
    bus_div->frame.x = meter_x+9;
    bus_div->frame.y = 5;
    zdj_menu_view_add_item( menu_view, bus_div );
    meter_x += 16;
    

    // Add analog out ports 2+3
    if( _single_meter_for_port( ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2 ) ) {
        meter_x += zdj_soundcard_view_add_meter_for_node( 
            menu_view, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2, true, false, meter_x 
        );
    } else {
        meter_x += zdj_soundcard_view_add_meter_for_node( 
            menu_view, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2, true, false, meter_x 
        );
        meter_x += zdj_soundcard_view_add_meter_for_node( 
            menu_view, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3, true, false, meter_x 
        );
    }

    // Add analog out ports 0+1
    if( _single_meter_for_port( ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0 ) ) {
        // printf( "Adding ports 0 + 1 stereo\n" );
        meter_x += zdj_soundcard_view_add_meter_for_node( 
            menu_view, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0, true, false, meter_x 
        );
    } else {
        // printf( "Adding port 0 mono\n" );
        meter_x += zdj_soundcard_view_add_meter_for_node( 
            menu_view, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0, true, false, meter_x 
        );
        // printf( "Adding port 1 mono\n" );
        meter_x += zdj_soundcard_view_add_meter_for_node( 
            menu_view, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1, true, false, meter_x 
        );
    }

    // Add analog in ports 0+1
    if( _single_meter_for_port( ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0 ) ) {
        meter_x += zdj_soundcard_view_add_meter_for_node( 
            menu_view, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0, true, false, meter_x 
        );
    } else {
        meter_x += zdj_soundcard_view_add_meter_for_node( 
            menu_view, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0, true, false, meter_x 
        );
        meter_x += zdj_soundcard_view_add_meter_for_node( 
            menu_view, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1, true, false, meter_x 
        );
    }

    // Add analog in ports 2+3
    if( _single_meter_for_port( ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2 ) ) {
        meter_x += zdj_soundcard_view_add_meter_for_node( 
            menu_view, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2, true, false, meter_x 
        );
    } else {
        meter_x += zdj_soundcard_view_add_meter_for_node( 
            menu_view, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2, true, false, meter_x 
        );
        meter_x += zdj_soundcard_view_add_meter_for_node( 
            menu_view, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3, true, false, meter_x 
        );
    }


    // Add Button Stack

    btn_x = meter_x;
    // Add Save btn
    zdj_view_t * save_btn = zdj_new_asset_menu_item( 
        ZDJ_UI_ASSET_MIXER_ADD_SAVE_BTN,
        ZDJ_UI_ASSET_MIXER_ADD_SAVE_BTN_HI,
        false
    );
    save_btn->handle_control_event = &_handle_save_btn;
    save_btn->frame.x = btn_x;
    save_btn->frame.y = 2;
    save_btn->frame.w = 16;
    save_btn->frame.h = 5;
    zdj_menu_view_add_item( menu_view, save_btn );

    // Add Load btn
    zdj_view_t * load_btn = zdj_new_asset_menu_item( 
        ZDJ_UI_ASSET_MIXER_ADD_LOAD_BTN,
        ZDJ_UI_ASSET_MIXER_ADD_LOAD_BTN_HI,
        false
    );
    // load_btn->handle_control_event = &_zdj_soundcard_view_handle_bus_btn;
    load_btn->frame.x = btn_x;
    load_btn->frame.y = 8;
    load_btn->frame.w = 16;
    load_btn->frame.h = 5;
    zdj_menu_view_add_item( menu_view, load_btn );

    // printf( "_update_layout done\n" );
}

static void _deinit_state( zdj_view_t * view ) {
    printf( "_deinit_state\n" );
    zdj_menu_view_state_t * state = (zdj_menu_view_state_t*)view->state;
    free( state );
    view->state = NULL;
    printf( "_deinit_state done\n" );
}

zdj_soundcard_node_config_context_t * zdj_soundcard_new_node_config_context( void ) {
    zdj_soundcard_node_config_context_t * context = calloc( 
        1, sizeof( zdj_soundcard_node_config_context_t ) 
    );
    return context;
}

int zdj_soundcard_view_add_meter_for_node( 
    zdj_view_t * menu, 
    zdj_soundcard_node_name_t meter_name,
    bool show_detail,
    bool mono,
    int x
) {
    zdj_soundcard_node_t * node = zdj_soundcard_get_node_for_name( zdj_soundcard, meter_name );
    zdj_soundcard_meter_label_t label = zdj_meter_label_for_node( node );
    // printf( "add_meter_for_node: %s, st:%d\n", zdj_soundcard_node_name[ node->name ], node->stereo );
    zdj_view_t * meter;

    if( zdj_soundcard_node_name_is_output( node->name ) ) {
        // Output nodes show meter for whatever is feeding them
        // with the output node's label in place.
        // printf( "0 %d\n", node->input_links->source_node );
        zdj_soundcard_node_t * source_node = zdj_soundcard_get_node_for_name( 
            zdj_soundcard, node->input_links->source_node 
        );
        // printf( "0.1: %p %d %d %d\n", source_node, label, show_detail, node->stereo );
        meter = zdj_soundcard_view_new_meter_for_node( source_node, label, show_detail, !node->stereo );
        // printf( "0.2\n" );
    // } else if ( zdj_soundcard_node_name_is_input( node->name ) ) {
    //     zdj_soundcard_node_t * linked_node = zdj_soundcard_get_node_for_name( 
    //         zdj_soundcard, node->output_links->dest_node
    //     );
    //     meter = zdj_soundcard_view_new_meter_for_node( linked_node, label, show_detail, !node->stereo );
    } else {
        meter = zdj_soundcard_view_new_meter_for_node( node, label, show_detail, mono );
    }
    if( !meter ) { return 0; }
    meter->frame.x = x;
    meter->frame.y = 2;
    meter->frame.h = 55;

    // Keep a ref to context inside meter state
    zdj_soundcard_node_config_context_t * context = zdj_soundcard_new_node_config_context( );
    context->soundcard = zdj_soundcard;
    context->node = node;
    context->main_view_cb = &_cb;
    context->main_view_state = (void*)_zdj_soundcard_view_state;
    context->meter_status_counter = 0;
    zdj_soundcard_meter_state_t * meter_state = (zdj_soundcard_meter_state_t*)meter->state;
    meter_state->config_context = context;

    zdj_menu_view_add_item( menu, meter );

    return meter->frame.w;
}

zdj_view_t * zdj_soundcard_view_new_meter_for_node( 
    zdj_soundcard_node_t * node, 
    zdj_soundcard_meter_label_t label,
    bool show_detail,
    bool mono
) {
    // printf( "zdj_soundcard_view_new_meter_for_node: %p %s, %d, %d\n",
    //     node, zdj_soundcard_node_name[ node->name ], label, show_detail
    // );

    zdj_view_t * meter = NULL;
    if( !node ) {
        printf( "attempting a null meter\n" );
        // Don't display a meter for something without linkage
        // Revise this to show a severed state
        return NULL;
    } else if ( zdj_soundcard_node_name_is_audio( node->name ) ) {
        if( node->stereo && !mono ) { meter = zdj_new_audio_stereo_meter_view( node, label, show_detail ); } 
        else { meter = zdj_new_audio_mono_meter_view( node, label, show_detail ); }
    } else if ( zdj_soundcard_node_name_is_clock( node->name ) ) {
        meter = zdj_new_clock_meter_view( node, label, show_detail );
    } else if ( zdj_soundcard_node_name_is_cv( node->name ) ) {
        meter = zdj_new_cv_meter_view( node, label, show_detail );
    } else if ( zdj_soundcard_node_name_is_usb( node->name ) ) {
        if( node->stereo ) { meter = zdj_new_usb_stereo_meter_view( node, label, show_detail ); } 
        else { meter = zdj_new_usb_mono_meter_view( node, label, show_detail ); }
    } else if ( zdj_soundcard_node_name_is_midi( node->name ) ) {
        meter = zdj_new_midi_meter_view( node, label, show_detail );
    }

    return meter;
}

static void _handle_back( zdj_view_t * menu_view ) {
    printf( "_handle_back\n" );
    zdj_soundcard_save_temp( zdj_soundcard );
    zdj_ui_panel_toggle( );
}

// Handle a callback invoked from somewhere within the souncard config view stack
static void _cb( void * _context ) {
    zdj_soundcard_node_config_context_t * context = (zdj_soundcard_node_config_context_t*)_context;
    zdj_soundcard_panel_state_t * state = (zdj_soundcard_panel_state_t*)context->main_view_state;
    // Set needs_layout_update
    state->needs_layout_update = true;
}

static void _handle_aux_bus_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    // Create a new context 
    zdj_soundcard_node_config_context_t * context = zdj_soundcard_new_node_config_context( );
    context->soundcard = zdj_soundcard;
    context->node = zdj_soundcard_get_available_aux_bus_node( zdj_soundcard );
    context->main_view_cb = &_cb;
    context->main_view_state = (void*)_zdj_soundcard_view_state;
    context->meter_status_counter = 0;

    zdj_push_subview( zdj_panel_view( ), zdj_new_soundcard_options( context ), true );
}

static void _handle_clock_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    // Create a new context 
    zdj_soundcard_node_config_context_t * context = zdj_soundcard_new_node_config_context( );
    context->soundcard = zdj_soundcard;
    context->node = zdj_soundcard_get_available_clock_bus_node( zdj_soundcard );
    context->main_view_cb = &_cb;
    context->main_view_state = (void*)_zdj_soundcard_view_state;
    context->meter_status_counter = 0;

    zdj_push_subview( zdj_panel_view( ), zdj_new_soundcard_options( context ), true );
}

static void _handle_cv_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    // Create a new context 
    zdj_soundcard_node_config_context_t * context = zdj_soundcard_new_node_config_context( );
    context->soundcard = zdj_soundcard;
    context->node = zdj_soundcard_get_available_cv_bus_node( zdj_soundcard );
    context->main_view_cb = &_cb;
    context->main_view_state = (void*)_zdj_soundcard_view_state;
    context->meter_status_counter = 0;

    zdj_push_subview( zdj_panel_view( ), zdj_new_soundcard_options( context ), true );
}

static void _handle_save_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    // Push text edit for soundcard name
    zdj_view_t * name_edit_view = zdj_new_text_input_view( 
        _text_input_cb, 
        "__test__"
    );
    zdj_push_subview( zdj_panel_view( ), name_edit_view, true );
}

static void _handle_load_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    // Open soundcard selection list
}

static bool _single_meter_for_port( zdj_soundcard_node_name_t name ) {
    zdj_soundcard_node_t * port_node;
    zdj_soundcard_node_t * source_node;
    zdj_soundcard_node_t * dest_node;
    switch ( name ) {
    case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0:
    case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1:
    case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2:
    case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3:
        // printf( "analog out\n" );
        port_node = zdj_soundcard_get_node_for_name( zdj_soundcard, name );
        source_node = zdj_soundcard_get_node_for_name( zdj_soundcard, port_node->input_links->source_node );
        if( zdj_soundcard_node_name_is_audio( source_node->name ) && port_node->stereo ) { return true; }
        else { return false; }
    case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0:
    case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1:
    case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2:
    case ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3:
        // printf( "analog in\n" );
        port_node = zdj_soundcard_get_node_for_name( zdj_soundcard, name );
        if( zdj_soundcard_node_name_is_audio( port_node->name ) && port_node->stereo ) { return true; }
        else { return false; }
    default: return false;
    }
}

static void _text_input_cb( zdj_text_input_view_action_t action, char * result ) { 
    switch ( action ) {
        case ZDJ_TEXT_INPUT_ACTION_OKAY:
            printf( "name edit exit okay: %s\n", result );
            // Update name in database
            // Invalidate name item layout
            // _name_menu_item
            break;
        case ZDJ_TEXT_INPUT_ACTION_CANCEL:
            printf( "name edit exit cancel\n" );
            break;
        default:
            break;
    }
    zdj_pop_subview_of( zdj_panel_view( ), true );
}
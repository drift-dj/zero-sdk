#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>


#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/menu_section_view/zdj_menu_section_view.h>
#include <zerodj/ui/view/modal_view/zdj_modal_view.h>
#include <zerodj/ui/panel/soundcard/zdj_soundcard_panel.h>
#include <zerodj/ui/panel/soundcard/options/zdj_soundcard_options.h>
#include <zerodj/ui/panel/soundcard/meters/zdj_soundcard_meter.h>
#include <zerodj/ui/panel/soundcard/select_node/zdj_soundcard_select_node.h>
#include <zerodj/ui/view/zdj_view_stack.h>


static void _handle_gain( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_pad( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_stereo( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_mute( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_eq_lo( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_eq_hi( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_eq_model( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_linkage( zdj_view_t * view, zdj_control_event_t * _event );
static void _cb( void * _context );

void zdj_soundcard_options_update_ext_deck_layout( zdj_view_t * view ) {
    printf( "zdj_soundcard_options_update_ext_layout\n" );
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)view->state;
    zdj_view_t * menu_view = options_state->menu;
    options_state->needs_layout_update = false;

    zdj_soundcard_node_t * page_node = options_state->config_context->node;

    zdj_menu_view_remove_all_items( menu_view );
    if( options_state->meter ) {
        zdj_remove_subview_of( view, options_state->meter );
    }

    zdj_menu_view_add_padding( menu_view, 1 );

    // Gain
    zdj_view_t * gain = zdj_new_data_menu_item( 
        "Gain", 
        ZDJ_MENU_ITEM_LAYOUT_DATA_R,
        ZDJ_MENU_ITEM_DATA_TYPE_DOUBLE_1,
        NULL,
        " dB" 
    );
    gain->handle_control_event = &_handle_gain;
    zdj_menu_item_view_state_t * gain_state = (zdj_menu_item_view_state_t*)gain->state;
    gain_state->data.f_val = zdj_signal_db_for_gain( page_node->dsp_dto->gain );
    gain_state->data.ptr = options_state;
    gain_state->captures_all_events = true;
    zdj_menu_view_add_item( menu_view, gain );

    // // Pan
    // zdj_view_t * pan = zdj_new_data_menu_item( 
    //     "Pan", 
    //     ZDJ_MENU_ITEM_LAYOUT_DATA_R,
    //     ZDJ_MENU_ITEM_DATA_TYPE_INT,
    //     NULL,
    //     NULL
    // );
    // zdj_menu_item_view_state_t * pan_state = (zdj_menu_item_view_state_t*)pan->state;
    // pan_state->data.i_val = page_node->pan;
    // pan_state->data.ptr = options_state;
    // zdj_menu_view_add_item( menu_view, pan );

    
    // // Stereo
    // zdj_view_t * stereo = zdj_new_menu_item( "Stereo", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    // stereo->handle_control_event = &_handle_stereo;
    // zdj_menu_item_view_state_t * stereo_state = (zdj_menu_item_view_state_t*)stereo->state;
    // stereo_state->data.b_val = page_node->stereo;
    // stereo_state->data.ptr = options_state;
    // zdj_menu_view_add_item( menu_view, stereo );

    // Mute
    zdj_view_t * mute = zdj_new_menu_item( "Mute", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    mute->handle_control_event = &_handle_mute;
    zdj_menu_item_view_state_t * mute_state = (zdj_menu_item_view_state_t*)mute->state;
    mute_state->data.b_val = page_node->mute;
    mute_state->data.ptr = options_state; 
    zdj_menu_view_add_item( menu_view, mute );

    // EQ Settings
    zdj_menu_view_add_section( menu_view, zdj_new_menu_section( "EQ Settings" ) );
    
    zdj_soundcard_dsp_stage_dto_t * eq_stage = (zdj_soundcard_dsp_stage_dto_t*)zdj_soundcard_dto_get_dsp_stage_for_type( page_node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ );

    zdj_view_t * xover_hi = zdj_new_data_menu_item( 
        "Hi X-Over", 
        ZDJ_MENU_ITEM_LAYOUT_DATA_R,
        ZDJ_MENU_ITEM_DATA_TYPE_DOUBLE_1,
        NULL,
        " Hz" 
    );
    // xover_hi->tag = ZDJ_SOUNDCARD_OPTIONS_EQ_HI;
    xover_hi->handle_control_event = &_handle_eq_hi;
    zdj_menu_item_view_state_t * xover_hi_state = (zdj_menu_item_view_state_t*)xover_hi->state;
    xover_hi_state->data.f_val = zdj_signal_hi_xover_hz_for_unit_val( eq_stage->knob_3 );
    xover_hi_state->data.ptr = options_state;
    xover_hi_state->captures_all_events = true;
    zdj_menu_view_add_item( menu_view, xover_hi );

    zdj_view_t * xover_lo = zdj_new_data_menu_item( 
        "Lo X-Over", 
        ZDJ_MENU_ITEM_LAYOUT_DATA_R,
        ZDJ_MENU_ITEM_DATA_TYPE_DOUBLE_1,
        NULL,
        " Hz" 
    );
    // xover_lo->tag = ZDJ_SOUNDCARD_OPTIONS_EQ_LO;
    xover_lo->handle_control_event = &_handle_eq_lo;
    zdj_menu_item_view_state_t * xover_lo_state = (zdj_menu_item_view_state_t*)xover_lo->state;
    xover_lo_state->data.f_val = zdj_signal_lo_xover_hz_for_unit_val( eq_stage->knob_4 );
    xover_lo_state->data.ptr = options_state;
    xover_lo_state->captures_all_events = true;
    zdj_menu_view_add_item( menu_view, xover_lo );

    zdj_view_t * eq_model = zdj_new_data_menu_item( 
        "EQ Model", 
        ZDJ_MENU_ITEM_LAYOUT_DATA_R,
        ZDJ_MENU_ITEM_DATA_TYPE_CHAR,
        NULL,
        NULL 
    );
    // eq_model->tag = ZDJ_SOUNDCARD_OPTIONS_EQ_MODEL;
    eq_model->handle_control_event = &_handle_eq_model;
    zdj_menu_item_view_state_t * eq_model_state = (zdj_menu_item_view_state_t*)eq_model->state;
    strcpy( 
        eq_model_state->data.c_val, 
        "Isolator"
    );
    eq_model_state->data.ptr = options_state;
    zdj_menu_view_add_item( menu_view, eq_model );


    // Outputs section
    zdj_menu_view_add_section( menu_view, zdj_new_menu_section( "Outputs" ) );
    // If there are outputs, show them, otherwise show add outputs button
    int out_node_count = zdj_soundcard_count_output_nodes_from_node( &zdj_soundcard->dto, page_node );
    if( out_node_count ) {
        for( int i=0; i<out_node_count; i++ ) {
            // If we're linked to a stereo io port, adjust name to show "out 1/2"
            // instead of just "out 1"
            char adjusted_name[ 64 ];
            if( zdj_soundcard_node_name_is_analog_input( page_node->output_links[ i ].dest_node ) ||
                zdj_soundcard_node_name_is_analog_output( page_node->output_links[ i ].dest_node ) 
            ) {
                zdj_soundcard_get_port_title_with_stereo( 
                    options_state->config_context->soundcard,
                    page_node->output_links[ i ].dest_node, 
                    adjusted_name 
                );
            } else {
                strcpy( adjusted_name, zdj_soundcard_node_name[ page_node->output_links[ i ].dest_node ] );
            }
            
            zdj_view_t * output = zdj_new_menu_item( 
                adjusted_name, 
                ZDJ_MENU_ITEM_LAYOUT_BASIC_R 
            );
            output->handle_control_event = &_handle_linkage;
            zdj_menu_item_view_state_t * output_state = (zdj_menu_item_view_state_t*)output->state;
            output_state->data.ptr = options_state;
            output_state->data.i_val = page_node->output_links[ i ].dest_node;
            zdj_menu_view_add_item( menu_view, output );
        }
    }

    // Add Output
    zdj_view_t * add_output = zdj_new_menu_item( "+ Add Output", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
    add_output->handle_control_event = &_handle_linkage;
    zdj_menu_item_view_state_t * add_output_state = (zdj_menu_item_view_state_t*)add_output->state;
    add_output_state->data.ptr = options_state;
    zdj_menu_view_add_item( menu_view, add_output );


    zdj_view_t * meter = zdj_soundcard_view_new_meter_for_node( 
        page_node, zdj_meter_label_for_node( page_node ), false, false 
    );
    if( meter ) { 
        // Since we're fudging a menu_item_view, we manually create the state data instance.
        zdj_soundcard_meter_state_t * meter_state = (zdj_soundcard_meter_state_t*)meter->state;
        meter_state->config_context = options_state->config_context;
        meter->frame.x = 5;
        meter->frame.y = 10;
        meter->frame.h = 47;
        meter->frame.w = 12;
        zdj_add_subview( view, meter );
        options_state->meter = meter;
    }
}

static void _cb( void * _context ) {
    zdj_soundcard_node_config_context_t * context = (zdj_soundcard_node_config_context_t*)_context;
    zdj_soundcard_options_state_t * state = (zdj_soundcard_options_state_t*)context->options_view_state;

    // Handle the selection of a new linked node.
    if ( context->remove_node_selection ) {
        printf( "_cb remove: %s -> %s\n",
            zdj_soundcard_node_name[ context->node->name ],
            zdj_soundcard_node_name[ context->remove_node_selection->name ]
        );
        zdj_soundcard_unlink_source_node_from_dest_node( 
            context->soundcard,
            context->node,
            context->remove_node_selection
        );
    } else if ( context->new_node_selection ) {
        printf( "_cb add/edit: %s -> %s\n",
            zdj_soundcard_node_name[ context->node->name ],
            zdj_soundcard_node_name[ context->new_node_selection->name ]
        );
        // If we launched the select_node view by tapping an existing node,
        // we need to remove the linkage to the original node before adding.
        if( context->node_selection_is_edit ) {
            zdj_soundcard_unlink_source_node_from_dest_node( 
                context->soundcard,
                context->node,
                zdj_soundcard_get_node_for_name( context->soundcard, context->edit_name )
            );
        }
        // Add new linkage.
        zdj_soundcard_link_source_node_to_dest_node( 
            context->soundcard,
            context->node, 
            context->new_node_selection 
        );
    }

    // Update the layout_update function since the signal type may have changed.
    state->update_layout = zdj_soundcard_options_get_update_layout_for_node( context->node );
    state->needs_layout_update = true;
}

static void _handle_gain( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_menu_item_view_state_t * stereo_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)stereo_state->data.ptr;
    printf( "handle gain event\n" );
    if( options_state->config_context &&
        options_state->config_context->node &&  
        options_state->config_context->node->dsp_dto
    ) {
        options_state->config_context->node->dsp_dto->adjust_gain( 
            options_state->config_context->node, _event->i_val 
        );
        options_state->needs_layout_update = true;
    }
}

static void _handle_pan( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_menu_item_view_state_t * stereo_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)stereo_state->data.ptr;
    printf( "handle pan event\n" );
}

static void _handle_eq_lo( zdj_view_t * view, zdj_control_event_t * _event ) {
    printf( "handle eq_lo event\n" );
    zdj_menu_item_view_state_t * stereo_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)stereo_state->data.ptr;
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)options_state->menu->state;
    zdj_soundcard_dsp_stage_dto_t * eq_stage = (zdj_soundcard_dsp_stage_dto_t*)zdj_soundcard_dto_get_dsp_stage_for_type( options_state->config_context->node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ );
    
    zdj_soundcard_dsp_process_knob_input( &eq_stage->knob_4, ZDJ_SOUNDCARD_DSP_KNOB_0_1_FAST, _event->i_val );
    options_state->needs_layout_update = true;
}

static void _handle_eq_hi( zdj_view_t * view, zdj_control_event_t * _event ) {
    printf( "handle eq_hi event\n" );
    zdj_menu_item_view_state_t * stereo_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)stereo_state->data.ptr;
    zdj_soundcard_dsp_stage_dto_t * eq_stage = (zdj_soundcard_dsp_stage_dto_t*)zdj_soundcard_dto_get_dsp_stage_for_type( options_state->config_context->node, ZDJ_SOUNDCARD_DSP_STAGE_TYPE_EQ );

    zdj_soundcard_dsp_process_knob_input( &eq_stage->knob_3, ZDJ_SOUNDCARD_DSP_KNOB_0_1_FAST, _event->i_val );
    options_state->needs_layout_update = true;
}

static void _handle_eq_model( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_menu_item_view_state_t * stereo_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)stereo_state->data.ptr;
    printf( "handle eq_model event\n" );
}

static void _handle_stereo( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_menu_item_view_state_t * stereo_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)stereo_state->data.ptr;
    // Toggle the node's stereo val and tell main screen to redraw with new vals
    zdj_soundcard_set_stereo_for_node( 
        options_state->config_context->node, 
        !stereo_state->data.b_val 
    );
    if( options_state->config_context->main_view_cb ) { 
        options_state->config_context->main_view_cb( options_state->config_context ); 
    }
    zdj_pop_subview_of( zdj_root_view( ), true );
}

static void _handle_mute( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_menu_item_view_state_t * stereo_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)stereo_state->data.ptr;
    // Toggle the node's stereo val and tell main screen to redraw with new vals
    zdj_soundcard_set_mute_for_node( 
        options_state->config_context->node, 
        !stereo_state->data.b_val 
    );
    options_state->needs_layout_update = true;
}

static void _handle_linkage( zdj_view_t * view, zdj_control_event_t * _event ) {
    printf( "_handle_linkage\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_soundcard_options_state_t * options_state = state->data.ptr;
    options_state->config_context->options_view_cb = _cb;
    // If we've tapped on an existing node, show the option to remove the link
    if( strcmp( state->title, "+ Add Output" ) ) {
        options_state->config_context->node_selection_is_edit = true;
        options_state->config_context->edit_name = state->data.i_val;
    } else {
        options_state->config_context->node_selection_is_edit = false;
    }
    zdj_view_t * select_node = zdj_new_soundcard_select_node( 
        options_state->config_context, state->data.i_val
    );
    zdj_push_subview( zdj_panel_view( ), select_node, true );
}
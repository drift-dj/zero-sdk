#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/controls/hmi/zdj_hmi.h>
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
#include <zerodj/ui/view/soundcard_view/zdj_soundcard_view.h>
#include <zerodj/ui/view/soundcard_view/options/zdj_soundcard_options.h>
#include <zerodj/ui/view/soundcard_view/meters/zdj_soundcard_meter.h>
#include <zerodj/ui/view/soundcard_view/select_node/zdj_soundcard_select_node.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _zdj_soundcard_options_port_output_handle_pad( zdj_view_t * view, void * _event );
static void _zdj_soundcard_options_port_output_handle_stereo( zdj_view_t * view, void * _event );
static void _zdj_soundcard_options_port_output_handle_mute( zdj_view_t * view, void * _event );
static void _zdj_soundcard_options_port_output_handle_linkage( zdj_view_t * view, void * _event );
static void _zdj_soundcard_options_port_output_cb( void * _context );

void zdj_soundcard_options_port_output_hmi( zdj_view_t * view, void * _event ) {
    zdj_hmi_event_t * e = (zdj_hmi_event_t *)_event;
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)view->state;
    zdj_view_t * menu_view = options_state->menu;
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)menu_view->state;
    zdj_soundcard_node_config_context_t * context = (zdj_soundcard_node_config_context_t*)options_state->config_context;

    // // Pick node to control based on type of node for this options page.
    // zdj_soundcard_node_t * node;
    // if( zdj_soundcard_node_name_is_output( context->node->name ) ) {
    //     node = zdj_soundcard_get_node_for_name( 
    //         context->soundcard, context->node->links->source_node 
    //     );
    // } else {
    //     node = options_state->config_context->node;
    // }

    // e->blocked = true;

    // // printf( "zdj_soundcard_options_port_output_hmi: %d\n", menu_state->scroll_index );
    // if( menu_state->scroll_index == options_state->menu_index_gain ) {
    //     node->gain += e->i_val * -2;
    //     if( node->gain > 255 ) { node->gain = 255; }
    //     if( node->gain < 0 ) { node->gain = 0; }
    //     options_state->needs_layout_update = true;
    // } else if( menu_state->scroll_index == options_state->menu_index_pan ) {
    //     node->pan += e->i_val * -1;
    //     if( node->pan > 127 ) { node->pan = 127; }
    //     if( node->pan < -127 ) { node->pan = -127; }
    //     options_state->needs_layout_update = true;
    // }
}

void zdj_soundcard_options_update_port_output_layout( zdj_view_t * view ) {
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)view->state;
    zdj_view_t * menu_view = options_state->menu;
    options_state->needs_layout_update = false;

    // For output ports, options point to a mix of source and dest link nodes
    // Ex. for analog out 0, show main lr bus settings for gain,
    // but analog out 0 settings for stereo + sig type.
    bool is_port_output = zdj_soundcard_node_name_is_output( 
        options_state->config_context->node->name 
    );
    zdj_soundcard_node_t * output_source_node = zdj_soundcard_get_node_for_name( 
        options_state->config_context->soundcard,
        options_state->config_context->node->links->source_node 
    );
    zdj_soundcard_node_t * port_node = options_state->config_context->node;

    zdj_menu_view_remove_all_items( menu_view );
    if( options_state->meter ) {
        zdj_remove_subview_of( view, options_state->meter );
    }

    zdj_menu_view_add_padding( menu_view, 1 );

    zdj_view_t * meter = zdj_soundcard_view_new_meter_for_node( 
        output_source_node, zdj_meter_label_for_node( output_source_node ), false 
    );
    if( meter ) { 
        // Since we're fudging a menu_item_view, we manually create the state data instance.
        zdj_soundcard_meter_state_t * meter_state = (zdj_soundcard_meter_state_t*)meter->state;
        meter_state->config_context = options_state->config_context;
        meter->frame->x = 3;
        meter->frame->y = 10;
        meter->frame->h = 47;
        zdj_add_bottom_subview_to( view, meter );
    }

    // Gain
    // zdj_view_t * gain = zdj_new_data_menu_item( 
    //     "Gain", 
    //     ZDJ_MENU_ITEM_LAYOUT_DATA_R,
    //     ZDJ_MENU_ITEM_DATA_TYPE_DOUBLE_1,
    //     NULL,
    //     " dB" 
    // );
    // zdj_menu_item_view_state_t * gain_state = (zdj_menu_item_view_state_t*)gain->state;
    // if( is_port_output ) { gain_state->data->f_val = zdj_calc_fader_db( output_source_node->gain ); }
    // else { gain_state->data->f_val = zdj_calc_fader_db( page_node->gain ); }
    // options_state->menu_index_gain = 0;
    // zdj_menu_view_add_item( menu_view, gain );
    
    // Show Pad for output port only 
    // if( is_port_output ) {
        zdj_view_t * sig_type = zdj_new_data_menu_item( 
            "Signal", 
            ZDJ_MENU_ITEM_LAYOUT_DATA_R,
            ZDJ_MENU_ITEM_DATA_TYPE_CHAR,
            NULL,
            NULL 
        );
        sig_type->handle_hmi_event = &_zdj_soundcard_options_port_output_handle_pad;
        zdj_menu_item_view_state_t * sig_type_state = (zdj_menu_item_view_state_t*)sig_type->state;
        sig_type_state->data->ptr = options_state;
        sig_type_state->data->c_val = zdj_soundcard_signal_name[ 
            zdj_soundcard_dto_get_sigtype_for_node_name( &zdj_soundcard->dto, output_source_node->name ) 
        ];
        options_state->menu_index_pad = 0;
        zdj_menu_view_add_item( menu_view, sig_type );
    // } else {
    //     options_state->menu_index_pad = -1;
    // }

    // Pan
    // if( !is_port_output && !page_node->stereo ) {
    //     zdj_view_t * pan = zdj_new_data_menu_item( 
    //         "Pan", 
    //         ZDJ_MENU_ITEM_LAYOUT_DATA_R,
    //         ZDJ_MENU_ITEM_DATA_TYPE_INT,
    //         NULL,
    //         NULL
    //     );
    //     zdj_menu_item_view_state_t * pan_state = (zdj_menu_item_view_state_t*)pan->state;
    //     pan_state->data->i_val = page_node->pan;
    //     if( options_state->menu_index_pad != -1 ){ 
    //         options_state->menu_index_pan = options_state->menu_index_pad + 1;; 
    //     } else { 
    //         options_state->menu_index_pan = options_state->menu_index_gain + 1;
    //     }
    //     zdj_menu_view_add_item( menu_view, pan );
    // } else {
    //     options_state->menu_index_pan = -1;
    // }
    
    // Stereo
    // zdj_view_t * stereo = zdj_new_menu_item( "Stereo", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    // stereo->handle_hmi_event = &_zdj_soundcard_options_port_output_handle_stereo;
    // zdj_menu_item_view_state_t * stereo_state = (zdj_menu_item_view_state_t*)stereo->state;
    // stereo_state->data->b_val = page_node->stereo;
    // stereo_state->data->ptr = options_state; // Ref to options view state to force update_needed on click
    // if( options_state->menu_index_pan != -1 ){ 
    //     options_state->menu_index_stereo = options_state->menu_index_pan + 1;; 
    // } else if( options_state->menu_index_pad != -1 ){ 
    //     options_state->menu_index_stereo = options_state->menu_index_pad + 1;
    // } else {
    //     options_state->menu_index_stereo = options_state->menu_index_gain + 1;
    // }
    // zdj_menu_view_add_item( menu_view, stereo );

    // Mute
    zdj_view_t * mute = zdj_new_menu_item( "Mute", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    mute->handle_hmi_event = &_zdj_soundcard_options_port_output_handle_mute;
    zdj_menu_item_view_state_t * mute_state = (zdj_menu_item_view_state_t*)mute->state;
    mute_state->data->b_val = output_source_node->mute;
    mute_state->data->ptr = options_state; // Ref to options view state to force update_needed on click
    options_state->menu_index_mute = 1;
    zdj_menu_view_add_item( menu_view, mute );

    // Inputs section
    zdj_menu_view_add_section( menu_view, zdj_new_menu_section( "Input" ) );
    // If there is an input, show it, otherwise show add input button
    if( (zdj_soundcard_count_input_nodes_to_node_name( &zdj_soundcard->dto, port_node->name ) > 0) ) {
        zdj_view_t * input = zdj_new_menu_item( 
            zdj_soundcard_node_name[ output_source_node->name ], 
            ZDJ_MENU_ITEM_LAYOUT_BASIC_R 
        );
        input->handle_hmi_event = &_zdj_soundcard_options_port_output_handle_linkage;
        zdj_menu_item_view_state_t * input_state = (zdj_menu_item_view_state_t*)input->state;
        input_state->data->ptr = options_state;
        zdj_menu_view_add_item( menu_view, input );
    } else {
        // Add Input
        zdj_view_t * add_input = zdj_new_menu_item( "+ Add Input", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
        zdj_menu_view_add_item( menu_view, add_input );
    }

}

void _zdj_soundcard_options_port_output_cb( void * _context ) {
    printf( "_zdj_soundcard_options_port_output_cb\n" );
    zdj_soundcard_node_config_context_t * context = (zdj_soundcard_node_config_context_t*)_context;

    zdj_soundcard_options_state_t * state = (zdj_soundcard_options_state_t*)context->options_view_state;
    // Handle the selection of a new linked node.
    // If we're an output node, we have to edit the link_map/linkage of the source node.
    if( zdj_soundcard_node_name_is_output( context->node->name ) ) {
        // Adding a new link

        // Only allow 1 input node to output ports
        zdj_soundcard_unlink_all_nodes_from_node( context->soundcard, context->node );

        // The node we selected from the menu will be in new_node_selection
        zdj_soundcard_link_source_node_to_dest_node( 
            context->soundcard,
            context->new_node_selection, 
            context->node 
        );

        // Because output links are build from other nodes, we need to refresh the output node.
        zdj_soundcard_pull_node_links_from_dto( context->soundcard, context->node );
    } else {
        if( context->remove_node_selection ) {
            // Removing an existing link
            printf( "removing link: %s\n", zdj_soundcard_node_name[ context->remove_node_selection->name ] );
            zdj_soundcard_unlink_source_node_from_dest_node( 
                context->soundcard,
                context->node,
                context->remove_node_selection
            );

        } else if ( context->new_node_selection ) {
            // Adding a new link
            printf( "adding link: %s\n", zdj_soundcard_node_name[ context->new_node_selection->name ] );
            // If we're an input node, we can just edit our own linkage
            zdj_soundcard_link_source_node_to_dest_node( 
                context->soundcard,
                context->node, 
                context->new_node_selection 
            );
        }
    }

    // Update the layout_update function since the signal type may have changed.
    state->update_layout = zdj_soundcard_options_get_update_layout_for_node( context->node );
    state->needs_layout_update = true;
}

void _zdj_soundcard_options_port_output_handle_pad( zdj_view_t * view, void * _event ) {
    zdj_menu_item_view_state_t * pad_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)pad_state->data->ptr;
    // Cycle thru the pad options based on node
    zdj_soundcard_cycle_pad_for_io_node( 
        options_state->config_context->soundcard, options_state->config_context->node 
    );
    options_state->needs_layout_update = true;
}

void _zdj_soundcard_options_port_output_handle_stereo( zdj_view_t * view, void * _event ) {
    zdj_menu_item_view_state_t * stereo_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)stereo_state->data->ptr;
    // Toggle the node's stereo val and tell main screen to redraw with new vals
    zdj_soundcard_set_stereo_for_node( 
        options_state->config_context->node, 
        !stereo_state->data->b_val 
    );
    if( options_state->config_context->main_view_cb ) { 
        options_state->config_context->main_view_cb( options_state->config_context ); 
    }
    zdj_pop_subview_of( zdj_root_view( ), true );
}

void _zdj_soundcard_options_port_output_handle_mute( zdj_view_t * view, void * _event ) {
    zdj_menu_item_view_state_t * stereo_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)stereo_state->data->ptr;
    // Toggle the node's stereo val and tell main screen to redraw with new vals
    zdj_soundcard_set_mute_for_node( 
        options_state->config_context->node, 
        !stereo_state->data->b_val 
    );
    options_state->needs_layout_update = true;
}

void _zdj_soundcard_options_port_output_handle_linkage( zdj_view_t * view, void * _event ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_soundcard_options_state_t * options_state = state->data->ptr;
    options_state->config_context->options_view_cb = _zdj_soundcard_options_port_output_cb;
    // If we've tapped on an existing node, show the option to remove the link
    bool is_edit = strcmp( state->title, "+ Add Output" );
    zdj_view_t * select_node = zdj_new_soundcard_select_node( 
        options_state->config_context, is_edit, state->data->i_val
    );
    zdj_push_subview( zdj_root_view( ), select_node, true );
}
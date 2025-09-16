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
#include <zerodj/ui/view/scope_view/zdj_scope_view.h>
#include <zerodj/ui/view/soundcard_view/zdj_soundcard_view.h>
#include <zerodj/ui/view/soundcard_view/options/zdj_soundcard_options.h>
#include <zerodj/ui/view/soundcard_view/meters/zdj_soundcard_meter.h>
#include <zerodj/ui/view/soundcard_view/select_node/zdj_soundcard_select_node.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _handle_scope( zdj_view_t * view, zdj_control_event_t * event );
static void _handle_linkage( zdj_view_t * view, zdj_control_event_t * _vent );
static void _options_cb( void * _context );

void zdj_soundcard_options_record_bus_event( zdj_view_t * view, zdj_control_event_t * event ) {
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

void zdj_soundcard_options_update_record_bus_layout( zdj_view_t * view ) {
    // printf( "zdj_soundcard_options_update_record_bus_layout\n" );

    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)view->state;
    zdj_view_t * menu_view = options_state->menu;
    options_state->needs_layout_update = false;

    zdj_soundcard_node_t * page_node = options_state->config_context->node;

    zdj_soundcard_node_t * source_node = zdj_soundcard_get_node_for_name( 
        options_state->config_context->soundcard,
        ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS
    );
    // zdj_soundcard_node_t * port_node = options_state->config_context->node;

    zdj_menu_view_remove_all_items( menu_view );
    if( options_state->meter ) {
        zdj_remove_subview_of( view, options_state->meter );
    }

    zdj_menu_view_add_padding( menu_view, 1 );

    zdj_view_t * meter = zdj_soundcard_view_new_meter_for_node( 
        source_node, zdj_meter_label_for_node( source_node ), false, false 
    );
    if( meter ) { 
        // Since we're fudging a menu_item_view, we manually create the state data instance.
        zdj_soundcard_meter_state_t * meter_state = (zdj_soundcard_meter_state_t*)meter->state;
        meter_state->config_context = options_state->config_context;
        meter->frame.x = 3;
        meter->frame.y = 10;
        meter->frame.h = 47;
        zdj_add_bottom_subview_to( view, meter );
    }



    // Settings section
    zdj_menu_view_add_section( menu_view, zdj_new_menu_section( "Settings" ) );

    // Gain
    zdj_view_t * gain = zdj_new_data_menu_item( 
        "Gain", 
        ZDJ_MENU_ITEM_LAYOUT_DATA_R,
        ZDJ_MENU_ITEM_DATA_TYPE_DOUBLE_1,
        NULL,
        " dB" 
    );
    zdj_menu_item_view_state_t * gain_state = (zdj_menu_item_view_state_t*)gain->state;
    gain_state->data->f_val = zdj_calc_fader_db( source_node->gain );
    options_state->menu_index_gain = 0;
    zdj_menu_view_add_item( menu_view, gain );

    // Scope View
    zdj_view_t * scope = zdj_new_menu_item( "Scope", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
    scope->handle_control_event = &_handle_scope;
    zdj_menu_item_view_state_t * scope_state = (zdj_menu_item_view_state_t*)scope->state;
    scope_state->data->ptr = options_state; // Ref to options view state to force update_needed on click
    options_state->menu_index_scope = options_state->menu_index_stereo + 1;;
    zdj_menu_view_add_item( menu_view, scope );



    // Inputs section
    zdj_menu_view_add_section( menu_view, zdj_new_menu_section( "Input" ) );

    int in_node_count = zdj_soundcard_count_input_nodes_to_node_name( &zdj_soundcard->dto, page_node->name );
    if( in_node_count ) {
        for( int i=0; i<in_node_count; i++ ) {
            // If we're linked to a stereo io port, adjust name to show "out 1/2"
            // instead of just "out 1"

            char adjusted_name[ 64 ];
            if( zdj_soundcard_node_name_is_analog_input( page_node->input_links[ i ].source_node ) ) {
                zdj_soundcard_get_port_title_with_stereo( 
                    options_state->config_context->soundcard,
                    page_node->input_links[ i ].source_node, 
                    adjusted_name 
                );
            } else {
                strcpy( adjusted_name, zdj_soundcard_node_name[ page_node->input_links[ i ].source_node ] );
            }

            zdj_view_t * input = zdj_new_menu_item( 
                adjusted_name, 
                ZDJ_MENU_ITEM_LAYOUT_BASIC_R 
            );
            input->handle_control_event = &_handle_linkage;
            zdj_menu_item_view_state_t * input_state = (zdj_menu_item_view_state_t*)input->state;
            input_state->data->ptr = options_state;
            input_state->data->i_val = page_node->input_links[ i ].source_node;
            zdj_menu_view_add_item( menu_view, input );
        }
    }

    // Add Input
    zdj_view_t * add_input = zdj_new_menu_item( "+ Add Input", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
    add_input->handle_control_event = &_handle_linkage;
    zdj_menu_item_view_state_t * add_input_state = (zdj_menu_item_view_state_t*)add_input->state;
    add_input_state->data->ptr = options_state;
    zdj_menu_view_add_item( menu_view, add_input );
}

void _options_cb( void * _context ) {
    zdj_soundcard_node_config_context_t * context = (zdj_soundcard_node_config_context_t*)_context;
    zdj_soundcard_options_state_t * state = (zdj_soundcard_options_state_t*)context->options_view_state;
    // Handle the selection of a new linked node.
    if( context->remove_node_selection ) {
        // Removing an existing link
        printf( "_options_cb remove select: %s -> %s\n",
            zdj_soundcard_node_name[ context->remove_node_selection->name ],
            zdj_soundcard_node_name[ context->node->name ] 
        );
        zdj_soundcard_unlink_source_node_from_dest_node( 
            context->soundcard,
            context->remove_node_selection,
            context->node
        );
    } else if ( context->new_node_selection ) {
        printf( "_options_cb add/edit select: %s -> %s\n",
            zdj_soundcard_node_name[ context->new_node_selection->name ],
            zdj_soundcard_node_name[ context->node->name ] 
        );
        // If we launched the select_node view by tapping an existing node,
        // we need to remove the linkage to the original node before adding.
        if( context->node_selection_is_edit ) {
            zdj_soundcard_unlink_source_node_from_dest_node( 
                context->soundcard,
                zdj_soundcard_get_node_for_name( context->soundcard, context->edit_name ),
                context->node
            );
            printf( "Uninking %s -> %s\n",
                zdj_soundcard_node_name[ context->edit_name ],
                zdj_soundcard_node_name[ context->node->name ]
            );
        }
        // Add new linkage
        zdj_soundcard_link_source_node_to_dest_node( 
            context->soundcard,
            context->new_node_selection,
            context->node
        );
        printf( "Linking %s -> %s\n",
            zdj_soundcard_node_name[ context->new_node_selection->name ],
            zdj_soundcard_node_name[ context->node->name ]
        );
    }

    // Set the signal type for the output port based on the node selected.
    if( zdj_soundcard_node_name_is_audio( context->node->input_links->source_node ) ) {
        context->node->signal_type = ZDJ_SOUNDCARD_SIGNAL_CON_0_DBV;
    } else if( zdj_soundcard_node_name_is_clock( context->node->input_links->source_node ) ) {
        context->node->signal_type = ZDJ_SOUNDCARD_SIGNAL_CLOCK_PPQN;
    } else if( zdj_soundcard_node_name_is_cv( context->node->input_links->source_node ) ) {
        context->node->signal_type = ZDJ_SOUNDCARD_SIGNAL_CV_BIPOLAR;
    } else if( zdj_soundcard_node_name_is_midi( context->node->input_links->source_node ) ) {
        context->node->signal_type = ZDJ_SOUNDCARD_SIGNAL_USB_MIDI;
    }

    // Update the layout_update function since the signal type may have changed.
    state->update_layout = zdj_soundcard_options_get_update_layout_for_node( context->node );
    state->needs_layout_update = true;
}

static void _handle_scope( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)item_state->data->ptr;

    zdj_view_t * scope_view = zdj_new_scope_view( 
        options_state->config_context->soundcard,
        options_state->config_context->node->name 
    );
    zdj_push_subview( zdj_root_view( ), scope_view, true );
}

static void _handle_linkage( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_soundcard_options_state_t * options_state = state->data->ptr;
    options_state->config_context->options_view_cb = _options_cb;
    // If we've tapped on an existing node, show the option to remove the link
    if( strcmp( state->title, "+ Add Input" ) ) {
        options_state->config_context->node_selection_is_edit = true;
        options_state->config_context->edit_name = state->data->i_val;
    } else {
        options_state->config_context->node_selection_is_edit = false;
    }
    // options_state->config_context->node_selection_is_edit = strcmp( state->title, "+ Add Input" );
    zdj_view_t * select_node = zdj_new_soundcard_select_node( 
        options_state->config_context, state->data->i_val
    );
    zdj_push_subview( zdj_root_view( ), select_node, true );
}
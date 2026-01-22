#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/signal/deck/zdj_deck_manager.h>
#include <zerodj/signal/deck/xport/zdj_deck_xport.h>
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

static void _handle_direction( zdj_view_t * view, zdj_control_event_t * event );
static void _handle_status( zdj_view_t * view, zdj_control_event_t * event );
static void _handle_sync( zdj_view_t * view, zdj_control_event_t * event );
static void _handle_ppqn( zdj_view_t * view, zdj_control_event_t * event );
static void _handle_bpm( zdj_view_t * view, zdj_control_event_t * event );
static void _handle_linkage( zdj_view_t * view, zdj_control_event_t * _event );
static void _cb( void * _context );



void zdj_soundcard_options_update_clock_layout( zdj_view_t * view ) {
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)view->state;
    zdj_view_t * menu_view = options_state->menu;
    zdj_deck_t * clock_deck = zdj_soundcard->clock_deck;
    zdj_xport_deck_state_t * deck_state = (zdj_xport_deck_state_t*)clock_deck->state;

    options_state->needs_layout_update = false;

    // For output ports, options point to a mix of source and dest link nodes
    // Ex. for analog out 0, show main lr bus settings for gain,
    // but analog out 0 settings for stereo + sig type.
    bool is_output_port = zdj_soundcard_node_name_is_output( 
        options_state->config_context->node->name 
    );
    zdj_soundcard_node_t * output_source_node = zdj_soundcard_get_node_for_name( 
        options_state->config_context->soundcard,
        options_state->config_context->node->output_links->source_node 
    );
    zdj_soundcard_node_t * page_node = options_state->config_context->node;

    zdj_menu_view_remove_all_items( menu_view );
    if( options_state->meter ) {
        zdj_remove_subview_of( view, options_state->meter );
    }

    zdj_menu_view_add_padding( menu_view, 1 );

    // Lead/Follow
    zdj_soundcard_clock_direction_t clock_direction = zdj_soundcard_dto_get_source_for_node_name( 
        &zdj_soundcard->dto, ZDJ_SOUNDCARD_NODE_NAME_XPORT_0 
    );

    zdj_view_t * direction = zdj_new_data_menu_item( 
        "Mode", 
        ZDJ_MENU_ITEM_LAYOUT_DATA_R,
        ZDJ_MENU_ITEM_DATA_TYPE_CHAR,
        NULL,
        NULL 
    );
    direction->handle_control_event = &_handle_direction;
    zdj_menu_item_view_state_t * direction_state = (zdj_menu_item_view_state_t*)direction->state;
    direction_state->data.ptr = options_state;
    if( clock_direction == ZDJ_SOUNDCARD_CLOCK_DIRECTION_OUTPUT ) {
        strcpy( direction_state->data.c_val, "Lead" );
    } else {
        strcpy( direction_state->data.c_val, "Follow" );
    }
    zdj_menu_view_add_item( menu_view, direction );

    // Status
    if( clock_direction == ZDJ_SOUNDCARD_CLOCK_DIRECTION_OUTPUT ) {
        // Output clock lets user start/stop from options panel
        zdj_view_t * status = zdj_new_data_menu_item( 
            "Status", 
            ZDJ_MENU_ITEM_LAYOUT_DATA_R,
            ZDJ_MENU_ITEM_DATA_TYPE_CHAR,
            NULL,
            NULL 
        );
        status->handle_control_event = &_handle_status;
        zdj_menu_item_view_state_t * status_state = (zdj_menu_item_view_state_t*)status->state;
        status_state->data.ptr = options_state;
        if( clock_deck->controls.platter.motor.enabled ) {
            strcpy( status_state->data.c_val, "Running" );
        } else {
            strcpy( status_state->data.c_val, "Stopped" );
        }
        zdj_menu_view_add_item( menu_view, status );
    } else {
        // Input clock only shows status - no selection
        // zdj_view_t * status = zdj_new_data_menu_item( 
        //     "Status", 
        //     ZDJ_MENU_ITEM_LAYOUT_DATA_R,
        //     ZDJ_MENU_ITEM_DATA_TYPE_CHAR,
        //     NULL,
        //     NULL 
        // );
        // status->handle_control_event = &_handle_status;
        // zdj_menu_item_view_state_t * status_state = (zdj_menu_item_view_state_t*)status->state;
        // status_state->data.ptr = options_state;
        // if( clock_is_output ) {
        //     strcpy( status_state->data.c_val, "Stopped" );
        // } else {
        //     strcpy( status_state->data.c_val, "Running" );
        // }
        // zdj_menu_view_add_item( menu_view, status );
    }

    // Sync
    zdj_view_t * sync = zdj_new_data_menu_item( 
        "Sync", 
        ZDJ_MENU_ITEM_LAYOUT_DATA_R,
        ZDJ_MENU_ITEM_DATA_TYPE_CHAR,
        NULL,
        NULL 
    );
    sync->handle_control_event = &_handle_sync;
    zdj_menu_item_view_state_t * sync_state = (zdj_menu_item_view_state_t*)sync->state;
    sync_state->data.ptr = options_state;
    switch( deck_state->sync_mode ) {
        case ZDJ_XPORT_DECK_SYNC_MODE_NORMAL:
            strcpy( sync_state->data.c_val, "Normal" );
            break;
        case ZDJ_XPORT_DECK_SYNC_MODE_HALF:
            strcpy( sync_state->data.c_val, "Half" );
            break;
        case ZDJ_XPORT_DECK_SYNC_MODE_DOUBLE:
            strcpy( sync_state->data.c_val, "Double" );
            break;
        case ZDJ_XPORT_DECK_SYNC_MODE_OFF:
            strcpy( sync_state->data.c_val, "Decoupled" );
            break;
    }
    zdj_menu_view_add_item( menu_view, sync );
    
    // PPQN
    zdj_view_t * ppqn = zdj_new_data_menu_item( 
        "PPQN", 
        ZDJ_MENU_ITEM_LAYOUT_DATA_R,
        ZDJ_MENU_ITEM_DATA_TYPE_INT,
        NULL,
        NULL 
    );
    ppqn->handle_control_event = &_handle_ppqn;
    zdj_menu_item_view_state_t * ppqn_state = (zdj_menu_item_view_state_t*)ppqn->state;
    ppqn_state->data.ptr = options_state;
    ppqn_state->data.i_val = deck_state->ppqn;
    zdj_menu_view_add_item( menu_view, ppqn );
    
    // BPM - Only show/enable control if clock is output and sync-decoupled
    if( clock_direction == ZDJ_SOUNDCARD_CLOCK_DIRECTION_OUTPUT && 
        deck_state->sync_mode == ZDJ_XPORT_DECK_SYNC_MODE_OFF 
    ) {
        zdj_view_t * bpm = zdj_new_data_menu_item( 
            "BPM", 
            ZDJ_MENU_ITEM_LAYOUT_DATA_R,
            ZDJ_MENU_ITEM_DATA_TYPE_DOUBLE_2,
            NULL,
            NULL 
        );
        bpm->handle_control_event = &_handle_bpm;
        zdj_menu_item_view_state_t * bpm_state = (zdj_menu_item_view_state_t*)bpm->state;
        bpm_state->data.ptr = options_state;
        bpm_state->data.f_val = deck_state->set_bpm;
        bpm_state->captures_all_events = true;
        zdj_menu_view_add_item( menu_view, bpm );
    }


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

    // Add Meter
    zdj_view_t * meter = zdj_soundcard_view_new_meter_for_node( 
        page_node, zdj_meter_label_for_node( page_node ), false, false
    );
    if( meter ) { 
        // Since we're fudging a menu_item_view, we manually create the state data instance.
        zdj_soundcard_meter_state_t * meter_state = (zdj_soundcard_meter_state_t*)meter->state;
        meter_state->config_context = options_state->config_context;
        meter->frame.x = 3;
        meter->frame.y = 10;
        meter->frame.h = 47;
        zdj_add_subview( view, meter );
    }
}

static void _cb( void * _context ) {
    zdj_soundcard_node_config_context_t * context = (zdj_soundcard_node_config_context_t*)_context;
    zdj_soundcard_options_state_t * state = (zdj_soundcard_options_state_t*)context->options_view_state;

    // Handle the selection of a new linked node.
    if ( context->remove_node_selection ) {
        printf( "_zdj_soundcard_options_clock_cb remove: %s -> %s\n",
            zdj_soundcard_node_name[ context->node->name ],
            zdj_soundcard_node_name[ context->remove_node_selection->name ]
        );
        zdj_soundcard_unlink_source_node_from_dest_node( 
            context->soundcard,
            context->node,
            context->remove_node_selection
        );
    } else if ( context->new_node_selection ) {
        printf( "_zdj_soundcard_options_clock_cb add/edit: %s -> %s\n",
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

static void _handle_ppqn( zdj_view_t * view, zdj_control_event_t * event ) {
    printf( "_handle_ppqn\n" );

    zdj_menu_item_view_state_t * view_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)view_state->data.ptr;

    // Build a ref to the xport deck
    zdj_deck_t * xport_deck = zdj_deck_manager_get_deck_for_station( ZDJ_DECK_STATION_XPORT );
    zdj_xport_deck_state_t * xport_state = (zdj_xport_deck_state_t*)xport_deck->state;

    switch( zdj_soundcard_dto_get_sigtype_for_node_name( &zdj_soundcard->dto, ZDJ_SOUNDCARD_NODE_NAME_XPORT_0 ) 
    ) {
        case ZDJ_SOUNDCARD_SIGNAL_XPORT_ANALOG_PPQN_1:
            xport_state->ppqn = 2;
            zdj_soundcard_dto_set_sigtype_for_node_name(
                &zdj_soundcard->dto,
                ZDJ_SOUNDCARD_NODE_NAME_XPORT_0,
                ZDJ_SOUNDCARD_SIGNAL_XPORT_ANALOG_PPQN_2
            );
            break;
        case ZDJ_SOUNDCARD_SIGNAL_XPORT_ANALOG_PPQN_2:
            xport_state->ppqn = 4;
            zdj_soundcard_dto_set_sigtype_for_node_name(
                &zdj_soundcard->dto,
                ZDJ_SOUNDCARD_NODE_NAME_XPORT_0,
                ZDJ_SOUNDCARD_SIGNAL_XPORT_ANALOG_PPQN_4
            );
            break;
        case ZDJ_SOUNDCARD_SIGNAL_XPORT_ANALOG_PPQN_4:
            xport_state->ppqn = 24;
            zdj_soundcard_dto_set_sigtype_for_node_name(
                &zdj_soundcard->dto,
                ZDJ_SOUNDCARD_NODE_NAME_XPORT_0,
                ZDJ_SOUNDCARD_SIGNAL_XPORT_ANALOG_PPQN_24
            );
            break;
        case ZDJ_SOUNDCARD_SIGNAL_XPORT_ANALOG_PPQN_24:
            xport_state->ppqn = 96;
            zdj_soundcard_dto_set_sigtype_for_node_name(
                &zdj_soundcard->dto,
                ZDJ_SOUNDCARD_NODE_NAME_XPORT_0,
                ZDJ_SOUNDCARD_SIGNAL_XPORT_ANALOG_PPQN_96
            );
            break;
        case ZDJ_SOUNDCARD_SIGNAL_XPORT_ANALOG_PPQN_96:
            xport_state->ppqn = 1;
            zdj_soundcard_dto_set_sigtype_for_node_name(
                &zdj_soundcard->dto,
                ZDJ_SOUNDCARD_NODE_NAME_XPORT_0,
                ZDJ_SOUNDCARD_SIGNAL_XPORT_ANALOG_PPQN_1
            );
            break;
    }
    options_state->needs_layout_update = true;
}

static void _handle_direction( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_menu_item_view_state_t * view_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)view_state->data.ptr;

    // Build a ref to the xport deck
    zdj_deck_t * xport_deck = zdj_deck_manager_get_deck_for_station( ZDJ_DECK_STATION_XPORT );
    zdj_xport_deck_state_t * xport_state = (zdj_xport_deck_state_t*)xport_deck->state;

    switch( zdj_soundcard_dto_get_source_for_node_name( 
            &zdj_soundcard->dto, ZDJ_SOUNDCARD_NODE_NAME_XPORT_0 
        ) 
    ) {
        case ZDJ_SOUNDCARD_CLOCK_DIRECTION_INPUT:
            xport_state->direction = ZDJ_XPORT_DECK_DIRECTION_OUTPUT;
            zdj_soundcard_dto_set_source_for_node_name( 
                &zdj_soundcard->dto, 
                ZDJ_SOUNDCARD_NODE_NAME_XPORT_0,
                ZDJ_SOUNDCARD_CLOCK_DIRECTION_OUTPUT
            );
            break;
        case ZDJ_SOUNDCARD_CLOCK_DIRECTION_OUTPUT:
            xport_state->direction = ZDJ_XPORT_DECK_DIRECTION_INPUT;
            zdj_soundcard_dto_set_source_for_node_name( 
                &zdj_soundcard->dto, 
                ZDJ_SOUNDCARD_NODE_NAME_XPORT_0,
                ZDJ_SOUNDCARD_CLOCK_DIRECTION_INPUT
            );
            break;
    }

    // printf( "clock dir: %d\n", zdj_soundcard_dto_get_source_for_node_name( 
    //         &zdj_soundcard->dto, ZDJ_SOUNDCARD_NODE_NAME_XPORT_0 
    //     ) );
    options_state->needs_layout_update = true;
}

static void _handle_sync( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_menu_item_view_state_t * view_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)view_state->data.ptr;

    // Build a ref to the xport deck
    zdj_deck_t * xport_deck = zdj_deck_manager_get_deck_for_station( ZDJ_DECK_STATION_XPORT );
    zdj_xport_deck_state_t * xport_state = (zdj_xport_deck_state_t*)xport_deck->state;


    switch( zdj_soundcard_dto_get_sync_for_node_name( 
            &zdj_soundcard->dto, ZDJ_SOUNDCARD_NODE_NAME_XPORT_0
        ) 
    ) {
        case ZDJ_SOUNDCARD_CLOCK_SYNC_NORMAL:
            xport_state->sync_mode = ZDJ_XPORT_DECK_SYNC_MODE_HALF;
            zdj_soundcard_dto_set_sync_for_node_name( 
                &zdj_soundcard->dto, 
                ZDJ_SOUNDCARD_NODE_NAME_XPORT_0, 
                ZDJ_SOUNDCARD_CLOCK_SYNC_HALF
            );
            break;
        case ZDJ_SOUNDCARD_CLOCK_SYNC_HALF:
            xport_state->sync_mode = ZDJ_XPORT_DECK_SYNC_MODE_DOUBLE;
            zdj_soundcard_dto_set_sync_for_node_name( 
                &zdj_soundcard->dto, 
                ZDJ_SOUNDCARD_NODE_NAME_XPORT_0, 
                ZDJ_SOUNDCARD_CLOCK_SYNC_DOUBLE
            );
            break;
        case ZDJ_SOUNDCARD_CLOCK_SYNC_DOUBLE:
            xport_state->sync_mode = ZDJ_XPORT_DECK_SYNC_MODE_OFF;
            zdj_soundcard_dto_set_sync_for_node_name( 
                &zdj_soundcard->dto, 
                ZDJ_SOUNDCARD_NODE_NAME_XPORT_0, 
                ZDJ_SOUNDCARD_CLOCK_SYNC_DECOUPLE
            );
            break;
        case ZDJ_SOUNDCARD_CLOCK_SYNC_DECOUPLE:
            xport_state->sync_mode = ZDJ_XPORT_DECK_SYNC_MODE_NORMAL;
            zdj_soundcard_dto_set_sync_for_node_name( 
                &zdj_soundcard->dto, 
                ZDJ_SOUNDCARD_NODE_NAME_XPORT_0, 
                ZDJ_SOUNDCARD_CLOCK_SYNC_NORMAL
            );
            break;
    }
    options_state->needs_layout_update = true;
}

static void _handle_status( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_menu_item_view_state_t * view_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)view_state->data.ptr;

    // Build a ref to the xport deck
    zdj_deck_t * xport_deck = zdj_deck_manager_get_deck_for_station( ZDJ_DECK_STATION_XPORT );
    zdj_xport_deck_state_t * xport_state = (zdj_xport_deck_state_t*)xport_deck->state;

    xport_deck->controls.platter.motor.enabled = !xport_deck->controls.platter.motor.enabled;
    options_state->needs_layout_update = true;
}

static void _handle_bpm( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_menu_item_view_state_t * view_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_soundcard_options_state_t * options_state = (zdj_soundcard_options_state_t*)view_state->data.ptr;

    // Build a ref to the xport deck
    zdj_deck_t * xport_deck = zdj_deck_manager_get_deck_for_station( ZDJ_DECK_STATION_XPORT );
    zdj_xport_deck_state_t * xport_state = (zdj_xport_deck_state_t*)xport_deck->state;

    options_state->needs_layout_update = true;
}

static void _handle_linkage( zdj_view_t * view, zdj_control_event_t * _event ) {
    printf( "_zdj_soundcard_options_clock_handle_linkage\n" );
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
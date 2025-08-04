#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>


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
#include <zerodj/ui/view/soundcard_view/select_node/zdj_soundcard_select_node.h>
#include <zerodj/ui/view/soundcard_view/meters/zdj_soundcard_meter.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _zdj_soundcard_select_node_draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_soundcard_select_node_handle_control( zdj_view_t * view, zdj_control_event_t * _event );
static void _zdj_soundcard_select_node_deinit_state( zdj_view_t * view );
static void _zdj_soundcard_select_node_handle_back( zdj_view_t * menu_view );

static zdj_error_type_t _zdj_soundcard_add_select_menu_item( 
    zdj_view_t * menu,
    zdj_soundcard_node_config_context_t * context,
    zdj_soundcard_node_name_t name
);
static void _zdj_soundcard_select_node_handle_select( zdj_view_t * view, zdj_control_event_t * _event );
static void _zdj_soundcard_select_node_handle_remove( zdj_view_t * view, zdj_control_event_t * _event );

zdj_view_t * zdj_new_soundcard_select_node( 
    zdj_soundcard_node_config_context_t * context,
    zdj_soundcard_node_name_t edit_node_name
) {
    zdj_view_t * soundcard_select_node = zdj_new_modal_view( zdj_modal_rect( ) );
    soundcard_select_node->draw = &_zdj_soundcard_select_node_draw;
    soundcard_select_node->deinit_state = &_zdj_soundcard_select_node_deinit_state;

    // Make menu
    zdj_view_t * menu = zdj_new_menu_view( ZDJ_VERTICAL, zdj_modal_rect( ) );
    zdj_add_subview( soundcard_select_node, menu );
    menu->frame->x = 0;
    menu->frame->y = 0;
    menu->frame->w = ZDJ_MODAL_WIDTH;
    menu->frame->h = ZDJ_MODAL_HEIGHT;
    
    // Set up header
    zdj_view_t * menu_header = zdj_new_menu_header( 
        "Select Linkage",
        zdj_soundcard_node_name[ context->node->name ],
        ZDJ_MENU_HEADER_STYLE_NORMAL,
        ZDJ_MENU_HEADER_BACK_STYLE_BACK
    );
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_header->state;
    header_state->handle_back = &_zdj_soundcard_select_node_handle_back;
    zdj_menu_view_add_header( menu, menu_header );

    // For non-output nodes, add remove section w/ selected node
    if( context->node_selection_is_edit ) {
        zdj_menu_view_add_padding( menu, 3 );

        // If we're linked to a stereo io port, adjust name to show "out 1/2"
        // instead of just "out 1"
        char adjusted_name[ 64 ];
        if( zdj_soundcard_node_name_is_analog_input( edit_node_name ) ||
            zdj_soundcard_node_name_is_analog_output( edit_node_name ) 
        ) {
            zdj_soundcard_get_port_title_with_stereo( 
                context->soundcard,
                edit_node_name, 
                adjusted_name 
            );
        } else {
            strcpy( adjusted_name, zdj_soundcard_node_name[ edit_node_name ] );
        }

        // Incorporate port name into 'remove' item title
        char remove_str[ 128 ];
        snprintf( remove_str, sizeof( remove_str ), "Remove %s", 
            adjusted_name
        );
        zdj_view_t * item = zdj_new_menu_item( 
            remove_str, 
            ZDJ_MENU_ITEM_LAYOUT_BASIC_R 
        );
        item->handle_control_event = &_zdj_soundcard_select_node_handle_remove;
        zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;
        item_state->data->ptr = context; // Store config context for handling selection
        item_state->data->i_val = edit_node_name; // Store this menu item's node name str
        zdj_menu_view_add_item( menu, item );

        zdj_menu_view_add_padding( menu, 2 );
    }

    // Add linkage options
    if( zdj_soundcard_node_name_is_input( context->node->name ) ) {
        zdj_soundcard_build_select_node_input_menu( menu, context );
    } else if( zdj_soundcard_node_name_is_output( context->node->name ) ) {
        zdj_soundcard_build_select_node_output_menu( menu, context );
    } else if( zdj_soundcard_node_name_is_internal_bus( context->node->name ) ) {
        zdj_soundcard_build_select_node_internal_bus_menu( menu, context );
    } else if( zdj_soundcard_node_name_is_aux_bus( context->node->name ) ) {
        zdj_soundcard_build_select_node_aux_bus_menu( menu, context );
    } else if( zdj_soundcard_node_name_is_clock( context->node->name ) ) {
        zdj_soundcard_build_select_node_clock_menu( menu, context );
    } else if( zdj_soundcard_node_name_is_cv( context->node->name ) ) {
        zdj_soundcard_build_select_node_cv_menu( menu, context );
    } else if( zdj_soundcard_node_name_is_midi( context->node->name ) ) {
        zdj_soundcard_build_select_node_midi_menu( menu, context );
    }

    return soundcard_select_node;
}

void _zdj_soundcard_select_node_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );
}

void _zdj_soundcard_select_node_deinit_state( zdj_view_t * view ) {
    // zdj_menu_view_state_t * state = (zdj_menu_view_state_t*)view->state;
    // free( state );
    // view->state = NULL;
}

void _zdj_soundcard_select_node_handle_back( zdj_view_t * menu_view ) {
    zdj_pop_subview_of( zdj_root_view( ), true );
}

void _zdj_soundcard_select_node_handle_select( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_menu_item_view_state_t * node_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_soundcard_node_config_context_t * context = (zdj_soundcard_node_config_context_t*)node_state->data->ptr;

    // Store the node linkage selection for use during callbacks
    context->new_node_selection = zdj_soundcard_get_node_for_name( 
        context->soundcard, 
        node_state->data->i_val 
    );
    context->remove_node_selection = NULL;
    // printf( "_zdj_soundcard_select_node_handle_select: %s %p %p %p\n",
    //     zdj_soundcard_node_name[ node_state->data->i_val ],
    //     context->new_node_selection,
    //     context->options_view_cb,
    //     context->main_view_cb
    // );
    if( context->options_view_cb ) { 
        // printf( "context: %p - %p\n", context, context->options_view_cb );
        context->options_view_cb( context ); 
    }
    if( context->main_view_cb ) { 
        context->main_view_cb( context ); 
    }
    // Select and pop to options
    zdj_pop_subview_of( zdj_root_view( ), true );

    // printf( "_zdj_soundcard_select_node_handle_select done\n" );
}

void _zdj_soundcard_select_node_handle_remove( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_menu_item_view_state_t * node_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_soundcard_node_config_context_t * context = (zdj_soundcard_node_config_context_t*)node_state->data->ptr;
    
    // Store the node linkage selection for use during callbacks
    context->remove_node_selection = zdj_soundcard_get_node_for_name( 
        context->soundcard, 
        node_state->data->i_val 
    );
    context->new_node_selection = NULL;
    // printf( "_zdj_soundcard_select_node_handle_remove: %s %p %p %p\n",
    //     zdj_soundcard_node_name[ node_state->data->i_val ],
    //     context->remove_node_selection,
    //     context->options_view_cb,
    //     context->main_view_cb
    // );
    if( context->options_view_cb ) { 
        // printf( "calling ops cb\n" );
        context->options_view_cb( context ); 
    }
    if( context->main_view_cb ) { 
        // printf( "calling main cb\n" );
        context->main_view_cb( context ); 
    }
    // Select and pop to options
    zdj_pop_subview_of( zdj_root_view( ), true );
}


zdj_error_type_t _zdj_soundcard_add_select_menu_item( 
    zdj_view_t * menu,
    zdj_soundcard_node_config_context_t * context,
    zdj_soundcard_node_name_t name
) {
    char adjusted_name[ 64 ];
    if( zdj_soundcard_node_name_is_analog_input( name ) ||
        zdj_soundcard_node_name_is_analog_output( name ) 
    ) {
        if( zdj_soundcard_get_node_for_name( context->soundcard, name )->stereo ) {
            zdj_soundcard_get_port_title_with_stereo( context->soundcard, name, adjusted_name );
        } else {
            strcpy( adjusted_name, zdj_soundcard_node_name[ name ] );
        }
    } else {
        strcpy( adjusted_name, zdj_soundcard_node_name[ name ] );
    }

    // Make a menu item linked to the config context
    zdj_view_t * item = zdj_new_menu_item( 
        adjusted_name, 
        ZDJ_MENU_ITEM_LAYOUT_BASIC_R 
    );
    item->handle_control_event = &_zdj_soundcard_select_node_handle_select;
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;
    item_state->data->ptr = context; // Store config context for handling selection
    item_state->data->i_val = name; // Store this menu item's node name
    zdj_menu_view_add_item( menu, item );
}

zdj_error_type_t zdj_soundcard_build_select_node_output_menu( 
    zdj_view_t * menu,
    zdj_soundcard_node_config_context_t * context 
) {
    // printf( "zdj_soundcard_build_select_node_output_menu\n" );
    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Input Ports" ) );
     _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0 );
    // Only show in 1 if in 0/1 are mono
    if( !zdj_soundcard_get_node_for_name( context->soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0 )->stereo ) {
        _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1 );
    }
    
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2 );
    // Only show in 3 if in 2/3 are mono
    if( !zdj_soundcard_get_node_for_name( context->soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2 )->stereo ) {
        _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3 );
    }

    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Busses" ) );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3 );

    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Decks" ) );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_PREFADE );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_BUS );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_PREFADE );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_BUS );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_PREFADE );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_BUS );

    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Clocks" ) );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_0 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_1 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_2 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_3 );

    zdj_menu_view_add_section( menu, zdj_new_menu_section( "CV" ) );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CV_0 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CV_1 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CV_2 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CV_3 );
}

zdj_error_type_t zdj_soundcard_build_select_node_input_menu( 
    zdj_view_t * menu,
    zdj_soundcard_node_config_context_t * context 
) {
    printf( "zdj_soundcard_build_select_node_input_menu\n" );
    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Output Ports" ) );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0 );
    // Only show out 1 if out 0/1 are mono
    if( !zdj_soundcard_get_node_for_name( context->soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0 )->stereo ) {
        _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1 );
    }
    
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2 );
    // Only show out 3 if out 2/3 are mono
    if( !zdj_soundcard_get_node_for_name( context->soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2 )->stereo ) {
        _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3 );
    }

    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Busses" ) );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3 );

    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Decks" ) );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_INPUT );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_INPUT );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT );

    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Clocks" ) );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_0 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_1 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_2 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_3 );

    zdj_menu_view_add_section( menu, zdj_new_menu_section( "CV" ) );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CV_0 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CV_1 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CV_2 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CV_3 );
}

zdj_error_type_t zdj_soundcard_build_select_node_internal_bus_menu( 
    zdj_view_t * menu,
    zdj_soundcard_node_config_context_t * context 
) {
    printf( "zdj_soundcard_build_select_node_internal_bus_menu\n" );
    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Output Ports" ) );
     _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0 );
    // Only show out 1 if out 0/1 are mono
    if( !zdj_soundcard_get_node_for_name( context->soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0 )->stereo ) {
        _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1 );
    }
    
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2 );
    // Only show out 3 if out 2/3 are mono
    if( !zdj_soundcard_get_node_for_name( context->soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2 )->stereo ) {
        _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3 );
    }

    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Busses" ) );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3 );
}

zdj_error_type_t zdj_soundcard_build_select_node_aux_bus_menu( 
    zdj_view_t * menu,
    zdj_soundcard_node_config_context_t * context 
) {
    printf( "zdj_soundcard_build_select_node_input_menu\n" );
    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Output Ports" ) );
     _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0 );
    // Only show out 1 if out 0/1 are mono
    if( !zdj_soundcard_get_node_for_name( context->soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0 )->stereo ) {
        _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1 );
    }
    
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2 );
    // Only show out 3 if out 2/3 are mono
    if( !zdj_soundcard_get_node_for_name( context->soundcard, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2 )->stereo ) {
        _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3 );
    }

    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Busses" ) );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_MAIN_BUS );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CUE_BUS );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANNOT_BUS );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_0 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_1 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_2 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_AUX_BUS_3 );

    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Decks" ) );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_INPUT );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_INPUT );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT );

    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Clocks" ) );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_0 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_1 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_2 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CLOCK_3 );

    zdj_menu_view_add_section( menu, zdj_new_menu_section( "CV" ) );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CV_0 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CV_1 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CV_2 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_CV_3 );
}

zdj_error_type_t zdj_soundcard_build_select_node_clock_menu( 
    zdj_view_t * menu,
    zdj_soundcard_node_config_context_t * context 
) {
    printf( "zdj_soundcard_build_select_node_clock_menu\n" );
    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Output Ports" ) );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3 );
}

zdj_error_type_t zdj_soundcard_build_select_node_cv_menu( 
    zdj_view_t * menu,
    zdj_soundcard_node_config_context_t * context 
) {
    printf( "zdj_soundcard_build_select_node_cv_menu\n" );
    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Output Ports" ) );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3 );

    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Recording" ) );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS );

    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Decks" ) );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_DECK_1_INPUT );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_DECK_2_INPUT );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_DECK_EXT_INPUT );
}

zdj_error_type_t zdj_soundcard_build_select_node_midi_menu( 
    zdj_view_t * menu,
    zdj_soundcard_node_config_context_t * context 
) {
    printf( "zdj_soundcard_build_select_node_midi_menu\n" );
    zdj_menu_view_add_section( menu, zdj_new_menu_section( "Output Ports" ) );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2 );
    _zdj_soundcard_add_select_menu_item( menu, context, ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3 );
}
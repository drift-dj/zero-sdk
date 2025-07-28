#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/controls/hmi/zdj_hmi.h>
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
#include <zerodj/ui/view/zdj_view_stack.h>

static void _zdj_soundcard_options_draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_soundcard_options_handle_hmi( zdj_view_t * view, void * _event );
static void _zdj_soundcard_options_deinit_state( zdj_view_t * view );
static void _zdj_soundcard_options_handle_back( zdj_view_t * menu_view );

static void _zdj_soundcard_options_handle_bus_btn( zdj_view_t * view, void * _event );
static void _zdj_soundcard_options_handle_save_btn( zdj_view_t * view, void * _event );
static void _zdj_soundcard_options_handle_load_btn( zdj_view_t * view, void * _event );

zdj_view_t * zdj_new_soundcard_options( zdj_soundcard_node_config_context_t * context ) {
    zdj_view_t * soundcard_options = zdj_new_modal_view( zdj_modal_rect( ) );
    soundcard_options->draw = &_zdj_soundcard_options_draw;
    soundcard_options->handle_hmi_event = &_zdj_soundcard_options_handle_hmi;
    soundcard_options->deinit_state = &_zdj_soundcard_options_deinit_state;

    // Add a state instance
    zdj_soundcard_options_state_t * state = calloc( 1, sizeof( zdj_soundcard_options_state_t ) );
    state->needs_layout_update = true;
    state->update_layout = zdj_soundcard_options_get_update_layout_for_node( context->node );
    state->config_context = context;
    soundcard_options->state = state;

    // Capture state in context
    context->options_view_state = state;

    // Make menu
    zdj_view_t * menu = zdj_new_menu_view( ZDJ_VERTICAL, zdj_modal_rect( ) );
    zdj_add_subview( soundcard_options, menu );
    menu->frame->x = 0;
    menu->frame->y = 0;
    menu->frame->w = ZDJ_MODAL_WIDTH;
    menu->frame->h = ZDJ_MODAL_HEIGHT;
    zdj_menu_view_set_scrollview_frame( menu, &(zdj_rect_t){22,0,ZDJ_MODAL_WIDTH-22,ZDJ_MODAL_HEIGHT} );
    state->menu = menu;
    
    // Set up header
    char options_title[ 64 ];
    char options_subtitle[ 64 ];
    if( zdj_soundcard_node_name_is_output( context->node->name ) ) {
        strcpy( options_title, "Output Setup" );
        if( context->node->stereo ) {
            if( context->node->name == ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0 ) {
                strcpy( options_subtitle, "Analog Out 1+2" );
            } else if( context->node->name == ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_2 ) {
                strcpy( options_subtitle, "Analog Out 3+4" );
            }
        } else {
            strcpy( options_subtitle, zdj_soundcard_node_name[ context->node->name ] );
        }
    } else if( zdj_soundcard_node_name_is_input( context->node->name ) ) {
        strcpy( options_title, "Input Setup" );
        if( context->node->stereo ) {
            if( context->node->name == ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0 ) {
                strcpy( options_subtitle, "Analog In 1+2" );
            } else if( context->node->name == ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2 ) {
                strcpy( options_subtitle, "Analog In 3+4" );
            }
        } else {
            strcpy( options_subtitle, zdj_soundcard_node_name[ context->node->name ] );
        }
    } else {
        strcpy( options_title, "Bus Setup" );
        strcpy( options_subtitle, zdj_soundcard_node_name[ context->node->name ] );
    }
    zdj_view_t * menu_header = zdj_new_menu_header( 
        options_title,
        options_subtitle,
        ZDJ_MENU_HEADER_STYLE_NORMAL,
        ZDJ_MENU_HEADER_BACK_STYLE_BACK
    );
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_header->state;
    header_state->handle_back = &_zdj_soundcard_options_handle_back;
    zdj_menu_view_add_header( menu, menu_header );

    // Add divider
    zdj_view_t * divider = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MIXER_DIV ], NULL );
    divider->frame->x = 18;
    divider->frame->y = 9;
    zdj_add_bottom_subview_to( soundcard_options, divider );

    return soundcard_options;
}

void _zdj_soundcard_options_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_soundcard_options_state_t * state = (zdj_soundcard_options_state_t*)view->state;

    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );

    if( state->needs_layout_update ) { state->update_layout( view ); }
}

soundcard_options_update_layout_t zdj_soundcard_options_get_update_layout_for_node( 
    zdj_soundcard_node_t * node 
) {
    
    if ( zdj_soundcard_node_name_is_audio( node->name ) ) {
        if ( zdj_soundcard_node_name_is_output( node->name ) ) {
            return &zdj_soundcard_options_update_port_output_layout;
        } else if( zdj_soundcard_node_name_is_input( node->name ) ) {
            return &zdj_soundcard_options_update_port_input_layout;
        } else {
            return &zdj_soundcard_options_update_audio_bus_layout;
        }
    } else if ( zdj_soundcard_node_name_is_clock( node->name ) ) {
        
    } else if ( zdj_soundcard_node_name_is_cv( node->name ) ) {
       
    } else if ( zdj_soundcard_node_name_is_usb( node->name) ) {
        
    } else if ( zdj_soundcard_node_name_is_midi( node->name ) ) {
        
    }
}

void _zdj_soundcard_options_handle_hmi( zdj_view_t * view, void * _event ) {
    zdj_hmi_event_t * e = (zdj_hmi_event_t *)_event;
    zdj_soundcard_options_state_t * state = (zdj_soundcard_options_state_t*)view->state;
    zdj_soundcard_node_config_context_t * context = (zdj_soundcard_node_config_context_t*)state->config_context;

    // Ignore events which have been blocked by layers above this one.
    if( e->blocked ) { return; }

    // Grab Tone 1,2,3 + Jog push turn to send controls into channels.
    if( (e->id == ZDJ_HMI_ENCO_2_JOG && e->type == ZDJ_HMI_EVENT_PRESS_ADJUST) ||
        (e->id == ZDJ_HMI_ENCO_3_TONE_1 && e->type == ZDJ_HMI_EVENT_ADJUST) ||
        (e->id == ZDJ_HMI_ENCO_3_TONE_1 && e->type == ZDJ_HMI_EVENT_RELEASE) ||
        (e->id == ZDJ_HMI_ENCO_4_TONE_2 && e->type == ZDJ_HMI_EVENT_ADJUST)
    ) {
        // Get current menu scroll index
        zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu->state;
        zdj_view_t * item = zdj_menu_view_item_at_current_scroll_index( state->menu );
        
        // Send events into view's hmi handler
        
        if ( zdj_soundcard_node_name_is_audio( context->node->name ) ) {
            if ( zdj_soundcard_node_name_is_output( context->node->name ) ) {
                zdj_soundcard_options_port_output_hmi( view, _event );
            } else if( zdj_soundcard_node_name_is_input( context->node->name ) ) {
                zdj_soundcard_options_port_input_hmi( view, _event );
            } else {
                zdj_soundcard_options_audio_bus_hmi( view, _event );
            }
        } else if ( zdj_soundcard_node_name_is_clock( context->node->name ) ) {
            
        } else if ( zdj_soundcard_node_name_is_cv( context->node->name ) ) {
        
        } else if ( zdj_soundcard_node_name_is_usb( context->node->name) ) {
            
        } else if ( zdj_soundcard_node_name_is_midi( context->node->name ) ) {
            
        }

    } else {
        // Send remaining events down into the menu
        state->menu->handle_hmi_event( state->menu, _event );
    }
}

void _zdj_soundcard_options_deinit_state( zdj_view_t * view ) {
    // zdj_menu_view_state_t * state = (zdj_menu_view_state_t*)view->state;
    // free( state );
    // view->state = NULL;
}

void _zdj_soundcard_options_handle_back( zdj_view_t * view ) {
    zdj_pop_subview_of( zdj_root_view( ), true );
}

void _zdj_soundcard_options_handle_bus_btn( zdj_view_t * view, void * _event ) {
    // Push bus mixer
    // zdj_view_t * soundcard_options = zdj_new_soundcard_soundcard_options( zdj_soundcard );
    // zdj_push_subview( zdj_root_view( ), soundcard_options, true );
}

void _zdj_soundcard_options_handle_save_btn( zdj_view_t * view, void * _event ) {
    // Push text edit for soundcard name
}

void _zdj_soundcard_options_handle_load_btn( zdj_view_t * view, void * _event ) {
    // Open soundcard selection list
}
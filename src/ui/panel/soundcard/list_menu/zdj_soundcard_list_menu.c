#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/system/uuid/zdj_uuid.h>
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
#include <zerodj/ui/panel/soundcard/list_menu/zdj_soundcard_list_menu.h>
#include <zerodj/ui/view/text_input_view/zdj_text_input_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _load_dto( zdj_view_t * view, zdj_control_event_t * _event );
static void _save_dto( zdj_view_t * view, zdj_control_event_t * _event );
static void _new_mixer( zdj_view_t * view, zdj_control_event_t * _event );
static void _deinit_state( zdj_view_t * view );
static void _handle_back( zdj_view_t * menu_view );
static void _text_input_cb( zdj_text_input_view_action_t action, char * result );

zdj_view_t * zdj_new_soundcard_list_menu( 
    zdj_soundcard_panel_state_t * panel_state,
    bool is_load
 ) {
    zdj_view_t * view = zdj_new_modal_view( zdj_modal_rect( ) );
    view->draw = &_draw;
    view->deinit_state = &_deinit_state;

    // Make menu
    zdj_view_t * menu = zdj_new_menu_view( ZDJ_VERTICAL, zdj_menu_rect_sm( ) );
    zdj_add_subview( view, menu );
    menu->frame.y = 0;
    menu->frame.h = ZDJ_MODAL_HEIGHT;
    
    // Set up header
    zdj_view_t * menu_header = zdj_new_menu_header( 
        "Select Mixer",
        " ",
        ZDJ_MENU_HEADER_STYLE_NORMAL,
        ZDJ_MENU_HEADER_BACK_STYLE_BACK
    );
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_header->state;
    header_state->handle_back = &_handle_back;
    zdj_menu_view_add_header( menu, menu_header );

    if( !is_load ) { 
        zdj_view_t * dto_item = zdj_new_menu_item( "+ New Mixer", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
        zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)dto_item->state;
        dto_item->handle_control_event = &_new_mixer;
        zdj_menu_view_add_item( menu, dto_item );
    }

    // Pull soundcards from DB
    int dto_count = zdj_soundcard_count_stored_records( );
    zdj_soundcard_dto_t ** dtos = calloc( dto_count, sizeof( zdj_soundcard_dto_t* ) );
    zdj_error_type_t err = zdj_soundcard_fetch_all_dtos( dto_count, dtos );

    for( int s=0; s<dto_count; s++ ) {
        printf( "List found Soundcard: %s\n", dtos[ s ]->name );
        // Skip the Default soundcard if we're saving
        if( !strcmp( dtos[ s ]->name, "Current" ) ) { continue; }
        if( !is_load && !strcmp( dtos[ s ]->name, "Default" ) ) { continue; }
        zdj_view_t * dto_item = zdj_new_menu_item( dtos[ s ]->name, ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
        zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)dto_item->state;
        // item_state->edit_options_type = ZDJ_MENU_ITEM_OPTIONS_DJ_PLAYLIST;
        // item_state->edit_enabled = true;
        item_state->data.ptr = panel_state;
        strcpy( item_state->data.c_val, dtos[ s ]->entity_id );
        if( is_load ) {
            dto_item->handle_control_event = &_load_dto;
        } else {
            dto_item->handle_control_event = &_save_dto;
        }
        zdj_menu_view_add_item( menu, dto_item );
        
        free( dtos[ s ] );
    }

    return view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );
}

static void _deinit_state( zdj_view_t * view ) {
    // zdj_menu_view_state_t * state = (zdj_menu_view_state_t*)view->state;
    // free( state );
    // view->state = NULL;
}

static void _handle_back( zdj_view_t * menu_view ) {
    // Request soundcard update and dismiss
    zdj_pop_subview_of( zdj_panel_view( ), true );
}

// Update soundcard state and update
// Request a reload cycle with new DTO
static void _load_dto( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_soundcard_panel_state_t * panel_state = (zdj_soundcard_panel_state_t*)state->data.ptr;
    
    // Teardown the layout before we start deleting nodes
    panel_state->needs_layout_teardown = true;
    zdj_soundcard_load_mixer( zdj_soundcard, state->data.c_val );

    // Request a refresh once soundcard is running again
    panel_state->needs_layout_update = true;

    zdj_pop_subview_of( zdj_panel_view( ), true );
}

static void _save_dto( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    // Fetch DTO based on entity_id stored in menu item
    zdj_soundcard_dto_t * dto = calloc( 1, sizeof( zdj_soundcard_dto_t ) );
    zdj_soundcard_fetch_dto( state->data.c_val, dto );
    // Copy data from current soundcard DTO to fetched DTO
    zdj_soundcard_copy_dto( &zdj_soundcard->dto, dto );
    // Store DTO
    zdj_soundcard_store_dto( dto );

    free( dto );

    zdj_pop_subview_of( zdj_panel_view( ), true );
}

static void _new_mixer( zdj_view_t * view, zdj_control_event_t * _event ) {
    char new_mixer_name[ 64 ];
    sprintf( new_mixer_name, "Mixer %03d", zdj_soundcard_count_stored_records( ) );
    // Push text edit for soundcard name
    zdj_view_t * name_edit_view = zdj_new_text_input_view( 
        _text_input_cb, 
        new_mixer_name
    );
    zdj_push_subview( zdj_panel_view( ), name_edit_view, true );
}

static void _text_input_cb( zdj_text_input_view_action_t action, char * result ) { 
    switch ( action ) {
        case ZDJ_TEXT_INPUT_ACTION_OKAY:
            printf( "name edit exit okay: %s\n", result );
            zdj_soundcard_dto_t * dto = calloc( 1, sizeof( zdj_soundcard_dto_t ) );
            // Copy data from current soundcard DTO to fetched DTO
            zdj_soundcard_copy_dto( &zdj_soundcard->dto, dto );
            strcpy( dto->name, result );
            // Create entity_id for new mixer
            zdj_put_uuid_no_dash( dto->entity_id );
            // Store DTO
            zdj_soundcard_store_dto( dto );
            // Set needs update
            break;
        case ZDJ_TEXT_INPUT_ACTION_CANCEL:
            printf( "name edit exit cancel\n" );
            break;
        default:
            break;
    }
    zdj_pop_n_subviews_of( zdj_panel_view( ), 2, true );
}
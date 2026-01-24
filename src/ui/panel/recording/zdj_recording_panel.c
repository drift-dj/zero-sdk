#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/signal/deck/zdj_deck_manager.h>
#include <zerodj/signal/pipeline/node/audio/record/zdj_audio_record_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/panel/recording/zdj_recording_panel.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/menu_section_view/zdj_menu_section_view.h>
#include <zerodj/ui/view/modal_view/zdj_modal_view.h>
#include <zerodj/ui/view/scroll_view/zdj_scroll_view.h>
#include <zerodj/ui/panel/soundcard/zdj_soundcard_panel.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _handle_control( zdj_view_t * view, zdj_control_event_t * event );
static void _handle_back( zdj_view_t * menu_view );
static void _refresh_menu( zdj_view_t * view );

static void _handle_record_toggle( zdj_view_t * view, zdj_control_event_t * event );
static void _handle_save_recording( zdj_view_t * view, zdj_control_event_t * event );
static void _handle_delete_recording( zdj_view_t * view, zdj_control_event_t * event );

static void _hms_string_for_pcm_sample( char * str, double sample, uint32_t sample_rate );
static void _ms_string_for_pcm_sample( char * str, double sample, uint32_t sample_rate );

zdj_view_t * zdj_new_recording_panel( void ) {
    // printf( "zdj_new_recording_panel\n" );
    zdj_view_t * view = zdj_new_modal_view( zdj_modal_rect( ) );
    view->draw = &_draw;
    view->handle_control_event = &_handle_control;
    view->map = ZDJ_CONTROL_MAP_RECORDING_PANEL;

    zdj_recording_panel_state_t * state = calloc( 1, sizeof( zdj_recording_panel_state_t ) );
    state->view_needs_refresh = true;
    view->state = state;

    // Make menu
    zdj_view_t * menu = zdj_new_menu_view( ZDJ_VERTICAL, zdj_modal_rect( ) );
    zdj_add_subview( view, menu );
    menu->frame.x = 0;
    menu->frame.y = 0;
    menu->frame.w = ZDJ_MODAL_WIDTH;
    menu->frame.h = ZDJ_MODAL_HEIGHT;
    state->menu = menu;
    
    // Set up header
    zdj_view_t * menu_header = zdj_new_menu_header( 
        "Recording",
        " ",
        ZDJ_MENU_HEADER_STYLE_NORMAL,
        ZDJ_MENU_HEADER_BACK_STYLE_EXIT
    );
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_header->state;
    header_state->handle_back = &_handle_back;
    zdj_menu_view_add_header( menu, menu_header );
    
    return view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // printf( "record_panel draw\n" );
    zdj_recording_panel_state_t * state = (zdj_recording_panel_state_t*)view->state;

    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );

    if( state->view_needs_refresh ) { _refresh_menu( view ); }
    // printf( "record_panel draw done\n" );
}

static void _handle_control( zdj_view_t * view, zdj_control_event_t * event ) {
    // Ignore events which have been blocked by layers above this one.
    if( event->blocked ) { return; }
    
    // Send events down into the subview stack
    zdj_recording_panel_state_t * state = (zdj_recording_panel_state_t*)view->state;
    state->menu->handle_control_event( state->menu, event );

    event->blocked = true;
}

static void _handle_back( zdj_view_t * menu_view ) {

}

static void _refresh_menu( zdj_view_t * view ) {
    // printf( "record_panel _refresh_menu\n" );
    zdj_recording_panel_state_t * state = (zdj_recording_panel_state_t*)view->state;
    
    if( !zdj_soundcard) { return; }
    zdj_audio_record_node_state_t * recording_node_state = (zdj_audio_record_node_state_t*)zdj_soundcard->recording_node->state;

    zdj_menu_view_remove_all_subviews( state->menu );

    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu->state;
    zdj_scroll_view_state_t * scroll_state = (zdj_scroll_view_state_t*)menu_state->scroll_view->state;
    
    // Add record toggle btn
    zdj_view_t * record_toggle_btn;
        
    if( recording_node_state->status == ZDJ_AUDIO_RECORD_ACTIVE ) {
        record_toggle_btn = zdj_new_asset_menu_item( 
            ZDJ_UI_ASSET_RECORD_PAUSE_BTN, ZDJ_UI_ASSET_RECORD_PAUSE_BTN_HI, false 
        );
    } else {
        record_toggle_btn = zdj_new_asset_menu_item( 
            ZDJ_UI_ASSET_RECORD_BTN, ZDJ_UI_ASSET_RECORD_BTN_HI, false 
        );
    }
    record_toggle_btn->handle_control_event = &_handle_record_toggle;
    zdj_menu_item_view_state_t * record_toggle_state = (zdj_menu_item_view_state_t*)record_toggle_btn->state;
    record_toggle_state->data.ptr = state;
    record_toggle_btn->frame.x = 6;
    record_toggle_btn->frame.y = 4;
    record_toggle_btn->frame.h = 14;
    zdj_menu_view_add_item( state->menu, record_toggle_btn );

    // If recording has started, add save btn
    if( state->has_open_recording ) { 
        zdj_view_t * save_recording_btn = zdj_new_asset_menu_item( 
            ZDJ_UI_ASSET_RECORD_SAVE_BTN, ZDJ_UI_ASSET_RECORD_SAVE_BTN_HI, false 
        );
        save_recording_btn->handle_control_event = &_handle_save_recording;
        zdj_menu_item_view_state_t * save_recording_state = (zdj_menu_item_view_state_t*)save_recording_btn->state;
        save_recording_state->data.ptr = state;
        save_recording_btn->frame.x = 26;
        save_recording_btn->frame.y = 8;
        zdj_menu_view_add_item( state->menu, save_recording_btn );

        zdj_view_t * delete_recording_btn = zdj_new_asset_menu_item( 
            ZDJ_UI_ASSET_RECORD_DELETE_BTN, ZDJ_UI_ASSET_RECORD_DELETE_BTN_HI, false 
        );
        delete_recording_btn->handle_control_event = &_handle_delete_recording;
        zdj_menu_item_view_state_t * delete_recording_state = (zdj_menu_item_view_state_t*)delete_recording_btn->state;
        delete_recording_state->data.ptr = state;
        delete_recording_btn->frame.x = 37;
        delete_recording_btn->frame.y = 8;
        zdj_menu_view_add_item( state->menu, delete_recording_btn );
    }

    // Add counter
    char time_str[ 64 ];
    _hms_string_for_pcm_sample( time_str, recording_node_state->sample_count, 44100 );
    zdj_view_t * hms_time = zdj_new_label_view( time_str, ZDJ_FONT_12, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    hms_time->frame.x = view->frame.w - hms_time->frame.w - 12;
    hms_time->frame.y = 2;
    zdj_menu_view_add_item( state->menu, hms_time );
   
    _ms_string_for_pcm_sample( time_str, recording_node_state->sample_count, 44100 );
    zdj_view_t * ms_time = zdj_new_label_view( time_str, ZDJ_FONT_12, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    ms_time->frame.x = view->frame.w - 12;
    ms_time->frame.y = 2;
    zdj_menu_view_add_item( state->menu, ms_time );

    zdj_menu_view_add_padding( state->menu, 2 );

    // Add Meter
    zdj_view_t * meter = zdj_new_record_meter_view( );
    zdj_menu_view_add_item( state->menu, meter );

    zdj_menu_view_add_padding( state->menu, 3 );

    // Add Input routing section
    zdj_menu_view_add_section( state->menu, zdj_new_menu_section( "Input" ) );
    zdj_menu_view_add_item( state->menu, zdj_new_menu_item( "Main Bus LR", ZDJ_MENU_ITEM_LAYOUT_BASIC_R ) );

    zdj_menu_view_add_padding( state->menu, 3 );
    // printf( "record_panel _refresh_menu done\n" );
}

static void _handle_record_toggle( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_recording_panel_state_t * panel_state = (zdj_recording_panel_state_t*)state->data.ptr;
    zdj_audio_record_node_state_t * recording_node_state = (zdj_audio_record_node_state_t*)zdj_soundcard->recording_node->state;

    if( recording_node_state->status == ZDJ_AUDIO_RECORD_ACTIVE ) {
        // Pause the recording
        recording_node_state->status = ZDJ_AUDIO_RECORD_INACTIVE;
    } else {
        if( panel_state->has_open_recording ) {
            recording_node_state->status = ZDJ_AUDIO_RECORD_ACTIVE;
        } else {
            panel_state->has_open_recording = true;
            zdj_new_audio_record_capture( zdj_soundcard->recording_node );
        }
    }
}

static void _handle_save_recording( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_recording_panel_state_t * panel_state = (zdj_recording_panel_state_t*)state->data.ptr;
    panel_state->has_open_recording = false;
    zdj_finish_audio_record_capture( zdj_soundcard->recording_node, true );
}

static void _handle_delete_recording( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_recording_panel_state_t * panel_state = (zdj_recording_panel_state_t*)state->data.ptr;
    panel_state->has_open_recording = false;
    zdj_finish_audio_record_capture( zdj_soundcard->recording_node, false );
}

static void _hms_string_for_pcm_sample( char * str, double sample, uint32_t sample_rate ) {
    int hrs = sample / sample_rate / 60 / 60;
    int mins = sample / sample_rate / 60;
    int secs = (sample / sample_rate) - (mins * 60);
    double secf = ((double)sample / (double)sample_rate);
    int msec = (int)((secf - secs) * 1000.0);

    snprintf( str, -1, "%d:%02d:%02d", 
        hrs,
        mins,
        secs
    );
}

static void _ms_string_for_pcm_sample( char * str, double sample, uint32_t sample_rate ) {
    int mins = sample / sample_rate / 60;
    int secs = (sample / sample_rate) - (mins * 60);
    double secf = ((double)sample / (double)sample_rate);
    int msec = (int)((secf - secs) * 10.0);

    snprintf( str, -1, ".%d", msec );
}
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/settings/zdj_settings.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/panel/zdj_ui_panel.h>
#include <zerodj/ui/panel/settings/zdj_settings_panel.h>
#include <zerodj/ui/panel/settings/ui/zdj_settings_ui_panel.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/menu_section_view/zdj_menu_section_view.h>
#include <zerodj/ui/view/modal_view/zdj_modal_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_back( zdj_view_t * menu_view );
static void _refresh_menu( zdj_view_t * view );

static void _refresh_rate_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _brightness_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _tooltip_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _handle_show_bpm( zdj_view_t * view, zdj_control_event_t * event );
static void _handle_show_key( zdj_view_t * view, zdj_control_event_t * event );
static void _handle_show_camelot( zdj_view_t * view, zdj_control_event_t * event );

zdj_view_t * zdj_new_settings_ui_panel( void (*cb)(void*) ) {
    zdj_view_t * view = zdj_new_modal_view( zdj_modal_rect( ) );
    view->draw = &_draw;
    view->handle_control_event = &_handle_control;
    view->map = ZDJ_CONTROL_MAP_SETTINGS_PANEL;

    zdj_settings_ui_panel_state_t * state = calloc( 1, sizeof( zdj_settings_ui_panel_state_t ) );
    state->needs_layout_update = true;
    state->exit_cb = cb;
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
        "Settings",
        " ",
        ZDJ_MENU_HEADER_STYLE_NORMAL,
        ZDJ_MENU_HEADER_BACK_STYLE_BACK
    );
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_header->state;
    header_state->handle_back = &_handle_back;
    zdj_menu_view_add_header( menu, menu_header );
    
    return view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_settings_ui_panel_state_t * state = (zdj_settings_ui_panel_state_t*)view->state;

    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );

    
    if( state->needs_layout_update ) { 
        _refresh_menu( view ); 
    }
}

static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {
    // Ignore events which have been blocked by layers above this one.
    if( _event->blocked ) { return; }

    zdj_settings_ui_panel_state_t * state = (zdj_settings_ui_panel_state_t*)view->state;
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu->state;

    if( (_event->id == ZDJ_UI_CONTROL_JOG_RELEASE_0 &&
        menu_state->scroll_index == -1) ||
        _event->id == ZDJ_UI_CONTROL_NAV_RELEASE_0
    ) {
        printf( "usb_status_view back_btn\n" );
        // Dump the top view on the stack (this view)
        zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
        zdj_pop_subview_of( panel_state->settings_panel, true );
        _event->blocked = true;
        // Return immediately since we're being dismissed
        return;
    } else { 
        // Send events down into the subview stack
        zdj_settings_ui_panel_state_t * state = (zdj_settings_ui_panel_state_t*)view->state;
        state->menu->handle_control_event( state->menu, _event );
    }

    _event->blocked = true;
}

static void _handle_back( zdj_view_t * menu_view ) {
    printf( "_handle_back\n" );
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_pop_subview_of( panel_state->settings_panel, true );
}

static void _refresh_menu( zdj_view_t * view ) {
    zdj_settings_ui_panel_state_t * state = (zdj_settings_ui_panel_state_t*)view->state;

    printf( "refresh menu\n" );
    zdj_menu_view_remove_all_subviews( state->menu );


    // Display
    zdj_menu_view_add_section( 
        state->menu, 
        zdj_new_menu_section( "Display" ) 
    );

    // Brightness
    zdj_view_t * brightness_btn = zdj_new_data_menu_item( 
        "Bright/Contrast", 
        ZDJ_MENU_ITEM_LAYOUT_DATA_R,
        ZDJ_MENU_ITEM_DATA_TYPE_CHAR,
        NULL,
        NULL 
    );
    brightness_btn->handle_control_event = &_brightness_btn;
    zdj_menu_item_view_state_t * brightness_state = (zdj_menu_item_view_state_t*)brightness_btn->state;
    brightness_state->data.ptr = view;
    zdj_setting_t * brightness_setting = zdj_setting_get( ZDJ_SETTING_DISPLAY_BRIGHTNESS );
    if( brightness_setting ) {
        int brightness = brightness_setting->i_val;
        sprintf( brightness_state->data.c_val, "%d", brightness );
    } else {
        strcpy( brightness_state->data.c_val, "---" );
    }
    zdj_menu_view_add_item( state->menu, brightness_btn );


    // Refresh Rate
    zdj_view_t * rate_btn = zdj_new_data_menu_item( 
        "Refresh Rate", 
        ZDJ_MENU_ITEM_LAYOUT_DATA_R,
        ZDJ_MENU_ITEM_DATA_TYPE_CHAR,
        NULL,
        NULL 
    );
    rate_btn->handle_control_event = &_refresh_rate_btn;
    zdj_menu_item_view_state_t * rate_state = (zdj_menu_item_view_state_t*)rate_btn->state;
    rate_state->data.ptr = view;
    zdj_setting_t * rate_setting = zdj_setting_get( ZDJ_SETTING_REFRESH_RATE );
    if( rate_setting ) {
        zdj_setting_refresh_rate_t rate = rate_setting->i_val;
        printf( "display rate: %d\n", rate );
        switch ( rate ) {
            case ZDJ_SETTING_REFRESH_RATE_115: sprintf( rate_state->data.c_val, "%d Hz", 115 ); break;
            case ZDJ_SETTING_REFRESH_RATE_60: sprintf( rate_state->data.c_val, "%d Hz", 60 ); break;
            case ZDJ_SETTING_REFRESH_RATE_30: sprintf( rate_state->data.c_val, "%d Hz", 30 );  break;
            case ZDJ_SETTING_REFRESH_RATE_20: sprintf( rate_state->data.c_val, "%d Hz", 20 );  break;
            default: sprintf( rate_state->data.c_val, "%d Hz", 115 ); break; 
        }
    } else {
        sprintf( rate_state->data.c_val, "%d Hz", 115 );
    }
    zdj_menu_view_add_item( state->menu, rate_btn );

    // Library
    zdj_menu_view_add_padding( state->menu, 3 );
    zdj_menu_view_add_section( 
        state->menu, 
        zdj_new_menu_section( "Library Menu" ) 
    );

    zdj_view_t * show_bpm_btn = zdj_new_menu_item( "Show BPM", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    show_bpm_btn->handle_control_event = &_handle_show_bpm;
    zdj_menu_item_view_state_t * show_bpm_state = (zdj_menu_item_view_state_t*)show_bpm_btn->state;
    show_bpm_state->data.ptr = view;
    zdj_setting_t * show_bpm_setting = zdj_setting_get( ZDJ_SETTING_LIB_MENU_SHOW_BPM );
    if( show_bpm_setting ) { show_bpm_state->data.b_val = show_bpm_setting->b_val; }
    zdj_menu_view_add_item( state->menu, show_bpm_btn );

    zdj_view_t * show_key_btn = zdj_new_menu_item( "Show Key", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    show_key_btn->handle_control_event = &_handle_show_key;
    zdj_menu_item_view_state_t * show_key_state = (zdj_menu_item_view_state_t*)show_key_btn->state;
    show_key_state->data.ptr = view;
    zdj_setting_t * show_key_setting = zdj_setting_get( ZDJ_SETTING_LIB_MENU_SHOW_KEY );
    if( show_key_setting ) { show_key_state->data.b_val = show_key_setting->b_val; }
    zdj_menu_view_add_item( state->menu, show_key_btn );

    zdj_view_t * show_camelot_btn = zdj_new_menu_item( "Show Camelot", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    show_camelot_btn->handle_control_event = &_handle_show_camelot;
    zdj_menu_item_view_state_t * show_camelot_state = (zdj_menu_item_view_state_t*)show_camelot_btn->state;
    show_camelot_state->data.ptr = view;
    zdj_setting_t * show_camelot_setting = zdj_setting_get( ZDJ_SETTING_LIB_MENU_SHOW_CAMELOT );
    if( show_camelot_setting ) { show_camelot_state->data.b_val = show_camelot_setting->b_val; }
    zdj_menu_view_add_item( state->menu, show_camelot_btn );


    // Hotkeys
    zdj_menu_view_add_padding( state->menu, 3 );
    zdj_menu_view_add_section( 
        state->menu, 
        zdj_new_menu_section( "Hotkeys" ) 
    );

    zdj_view_t * record_btn = zdj_new_data_menu_item( 
        "Toggle Recording", 
        ZDJ_MENU_ITEM_LAYOUT_DATA_R,
        ZDJ_MENU_ITEM_DATA_TYPE_CHAR,
        NULL,
        NULL 
    );
    zdj_menu_item_view_state_t * record_state = (zdj_menu_item_view_state_t*)record_btn->state;
    strcpy( record_state->data.c_val, "Shift + Play" );
    zdj_menu_view_add_item( state->menu, record_btn );
    

    zdj_view_t * screenshot_btn = zdj_new_data_menu_item( 
        "Screencap", 
        ZDJ_MENU_ITEM_LAYOUT_DATA_R,
        ZDJ_MENU_ITEM_DATA_TYPE_CHAR,
        NULL,
        NULL 
    );
    zdj_menu_item_view_state_t * screenshot_state = (zdj_menu_item_view_state_t*)screenshot_btn->state;
    strcpy( screenshot_state->data.c_val, "Shift + Vol" );
    zdj_menu_view_add_item( state->menu, screenshot_btn );


    zdj_view_t * f_quit_btn = zdj_new_data_menu_item( 
        "Force Quit", 
        ZDJ_MENU_ITEM_LAYOUT_DATA_R,
        ZDJ_MENU_ITEM_DATA_TYPE_CHAR,
        NULL,
        NULL 
    );
    zdj_menu_item_view_state_t * f_quit_state = (zdj_menu_item_view_state_t*)f_quit_btn->state;
    strcpy( f_quit_state->data.c_val, "Shift + Vol(Long)" );
    zdj_menu_view_add_item( state->menu, f_quit_btn );



    // Widgets
    zdj_menu_view_add_padding( state->menu, 3 );
    zdj_menu_view_add_section( 
        state->menu, 
        zdj_new_menu_section( "Widgets" ) 
    );

    zdj_view_t * perf_widget_btn = zdj_new_data_menu_item( 
        "Performance", 
        ZDJ_MENU_ITEM_LAYOUT_DATA_R,
        ZDJ_MENU_ITEM_DATA_TYPE_CHAR,
        NULL,
        NULL 
    );
    zdj_menu_item_view_state_t * perf_widget_state = (zdj_menu_item_view_state_t*)perf_widget_btn->state;
    strcpy( perf_widget_state->data.c_val, "Shift + Ext. Deck" );
    zdj_menu_view_add_item( state->menu, perf_widget_btn );

    zdj_view_t * debug_widget_btn = zdj_new_data_menu_item( 
        "Debug", 
        ZDJ_MENU_ITEM_LAYOUT_DATA_R,
        ZDJ_MENU_ITEM_DATA_TYPE_CHAR,
        NULL,
        NULL 
    );
    zdj_menu_item_view_state_t * debug_widget_state = (zdj_menu_item_view_state_t*)debug_widget_btn->state;
    strcpy( debug_widget_state->data.c_val, "Shift + Deck 2" );
    zdj_menu_view_add_item( state->menu, debug_widget_btn );

    state->needs_layout_update = false;
}

static void _refresh_rate_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * ui_panel_view = (zdj_view_t*)item_state->data.ptr;
    zdj_settings_ui_panel_state_t * ui_panel_state = (zdj_settings_ui_panel_state_t*)ui_panel_view->state;

    char rate_data[ 8 ];
    zdj_setting_t * rate_setting = zdj_setting_get( ZDJ_SETTING_REFRESH_RATE );
    if( rate_setting ) {
        zdj_setting_refresh_rate_t rate = rate_setting->i_val;
        int hz = 30;
        switch ( rate ) {
            case ZDJ_SETTING_REFRESH_RATE_115: rate = ZDJ_SETTING_REFRESH_RATE_60; hz = 62; break;
            case ZDJ_SETTING_REFRESH_RATE_60: rate = ZDJ_SETTING_REFRESH_RATE_30; hz = 30; break;
            case ZDJ_SETTING_REFRESH_RATE_30: rate = ZDJ_SETTING_REFRESH_RATE_20; hz = 20; break;
            case ZDJ_SETTING_REFRESH_RATE_20: rate = ZDJ_SETTING_REFRESH_RATE_115; hz = 115; break;
            default: rate = ZDJ_SETTING_REFRESH_RATE_115; break;
        }
        zdj_setting_set_int( ZDJ_SETTING_REFRESH_RATE, rate );
        zdj_ui_set_refresh_hz( hz );
        printf( "setting rate: %d\n", rate );
    }

    ui_panel_state->needs_layout_update = true;
}

static void _brightness_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * ui_panel_view = (zdj_view_t*)item_state->data.ptr;
    zdj_settings_ui_panel_state_t * ui_panel_state = (zdj_settings_ui_panel_state_t*)ui_panel_view->state;
}

static void _tooltip_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    // Toggle tooltips
}

static void _handle_show_bpm( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_setting_flip_bool( ZDJ_SETTING_LIB_MENU_SHOW_BPM );

    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * ui_panel_view = (zdj_view_t*)item_state->data.ptr;
    zdj_settings_ui_panel_state_t * ui_panel_state = (zdj_settings_ui_panel_state_t*)ui_panel_view->state;
    ui_panel_state->needs_layout_update = true;

}

static void _handle_show_key( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_setting_flip_bool( ZDJ_SETTING_LIB_MENU_SHOW_KEY );

    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * ui_panel_view = (zdj_view_t*)item_state->data.ptr;
    zdj_settings_ui_panel_state_t * ui_panel_state = (zdj_settings_ui_panel_state_t*)ui_panel_view->state;
    ui_panel_state->needs_layout_update = true;
}

static void _handle_show_camelot( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_setting_flip_bool( ZDJ_SETTING_LIB_MENU_SHOW_CAMELOT );

    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * ui_panel_view = (zdj_view_t*)item_state->data.ptr;
    zdj_settings_ui_panel_state_t * ui_panel_state = (zdj_settings_ui_panel_state_t*)ui_panel_view->state;
    ui_panel_state->needs_layout_update = true;
}
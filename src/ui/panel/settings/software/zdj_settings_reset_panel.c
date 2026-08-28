#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/reboot.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/zdj_data_type.h>
#include <zerodj/controls/zdj_controls.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/system/installer/zdj_installer.h>
#include <zerodj/system/registry/zdj_registry.h>
#include <zerodj/system/settings/zdj_settings.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/panel/zdj_ui_panel.h>
#include <zerodj/ui/panel/settings/zdj_settings_panel.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/dialog_view/zdj_dialog_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/menu_section_view/zdj_menu_section_view.h>
#include <zerodj/ui/view/modal_view/zdj_modal_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>


static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_back( zdj_view_t * view );

static void _refresh_menu( zdj_view_t * view );

static void _library_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _prefs_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _mixer_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _usb_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _usb_db_btn( zdj_view_t * view, zdj_control_event_t * _event );

static void _reset_btn( zdj_view_t * view, zdj_control_event_t * _event );
static void _reset_btn_layout_init( zdj_view_t * view );
static void _reset_exit( zdj_view_t * view, void * data, bool selection );

typedef struct {
    bool library;
    bool settings;
    bool usb;
    bool usb_db;
    bool mixer;
    zdj_view_t * menu;
    bool needs_layout_update;
} _zdj_reset_panel_state_t;

zdj_view_t * zdj_new_settings_reset_panel( void ) {
    zdj_view_t * view = zdj_new_modal_view( zdj_modal_rect( ) );
    view->draw = &_draw;
    view->handle_control_event = &_handle_control;
    view->map = ZDJ_CONTROL_MAP_SETTINGS_PANEL;

    _zdj_reset_panel_state_t * state = calloc( 1, sizeof( _zdj_reset_panel_state_t ) );
    state->needs_layout_update = true;
    state->library = true;
    state->usb = false;
    state->usb_db = false;
    state->mixer = false;
    state->settings = false;
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
    _zdj_reset_panel_state_t * state = (_zdj_reset_panel_state_t*)view->state;

    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );

    if( state->needs_layout_update ) { _refresh_menu( view ); }
}


static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {
    // Ignore events which have been blocked by layers above this one.
    if( _event->blocked ) { return; }

    _zdj_reset_panel_state_t * state = (_zdj_reset_panel_state_t*)view->state;
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)state->menu->state;

    if( (_event->id == ZDJ_UI_CONTROL_JOG_RELEASE_0 &&
        menu_state->scroll_index == -1) ||
        _event->id == ZDJ_UI_CONTROL_NAV_RELEASE_0
    ) {
        printf( "app_view back_btn\n" );
        // Dump the top view on the stack (this view)
        zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
        zdj_pop_subview_of( panel_state->settings_panel, true );
        _event->blocked = true;
        // Return immediately since we're being dismissed
        return;
    } else { 
        // Send events down into the subview stack
        state->menu->handle_control_event( state->menu, _event );
    }

    _event->blocked = true;
}

static void _handle_back( zdj_view_t * view ) {
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_pop_subview_of( panel_state->settings_panel, true );
}

static void _refresh_menu( zdj_view_t * view ) {
    _zdj_reset_panel_state_t * state = (_zdj_reset_panel_state_t*)view->state;

    zdj_menu_view_remove_all_subviews( state->menu );

    zdj_view_t * launch = zdj_new_label_view( "SYSTEM RESET", ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    launch->frame.x = 3;
    launch->frame.y = 2;
    zdj_menu_view_add_item( state->menu, launch );

    char str[ 128 ];
    // Build the launch btn title
    if( state->library && 
        state->mixer &&
        state->settings &&
        state->usb &&
        state->usb_db
    ) {
        strcpy( str, "Factory Reset" );
    } else {
        bool needs_plus = false;
        bool is_start = true;
        if( state->library ) { 
            strcpy( str, "Lib" );
            needs_plus = true;
            is_start = false;
        }
        if( state->settings ) {
            if( is_start ) { 
                strcpy( str, "Prefs" );
                is_start = false;
            } else {
                if( needs_plus ) { sprintf( str, "%s+", str ); }
                sprintf( str, "%sPrefs", str );
            }
            needs_plus = true;
        }
        if( state->mixer ) {
            if( is_start ) {
                strcpy( str, "Mix" );
                is_start = false;
            } else {
                if( needs_plus ) { sprintf( str, "%s+", str ); }
                sprintf( str, "%sMix", str );
            }
            needs_plus = true;
        }
        if( state->usb || state->usb_db ) {
            if( is_start ) {
                strcpy( str, "USB" );
                is_start = false;
            } else {
                if( needs_plus ) { sprintf( str, "%s+", str ); }
                sprintf( str, "%sUSB", str );
            }
        }
    }
    
    // Start button
    zdj_view_t * reset_btn = zdj_new_menu_item( "System Reset", ZDJ_MENU_ITEM_LAYOUT_CUSTOM );
    reset_btn->frame.x = 2;
    reset_btn->frame.y = 3;
    reset_btn->frame.h = 20;
    reset_btn->handle_control_event = &_reset_btn;
    zdj_menu_item_view_state_t * reset_state = (zdj_menu_item_view_state_t*)reset_btn->state;
    reset_state->init_layout = _reset_btn_layout_init;
    reset_state->handles_hmi = true;
    strcpy( reset_state->data.c_val, str );
    reset_state->data.ptr = view;
    zdj_menu_view_add_item( state->menu, reset_btn );

    zdj_menu_view_add_padding( state->menu, 3 );
    zdj_view_t * divider = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_MID_H_DIV ], NULL );
    divider->frame.x = 0;
    divider->frame.w = 128;
    zdj_menu_view_add_item( state->menu, divider );
    zdj_menu_view_add_padding( state->menu, 4 );

    zdj_view_t * library_btn = zdj_new_menu_item( "Library DB", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    library_btn->handle_control_event = &_library_btn;
    zdj_menu_item_view_state_t * library_btn_state = (zdj_menu_item_view_state_t*)library_btn->state;
    library_btn_state->data.ptr = view;
    library_btn_state->data.b_val = state->library;
    zdj_menu_view_add_item( state->menu, library_btn );

    zdj_view_t * prefs_btn = zdj_new_menu_item( "User Settings", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    prefs_btn->handle_control_event = &_prefs_btn;
    zdj_menu_item_view_state_t * prefs_btn_state = (zdj_menu_item_view_state_t*)prefs_btn->state;
    prefs_btn_state->data.ptr = view;
    prefs_btn_state->data.b_val = state->settings;
    zdj_menu_view_add_item( state->menu, prefs_btn );

    zdj_view_t * mixer_btn = zdj_new_menu_item( "Mixer Settings", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    mixer_btn->handle_control_event = &_mixer_btn;
    zdj_menu_item_view_state_t * mixer_btn_state = (zdj_menu_item_view_state_t*)mixer_btn->state;
    mixer_btn_state->data.ptr = view;
    mixer_btn_state->data.b_val = state->mixer;
    zdj_menu_view_add_item( state->menu, mixer_btn );

    zdj_view_t * usb_btn = zdj_new_menu_item( "USB State", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    usb_btn->handle_control_event = &_usb_btn;
    zdj_menu_item_view_state_t * usb_btn_state = (zdj_menu_item_view_state_t*)usb_btn->state;
    usb_btn_state->data.ptr = view;
    usb_btn_state->data.b_val = state->usb;
    zdj_menu_view_add_item( state->menu, usb_btn );

    zdj_view_t * usb_db_btn = zdj_new_menu_item( "USB Device DB", ZDJ_MENU_ITEM_LAYOUT_TOGGLE );
    usb_db_btn->handle_control_event = &_usb_db_btn;
    zdj_menu_item_view_state_t * usb_db_btn_state = (zdj_menu_item_view_state_t*)usb_db_btn->state;
    usb_db_btn_state->data.ptr = view;
    usb_db_btn_state->data.b_val = state->usb_db;
    zdj_menu_view_add_item( state->menu, usb_db_btn );


    state->needs_layout_update = false;
}

static void _library_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * panel_view = (zdj_view_t*)item_state->data.ptr;
    _zdj_reset_panel_state_t * panel_state = (_zdj_reset_panel_state_t*)panel_view->state;
    
    panel_state->library = !panel_state->library;
    panel_state->needs_layout_update = true;
}

static void _prefs_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * panel_view = (zdj_view_t*)item_state->data.ptr;
    _zdj_reset_panel_state_t * panel_state = (_zdj_reset_panel_state_t*)panel_view->state;
    
    panel_state->settings = !panel_state->settings;
    panel_state->needs_layout_update = true;
}

static void _mixer_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * panel_view = (zdj_view_t*)item_state->data.ptr;
    _zdj_reset_panel_state_t * panel_state = (_zdj_reset_panel_state_t*)panel_view->state;
    
    panel_state->mixer = !panel_state->mixer;
    panel_state->needs_layout_update = true;
}

static void _usb_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * panel_view = (zdj_view_t*)item_state->data.ptr;
    _zdj_reset_panel_state_t * panel_state = (_zdj_reset_panel_state_t*)panel_view->state;
    
    panel_state->usb = !panel_state->usb;
    panel_state->needs_layout_update = true;
}

static void _usb_db_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * panel_view = (zdj_view_t*)item_state->data.ptr;
    _zdj_reset_panel_state_t * panel_state = (_zdj_reset_panel_state_t*)panel_view->state;
    
    panel_state->usb_db = !panel_state->usb_db;
    panel_state->needs_layout_update = true;
}

static void _reset_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * panel_view = (zdj_view_t*)item_state->data.ptr;
    _zdj_reset_panel_state_t * state = (_zdj_reset_panel_state_t*)panel_view->state;

    // Build the settings string
    char str[ 1024 ];
    // Build the launch btn title
    if( state->library && 
        state->mixer &&
        state->settings &&
        state->usb &&
        state->usb_db
    ) {
        strcpy( str, "Perform Full Factory Reset" );
    } else {
        strcpy( str, "Reset: " );
        bool needs_plus = false;
        if( state->library ) { 
            sprintf( str, "%s Library Database", str );
            needs_plus = true;
        }
        if( state->settings ) {
            if( needs_plus ) { sprintf( str, "%s +", str ); }
            sprintf( str, "%s All User Settings", str );
            needs_plus = true;
        }
        if( state->mixer ) {
            if( needs_plus ) { sprintf( str, "%s +", str ); }
            sprintf( str, "%s Mixer Routing Setup", str );
            needs_plus = true;
        }
        if( state->usb || state->usb_db ) {
            if( needs_plus ) { sprintf( str, "%s +", str ); }
            sprintf( str, "%s USB Settings", str );
        }
    }

    // Pop confirm dialog
     zdj_view_t * dialog = zdj_new_dialog_view( 
        ZDJ_DIALOG_VIEW_TYPE_OKAY_CANCEL,
        "Confirm",
        str,
        "Are you sure?"
    );
    zdj_dialog_view_state_t * dialog_state = (zdj_dialog_view_state_t*)dialog->state;
    dialog_state->handle_dialog_exit = &_reset_exit;
    dialog_state->selection_data = panel_view;

    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_push_subview( panel_state->settings_panel, dialog, true );
}

static void _reset_exit( zdj_view_t * view, void * data, bool selection ) {
    printf( "_reset_exit: %d\n", selection );
    if( selection == true ) {
        zdj_view_t * panel_view = (zdj_view_t*)data;
        _zdj_reset_panel_state_t * panel_state = (_zdj_reset_panel_state_t*)panel_view->state;

        if( panel_state->library ) { 
            printf( "resetting lib\n" );
            zdj_library_reset_db( );
        }
        if( panel_state->settings ) {
            printf( "resetting user settings\n" );
            zdj_drop_settings( );
        }
        if( panel_state->mixer ) {
            printf( "resetting soundcard\n" );
            if( zdj_soundcard ) { zdj_soundcard_deinit( zdj_soundcard ); }
            zdj_drop_soundcard( );
        }

        if( panel_state->usb ) {
            printf( "resetting usb state\n" );
            remove( ZDJ_USB_STATUS_PATH );
            zdj_usb_mode_state_t req;
            memset( &req, 0, sizeof( zdj_usb_mode_state_t ) );
            req.mode = ZDJ_USB_MODE_OFFLINE;
            zdj_usb_enable_mode( &req );
        }
        if( panel_state->usb_db ) {
            printf( "resetting usb device db\n" );
            zdj_usb_reset_devices_db( );
        }
        
        sync( );
        reboot( RB_AUTOBOOT );
    }
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_pop_subview_of( panel_state->settings_panel, true );
    // printf( "_startup_exit done\n" );
}

static void _reset_btn_layout_init( zdj_view_t * view ) {

    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    // Clear out the normal/hilite views' subviews
    if( state->hilite_view ) { 
        zdj_remove_all_subviews_of( state->hilite_view ); 
    } else {
        state->hilite_view = zdj_new_view( NULL );
        zdj_add_subview( view, state->hilite_view );
        state->hilite_view->frame.w = view->frame.w;
        state->hilite_view->frame.h = view->frame.h;
    }
    if( state->normal_view ) { 
        zdj_remove_all_subviews_of( state->normal_view );
    } else {
        state->normal_view = zdj_new_view( NULL );
        zdj_add_subview( view, state->normal_view );
        state->normal_view->frame.w = view->frame.w;
        state->normal_view->frame.h = view->frame.h;
    }

    float btn_w = 0.0;

    // Setup normal view
    zdj_view_t * title = zdj_new_label_view( state->data.c_val, ZDJ_FONT_12_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( state->normal_view, title );
    title->frame.x = 2;
    title->frame.y = 4;
    btn_w += title->frame.w;
    
    // Setup hilite view
    btn_w = fmax( 35, btn_w );
    zdj_view_t * hilite_bg_l = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BIG_ACTION_L ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg_l );
    hilite_bg_l->frame.w = btn_w;
    hilite_bg_l->frame.x = 0;
    hilite_bg_l->frame.h = 20;
    zdj_view_t * hilite_bg_r = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BIG_ACTION_R ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg_r );
    hilite_bg_r->frame.x = btn_w;
    hilite_bg_r->frame.h = 20;

    zdj_view_t * title_hi = zdj_new_label_view( state->data.c_val, ZDJ_FONT_12_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    zdj_add_subview( state->hilite_view, title_hi );
    title_hi->frame.x = 2;
    title_hi->frame.y = 4;

    zdj_view_t * launch_str_hi = zdj_new_label_view( "RUN", ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    zdj_add_subview( state->hilite_view, launch_str_hi );
    launch_str_hi->frame.x = 3;
    launch_str_hi->frame.y = 0;

    state->needs_layout_init = false;
}
#include <stdio.h>
#include <stdbool.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/system/display/zdj_display.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/system/registry/zdj_registry.h>
#include <zerodj/system/settings/zdj_settings.h>
#include <zerodj/system/usb/zdj_usb.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/image_viewer/zdj_image_viewer.h>
#include <zerodj/ui/panel/zdj_ui_panel.h>
#include <zerodj/ui/panel/settings/zdj_settings_panel.h>
#include <zerodj/ui/panel/settings/developer/zdj_settings_developer_panel.h>
#include <zerodj/ui/view/dialog_view/zdj_dialog_view.h>
#include <zerodj/ui/view/file_browser_view/zdj_file_browser_view.h>
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

static void _relaunch_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _usb_offline_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _viewer_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _flip_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _image_browser_exit( zdj_view_t * browser, zdj_file_browser_exit_context_t * context );
static void _lib_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _drop_library_dialog_exit( zdj_view_t * view, void * data, bool selection );
static void _soundcard_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _drop_soundcard_dialog_exit( zdj_view_t * view, void * data, bool selection );
static void _settings_btn( zdj_view_t * view, zdj_control_event_t * event );
static void _drop_settings_dialog_exit( zdj_view_t * view, void * data, bool selection );

zdj_view_t * zdj_new_settings_developer_panel( void (*cb)(void*) ) {
    zdj_view_t * view = zdj_new_modal_view( zdj_modal_rect( ) );
    view->draw = &_draw;
    view->handle_control_event = &_handle_control;
    view->map = ZDJ_CONTROL_MAP_SETTINGS_PANEL;

    zdj_settings_panel_state_t * state = calloc( 1, sizeof( zdj_settings_panel_state_t ) );
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
    zdj_settings_panel_state_t * state = (zdj_settings_panel_state_t*)view->state;

    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );

    if( state->needs_layout_update ) { _refresh_menu( view ); }
}

static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {
    // Ignore events which have been blocked by layers above this one.
    if( _event->blocked ) { return; }

    // Send events down into the subview stack
    zdj_settings_panel_state_t * state = (zdj_settings_panel_state_t*)view->state;
    state->menu->handle_control_event( state->menu, _event );

    _event->blocked = true;
}

static void _handle_back( zdj_view_t * menu_view ) {
    // printf( "_handle_back\n" );
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_pop_subview_of( panel_state->settings_panel, true );
}

static void _refresh_menu( zdj_view_t * view ) {
    zdj_settings_panel_state_t * state = (zdj_settings_panel_state_t*)view->state;

    zdj_menu_view_remove_all_subviews( state->menu );

    bool flag = zdj_setting_get_dev_zerod_flag( );
    if( flag ) {
        zdj_view_t * relaunch_btn = zdj_new_menu_item( "Enable App Relaunch", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
        relaunch_btn->handle_control_event = _relaunch_btn;
        zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)relaunch_btn->state;
        item_state->data.ptr = state;
        zdj_menu_view_add_item( state->menu, relaunch_btn );
    } else {
        zdj_view_t * relaunch_btn = zdj_new_menu_item( "Disable App Relaunch", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
        relaunch_btn->handle_control_event = _relaunch_btn;
        zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)relaunch_btn->state;
        item_state->data.ptr = state;
        zdj_menu_view_add_item( state->menu, relaunch_btn );
    }

    zdj_view_t * usb_offline_btn = zdj_new_menu_item( "USB -> Offline", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    usb_offline_btn->handle_control_event = _usb_offline_btn;
    zdj_menu_view_add_item( state->menu, usb_offline_btn );

    zdj_view_t * viewer_btn = zdj_new_menu_item( "Image Viewer", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    viewer_btn->handle_control_event = _viewer_btn;
    zdj_menu_item_view_state_t * viewer_state = (zdj_menu_item_view_state_t*)viewer_btn->state;
    viewer_state->data.ptr = state;
    zdj_menu_view_add_item( state->menu, viewer_btn );

    zdj_view_t * flip_btn = zdj_new_menu_item( "GoodMolecule Flip", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    flip_btn->handle_control_event = _flip_btn;
    zdj_menu_view_add_item( state->menu, flip_btn );
    
    zdj_view_t * lib_btn = zdj_new_menu_item( "Drop Library Tables", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    lib_btn->handle_control_event = _lib_btn;
    zdj_menu_item_view_state_t * lib_state = (zdj_menu_item_view_state_t*)lib_btn->state;
    lib_state->data.ptr = state;
    zdj_menu_view_add_item( state->menu, lib_btn );

    zdj_view_t * soundcard_btn = zdj_new_menu_item( "Drop Soundcard Tables", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    soundcard_btn->handle_control_event = _soundcard_btn;
    zdj_menu_item_view_state_t * soundcard_state = (zdj_menu_item_view_state_t*)soundcard_btn->state;
    soundcard_state->data.ptr = state;
    zdj_menu_view_add_item( state->menu, soundcard_btn );

    zdj_view_t * settings_btn = zdj_new_menu_item( "Drop Settings Tables", ZDJ_MENU_ITEM_LAYOUT_BASIC_L );
    settings_btn->handle_control_event = _settings_btn;
    zdj_menu_item_view_state_t * settings_state = (zdj_menu_item_view_state_t*)settings_btn->state;
    settings_state->data.ptr = state;
    zdj_menu_view_add_item( state->menu, settings_btn );

    state->needs_layout_update = false;
}

static void _relaunch_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_settings_panel_state_t * panel_state = (zdj_settings_panel_state_t*)state->data.ptr;

    bool flag = zdj_setting_get_dev_zerod_flag( );
    zdj_setting_set_dev_zerod_flag( !flag );

    panel_state->needs_layout_update = true;
}

static void _usb_offline_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    zdj_usb_mode_state_t req;
    memset( &req, 0, sizeof( zdj_usb_mode_state_t ) );
    req.mode = ZDJ_USB_MODE_OFFLINE;
    zdj_usb_enable_mode( &req );
}

static void _flip_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    // Update screen rotation setting
    if( zdj_setting_get( ZDJ_SETTING_DISPLAY_FLIP ) ) {
        zdj_setting_flip_bool( ZDJ_SETTING_DISPLAY_FLIP );
    } else { 
        zdj_setting_set_bool( ZDJ_SETTING_DISPLAY_FLIP, true );
    }
    // zdj_display_flip = zdj_setting_get( ZDJ_SETTING_DISPLAY_FLIP );
    zdj_display_flip = !zdj_display_flip;
    printf( "display flip: %d\n", zdj_display_flip );
}

static void _viewer_btn( zdj_view_t * view, zdj_control_event_t * event ) {
    // Show file browser
    printf( "_viewer_btn\n" );
    zdj_view_t * browser = zdj_new_file_browser_view( 
        zdj_modal_rect( ), "/media/internal/.developer/image_viewer", 
        true, 
        true, 
        ZDJ_FILE_BROWSER_TYPE_SELECT_ANY, 
        "Scan this dir",
        false 
    );
    if( !browser ) {
        printf( "Unable to open browser -- exiting\n" );
        exit( 1 );
    }
    
    // Add a select callback
    zdj_file_browser_view_state_t * browser_state = (zdj_file_browser_view_state_t *)browser->state;
    browser_state->handle_file_browser_exit = &_image_browser_exit;
    // Add the menu to the top of the stack
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_push_subview( panel_state->settings_panel, browser, true );
}

static void _image_browser_exit( zdj_view_t * browser, zdj_file_browser_exit_context_t * context ) {
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;

    if( context->status == ZDJ_FILE_BROWSER_EXIT_STATUS_SELECT ) {
        // // Run Import Path Logic on selected filepath to determine import workflow.
        // zdj_library_import_type_t import_type = zdj_library_get_import_type_for_path( browser_select_path );

        // // Build an import modal to show status of the import sequence.
        // zdj_view_t * import_modal = new_add_music_import( import_type, browser_select_path );

        // // Simply pop if scan_modal fails to create
        // zdj_pop_subview_of( zdj_root_view( ), true );
        // // Push import modal onto menu stack *behind* file browser
        // if( import_modal ) { zdj_push_subview( ui_state->menu_stack, import_modal, true ); }


        // Check for filename on exit context.  If it is empty, we've selected a dir
        zdj_image_viewer_type_t type;
        if( strlen( context->filename ) > 0 ) {
            type = ZDJ_IMAGE_VIEWER_TYPE_FILE;
        } else {
            type = ZDJ_IMAGE_VIEWER_TYPE_DIR;
        }

        // Insert a new image view behind the browser
        zdj_view_t * image_viewer = zdj_new_image_viewer( context->filepath, type );
        
        zdj_add_subview_behind( 
            panel_state->settings_panel, 
            zdj_view_stack_top_subview_of( panel_state->settings_panel ),
            image_viewer
        );
    }

    // Pop the browser
    zdj_pop_subview_of( panel_state->settings_panel, true );
}


static void _lib_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    // Launch drop lib confirm dialog
    zdj_view_t * dialog = zdj_new_dialog_view( 
        ZDJ_DIALOG_VIEW_TYPE_OKAY_CANCEL,
        "Confirm",
        "Remove all library data.",
        "Are you sure?"
    );
    zdj_dialog_view_state_t * dialog_state = (zdj_dialog_view_state_t*)dialog->state;
    dialog_state->handle_dialog_exit = &_drop_library_dialog_exit;
    dialog_state->selection_data = view;
    // zdj_push_subview( zdj_root_view( ), dialog, true );
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_push_subview( panel_state->settings_panel, dialog, true );
}

static void _drop_library_dialog_exit( zdj_view_t * view, void * data, bool selection ) {
    // printf( "_drop_library_dialog_exit %d\n", selection );
    zdj_library_reset_db( );
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_pop_subview_of( panel_state->settings_panel, true );
}

static void _settings_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    // Launch drop lib confirm dialog
    zdj_view_t * dialog = zdj_new_dialog_view( 
        ZDJ_DIALOG_VIEW_TYPE_OKAY_CANCEL,
        "Confirm",
        "Reset all device settings.",
        "Are you sure?"
    );
    zdj_dialog_view_state_t * dialog_state = (zdj_dialog_view_state_t*)dialog->state;
    dialog_state->handle_dialog_exit = &_drop_settings_dialog_exit;
    dialog_state->selection_data = view;
    // zdj_push_subview( zdj_root_view( ), dialog, true );
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_push_subview( panel_state->settings_panel, dialog, true );
}

static void _drop_settings_dialog_exit( zdj_view_t * view, void * data, bool selection ) {
    // printf( "_drop_settings_dialog_exit %d\n", selection );
    zdj_drop_settings( );
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_pop_subview_of( panel_state->settings_panel, true );
}

static void _soundcard_btn( zdj_view_t * view, zdj_control_event_t * _event ) {
    // Launch drop lib confirm dialog
    zdj_view_t * dialog = zdj_new_dialog_view( 
        ZDJ_DIALOG_VIEW_TYPE_OKAY_CANCEL,
        "Confirm",
        "Remove all soundcard setups.",
        "Are you sure?"
    );
    zdj_dialog_view_state_t * dialog_state = (zdj_dialog_view_state_t*)dialog->state;
    dialog_state->handle_dialog_exit = &_drop_soundcard_dialog_exit;
    dialog_state->selection_data = view;
    // zdj_push_subview( zdj_root_view( ), dialog, true );
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_push_subview( panel_state->settings_panel, dialog, true );
}

static void _drop_soundcard_dialog_exit( zdj_view_t * view, void * data, bool selection ) {
    if( selection ) {
        printf( "dropping soundcard\n" );
        zdj_drop_soundcard( );
        printf( "init soundcard\n" );
        zdj_soundcard_init( "__temp__" );
    }
    zdj_panel_state_t * panel_state = (zdj_panel_state_t*)zdj_panel_view( )->state;
    zdj_pop_subview_of( panel_state->settings_panel, true );
}
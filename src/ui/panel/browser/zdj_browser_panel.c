#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/system/usb/zdj_usb.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/panel/zdj_ui_panel.h>
#include <zerodj/ui/panel/browser/zdj_browser_panel.h>
#include <zerodj/ui/view/dialog_view/zdj_dialog_view.h>
#include <zerodj/ui/view/log_view/zdj_log_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/modal_view/zdj_modal_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event );
static void _handle_back( zdj_view_t * menu_view );

static void _delete_file_dialog_exit( zdj_view_t * view, void * data, bool selection );

zdj_view_t * zdj_new_browser_panel( void ) {
    char path[ 256 ];
    strcpy( path, "/media/internal" );

    printf( "zdj_new_file_browser_panel\n" );
    // Check path and fail before we do anything
    if( access( path, F_OK ) != 0 ) { return NULL; }

    zdj_view_t * browser_view = zdj_new_modal_view( zdj_modal_rect( ) );
    browser_view->type = ZDJ_VIEW_BROWSER;
    browser_view->draw = &_draw;
    browser_view->handle_control_event = _handle_control;
    browser_view->map = ZDJ_CONTROL_MAP_SETTINGS_PANEL;

    // Add a state instance
    zdj_browser_panel_state_t * state = calloc( 1, sizeof( zdj_browser_panel_state_t ) );
    strcpy( state->path, path );
    state->read_only = true;
    state->allow_nav = true;
    state->usb_host_counter = 0;
    state->show_hidden = false;
    strcpy( state->select_dir_title, "test" );
    browser_view->state = state;

    // Add header view
    zdj_view_t * header = zdj_new_menu_header( 
        "Browser",
        path,
        ZDJ_MENU_HEADER_STYLE_NORMAL,
        ZDJ_MENU_HEADER_BACK_STYLE_NONE
    );
    state->header_view = header;
    header->frame.y = 0;
    header->frame.w = browser_view->frame.w;
    header->frame.h = 5;
    zdj_add_subview( browser_view, header );

    // Add the menu container view
    zdj_rect_t container_frame = { 0, 5, browser_view->frame.w, browser_view->frame.h };
    zdj_view_t * menu_container = zdj_new_view( &container_frame );
    state->menu_container = menu_container;
    zdj_add_subview( browser_view, menu_container );

    state->devices_menu = zdj_new_browser_panel_device_menu( browser_view, zdj_modal_rect( ) );
    if( state->devices_menu ){ zdj_add_subview( menu_container, state->devices_menu ); }

    // printf( "zdj_new_file_browser_view done\n" );

    state->overlay = zdj_ui_panel_new_overlay( "Files" );
    zdj_add_subview( browser_view, state->overlay );
    
    return browser_view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // printf( "browser_panel _draw\n" );
    zdj_browser_panel_state_t * state = (zdj_browser_panel_state_t*)view->state;

    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );

    if( state->overlay_counter > 0 ) { state->overlay->frame.x = 0; state->overlay_counter--; }
    else { state->overlay->frame.x = 129; }

    // Update the devices menu when panel controller sets needs_layout_update
    if( state->needs_layout_update ) {
        zdj_browser_panel_refresh_devices_menu( view, state->devices_menu );
        state->needs_layout_update = false;
    }

    // Update the devices menu when the set of attached devices changes
    if( zdj_usb_state->mode_state.mode == ZDJ_USB_MODE_HOST &&
        zdj_usb_state->host_state.has_browser_panel_update 
    ) {
        zdj_usb_state->host_state.has_browser_panel_update = false;
        zdj_browser_panel_refresh_devices_menu( view, state->devices_menu );
    }
}

static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {
    // printf( "browser _handle_control: %d\n", _event->id );
    zdj_control_event_t * e = (zdj_control_event_t *)_event;
    zdj_browser_panel_state_t * browser_state = (zdj_browser_panel_state_t *)view->state;
    zdj_view_t * header_view = browser_state->header_view;

    // Bug out early if there's no top menu
    zdj_view_t * top_menu = zdj_view_stack_top_subview_of( browser_state->menu_container );
    if( !top_menu ) { return; }

    zdj_menu_view_state_t * top_menu_state = (zdj_menu_view_state_t*)top_menu->state;
    // Grab a scroll_index pre- and post- hmi event handler.
    // We'll use this to show/hide the browser back button.
    int menu_scroll_index = top_menu_state->scroll_index;

    // Pass events down into the menu view stack.
    // Note that browser/subviews may be deleted during handle_control_event.
    // Be careful accessing them after this line.
    top_menu->handle_control_event( top_menu, _event );

    // Prevent views/menus below this one from getting jog wheel events
    e->blocked = true;

    // printf( "browser _handle_control done\n" );
}

static void _handle_back( zdj_view_t * menu_view ) {
    printf( "_handle_back\n" );
    // zdj_ui_panel_toggle( );
}

// Callback from a single menu_item within a menu.
void zdj_browser_panel_item_hmi_delegate( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * browser;
    zdj_browser_panel_state_t * browser_state;  
    
    zdj_view_t * dialog;
    zdj_dialog_view_state_t * dialog_state;

    switch ( item_state->action ) {
        case ZDJ_MENU_ITEM_ACTION_DIR_BACK:
            // Note that the back button carries a pointer to the browser in its data struct
            if( item_state->data.ptr ) {
                // If there's no menu behind the current one, we'll need to create it.
                // It will be either a parent directory, or the device list if we're 
                // already at the root of /media/internal
                browser = item_state->data.ptr;
                browser_state = (zdj_browser_panel_state_t*)browser->state;
                browser_state->is_device_menu = false;

                zdj_view_t * current_menu = zdj_view_stack_top_subview_of( browser_state->menu_container );
                if( !current_menu->prev ) { // cur menu has no previous menu
                    if( !strcmp( "/media", item_state->link ) ) {
                        if( zdj_usb_state->mode_state.mode == ZDJ_USB_MODE_HOST ) {
                            // Make a new device menu
                            zdj_view_t * new_menu = zdj_new_browser_panel_device_menu( browser, &(zdj_rect_t){0, 10, browser->frame.w, browser->frame.h-10} );
                            // Insert it behind the current menu
                            zdj_push_subview_behind( browser_state->menu_container, current_menu, new_menu, true );
                            // Set device menu to true so we track device attach/detach
                            browser_state->is_device_menu = true;
                        }
                    } else {
                        printf( "browser dir_back: %s\n", item_state->link );
                        // Make a new file menu
                        zdj_view_t * new_menu = zdj_new_browser_panel_file_menu_for_path( 
                            browser, 
                            &(zdj_rect_t){0, 10, browser->frame.w, browser->frame.h-10}, 
                            item_state->link, 
                            browser_state->select_dir_title
                        );
                        // Insert it behind the current menu
                        zdj_push_subview_behind( browser_state->menu_container, current_menu, new_menu, true );
                    }
                } else {
                    // Pop to the menu below
                    zdj_pop_subview_of( browser_state->menu_container, true );
                }
            }
            break;
        case ZDJ_MENU_ITEM_ACTION_DIR_ENTER: // push menu w/path
            // printf( "browser dir_enter: %s\n", item_state->link );
            browser = item_state->data.ptr;
            browser_state = (zdj_browser_panel_state_t*)browser->state;
            browser_state->is_device_menu = false;
            zdj_view_t * new_menu = zdj_new_browser_panel_file_menu_for_path( 
                browser, 
                &(zdj_rect_t){0, 10, browser->frame.w, browser->frame.h-10}, 
                item_state->link, 
                browser_state->select_dir_title
            );
            zdj_push_subview( browser_state->menu_container, new_menu, true );
            break;
        case ZDJ_MENU_ITEM_ACTION_DIR_SELECT: // exit browser w/dir path
            // printf( "browser dir_select: %s\n", item_state->link );
            browser = (zdj_view_t*)item_state->data.ptr;
            if( !browser ){ break; }
            browser_state = (zdj_browser_panel_state_t*)browser->state;
            browser_state->is_device_menu = false;

            break;
        case ZDJ_MENU_ITEM_ACTION_FILE_SELECT: // exit browser w/file path
            browser = (zdj_view_t*)item_state->data.ptr;
            if( !browser ){ break; }
            browser_state = (zdj_browser_panel_state_t*)browser->state;
            browser_state->is_device_menu = false;
            
            break;
        case ZDJ_MENU_ITEM_ACTION_FILE_OPEN: // detect file type and push viewer
            // printf( "opening:%s\n", item_state->link );
            browser = (zdj_view_t*)item_state->data.ptr;
            if( !browser ){ break; }
            browser_state = (zdj_browser_panel_state_t*)browser->state;

            // Detect file type and open appropriate view
            // Image, audio, cat, etc.
            if( zdj_fs_path_is_logfile( item_state->link ) ) {
                zdj_view_t * log_view = zdj_new_log_view( 
                    item_state->link, 
                    ZDJ_LOG_VIEW_TYPE_LOG,
                    browser_state->menu_container,
                    zdj_modal_rect( )
                );
                zdj_push_subview( browser_state->menu_container, log_view, true );
            } else if( zdj_fs_path_is_image_filename( item_state->link ) ) {
                
            } else if( zdj_fs_path_is_audio_filename( item_state->link ) ) {
                
            } else {
                zdj_view_t * log_view = zdj_new_log_view( 
                    item_state->link, 
                    ZDJ_LOG_VIEW_TYPE_CAT,
                    browser_state->menu_container,
                    zdj_modal_rect( )
                );
                zdj_push_subview( browser_state->menu_container, log_view, true );
            }

            
            break;
        case ZDJ_MENU_ITEM_ACTION_FILE_DELETE: // detect file type and push viewer
            // printf( "deleting:%s, %p\n", item_state->link, item_state->data.ptr );
            browser = (zdj_view_t*)item_state->data.ptr;
            if( !browser ){ break; }
            browser_state = (zdj_browser_panel_state_t*)browser->state;
            strcpy( browser_state->selected_file_path, item_state->link );

            // push delete dialog
            dialog = zdj_new_dialog_view( 
                ZDJ_DIALOG_VIEW_TYPE_OKAY,
                "Confirm",
                "Delete File?",
                browser_state->selected_file_path
            );
            dialog_state = (zdj_dialog_view_state_t*)dialog->state;
            dialog_state->handle_dialog_exit = &_delete_file_dialog_exit;
            dialog_state->selection_data = browser;
            zdj_push_subview( browser_state->menu_container, dialog, true );
            break;
        default:
            break;
    }
}

static void _delete_file_dialog_exit( zdj_view_t * view, void * data, bool selection ) {
    // printf( "_drop_library_dialog_exit %d\n", selection );
    if( selection ) {
        zdj_view_t * browser = (zdj_view_t*)data;
        if( browser ) {
            zdj_browser_panel_state_t * browser_state = (zdj_browser_panel_state_t*)browser->state;
            remove( browser_state->selected_file_path );
            strcpy( browser_state->selected_file_path, " " );
            zdj_pop_subview_of( browser_state->menu_container, true );
        }
    }
}
#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/usb/zdj_usb.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/file_browser_view/zdj_file_browser_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _handle_control( zdj_view_t * view, zdj_control_event_t * _event );
static void _deinit_state( zdj_view_t * view );

// void _zdj_file_browser_handle_cancel_btn( zdj_view_t * view );

zdj_view_t * zdj_new_file_browser_view( 
    zdj_rect_t * frame, 
    char * path,
    bool read_only,
    bool allow_nav,
    zdj_file_browser_type_t type,
    char * select_dir_title,
    bool show_hidden
) {
    // printf( "zdj_new_file_browser_view\n" );
    // Check path and fail before we do anything
    if( access( path, F_OK ) != 0 ) { return NULL; }

    zdj_view_t * browser_view = zdj_new_view( frame );
    browser_view->type = ZDJ_VIEW_BROWSER;
    browser_view->draw = &_draw;
    browser_view->handle_control_event = _handle_control;
    browser_view->deinit_state = &_deinit_state;
    zdj_set_anim( &browser_view->in_anim, ZDJ_ANIM_MODAL_SHOW );
    zdj_set_anim( &browser_view->out_anim, ZDJ_ANIM_MODAL_HIDE );

    browser_view->frame.x = ZDJ_MODAL_X;
    browser_view->frame.y = ZDJ_SCREEN_H+1;
    browser_view->frame.w = ZDJ_MODAL_WIDTH;

    // Add a state instance
    zdj_file_browser_view_state_t * state = calloc( 1, sizeof( zdj_file_browser_view_state_t ) );
    strcpy( state->path, path );
    state->read_only = read_only;
    state->allow_nav = allow_nav;
    state->type = type;
    state->usb_host_counter = 0;
    state->show_hidden = show_hidden;
    if( select_dir_title ){ strcpy( state->select_dir_title, select_dir_title ); }
    browser_view->state = state;

    // Add header view
    zdj_view_t * header = zdj_new_menu_header( 
        "Browser",
        path,
        ZDJ_MENU_HEADER_STYLE_NORMAL,
        ZDJ_MENU_HEADER_BACK_STYLE_BACK
    );
    state->header_view = header;
    header->frame.y = 0;
    header->frame.w = browser_view->frame.w;
    header->frame.h = 5;
    zdj_add_subview( browser_view, header );

    // Add the menu container view
    zdj_rect_t container_frame = { 0, 5, frame->w, frame->h };
    zdj_view_t * menu_container = zdj_new_view( &container_frame );
    state->menu_container = menu_container;
    zdj_add_subview( browser_view, menu_container );

    // Add first menu to stack
    zdj_view_t * menu = zdj_new_file_browser_menu_for_path( 
        browser_view,
        &(zdj_rect_t){frame->x, frame->y, ZDJ_MODAL_WIDTH, frame->h-9}, 
        state->path, 
        state->select_dir_title
    );
    if( menu ) { zdj_push_subview( menu_container, menu, false ); }

    // printf( "zdj_new_file_browser_view done\n" );

    return browser_view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // printf( "browser _draw\n" );
    zdj_file_browser_view_state_t * state = (zdj_file_browser_view_state_t*)view->state;
    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );

    // if( zdj_usb_state->mode_state.mode == ZDJ_USB_MODE_HOST ) {
    //     state->usb_host_counter++;
    //     state->usb_host_counter %= 100;

    //     // if( state->usb_host_counter == 0 ) { printf( "file browser host-mode check\n" ); }

    //     // Check for attach/detach of USB drive
    //     // if ( state->usb_host_counter == 0 && zdj_usb_host_has_devices_update( ) ) {
    //     if ( state->usb_host_counter == 0 && zdj_usb_state->host_state.has_file_browser_update ) {
    //         // If we're looking at the devices menu, refresh the menu
    //         if( state->is_device_menu ) {
    //             // printf( "file browser host-mode device update\n" );
    //             // state.menu_stack->top_subview().needs_refresh = true

    //         // If we're looking at a file menu,
    //         } else {
    //             // if we're inside an external drive, and the drive is no longer available,
    //             // if( zdj_fs_path_is_external( xxx ) && !access( path ) ) {
    //             // Assume -the drive has been detached, force the browser modal to close.
    //             // }
    //         }

    //     }
    // }
    // Update the devices menu when the set of attached devices changes
    if( state->is_device_menu &&
        zdj_usb_state->mode_state.mode == ZDJ_USB_MODE_HOST &&
        zdj_usb_state->host_state.has_file_browser_update 
    ) {
        zdj_usb_state->host_state.has_file_browser_update = false;
        zdj_refresh_device_browser_menu( view, state->devices_menu );
    }
    // printf( "browser _draw done\n" );
}

// Pass appropriate hmi events down into the top menu_view
static void _handle_control( zdj_view_t * browser, zdj_control_event_t * _event ) {
    // printf( "browser _handle_control\n" );
    zdj_control_event_t * e = (zdj_control_event_t *)_event;
    zdj_file_browser_view_state_t * browser_state = (zdj_file_browser_view_state_t *)browser->state;
    zdj_view_t * header_view = browser_state->header_view;

    // Bug out early if there's no top menu
    zdj_view_t * top_menu = zdj_view_stack_top_subview_of( browser_state->menu_container );
    if( !top_menu ) { return; }

    // Handle a cancel button press
    zdj_menu_view_state_t * top_menu_state = (zdj_menu_view_state_t*)top_menu->state;
    if( (top_menu_state->scroll_index == -1 && e->id == ZDJ_UI_CONTROL_JOG_RELEASE_0) ||
         e->id == ZDJ_UI_CONTROL_NAV_RELEASE_0
    ) {
        zdj_file_browser_exit_context_t exit_context = { ZDJ_FILE_BROWSER_EXIT_STATUS_CANCEL };
        browser_state->handle_file_browser_exit( browser, &exit_context );
        e->blocked = true;
        return;
    }

    // Grab a scroll_index pre- and post- hmi event handler.
    // We'll use this to show/hide the browser back button.
    int menu_scroll_index = top_menu_state->scroll_index;

    // Pass events down into the menu view stack.
    // Note that browser/subviews may be deleted during handle_control_event.
    // Be careful accessing them after this line.
    top_menu->handle_control_event( top_menu, _event );

    // Show/hide header's cancel button
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)header_view->state;
    
    if( e->id == ZDJ_UI_CONTROL_JOG_ADJUST_0 &&
        header_state
    ) {
        if( top_menu_state->scroll_index == -1 &&
            header_state->back_hidden
        ) {
            header_state->show_back = true;
        } else if( 
            top_menu_state->scroll_index == 0 &&
            !header_state->back_hidden 
        ) {
            header_state->hide_back = true;
        }
    }

    // Prevent views/menus below this one from getting jog wheel events
    e->blocked = true;

    // printf( "browser _handle_control done\n" );
}

static void _deinit_state( zdj_view_t * view ) {
    zdj_menu_view_state_t * state = (zdj_menu_view_state_t*)view->state;
    free( state );
    view->state = NULL;
}

// Callback from a single menu_item within a menu.
void zdj_file_browser_item_hmi_delegate( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * browser;
    zdj_file_browser_view_state_t * browser_state;    

    switch ( item_state->action ) {
        case ZDJ_MENU_ITEM_ACTION_DIR_BACK:
            // Note that the back button carries a pointer to the browser in its data struct
            if( item_state->data.ptr ) {
                // If there's no menu behind the current one, we'll need to create it.
                // It will be either a parent directory, or the device list if we're 
                // already at the root of /media/internal
                browser = item_state->data.ptr;
                browser_state = (zdj_file_browser_view_state_t*)browser->state;
                browser_state->is_device_menu = false;

                zdj_view_t * current_menu = zdj_view_stack_top_subview_of( browser_state->menu_container );
                if( !current_menu->prev ) { // cur menu has no previous menu
                    if( !strcmp( "/media", item_state->link ) ) {
                        // Make a new device menu
                        browser_state->devices_menu = zdj_new_device_browser_menu( browser, &(zdj_rect_t){0, 10, browser->frame.w, browser->frame.h-10} );
                        // Insert it behind the current menu
                        zdj_push_subview_behind( browser_state->menu_container, current_menu, browser_state->devices_menu, true );
                        
                        // Set device menu to true so we track device attach/detach
                        browser_state->is_device_menu = true;
                    } else {
                        printf( "browser dir_back: %s\n", item_state->link );
                        // Make a new file menu
                        zdj_view_t * new_menu = zdj_new_file_browser_menu_for_path( 
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
            printf( "browser dir_enter: %s\n", item_state->link );
            browser = item_state->data.ptr;
            browser_state = (zdj_file_browser_view_state_t*)browser->state;
            browser_state->is_device_menu = false;
            zdj_view_t * new_menu = zdj_new_file_browser_menu_for_path( 
                browser, 
                &(zdj_rect_t){0, 10, browser->frame.w, browser->frame.h-10}, 
                item_state->link, 
                browser_state->select_dir_title
            );
            zdj_push_subview( browser_state->menu_container, new_menu, true );
            break;
        case ZDJ_MENU_ITEM_ACTION_DIR_SELECT: // exit browser w/dir path
        printf( "browser dir_select: %s\n", item_state->link );
            browser = (zdj_view_t*)item_state->data.ptr;
            if( !browser ){ break; }
            browser_state = (zdj_file_browser_view_state_t*)browser->state;
            browser_state->is_device_menu = false;
            if( browser_state->handle_file_browser_exit ) {
                zdj_file_browser_exit_context_t exit_context = { ZDJ_FILE_BROWSER_EXIT_STATUS_SELECT };
                strcpy( exit_context.dir, item_state->link );
                strcpy( exit_context.filepath, item_state->link );
                browser_state->handle_file_browser_exit( browser, &exit_context );
            }
            break;
        case ZDJ_MENU_ITEM_ACTION_FILE_SELECT: // exit browser w/file path
            browser = (zdj_view_t*)item_state->data.ptr;
            if( !browser ){ break; }
            browser_state = (zdj_file_browser_view_state_t*)browser->state;
            browser_state->is_device_menu = false;
            if( browser_state->handle_file_browser_exit ) {
                zdj_file_browser_exit_context_t exit_context = { ZDJ_FILE_BROWSER_EXIT_STATUS_SELECT };
                strcpy( exit_context.dir, browser_state->path );
                strcpy( exit_context.filename, basename( item_state->link ) );
                strcpy( exit_context.filepath, item_state->link );
                browser_state->handle_file_browser_exit( browser, &exit_context );
            }
            break;
        default:
            break;
    }
}
#include <stdio.h>
#include <dirent.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/hmi/zdj_hmi.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/file_browser_view/zdj_file_browser_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

void _zdj_file_browser_draw( zdj_view_t * view, zdj_view_clip_t * clip );
void _zdj_file_browser_handle_hmi( zdj_view_t * view, void * _event );
void _zdj_file_browser_deinit_state( zdj_view_t * view );

// void _zdj_file_browser_handle_cancel_btn( zdj_view_t * view );

zdj_view_t * zdj_new_file_browser_view( 
    zdj_rect_t * frame, 
    char * path,
    bool read_only,
    bool allow_nav,
    zdj_file_browser_select_type_t select_type 
) {
    zdj_view_t * browser_view = zdj_new_view( frame );
    browser_view->draw = &_zdj_file_browser_draw;
    browser_view->handle_hmi_event = _zdj_file_browser_handle_hmi;
    browser_view->deinit_state = &_zdj_file_browser_deinit_state;

    // Add a state instance
    zdj_file_browser_view_state_t * state = calloc( 1, sizeof( zdj_file_browser_view_state_t ) );
    state->read_only = read_only;
    state->allow_nav = allow_nav;
    browser_view->state = state;

    // Add header view
    zdj_view_t * header = zdj_new_menu_header( 
        "Browser",
        path,
        ZDJ_MENU_HEADER_STYLE_NORMAL,
        ZDJ_MENU_HEADER_BACK_STYLE_CANCEL
    );
    state->header_view = header;
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)header->state;
    zdj_add_subview( browser_view, header );
    header->frame->w = browser_view->frame->w;
    header->frame->h = 10;

    // Add first menu to stack
    zdj_view_t * menu = zdj_new_file_browser_menu_for_path( 
        browser_view,
        &(zdj_rect_t){frame->x, frame->y+10, frame->w, frame->h-10}, 
        path, 
        read_only, 
        allow_nav, 
        select_type 
    );
    zdj_add_subview( browser_view, menu );

    return browser_view;
}

void _zdj_file_browser_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF000000 );
}

// Pass appropriate hmi events down into the top menu_view
void _zdj_file_browser_handle_hmi( zdj_view_t * browser, void * _event ) {
    zdj_hmi_event_t * e = (zdj_hmi_event_t *)_event;
    zdj_file_browser_view_state_t * browser_state = (zdj_file_browser_view_state_t *)browser->state;
    zdj_view_t * header_view = browser_state->header_view;
    // Handle all the jog-wheel stuff (scroll, mod scroll, press/long press, etc.)
    if( e->id == ZDJ_HMI_ENCO_2_JOG ) {
        // Catch a jog-press when menu scroll_index == -1
        zdj_view_t * top_menu = zdj_view_stack_top_subview_of( browser );

        if( top_menu ) {
            zdj_menu_view_state_t * top_menu_state = (zdj_menu_view_state_t*)top_menu->state;
            
            // Show hide header's cancel button
            zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)header_view->state;
            if( browser_state->header_view && top_menu_state->scroll_index == -1 ) {
                header_state->show_back = true;
                header_state->has_valid_display = false;
            } else {
                header_state->hide_back = true;
                header_state->has_valid_display = false;
            }

            // Handle a cancel button press - return immediately since we're being dismissed
            if( top_menu_state->scroll_index == -1 && e->type == ZDJ_HMI_EVENT_RELEASE ) {
                zdj_file_browser_exit_context_t exit_context = {
                    ZDJ_FILE_BROWSER_EXIT_STATUS_CANCEL, NULL
                };
                browser_state->handle_file_browser_exit( browser, &exit_context );
                e->blocked = true;
                return;
            }

            // Pass events down into the menu view stack.
            // Note that browser/subviews may be deleted during handle_hmi_event.
            // Be careful accessing them after this line.
            top_menu->handle_hmi_event( top_menu, _event );
        }
        
        // Prevent views/menus below this one from getting jog wheel events
        e->blocked = true;
    }
}

void _zdj_file_browser_deinit_state( zdj_view_t * view ) {
    zdj_menu_view_state_t * state = (zdj_menu_view_state_t*)view->state;
    free( state );
    view->state = NULL;
}

// Callback from a single menu_item within a menu.
void zdj_file_browser_item_hmi_delegate( zdj_view_t * view, void * _event ) {
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * browser;
    zdj_file_browser_view_state_t * browser_state;
    switch ( item_state->action ) {
        case ZDJ_MENU_ITEM_ACTION_DIR_BACK:
            // Note that the back button carries a pointer to the browser in its data struct
            if( item_state->link && item_state->data->ptr ) {
                // If there's no menu behind the current one, we'll need to create it.
                // It will be either a parent directory, or the device list if we're 
                // already at the root of /media/internal
                browser = item_state->data->ptr;
                browser_state = (zdj_file_browser_view_state_t*)browser->state;
                zdj_view_t * current_menu = zdj_view_stack_top_subview_of( browser );
                if( current_menu->prev && current_menu->prev->type != ZDJ_VIEW_MENU ) { // cur menu has no previous menu
                    if( !strcmp( "/media", item_state->link ) ) {
                        // Make a new device menu
                        zdj_view_t * new_menu = zdj_new_device_browser_menu( browser, &(zdj_rect_t){0, 10, browser->frame->w, browser->frame->h-10} );
                        // Insert it behind the current menu
                        zdj_add_subview_below( browser, current_menu, new_menu );
                    } else {
                        // Make a new file menu
                        zdj_view_t * new_menu = zdj_new_file_browser_menu_for_path( 
                            browser, 
                            &(zdj_rect_t){0, 10, browser->frame->w, browser->frame->h-10}, 
                            strdup( item_state->link ), 
                            browser_state->read_only, 
                            browser_state->allow_nav, 
                            browser_state->select_type 
                        );
                        // Insert it behind the current menu
                        zdj_add_subview_below( browser, current_menu, new_menu );
                    }
                }
                // Pop to the menu below
                zdj_pop_subview_of( browser );
            }
            break;
        case ZDJ_MENU_ITEM_ACTION_DIR_ENTER: // push menu w/path
            browser = item_state->data->ptr;
            browser_state = (zdj_file_browser_view_state_t*)browser->state;
            zdj_view_t * new_menu = zdj_new_file_browser_menu_for_path( 
                browser, 
                &(zdj_rect_t){0, 10, browser->frame->w, browser->frame->h-10}, 
                strdup( item_state->link ), 
                browser_state->read_only, 
                browser_state->allow_nav, 
                browser_state->select_type 
            );
            zdj_add_subview( browser, new_menu );
            break;
        case ZDJ_MENU_ITEM_ACTION_DIR_SELECT: // exit browser w/dir path
        case ZDJ_MENU_ITEM_ACTION_FILE_SELECT: // exit browser w/file path
            browser = item_state->data->ptr;
            browser_state = (zdj_file_browser_view_state_t*)browser->state;
            if( browser_state->handle_file_browser_exit ) {
                zdj_file_browser_exit_context_t exit_context = {
                    ZDJ_FILE_BROWSER_EXIT_STATUS_SELECT, item_state->link
                };
                browser_state->handle_file_browser_exit( browser, &exit_context );
            }
            break;
        default:
            break;
    }
}
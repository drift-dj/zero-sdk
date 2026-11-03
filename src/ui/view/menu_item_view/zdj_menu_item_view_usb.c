#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>

void zdj_menu_item_usb_device_init_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    view->frame.h = 16;
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

    // Setup normal view
    zdj_usb_device_t * device = (zdj_usb_device_t*)state->data.ptr;

    zdj_view_t * title = zdj_new_label_view( device->name_user, ZDJ_FONT_TINY5, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( view, title );
    // if( title_norm->frame.w < view->frame.w ) {
    //     title_norm->frame.x =  (view->frame.w / 2) - (title_norm->frame.w / 2);
    // }
    title->frame.x = 5;

    float _y = 12.0;
    if( device->has_msd ) {
        zdj_view_t * icon = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_USB_GADGET_MSD ], NULL );
        zdj_add_subview( view, icon );
        icon->frame.x =  10;
        icon->frame.y = _y;

        // zdj_view_t * path = zdj_new_label_view( device->mount_path, ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
        zdj_view_t * path = zdj_new_label_view( "/media/usb0", ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
        zdj_add_subview( view, path );
        path->frame.x = 28;
        path->frame.y = _y;

        _y = _y + 12.0;
        view->frame.h = view->frame.h + 12.0;
    }
    
    if( device->has_audio ) {
        zdj_view_t * icon = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_USB_GADGET_AUDIO ], NULL );
        zdj_add_subview( view, icon );
        icon->frame.x =  10;
        icon->frame.y = _y;

        char audio_str[ 64 ];
        if( device->has_audio_in && device->has_audio_out ) { 
            strcpy( audio_str, "Audio Input + Output" );
        } else if( device->has_audio_in ) {
            strcpy( audio_str, "Audio Input" );
        } else if( device->has_audio_out ) {
            strcpy( audio_str, "Audio Output" );
        }
        zdj_view_t * path = zdj_new_label_view( audio_str, ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
        zdj_add_subview( view, path );
        path->frame.x = 28;
        path->frame.y = _y + 1;

        _y += 14;
        view->frame.h = view->frame.h + 14;
    }

    if( device->has_midi ) {
        zdj_view_t * icon = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_USB_GADGET_MIDI ], NULL );
        zdj_add_subview( view, icon );
        icon->frame.x =  10;
        icon->frame.y = _y;

        char midi_str[ 64 ];
        if( device->has_midi_in && device->has_midi_out ) { 
            strcpy( midi_str, "MIDI Input + Output" );
        } else if( device->has_midi_in ) {
            strcpy( midi_str, "MIDI Input" );
        } else if( device->has_midi_out ) {
            strcpy( midi_str, "MIDI Output" );
        }
        zdj_view_t * path = zdj_new_label_view( midi_str, ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
        zdj_add_subview( view, path );
        path->frame.x = 28;
        path->frame.y = _y + 1;

        _y += 14;
        view->frame.h = view->frame.h + 14;
    }
    state->needs_layout_init = false;
}
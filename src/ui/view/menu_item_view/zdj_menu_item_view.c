#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

void _zdj_menu_item_draw( zdj_view_t * view, zdj_view_clip_t * clip );
void _zdj_menu_item_hmi_event( zdj_view_t * view, zdj_control_event_t * _event );
void _zdj_menu_item_deinit_state( zdj_view_t * view );

void _zdj_menu_item_set_hilite( zdj_menu_item_view_state_t * state, zdj_view_clip_t * clip, bool hilite );

// Parse a static XML node into a menu_item_view.
zdj_view_t * zdj_new_menu_item( char * title, zdj_menu_item_view_layout_t layout ) {
    // Make view
    zdj_view_t * menu_item = zdj_new_view( NULL );
    menu_item->type = ZDJ_VIEW_MENU_ITEM;
    menu_item->draw = &_zdj_menu_item_draw;
    menu_item->deinit_state = &_zdj_menu_item_deinit_state;

    // Build state
    zdj_menu_item_view_state_t * state = calloc( 1, sizeof( zdj_menu_item_view_state_t ) );
    menu_item->state = (void*)state;
    if( title ) { strcpy( state->title, title ); } else { strcpy( state->title, "Undefined" ); }
    state->layout = layout;
    state->data.ptr = NULL;
    state->owned_ptr = NULL;
    state->needs_layout_init = true;
    state->init_layout = NULL;
    state->needs_layout_update = false;
    state->update_layout = NULL;
    state->is_blinking = false;
    state->blink_timer = 0;
    state->blink_length = zdj_ui_msec_to_frames( 100 );
    state->blink_period = zdj_ui_msec_to_frames( 50 );
    state->blink_duty = state->blink_period / 2;
    state->handles_hmi = false;
    state->edit_enabled = false;
    zdj_menu_item_set_layout( menu_item, state->layout );

    return menu_item;
}

// Special init for icon+asset menu_items which set icon asset after creation.
zdj_view_t * zdj_new_icon_menu_item( 
    char * title, 
    zdj_ui_asset_t icon,
    zdj_ui_asset_t icon_hi 
) {
    zdj_view_t * menu_item = zdj_new_menu_item( title, ZDJ_MENU_ITEM_LAYOUT_ICON );
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)menu_item->state;
    item_state->icon = icon;
    item_state->icon_hi = icon_hi;
    return menu_item;
}

zdj_view_t * zdj_new_asset_menu_item( 
    zdj_ui_asset_t icon,
    zdj_ui_asset_t icon_hi,
    bool hide_normal
) {
    zdj_view_t * menu_item = zdj_new_menu_item( " ", ZDJ_MENU_ITEM_LAYOUT_ASSET );
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)menu_item->state;
    if( !hide_normal ) { item_state->icon = icon; }
    item_state->icon_hi = icon_hi;
    return menu_item;
}

zdj_view_t * zdj_new_data_menu_item( 
    char * title, 
    zdj_menu_item_view_layout_t layout,
    zdj_menu_item_data_display_type_t data_type,
    char * prefix,
    char * suffix
) {
    zdj_view_t * menu_item = zdj_new_menu_item( title, layout );
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)menu_item->state;
    item_state->data_type = data_type;
    if( prefix ) { strcpy( item_state->data_prefix, prefix ); } 
    else { item_state->data_prefix[ 0 ] = '\0'; }
    if( suffix ) { strcpy( item_state->data_suffix, suffix ); } 
    else { item_state->data_suffix[ 0 ] = '\0'; }
    return menu_item;
}

zdj_view_t * zdj_new_cuepoint_menu_item( char * name, char * cuepoint_eid ) {
    zdj_view_t * menu_item = zdj_new_menu_item( name, ZDJ_MENU_ITEM_LAYOUT_CUEPOINT );
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)menu_item->state;
    strcpy( item_state->title, name );
    strcpy( item_state->data.c_val, cuepoint_eid );
    return menu_item;
}

zdj_view_t * zdj_new_usb_device_menu_item( zdj_usb_device_t * device_dto ) {
    zdj_view_t * menu_item = zdj_new_menu_item( "USB", ZDJ_MENU_ITEM_LAYOUT_USB_DEVICE );
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)menu_item->state;
    // strcpy( item_state->title, device_dto->name_user );
    // strcpy( item_state->data.c_val, device_dto->name_user );
    item_state->data.ptr = device_dto;
    return menu_item;
}

zdj_view_t * zdj_new_browser_device_menu_item( 
    char * title,
    char * path, 
    zdj_menu_item_view_browser_device_type_t type 
) {
    zdj_view_t * menu_item = zdj_new_menu_item( title, ZDJ_MENU_ITEM_LAYOUT_BROWSER_DEVICE );
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)menu_item->state;
    strcpy( item_state->link, path );
    item_state->data.i_val = type;
    return menu_item;
}

zdj_view_t * zdj_new_song_menu_item( 
    // zdj_library_song_t * song,
    zdj_library_menu_row_t * menu_row,
    bool show_title_and_artist,
    bool show_key,
    bool show_camelot,
    bool show_bpm,
    zdj_menu_item_view_layout_t layout
 ) {
    if( !menu_row ) { return NULL; }
    char label[ 256 ];
    if( show_title_and_artist ) {
        snprintf( label, sizeof( label ), "%s - %s", menu_row->artist, menu_row->title );
    } else {
        strcpy( label, menu_row->title );
    }
    zdj_view_t * menu_item = zdj_new_menu_item( label, layout );
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)menu_item->state;
    
    item_state->owned_ptr = menu_row;
    // Use i_val as bitfield
    // Pack show/hide bpm/key/cam states, BPM val, and key val into i_val bitfield
    item_state->data.i_val = 0;
    if( show_camelot ) {
        // Store song key in i_val as bitfield
        item_state->data.i_val += ( (show_camelot & 0x1) << 1 );
    }
    if( show_key ) {
        // Store song key in i_val as bitfield
        item_state->data.i_val += ( show_key & 0x1 );
    }
    if( show_key || show_camelot ) {
        item_state->data.i_val += ( (menu_row->key & 0xFF) << 8 );
        // printf( "song key:%d i_val: %d\n", song->performance->key, item_state->data.i_val );
    }
    if( show_bpm && menu_row->bpm > zdj_eps ) {
        item_state->data.i_val += ( (show_bpm & 0x1) << 2 );
        int bpm = round( menu_row->bpm );
        item_state->data.i_val += ( (bpm & 0xFF) << 16 );
    }
    if( menu_row->has_error ) {
        item_state->data.i_val += ( 0x1 << 3 );
    }
    return menu_item;
}

// March through any view's subviews, looking for a menu_item_view
// with a matching scroll_index.
zdj_view_t * zdj_menu_item_for_scroll_index( zdj_view_t * view, int index ) {
    zdj_view_t * subview = view->subviews;
    while( subview ) {
        if( subview->type == ZDJ_VIEW_MENU_ITEM ) { 
            // Only interested in menu_items
            zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)subview->state;
            if( state->scroll_index == index ) {
                // Exit the loop if we find our view
                return subview;
            }
        }
        subview = subview->next;
    }
    return NULL;
}

// Update the selected option in an editiable menu item
void zdj_menu_item_scroll_options( zdj_view_t * item, int dir ) {
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;
    item_state->edit_option_index += dir * 0.2;
    if( item_state->edit_option_index < 0.0 ) { item_state->edit_option_index = 0.0; }
    double edit_option_count;
    switch (item_state->edit_options_type ) {
        case ZDJ_MENU_ITEM_OPTIONS_LIB_PLAYLIST:
            edit_option_count = 10;
            break;
        case ZDJ_MENU_ITEM_OPTIONS_DJ_PLAYLIST:
            edit_option_count = 11;
            break;
        case ZDJ_MENU_ITEM_OPTIONS_FILE:
            edit_option_count = 11;
            break;
        default:
            edit_option_count = 0;
            break;
    }
    if( item_state->edit_option_index > edit_option_count ) { item_state->edit_option_index = edit_option_count; }
    // printf( "zdj_menu_item_scroll_options: %d %1.1f %1.0f / %1.0f\n", dir, item_state->edit_option_index, round( item_state->edit_option_index ), edit_option_count );
    item_state->needs_layout_update = true;
}

void zdj_menu_item_enter_move_mode( zdj_view_t * item ) {
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;

    item_state->edit_active = true;
    item_state->needs_layout_update = true;
    item_state->edit_action = ZDJ_MENU_ITEM_ACTION_END_MOVE;
}

void zdj_menu_item_exit_move_mode( zdj_view_t * item ) {
    printf( "zdj_menu_item_exit_move_mode\n" );
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;

    if( item_state->exit_edit_mode ) {
        item_state->exit_edit_mode( item );
    }
}

// Get a matching draw function for a menu_item node's layout attribute
void _zdj_menu_item_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // debug bg
    // boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF990000 );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    // if( state->update_data ) { state->update_data( state ); }
    if( state->needs_layout_init && state->init_layout ) { state->init_layout( view ); }
    if( state->needs_layout_update && state->update_layout ) { state->update_layout( view ); }
    
    if( state->is_blinking ) {
        // if( state->blink_timer++ > ZDJ_BLINK_LENGTH ) {
        if( state->blink_timer++ > state->blink_length ) {
            state->is_blinking = false;
            state->blink_timer = 0;
            state->is_hilite = true;
        } else {
            // blink on a cycle
            if( (state->blink_timer % state->blink_period) >= state->blink_duty ) {
                _zdj_menu_item_set_hilite( state, clip, false );
            } else {
                _zdj_menu_item_set_hilite( state, clip, true );
            }
        }
    } else {
        if( state->handles_hmi ) {
            _zdj_menu_item_set_hilite( state, clip, state->is_hilite );
        }
    }
}

void _zdj_menu_item_set_hilite( zdj_menu_item_view_state_t * state, zdj_view_clip_t * clip, bool hilite ) {
    if( !state->normal_view || !state->hilite_view ) { 
        // Fail silently if hilite views aren't set up properly
        printf( "state hilite views bad\n" );
        return;
    } else {
        if( hilite ) {
            state->normal_view->frame.y = -100;
            state->hilite_view->frame.y = 0;
        } else {
            state->normal_view->frame.y = 0;
            state->hilite_view->frame.y = -100;
        }
    }
}

void _zdj_menu_item_deinit_state( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    if( state->owned_ptr ){ free( state->owned_ptr ); }
    free( state );
    view->state = NULL;
}

void zdj_menu_item_set_layout( zdj_view_t * menu_item, zdj_menu_item_view_layout_t layout ) {
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)menu_item->state;
    switch ( layout ) {
        case ZDJ_MENU_ITEM_LAYOUT_BASIC_L:
            item_state->init_layout = zdj_menu_item_basic_l_init_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_BASIC_R:
            item_state->init_layout = zdj_menu_item_basic_r_init_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_BASIC_LG:
            item_state->init_layout = zdj_menu_item_basic_lg_init_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_BASIC_LAUNCH_R:
            item_state->init_layout = zdj_menu_item_basic_launch_r_init_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_BASIC_L_EDIT:
            item_state->init_layout = zdj_menu_item_basic_l_edit_init_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_BASIC_MED_L_EDIT:
            item_state->init_layout = zdj_menu_item_basic_med_l_edit_init_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_DATA_L:
            item_state->init_layout = zdj_menu_item_data_l_init_layout;
            item_state->update_layout = zdj_menu_item_data_l_update_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_DATA_R:
            item_state->init_layout = zdj_menu_item_data_r_init_layout;
            item_state->update_layout = zdj_menu_item_data_r_update_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_DIR:
            item_state->init_layout = zdj_menu_item_dir_init_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_DIR_SELECT:
            item_state->init_layout = zdj_menu_item_dir_select_init_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_DIR_UP:
            item_state->init_layout = zdj_menu_item_dir_up_init_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_FILE:
            item_state->init_layout = zdj_menu_item_file_init_layout;
            item_state->update_layout = zdj_menu_item_file_update_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_ICON:
            item_state->init_layout = zdj_menu_item_icon_init_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_INERT:
            item_state->init_layout = zdj_menu_item_inert_init_layout;
            item_state->handles_hmi = false;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_INERT_DATA:
            item_state->init_layout = zdj_menu_item_inert_data_init_layout;
            item_state->update_layout = zdj_menu_item_inert_data_update_layout;
            item_state->handles_hmi = false;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_INERT_STATUS:
            item_state->init_layout = zdj_menu_item_inert_status_init_layout;
            item_state->update_layout = zdj_menu_item_inert_status_update_layout;
            item_state->handles_hmi = false;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_LAUNCH_BIG:
            item_state->init_layout = zdj_menu_item_launch_big_init_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_LAUNCH_SM:
            item_state->init_layout = zdj_menu_item_launch_sm_init_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_PLAYLIST:
            item_state->init_layout = zdj_menu_item_playlist_init_layout;
            item_state->update_layout = zdj_menu_item_playlist_update_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_SLIDER:
            item_state->init_layout = zdj_menu_item_slider_init_layout;
            item_state->update_layout = zdj_menu_item_slider_update_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_SONG:
            item_state->init_layout = zdj_menu_item_song_init_layout;
            item_state->update_layout = zdj_menu_item_song_update_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_SONG_PLAYLIST:
            item_state->init_layout = zdj_menu_item_song_init_layout;
            item_state->update_layout = zdj_menu_item_song_playlist_update_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_SONG_IMPORT:
            item_state->init_layout = zdj_menu_item_song_import_init_layout;
            item_state->update_layout = zdj_menu_item_song_import_update_layout;
            item_state->handles_hmi = false;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_TOGGLE:
            item_state->init_layout = zdj_menu_item_toggle_init_layout;
            item_state->update_layout = zdj_menu_item_toggle_update_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_ASSET:
            item_state->init_layout = zdj_menu_item_asset_init_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_CUEPOINT:
            item_state->init_layout = zdj_menu_item_cuepoint_init_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_USB_DEVICE:
            item_state->init_layout = zdj_menu_item_usb_device_init_layout;
            item_state->handles_hmi = true;
            break;
        case ZDJ_MENU_ITEM_LAYOUT_BROWSER_DEVICE:
            item_state->init_layout = zdj_menu_item_browser_device_init_layout;
            item_state->handles_hmi = true;
            break;
    }
}
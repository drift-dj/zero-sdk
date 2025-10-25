#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

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
    state->needs_layout_init = true;
    state->init_layout = NULL;
    state->needs_layout_update = false;
    state->update_layout = NULL;
    state->is_blinking = false;
    state->blink_timer = 0;
    state->handles_hmi = false;
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

// March through any view's subviews, looking for a menu_item_view
// with a matching scroll_index.
zdj_view_t * zdj_menu_item_for_scroll_index( zdj_view_t * view, int index ) {
    // printf( "zdj_menu_item_for_scroll_index: %d\n", index );
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

// Get a matching draw function for a menu_item node's layout attribute
void _zdj_menu_item_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // debug bg
    // boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFF990000 );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    // if( state->update_data ) { state->update_data( state ); }
    if( state->needs_layout_init && state->init_layout ) { state->init_layout( view ); }
    if( state->needs_layout_update && state->update_layout ) { state->update_layout( view ); }
    
    if( state->is_blinking ) {
        if( state->blink_timer++ > ZDJ_BLINK_LENGTH ) {
            state->is_blinking = false;
            state->blink_timer = 0;
            state->is_hilite = true;
        } else {
            // blink on a cycle
            if( state->blink_timer % ZDJ_BLINK_PERIOD > ZDJ_BLINK_DUTY ) {
                _zdj_menu_item_set_hilite( state, clip, true );
            } else {
                _zdj_menu_item_set_hilite( state, clip, false );
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
        case ZDJ_MENU_ITEM_LAYOUT_BASIC_LAUNCH_R:
            item_state->init_layout = zdj_menu_item_basic_launch_r_init_layout;
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
        case ZDJ_MENU_ITEM_LAYOUT_SLIDER:
            item_state->init_layout = zdj_menu_item_slider_init_layout;
            item_state->update_layout = zdj_menu_item_slider_update_layout;
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
    }
}
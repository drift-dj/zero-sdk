#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/battery_icon_view/zdj_battery_icon_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

void _zdj_menu_header_draw( zdj_view_t * view, zdj_view_clip_t * clip );
void _zdj_menu_header_update_layout( zdj_view_t * view, zdj_view_clip_t * clip );
void _zdj_menu_header_deinit_state( zdj_view_t * view );

// Parse a static XML node into a menu_item_view.
zdj_view_t * zdj_new_menu_header( 
    char * name, 
    char * title, 
    zdj_menu_view_header_style_t style, 
    zdj_menu_view_header_back_style_t back_btn_style 
) {
    // Make view
    zdj_view_t * menu_header = zdj_new_view( NULL );
    menu_header->type = ZDJ_VIEW_MENU_HEADER;
    menu_header->draw = &_zdj_menu_header_draw;
    menu_header->deinit_state = &_zdj_menu_header_deinit_state;
    
    // Build state
    zdj_menu_header_view_state_t * state = calloc( 1, sizeof( zdj_menu_header_view_state_t ) );
    menu_header->state = state;
    strcpy( state->name, name );
    strcpy( state->title, title );
    state->style = style;
    state->back_style = back_btn_style;
    state->has_back = (back_btn_style > ZDJ_MENU_HEADER_BACK_STYLE_NONE) ? true : false;
    state->show_back = false;
    state->hide_back = false;
    state->back_hidden = true;
    state->has_valid_display = false;

    // Add back btn show/hide anims
    switch ( state->back_style ) {
        case ZDJ_MENU_HEADER_BACK_STYLE_BACK:
            zdj_set_anim( &menu_header->in_anim, ZDJ_ANIM_HEADER_BACK_ACTIVATE );
            zdj_set_anim( &menu_header->out_anim, ZDJ_ANIM_HEADER_BACK_DEACTIVATE );
            break;
        case ZDJ_MENU_HEADER_BACK_STYLE_CANCEL:
        case ZDJ_MENU_HEADER_BACK_STYLE_EXIT:
            zdj_set_anim( &menu_header->in_anim, ZDJ_ANIM_HEADER_CLOSE_ACTIVATE );
            zdj_set_anim( &menu_header->out_anim, ZDJ_ANIM_HEADER_CLOSE_DEACTIVATE );
            break;
        default:
            break;
    }
    
    return menu_header;
}

void _zdj_menu_header_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_menu_header_view_state_t * state = (zdj_menu_header_view_state_t*)view->state;

    // BG
    // boxColor( zdj_renderer( ), clip->dst.x+6, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h-1, ZDJ_WHITE );

    if( !state->has_valid_display ) {
        _zdj_menu_header_update_layout( view, clip );
    }

    // If show_back is requested by menu system, animate back button into view
    if( state->show_back && state->back_hidden ) {
        // activate header in_anim
        ((anim_init_t)view->in_anim.init_fn)( &view->in_anim, view );
        view->anim = &view->in_anim;
        state->show_back = false;
        state->back_hidden = false;
    }
    
    // If hide_back is requested by menu system, animate back button out of view
    if( state->hide_back && !state->back_hidden ) {
        // activate header out_anim
        ((anim_init_t)view->out_anim.init_fn)( &view->out_anim, view );
        view->anim = &view->out_anim;
        state->hide_back = false;
        state->back_hidden = true;
    }

    // Run the blink cycle
    if( state->is_blinking ) {
        if( state->blink_timer++ > ZDJ_BLINK_LENGTH ) {
            // exit blink state after time
            state->is_blinking = false;
            state->blink_timer = 0;
        } else {
            // blink on a duty cycle
            if( state->blink_timer % ZDJ_BLINK_PERIOD > ZDJ_BLINK_DUTY ) {
                boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFFFF0000 );
            }
        }
    }
}

void _zdj_menu_header_deinit_state( zdj_view_t * view ) {
    zdj_menu_header_view_state_t * state = (zdj_menu_header_view_state_t*)view->state;
    free( state );
    view->state = NULL;
}

void _zdj_menu_header_update_layout( zdj_view_t * header, zdj_view_clip_t * clip ) {
    zdj_menu_header_view_state_t * state = (zdj_menu_header_view_state_t*)header->state;

    // Remove all subviews
    zdj_remove_all_subviews_of( header ); 

    // Add bar
    state->bar = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_WHITE ], NULL );
    state->bar->frame.x = 0;
    state->bar->frame.y = 0;
    state->bar->frame.w = header->frame.w;
    state->bar->frame.h = 5;
    zdj_add_subview( header, state->bar );

    if( state->has_back ){
        if( state->back_style == ZDJ_MENU_HEADER_BACK_STYLE_BACK ) {
            state->back_view = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HEADER_ARROWS ], NULL );
            state->back_view->frame.x = -14;
            state->back_view->frame.y = 0;
            state->back_view->frame.w = 14;
            zdj_add_subview( header, state->back_view );
        } else if( state->back_style == ZDJ_MENU_HEADER_BACK_STYLE_CANCEL || 
                   state->back_style == ZDJ_MENU_HEADER_BACK_STYLE_EXIT
        ) {
            state->back_view = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HEADER_X ], NULL );
            state->back_view->frame.x = 0;
            state->back_view->frame.y = 6;
            zdj_add_subview( header, state->back_view );
        }
    }

    // Add Battery Icon
    state->battery_view = zdj_new_battery_icon_view( ZDJ_BATTERY_ICON_TYPE_SMALL );
    zdj_add_subview( header, state->battery_view );
    state->battery_view->frame.x = header->frame.w - 11;

    state->has_valid_display = true;
}
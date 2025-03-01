#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
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
    menu_header->draw = &_zdj_menu_header_draw;
    menu_header->deinit_state = &_zdj_menu_header_deinit_state;
    
    // Build state
    zdj_menu_header_view_state_t * state = calloc( 1, sizeof( zdj_menu_header_view_state_t ) );
    menu_header->state = state;
    state->name = strdup( name );
    state->title = strdup( title );
    state->style = style;
    state->back_style = back_btn_style;
    state->has_back = (back_btn_style > ZDJ_MENU_HEADER_BACK_STYLE_NONE) ? true : false;
    state->has_valid_display = false;
    
    return menu_header;
}

void _zdj_menu_header_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_menu_header_view_state_t * state = (zdj_menu_header_view_state_t*)view->state;

    // BG
    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_WHITE );

    if( !state->has_valid_display ) {
        _zdj_menu_header_update_layout( view, clip );
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
    free( state->name );
    free( state->title );
    free( state );
    view->state = NULL;
}

void _zdj_menu_header_update_layout( zdj_view_t * header, zdj_view_clip_t * clip ) {
    zdj_menu_header_view_state_t * state = (zdj_menu_header_view_state_t*)header->state;

    // Remove all subviews
    zdj_remove_subviews_of( header ); 

    // Setup name label
    zdj_view_t * name_label = zdj_new_label_view( state->name, ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    state->name_label = name_label;
    zdj_add_subview( header, name_label );

    // Setup title divider
    // zdj_view_t * title_divider = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DOT_PIPE_HI ], NULL );
    zdj_view_t * title_divider = zdj_new_asset_view( &(SDL_Rect){13,81,1,6}, NULL );
    state->title_divider = title_divider;
    zdj_add_subview( header, title_divider );

    // Setup title ticker
    zdj_view_t * title_ticker = zdj_new_ticker_view( state->title, ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    state->title_ticker = title_ticker;
    zdj_add_subview( header, title_ticker );
    state->title_ticker->frame->h = 10;

    // Setup back button if needed
    if( state->has_back ) {
        zdj_view_t * back_view;
        switch ( state->back_style ) {
        case ZDJ_MENU_HEADER_BACK_STYLE_BACK:
            back_view = zdj_new_label_view( "<BACK", ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
            break;
        case ZDJ_MENU_HEADER_BACK_STYLE_CANCEL:
            back_view = zdj_new_label_view( "<CANCEL", ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
            break;
        default:
            break;
        }
        back_view->frame->x = -100;
        state->back_view = back_view;
        zdj_add_subview( header, back_view );
    }

    state->name_label->frame->x = 1;
    state->title_divider->frame->x = state->name_label->frame->w + 3;
    state->title_divider->frame->y = 1;
    state->title_ticker->frame->x = state->title_divider->frame->x + 4;
    state->title_ticker->frame->w = header->frame->w - state->title_ticker->frame->x;

    // If show_back is requested by menu system, animate back button into view
    if( state->show_back ) {
        state->name_label->frame->x = -100;
        state->back_view->frame->x = 1;
        state->title_divider->frame->x = state->back_view->frame->w + 2;
        state->title_ticker->frame->x = state->title_divider->frame->x + 4;
        state->title_ticker->frame->w = header->frame->w - state->title_ticker->frame->x;
        state->show_back = false;
    }
    
    // If hide_back is requested by menu system, animate back button out of view
    if( state->hide_back ) {
        state->name_label->frame->x = 1;
        state->back_view->frame->x = -100;
        state->title_divider->frame->x = state->name_label->frame->w + 2;
        state->title_ticker->frame->x = state->title_divider->frame->x + 4;
        state->title_ticker->frame->w = header->frame->w - state->title_ticker->frame->x;
        state->hide_back = false;
    }

    state->has_valid_display = true;
}
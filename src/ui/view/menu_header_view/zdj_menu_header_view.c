#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

void _zdj_menu_header_draw( zdj_view_t * view, zdj_view_clip_t * clip );
void _zdj_menu_header_deinit_state( zdj_view_t * view );

// Parse a static XML node into a menu_item_view.
zdj_view_t * zdj_new_menu_header( 
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
    menu_header->state = (void*)state;
    state->style = style;
    state->back_style = back_btn_style;
    if( back_btn_style > ZDJ_MENU_HEADER_BACK_STYLE_NONE ) {
        state->has_back = true;
    }

    // Setup title ticker
    zdj_view_t * title_ticker = zdj_new_ticker_view( title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    state->title_ticker = title_ticker;
    zdj_add_subview( menu_header, title_ticker );

    // Setup back button if needed
    if( state->has_back ) {
        zdj_view_t * back_view = zdj_new_label_view( "< BACK", ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
        back_view->frame->x = -22;
        state->back_view = back_view;
        zdj_add_subview( menu_header, back_view );
    }
    
    return menu_header;
}

void _zdj_menu_header_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_menu_header_view_state_t * state = (zdj_menu_header_view_state_t*)view->state;

    // BG
    boxColor( zdj_renderer( ), clip->src.x, clip->src.y, clip->src.x+clip->src.w, clip->src.y+clip->src.h, ZDJ_BLACK );
    
    // Put down h-div
    zdj_ui_asset_draw( ZDJ_UI_ASSET_WIDE_H_DIV, &(zdj_rect_t){0, 8, 128, 1} );
    
    state->title_ticker->frame->w = view->frame->w - 27;
    state->title_ticker->frame->h = 10;

    // If show_back is requested by menu system, animate back button into view
    if( state->show_back ) {
        state->back_view->frame->x = 0;
        state->title_ticker->frame->x = 27;
        state->show_back = false;
    }
    
    // If hide_back is requested by menu system, animate back button out of view
    if( state->hide_back ) {
        state->back_view->frame->x = -22;
        state->title_ticker->frame->x = 0;
        state->hide_back = false;
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
    free( state->title );
    free( state );
    view->state = NULL;
}
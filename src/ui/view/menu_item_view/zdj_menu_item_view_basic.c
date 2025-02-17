#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>

void _zdj_menu_item_view_basic_set_hilite( zdj_menu_item_view_state_t * state, zdj_view_clip_t * clip, bool hilite );

void zdj_menu_item_draw_basic_l( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    if( !state->has_valid_display ) {
        zdj_remove_subviews( view );
        // Setup title ticker
        state->title_ticker = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
        zdj_add_subview( view, state->title_ticker );
        state->title_ticker_hi = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
        zdj_add_subview( view, state->title_ticker_hi );
        state->has_valid_display = true;
    }

    if( state->is_blinking ) {
        if( state->blink_timer++ > ZDJ_BLINK_LENGTH ) {
            // exit blink state after time
            state->is_blinking = false;
            state->blink_timer = 0;
            state->is_hilite = false;
        } else {
            // blink on a cycle
            if( state->blink_timer % ZDJ_BLINK_PERIOD > ZDJ_BLINK_DUTY ) {
                _zdj_menu_item_view_basic_set_hilite( state, clip, true );
            } else {
                _zdj_menu_item_view_basic_set_hilite( state, clip, false );
            }
        }
    } else {
        _zdj_menu_item_view_basic_set_hilite( state, clip, state->is_hilite );
    }
    
    if( state->title_ticker ) {
        state->title_ticker->frame->x = ZDJ_MENU_ITEM_MARGIN_L;
        state->title_ticker->frame->w = view->frame->w - ZDJ_MENU_ITEM_MARGIN_L;
        state->title_ticker->frame->h = view->frame->h;
    }
    if( state->title_ticker_hi ) {
        state->title_ticker_hi->frame->x = ZDJ_MENU_ITEM_MARGIN_L + 1;
        state->title_ticker_hi->frame->w = view->frame->w - ZDJ_MENU_ITEM_MARGIN_L;
        state->title_ticker_hi->frame->h = view->frame->h;
    }
}

void zdj_menu_item_draw_basic_r( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    if( !state->has_valid_display ) {
        zdj_remove_subviews( view );
        // Setup title ticker
        state->title_ticker = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE );
        zdj_add_subview( view, state->title_ticker );
        state->title_ticker_hi = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_BLACK );
        zdj_add_subview( view, state->title_ticker_hi );
        state->has_valid_display = true;
    }
    
    if( state->is_blinking ) {
        if( state->blink_timer++ > ZDJ_BLINK_LENGTH ) {
            // exit blink state after time
            state->is_blinking = false;
            state->blink_timer = 0;
            state->is_hilite = false;
        } else {
            // blink on a cycle
            if( state->blink_timer % ZDJ_BLINK_PERIOD > ZDJ_BLINK_DUTY ) {
                _zdj_menu_item_view_basic_set_hilite( state, clip, true );
            } else {
                _zdj_menu_item_view_basic_set_hilite( state, clip, false );
            }
        }
    } else {
        _zdj_menu_item_view_basic_set_hilite( state, clip, state->is_hilite );
    }
    
    if( state->title_ticker ) {
        state->title_ticker->frame->x = 0;
        state->title_ticker->frame->w = view->frame->w - ZDJ_MENU_ITEM_MARGIN_R;
        state->title_ticker->frame->h = view->frame->h;
    }
    if( state->title_ticker_hi ) {
        state->title_ticker_hi->frame->x = 0;
        state->title_ticker_hi->frame->w = view->frame->w - ZDJ_MENU_ITEM_MARGIN_R - 1;
        state->title_ticker_hi->frame->h = view->frame->h;
    }
}

void _zdj_menu_item_view_basic_set_hilite( zdj_menu_item_view_state_t * state, zdj_view_clip_t * clip, bool hilite ) {
    if( hilite ) {
        boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFFFF0000 );
        if( state->title_ticker ) { state->title_ticker->frame->y = -10; }
        if( state->title_ticker_hi ) { state->title_ticker_hi->frame->y = 0; }
    } else {
        if( state->title_ticker ) { state->title_ticker->frame->y = 0; }
        if( state->title_ticker_hi ) { state->title_ticker_hi->frame->y = -10; }
    }
}
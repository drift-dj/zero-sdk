#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>

// void zdj_menu_item_draw_toggle_l( zdj_view_t * view, zdj_view_clip_t * clip ) {
//     zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

//     if( !state->has_valid_display ) {
//         zdj_remove_subviews( view );
//         // Setup title ticker
//         state->title_ticker = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
//         zdj_add_subview( view, state->title_ticker );
//         state->has_valid_display = true;
//     }

//     if( state->is_blinking ) {
//         if( state->blink_timer++ > 20 ) {
//             // exit blink state after time
//             state->is_blinking = false;
//             state->blink_timer = 0;
//             state->is_hilite = false;
//         } else {
//             // blink on a cycle
//             if( state->blink_timer % 7 > 3 ) {
//                 boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFFFF0000 );
//             }
//         }
//     } else if( state->is_hilite ) {
//         boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFFFF0000 );
//     }
    
//     if( state->title_ticker ) {
//         state->title_ticker->frame->x = 0;
//         state->title_ticker->frame->y = 0;
//         state->title_ticker->frame->w = view->frame->w;
//         state->title_ticker->frame->h = view->frame->h;
//     }
// }

// void zdj_menu_item_draw_toggle_r( zdj_view_t * view, zdj_view_clip_t * clip ) {
//     zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

//     if( !state->has_valid_display ) {
//         zdj_remove_subviews( view );
//         // Setup title ticker
//         state->title_ticker = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE );
//         zdj_add_subview( view, state->title_ticker );
//         state->has_valid_display = true;
//     }
    
//     if( state->is_blinking ) {
//         if( state->blink_timer++ > 20 ) {
//             // exit blink state after time
//             state->is_blinking = false;
//             state->blink_timer = 0;
//             state->is_hilite = false;
//         } else {
//             // blink on a cycle
//             if( state->blink_timer % 7 > 3 ) {
//                 boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFFFF0000 );
//             }
//         }
//     } else if( state->is_hilite ) {
//         boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFFFF0000 );
//     }
    
//     if( state->title_ticker ) {
//         state->title_ticker->frame->x = 0;
//         state->title_ticker->frame->y = 0;
//         state->title_ticker->frame->w = view->frame->w;
//         state->title_ticker->frame->h = view->frame->h;
//     }
// }
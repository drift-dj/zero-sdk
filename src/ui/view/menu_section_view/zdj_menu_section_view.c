#include <stdio.h>
#include <string.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/menu_section_view/zdj_menu_section_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

void _zdj_menu_section_deinit_state( zdj_view_t * view );

// Parse a static XML node into a menu_item_view.
zdj_view_t * zdj_new_menu_section( char * title ) {
    // Make view
    zdj_view_t * menu_section = zdj_new_view( NULL );
    menu_section->draw = &zdj_menu_section_static_draw;
    menu_section->deinit_state = &_zdj_menu_section_deinit_state;
    // Build state
    zdj_menu_section_view_state_t * state = calloc( 1, sizeof( zdj_menu_section_view_state_t ) );
    menu_section->state = (void*)state;

    // Build ticker
    zdj_view_t * title_ticker = zdj_new_ticker_view( title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    state->title_ticker = title_ticker;
    zdj_add_subview( menu_section, title_ticker );

    // Build divider asset
    
    return menu_section;
}

void zdj_menu_section_static_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_menu_section_view_state_t * state = (zdj_menu_section_view_state_t*)view->state;

    // Update divider asset's frame based on view's frame/ticker width

    // Update ticker's frame based on view's frame
    if( state->title_ticker ) {
        state->title_ticker->frame->x = 0;
        state->title_ticker->frame->y = 0;
        state->title_ticker->frame->w = view->frame->w-10;
        state->title_ticker->frame->h = view->frame->h;
    }
}

void _zdj_menu_section_deinit_state( zdj_view_t * view ) {
    zdj_menu_section_view_state_t * state = (zdj_menu_section_view_state_t*)view->state;
    free( state );
    view->state = NULL;
}
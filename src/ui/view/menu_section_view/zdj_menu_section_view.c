#include <stdio.h>
#include <string.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
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
    zdj_view_t * title_ticker = zdj_new_ticker_view( title, ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    state->title_ticker = title_ticker;
    zdj_add_subview( menu_section, title_ticker );

    // Build divider asset
    zdj_view_t * divider = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_WIDE_H_DIV ], NULL );
    state->divider = divider;
    zdj_add_subview( menu_section, divider );
    divider->frame->h = 1;
    divider->frame->y = 5;
    
    return menu_section;
}

void zdj_menu_section_static_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_menu_section_view_state_t * state = (zdj_menu_section_view_state_t*)view->state;

    // Update ticker's frame based on view's frame
    if( state->title_ticker ) {
        state->title_ticker->frame->x = 0;
        state->title_ticker->frame->y = 0;
        state->title_ticker->frame->w = view->frame->w-10;
        state->title_ticker->frame->h = view->frame->h;

        // Update divider asset's frame based on view's frame/ticker width
        if( state->divider ) {
            int ticker_w = zdj_ticker_view_get_text_w( state->title_ticker );
            state->divider->frame->w = view->frame->w - ticker_w - 2;
            state->divider->frame->x = ticker_w + 2;
        }
    }
}

void _zdj_menu_section_deinit_state( zdj_view_t * view ) {
    zdj_menu_section_view_state_t * state = (zdj_menu_section_view_state_t*)view->state;
    free( state );
    view->state = NULL;
}
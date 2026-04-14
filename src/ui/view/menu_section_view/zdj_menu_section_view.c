#include <stdio.h>
#include <string.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_section_view/zdj_menu_section_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_menu_section( char * title ) {

    // Make view
    zdj_view_t * menu_section = zdj_new_view( NULL );
    menu_section->type = ZDJ_VIEW_MENU_SECTION;
    menu_section->draw = &zdj_menu_section_static_draw;
    menu_section->deinit_state = &_deinit_state;
    
    // Build state
    zdj_menu_section_view_state_t * state = calloc( 1, sizeof( zdj_menu_section_view_state_t ) );
    menu_section->state = (void*)state;

    // Build ticker
    // zdj_view_t * title_ticker = zdj_new_ticker_view( title, ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    // state->title_ticker = title_ticker;
    // zdj_add_subview( menu_section, title_ticker );

    // // Build BG
    // zdj_view_t * bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_SM_HATCH_TEX ], NULL );
    // state->bg = bg;
    // zdj_add_subview( menu_section, bg );
    // bg->frame.h = 9;

    // // Build text block BG
    // zdj_view_t * text_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BLACK ], NULL );
    // state->bg = text_bg;
    // zdj_add_subview( menu_section, text_bg );
    // text_bg->frame.h = 5;
    // text_bg->frame.y = 1;

    // printf( "%p %s\n", title, title );
    char str[256];
    if( strlen( title ) > 0 ) {
        strcpy( str, title );
    } else {
        strcpy( str, "No Artist" );
    }
    zdj_view_t * title_norm = zdj_new_label_view( str, ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    state->title_norm = title_norm;
    // title_norm->frame.x = 3;
    // title_norm->frame.y = -1;
    zdj_add_subview( menu_section, title_norm );

    // zdj_label_state_t * title_norm_state = (zdj_label_state_t*)title_norm->state;
    // text_bg->frame.w = title_norm_state->tex_w + 2;
    // text_bg->frame.h = 7;
    // text_bg->frame.x = 2; 

    // Build divider asset
    zdj_view_t * divider = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_WIDE_H_DIV ], NULL );
    state->divider = divider;
    zdj_add_subview( menu_section, divider );
    divider->frame.h = 1;
    divider->frame.y = 6;
    
    return menu_section;
}

void zdj_menu_section_static_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_menu_section_view_state_t * state = (zdj_menu_section_view_state_t*)view->state;

    // Update ticker's frame based on view's frame
    if( state->title_norm ) {
        state->title_norm->frame.x = 1;
        // state->title_norm->frame.x = fmax( view->frame.w - state->title_norm->frame.w, 10 );
        // state->title_norm->frame.y = 0;

        // // Update divider asset's frame based on view's frame/ticker width
        if( state->divider ) {
        //     // int ticker_w = zdj_ticker_view_get_text_w( state->title_ticker );
        //     // int ticker_w = state->title_norm->frame.w;
        //     // state->divider->frame.w = view->frame.w - ticker_w + 5;
        //     // state->divider->frame.x = ticker_w - 5;

            state->divider->frame.w = state->title_norm->frame.x - 2;
        }


    }
}

static void _deinit_state( zdj_view_t * view ) {
    zdj_menu_section_view_state_t * state = (zdj_menu_section_view_state_t*)view->state;
    free( state );
    view->state = NULL;
}
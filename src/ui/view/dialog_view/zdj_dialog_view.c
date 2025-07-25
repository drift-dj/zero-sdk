#include <stdio.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/controls/hmi/zdj_hmi.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/view/zdj_view_stack.h>
#include <zerodj/ui/view/dialog_view/zdj_dialog_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>

void _zdj_dialog_view_draw( zdj_view_t * view, zdj_view_clip_t * clip );
void _zdj_dialog_view_handle_hmi( zdj_view_t * menu_stack, void * _event );
void _zdj_dialog_view_deinit_state( zdj_view_t * dialog_view );
void _zdj_dialog_okay_btn_handle_event( zdj_view_t * view, void * _event );

zdj_view_t * zdj_new_dialog_view( 
    zdj_dialog_view_type_t type,
    char * header,
    char * body_1,
    char * body_2
) {
    zdj_view_t * dialog_view = zdj_new_view( zdj_dialog_rect( ) );
    dialog_view->type = ZDJ_VIEW_DIALOG;
    dialog_view->draw = &_zdj_dialog_view_draw;
    dialog_view->handle_hmi_event = _zdj_dialog_view_handle_hmi;
    dialog_view->deinit_state = &_zdj_dialog_view_deinit_state;

    dialog_view->frame->x = ZDJ_DIALOG_X;
    dialog_view->frame->y = ZDJ_SCREEN_H+2;
    
    dialog_view->in_anim = zdj_new_anim( ZDJ_ANIM_DIALOG_SHOW );
    dialog_view->out_anim = zdj_new_anim( ZDJ_ANIM_DIALOG_HIDE );

    // Add a state instance
    zdj_dialog_view_state_t * state = calloc( 1, sizeof( zdj_dialog_view_state_t ) );
    dialog_view->state = state;

    // Add menu
    zdj_view_t * _menu = zdj_new_menu_view( ZDJ_VERTICAL, zdj_dialog_rect( ) );
    zdj_add_subview( dialog_view, _menu );
    _menu->frame->x = 0;
    _menu->frame->y = 0;
    _menu->frame->w = ZDJ_DIALOG_W;
    _menu->frame->h = ZDJ_DIALOG_H;

    // Add body
    if( body_1 ) {
        zdj_view_t * body_1_ticker = zdj_new_ticker_view( body_1, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
        body_1_ticker->frame->x = 7;
        body_1_ticker->frame->y = 4;
        body_1_ticker->frame->w = dialog_view->frame->w - 7 - ZDJ_MENU_ITEM_MARGIN_R;
        zdj_menu_view_add_item( _menu, body_1_ticker );
    }
    if( body_2 ) {
        zdj_view_t * body_2_ticker = zdj_new_ticker_view( body_2, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
        body_2_ticker->frame->x = 7;
        body_2_ticker->frame->w = dialog_view->frame->w - 7 - ZDJ_MENU_ITEM_MARGIN_R;
        zdj_menu_view_add_item( _menu, body_2_ticker );
    }

    // Add buttons
    zdj_view_t * okay_btn = zdj_new_menu_item( "OKAY", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
    zdj_menu_item_view_state_t * okay_btn_state = (zdj_menu_item_view_state_t*)okay_btn->state;
    okay_btn_state->data->ptr = dialog_view;
    okay_btn->handle_hmi_event = &_zdj_dialog_okay_btn_handle_event;
    okay_btn->frame->x = 76;
    okay_btn->frame->y = 25;
    okay_btn->frame->w = 19;
    okay_btn->frame->h = 10;
    zdj_menu_view_add_item( _menu, okay_btn );

    // Add header
    zdj_view_t * menu_header = zdj_new_menu_header( 
        header,
        "",
        ZDJ_MENU_HEADER_STYLE_NORMAL,
        ZDJ_MENU_HEADER_BACK_STYLE_CANCEL
    );
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)menu_header->state;
    zdj_menu_view_add_header( _menu, menu_header );

    return dialog_view;
}

// Drop in a dotted BG to obscure the views below
void _zdj_dialog_view_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // boxColor( zdj_renderer( ), 0, 0, ZDJ_SCREEN_W, ZDJ_SCREEN_H, ZDJ_BLACK );
    // SDL_RenderCopy( zdj_renderer( ), zdj_asset_atlas( ), &zdj_ui_assets[ ZDJ_UI_ASSET_DOT_BG ], &(SDL_Rect){0,0,127,64} );
}

void _zdj_dialog_view_handle_hmi( zdj_view_t * dialog_view, void * _event ) {
    zdj_hmi_event_t * e = (zdj_hmi_event_t *)_event;
    
    // Ignore events which have been blocked by layers above this one.
    if( e->blocked ) { return; }

    // Send events down into the subview stack
    zdj_view_t * top_subview = zdj_view_stack_top_subview_of( dialog_view );
    top_subview->handle_hmi_event( top_subview, _event );
}

void _zdj_dialog_okay_btn_handle_event( zdj_view_t * view, void * _event ) {
    zdj_menu_item_view_state_t * view_state = (zdj_menu_item_view_state_t*)view->state;
    zdj_view_t * dialog = view_state->data->ptr;
    zdj_dialog_view_state_t * dialog_state = (zdj_dialog_view_state_t*)dialog->state;
    if( dialog_state->handle_dialog_exit && dialog_state->selection_data ) {
        printf( "handling dialog_exit okay: %p, %p, %p\n", dialog, dialog_state->handle_dialog_exit, dialog_state->selection_data );
        dialog_state->handle_dialog_exit( dialog, dialog_state->selection_data, true );
    }
}

void _zdj_dialog_view_deinit_state( zdj_view_t * dialog_view ) {
    zdj_dialog_view_state_t * state = (zdj_dialog_view_state_t*)dialog_view->state;
    free( state );
    dialog_view->state = NULL;
}
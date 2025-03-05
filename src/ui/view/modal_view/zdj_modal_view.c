#include <stdio.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/hmi/zdj_hmi.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/view/zdj_view_stack.h>
#include <zerodj/ui/view/modal_view/zdj_modal_view.h>

void _zdj_modal_view_draw( zdj_view_t * view, zdj_view_clip_t * clip );
void _zdj_modal_view_handle_hmi( zdj_view_t * menu_stack, void * _event );
void _zdj_modal_view_deinit_state( zdj_view_t * modal_view );

zdj_view_t * zdj_new_modal_view( zdj_rect_t * frame ) {
    zdj_view_t * modal_view = zdj_new_view( frame );
    modal_view->draw = &_zdj_modal_view_draw;
    modal_view->handle_hmi_event = _zdj_modal_view_handle_hmi;
    modal_view->deinit_state = &_zdj_modal_view_deinit_state;
    modal_view->in_anim = zdj_new_anim( ZDJ_ANIM_MODAL_SHOW );
    modal_view->out_anim = zdj_new_anim( ZDJ_ANIM_MODAL_HIDE );
    
    modal_view->frame->x = ZDJ_MODAL_X;
    modal_view->frame->y = ZDJ_SCREEN_H;

    // Add a state instance
    zdj_modal_view_state_t * state = calloc( 1, sizeof( zdj_modal_view_state_t ) );
    modal_view->state = state;

    return modal_view;
}

// Drop in a dotted BG to obscure the views below
void _zdj_modal_view_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // boxColor( zdj_renderer( ), 0, 0, ZDJ_SCREEN_W, ZDJ_SCREEN_H, ZDJ_BLACK );
    // SDL_RenderCopy( zdj_renderer( ), zdj_asset_atlas( ), &zdj_ui_assets[ ZDJ_UI_ASSET_DOT_BG ], &(SDL_Rect){0,0,127,64} );
}

void _zdj_modal_view_handle_hmi( zdj_view_t * modal_view, void * _event ) {
    zdj_hmi_event_t * e = (zdj_hmi_event_t *)_event;
    
    // Ignore events which have been blocked by layers above this one.
    if( e->blocked ) { return; }

    // Send events down into the subview stack
    zdj_view_t * top_subview = zdj_view_stack_top_subview_of( modal_view );
    top_subview->handle_hmi_event( top_subview, _event );
}

void _zdj_modal_view_deinit_state( zdj_view_t * modal_view ) {
    zdj_modal_view_state_t * state = (zdj_modal_view_state_t*)modal_view->state;
    free( state );
    modal_view->state = NULL;
}
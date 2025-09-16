#include <stdio.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/view/zdj_view_stack.h>
#include <zerodj/ui/view/modal_view/zdj_modal_view.h>

void _zdj_modal_view_draw( zdj_view_t * view, zdj_view_clip_t * clip );
void _zdj_modal_view_handle_control( zdj_view_t * menu_stack, zdj_control_event_t * event );
void _zdj_modal_view_deinit_state( zdj_view_t * modal_view );

zdj_view_t * zdj_new_modal_view( zdj_rect_t * frame ) {
    zdj_view_t * modal_view = zdj_new_view( frame );
    modal_view->type = ZDJ_VIEW_MODAL;
    modal_view->draw = &_zdj_modal_view_draw;
    modal_view->handle_control_event = _zdj_modal_view_handle_control;
    modal_view->deinit_state = &_zdj_modal_view_deinit_state;

    modal_view->frame.x = ZDJ_MODAL_X;
    modal_view->frame.y = ZDJ_SCREEN_H;
    
    zdj_set_anim( &modal_view->in_anim, ZDJ_ANIM_MODAL_SHOW );
    zdj_set_anim( &modal_view->out_anim, ZDJ_ANIM_MODAL_HIDE );

    // Add a state instance
    zdj_modal_view_state_t * state = calloc( 1, sizeof( zdj_modal_view_state_t ) );
    modal_view->state = state;

    return modal_view;
}

// Drop in a dotted BG to obscure the views below
void _zdj_modal_view_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_BLACK );
}

void _zdj_modal_view_handle_control( zdj_view_t * modal_view, zdj_control_event_t * event ) {
    // printf( "_zdj_modal_view_handle_control\n" );
    
    // Ignore events which have been blocked by layers above this one.
    if( event->blocked ) { return; }

    // Send events down into the subview stack
    zdj_view_t * top_subview = zdj_view_stack_top_subview_of( modal_view );
    top_subview->handle_control_event( top_subview, event );
}

void _zdj_modal_view_deinit_state( zdj_view_t * modal_view ) {
    zdj_modal_view_state_t * state = (zdj_modal_view_state_t*)modal_view->state;
    free( state );
    modal_view->state = NULL;
}
#include <stdio.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/view/zdj_view_stack.h>
#include <zerodj/ui/view/menu_stack/zdj_menu_stack.h>

void _zdj_menu_stack_handle_control( zdj_view_t * menu_stack, zdj_control_event_t * _event );
void _zdj_menu_stack_deinit_state( zdj_view_t * menu_stack );

zdj_view_t * zdj_new_menu_stack( zdj_rect_t * frame ) {
    zdj_view_t * menu_stack = zdj_new_view( frame );
    menu_stack->type = ZDJ_VIEW_MENU_STACK;
    menu_stack->handle_control_event = _zdj_menu_stack_handle_control;
    menu_stack->deinit_state = &_zdj_menu_stack_deinit_state;
    menu_stack->in_anim = zdj_new_anim( ZDJ_ANIM_MENU_STACK_SHOW );
    menu_stack->out_anim = zdj_new_anim( ZDJ_ANIM_MENU_STACK_HIDE );

    menu_stack->frame->x = ZDJ_MENU_X;
    menu_stack->frame->y = ZDJ_SCREEN_H+2;

    // Add a state instance
    zdj_menu_stack_state_t * state = malloc( sizeof( zdj_menu_stack_state_t ) );
    memset( state, 0, sizeof( zdj_menu_stack_state_t ) );
    menu_stack->state = state;

    return menu_stack;
}

void _zdj_menu_stack_handle_control( zdj_view_t * menu_stack, zdj_control_event_t * _event ) {
    zdj_control_event_t * e = (zdj_control_event_t *)_event;
    
    // Ignore events which have been blocked by layers above this one.
    if( e->blocked ) { return; }

    // Send events down into the top menu
    // Note that menu_stack/subviews may be deleted during handle_control_event.
    // Be careful accessing them after calling handle_control_event.
    zdj_view_t * top_menu = zdj_view_stack_top_subview_of( menu_stack );
    top_menu->handle_control_event( top_menu, _event );
}

void _zdj_menu_stack_deinit_state( zdj_view_t * menu_stack ) {
    zdj_menu_stack_state_t * state = (zdj_menu_stack_state_t*)menu_stack->state;
    free( state );
    menu_stack->state = NULL;
}
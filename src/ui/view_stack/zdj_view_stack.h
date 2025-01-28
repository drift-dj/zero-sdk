#ifndef ZDJ_VIEW_STACK_H
#define ZDJ_VIEW_STACK_H

#include <zerodj/ui/zdj_ui.h>

zdj_view_t * zdj_view_stack;
void zdj_init_view_stack( void );
void zdj_draw_view_stack( void );

// Any view block any hmi_event from traveling further down the stack.
// Any view can block all hmi_events from traveling further down the stack.

bool zdj_view_stack_handle_hmi_events( void );

void zdj_view_stack_push( zdj_view_t * view );
zdj_view_t * zdj_view_stack_pop( void );
void zdj_view_stack_remove( zdj_view_t * view );

zdj_view_t * zdj_view_stack_peek_index( int index );
zdj_view_t * zdj_view_stack_insert_at( int index );
zdj_view_t * zdj_view_stack_insert_above( int index );
zdj_view_t * zdj_view_stack_insert_below( int index );

#endif
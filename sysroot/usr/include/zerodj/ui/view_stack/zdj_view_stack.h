#ifndef ZDJ_VIEW_STACK_H
#define ZDJ_VIEW_STACK_H

#include <zerodj/ui/zdj_ui.h>

void zdj_view_stack_push( zdj_view_t * view );
zdj_view_t * zdj_view_stack_pop( void );
void zdj_view_stack_remove( zdj_view_t * view );

zdj_view_t * zdj_view_stack_peek_index( int index );
zdj_view_t * zdj_view_stack_insert_at_index( int index );
zdj_view_t * zdj_view_stack_insert_above_index( int index );
zdj_view_t * zdj_view_stack_insert_below_index( int index );
zdj_view_t * zdj_view_stack_get_id( int id );

#endif
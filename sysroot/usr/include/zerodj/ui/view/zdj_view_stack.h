#ifndef ZDJ_VIEW_STACK_H
#define ZDJ_VIEW_STACK_H

#include <zerodj/ui/zdj_ui.h>

extern zdj_view_t * zdj_view_stack_root_view;

zdj_view_t * zdj_root_view( void );

void zdj_view_stack_init( void );
void zdj_view_stack_deinit( void );
void zdj_view_stack_update( void );

void zdj_view_stack_clear_screen( void );

void zdj_view_stack_handle_events( zdj_view_t * view );
void zdj_view_stack_draw( zdj_view_t * view, zdj_view_clip_t * clip );

void zdj_view_stack_update_subview_clip( zdj_view_t * subview, zdj_view_clip_t * superview_clip );
zdj_view_t * zdj_view_stack_top_subview_of( zdj_view_t * view );
zdj_view_t * zdj_view_stack_bottom_subview_of( zdj_view_t * view );

#endif
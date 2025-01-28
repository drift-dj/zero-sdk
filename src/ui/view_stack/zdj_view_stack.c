#include <stdlib.h>

#include <zerodj/ui/view_stack/zdj_view_stack.h>

zdj_view_t * zdj_view_stack_base;
zdj_view_t * zdj_view_stack_tip( void );

zdj_view_t * zdj_view_stack_tip( void ) {
    zdj_view_t * v = zdj_view_stack_base;
    while( v->stack_up ) {
        v = v->stack_up;
    }
    return v;
}

void zdj_init_view_stack( void ) {
    // Reset head
}

void zdj_draw_view_stack( void ) {
    // Start at base, calling draw on everything as we stride up to tip.
}

void zdj_view_stack_handle_hmi( void ) {
    // Start at tip, calling handle_hmi as we stride toward base.
    // Any handle_ui_events invocation can return true, which blocks
    // ui events for views below the view which returns true.
}

void zdj_view_stack_push( zdj_view_t * view ) {

}

zdj_view_t * zdj_view_stack_pop( void ) {
    return NULL;
}

void zdj_view_stack_remove( zdj_view_t * view ) {

}


zdj_view_t * zdj_view_stack_peek_index( int index ) {
    return NULL;
}
 
zdj_view_t * zdj_view_stack_insert_at( int index ) {
    return NULL;
}

zdj_view_t * zdj_view_stack_insert_above( int index ) {
    return NULL;
}

zdj_view_t * zdj_view_stack_insert_below( int index ) {
    return NULL;
}
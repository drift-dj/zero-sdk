#include <stdio.h>
#include <string.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/menu_section_view/zdj_menu_section_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

// Parse a static XML node into a menu_item_view.
zdj_view_t * zdj_new_menu_section( char * title ) {
    // Make view
    zdj_view_t * menu_section = zdj_new_view( NULL );
    menu_section->draw = &zdj_menu_section_static_draw;
    // Build state
    zdj_menu_section_view_state_t * state = calloc( 1, sizeof( zdj_menu_section_view_state_t ) );
    menu_section->state = (void*)state;

    // Build ticker

    
    return menu_section;
}

void zdj_menu_section_static_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {

}
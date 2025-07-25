#include <stdio.h>
#include <string.h>


#include <zerodj/controls/hmi/zdj_hmi.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>

void zdj_menu_view_add_scroll_filter_input( zdj_view_t * view, int input ) {
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)view->state;
    menu_state->scroll_filter->jog_force += (float)input;
}

void zdj_menu_view_update_scroll_filter( zdj_view_t * view ) {
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)view->state;
    zdj_menu_view_scroll_filter_t * scroll_filter = (zdj_menu_view_scroll_filter_t*)menu_state->scroll_filter;

    // Lowpass jog_force and jog_input
    scroll_filter->jog_input *= 0.1;
    if( fabs(scroll_filter->jog_input) < 0.5 ) { scroll_filter->jog_input = 0.0f; }

    scroll_filter->jog_force += scroll_filter->jog_input;
    scroll_filter->jog_force *= 0.4;
    if( fabs(scroll_filter->jog_force) < 0.1 ) { scroll_filter->jog_force = 0.0f; }

    // Build a gravity force so position settles near whole numbers.
    float gravity_mag;
    float index_offset = scroll_filter->position - (float)floor( scroll_filter->position );
    if( index_offset < 0.5 ) {
        // Closer to prev index, push toward prev
        gravity_mag = index_offset * -1;
    } else {
        // Closer to next index, push toward next
        gravity_mag = (float)floor( scroll_filter->position+1 ) - scroll_filter->position;
    }
    gravity_mag *= 0.05;
    if( fabs(gravity_mag) < 0.001 ) { gravity_mag = 0.0f; }

    // Add forces and slow them with drag
    scroll_filter->velocity = (scroll_filter->jog_force * scroll_filter->drag) + gravity_mag;
    if( fabs(scroll_filter->velocity) < 0.001 ) { scroll_filter->velocity = 0.0f; }

    // Move position/out_index
    scroll_filter->position += scroll_filter->velocity;
    scroll_filter->out_index = round( scroll_filter->position );

    // Observe menu scroll_index bounds
    if( scroll_filter->out_index >= menu_state->item_count) {
        scroll_filter->position = (float)(menu_state->item_count-1);
        scroll_filter->out_index = menu_state->item_count-1;
    } else if( scroll_filter->out_index < 0 ) {
        // Stop at -1 if back button is present
        if( menu_state->has_back ) {
            if( scroll_filter->out_index < ZDJ_BACK_INDEX ) {
                scroll_filter->position = (float)ZDJ_BACK_INDEX;
                scroll_filter->out_index = ZDJ_BACK_INDEX;
            }
        } else {
            scroll_filter->position = 0.0f;
            scroll_filter->out_index = 0;
        }
    }
}
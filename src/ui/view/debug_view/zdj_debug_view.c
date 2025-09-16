#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/ui/zdj_ui.h>
// #include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/debug_view/zdj_debug_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
// #include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
// #include <zerodj/ui/view/menu_section_view/zdj_menu_section_view.h>
#include <zerodj/ui/view/scroll_view/zdj_scroll_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _zdj_debug_view_draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_debug_view_update_layout( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_debug_view_handle_control( zdj_view_t * view, zdj_control_event_t * _event );

zdj_view_t * zdj_new_debug_view( zdj_rect_t * frame ) {
    // printf( "zdj_new_debug_view: %s\n", log_path );
    zdj_view_t * view = zdj_new_view( frame );
    view->type = ZDJ_VIEW_BASE;
    view->draw = &_zdj_debug_view_draw;
    view->handle_control_event = &_zdj_debug_view_handle_control;

    // Add state
    zdj_debug_view_state_t * state = calloc( 1, sizeof( zdj_debug_view_state_t ) );
    view->state = state;
    
    // Add memory label
    state->mem_label = zdj_new_data_menu_item( "Memory", ZDJ_MENU_ITEM_LAYOUT_DATA_R, ZDJ_MENU_ITEM_DATA_TYPE_CHAR, NULL, NULL );
    state->mem_label->frame.x = 0;
    state->mem_label->frame.y = 2;
    state->mem_label->frame.w = ZDJ_DEBUG_PANEL_WIDTH - 4;
    state->mem_label->frame.h = 7;
    zdj_add_subview( view, state->mem_label );
    zdj_menu_item_view_state_t * mem_state = (zdj_menu_item_view_state_t*)state->mem_label->state;
    strcpy( mem_state->data->c_val, "..." );

    // Add View counter
    state->view_label = zdj_new_data_menu_item( "Views", ZDJ_MENU_ITEM_LAYOUT_DATA_R, ZDJ_MENU_ITEM_DATA_TYPE_CHAR, NULL, NULL );
    state->view_label->frame.x = 0;
    state->view_label->frame.y = 10;
    state->view_label->frame.w = ZDJ_DEBUG_PANEL_WIDTH - 4;
    state->view_label->frame.h = 7;
    zdj_add_subview( view, state->view_label );
    zdj_menu_item_view_state_t * view_state = (zdj_menu_item_view_state_t*)state->view_label->state;
    strcpy( view_state->data->c_val, "..." );

    return view;
}

void _zdj_debug_view_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_WHITE );

    zdj_debug_view_state_t * state = (zdj_debug_view_state_t*)view->state;
    
    // Redraw the log every n seconds
    if( state->counter++ > 120 ) {
        state->counter = 0;
        _zdj_debug_view_update_layout( view, clip );
    }
}

int parseLine(char* line){
    // This assumes that a digit will be found and the line ends in " Kb".
    int i = strlen(line);
    const char* p = line;
    while (*p <'0' || *p > '9') p++;
    line[i-3] = '\0';
    i = atoi(p);
    return i;
}

void _zdj_debug_view_update_layout( zdj_view_t * view, zdj_view_clip_t * clip ) {

    zdj_debug_view_state_t * state = (zdj_debug_view_state_t*)view->state;
    
    // Add memory state fd
    char str[ 256 ];
    state->mem_fd = fopen( "/proc/self/status", "r" );
    char line[128];
    while ( fgets( line, 128, state->mem_fd ) != NULL ) {
        if ( strncmp( line, "VmSize:", 7 ) == 0 ) {
            parseLine( line );
            strcpy( str, &line[9] );
            break;
        }
    }
    fclose( state->mem_fd );
    zdj_menu_item_view_state_t * mem_state = (zdj_menu_item_view_state_t*)state->mem_label->state;
    strcpy( mem_state->data->c_val, str );
    mem_state->needs_layout_init = true;

    snprintf( str, sizeof( str ), "%d", zdj_view_count );
    zdj_menu_item_view_state_t * view_state = (zdj_menu_item_view_state_t*)state->view_label->state;
    strcpy( view_state->data->c_val, str );
    view_state->needs_layout_init = true;
}

void _zdj_debug_view_handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_control_event_t * e = (zdj_control_event_t *)_event;
    zdj_debug_view_state_t * state = (zdj_debug_view_state_t*)view->state;

    // Ignore events which have been blocked by layers above this one.
    if( e->blocked ) { return; }
    
    // if( e->id == ZDJ_UI_CONTROL_JOG_ADJUST_0 ) {
    //     e->blocked = true;
    //     // Scroll the scroll view
    //     zdj_point_t point;
    //     point.x = 0;
    //     point.y = (float)e->i_val;
    //     zdj_scroll_view_by_point( state->scroll_view, &point );
    // } else if( e->id == ZDJ_UI_CONTROL_TONE_1_ADJUST_0 ) {
    //     e->blocked = true;
    //     // Scroll the scroll view
    //     zdj_point_t point;
    //     point.x = 0;
    //     point.y = e->i_val;
    //     zdj_scroll_view_by_point( state->scroll_view, &point );
    // } else if( e->id == ZDJ_UI_CONTROL_TONE_2_ADJUST_0 ) {
    //     e->blocked = true;
    //     // Scroll the scroll view
    //     zdj_point_t point;
    //     point.x = e->i_val;
    //     point.y = 0;
    //     zdj_scroll_view_by_point( state->scroll_view, &point );
    // }
}
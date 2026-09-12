#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/system/log/zdj_log.h>
#include <zerodj/system/settings/zdj_settings.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/widget/zdj_ui_widget.h>
#include <zerodj/ui/widget/log/zdj_log_widget.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _toggle( zdj_view_t * view );
static void _deploy( zdj_view_t * view );
static void _retract( zdj_view_t * view );

static void _clear_log_lines( void );
static void _draw_container( zdj_view_t * view, zdj_view_clip_t * clip );

zdj_view_t * zdj_new_log_widget( void ) {
    // printf( "zdj_new_volume_widget\n" );
    zdj_view_t * view = zdj_new_view( zdj_screen_rect( ) );
    view->type = ZDJ_VIEW_BASE;

    // Add a container view for animations/clipping
    zdj_view_t * container_view = zdj_new_view( &(zdj_rect_t){ 0, -2.0 - ZDJ_SCREEN_H, ZDJ_SCREEN_W, ZDJ_SCREEN_H } );
    zdj_add_subview( view, container_view );
    container_view->type = ZDJ_VIEW_BASE;
    container_view->draw = &_draw_container;

    // zdj_set_anim( &container_view->in_anim, ZDJ_ANIM_LOG_WIDGET_SHOW );
    // zdj_set_anim( &container_view->out_anim, ZDJ_ANIM_LOG_WIDGET_HIDE );

    // Add state
    zdj_log_widget_state_t * state = calloc( 1, sizeof( zdj_log_widget_state_t ) );
    view->state = state;
    container_view->state = state;
    state->container = container_view;
    state->update_counter = 0;
    state->toggle = &_toggle;
    state->lines = NULL;
    state->line_count = 0;

    return view;
}

static void _draw_container( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // printf( "log widget _draw_container\n" );
    zdj_log_widget_state_t * state = (zdj_log_widget_state_t*)view->state;
    // Draw box and border
    // boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_BLACK );

    zdj_remove_all_subviews_of( view );

    // Loop thru lines, adding labels to screen
    if( state->lines ) { 
        zdj_log_line_t * line = state->lines;
        zdj_log_line_t * next_line = NULL;
        zdj_log_line_t * prev_line = NULL;
        int iter = 0;
        while( line && iter++ < 1000 ) {
            float y = 55 - (iter-1)*8;
            zdj_view_t * label = zdj_new_label_view( line->buf, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
            // Draw a black box behind the label
            boxColor( zdj_renderer( ), 0, y, label->frame.w, y+8, ZDJ_BLACK );
            label->frame.y = y;
            zdj_add_subview( view, label );

            // Stitch out any old lines
            next_line = line->next;
            if( line->counter++ < 2000 ) { 
                prev_line = line;
            } else {
                if( line == state->lines ) { state->lines = NULL; }
                free( line );
                if( prev_line ) { prev_line->next = next_line; }
            }

            line = next_line;
        }
    }
    // printf( "log widget _draw_container done\n" );
}

static void _toggle( zdj_view_t * view ) {
    // printf( "Log Widget Toggle\n" );
    zdj_log_widget_state_t * state = (zdj_log_widget_state_t*)view->state;
    if( state->deployed ) {
        _retract( view );
    } else {
        _deploy( view );
    }
}

static void _deploy( zdj_view_t * view ) {
    // printf( "Log Widget Deploy\n" );
    zdj_log_widget_state_t * state = (zdj_log_widget_state_t*)view->state;
    state->deployed = true;
    // printf( "debug deploy: %p\n", state->container );

    zdj_log_widget_append_line( "LOG" );

    state->container->frame.y = 0;
}

static void _retract( zdj_view_t * view ) {
    // printf( "log retract\n" );
    zdj_log_widget_state_t * state = (zdj_log_widget_state_t*)view->state;
    state->deployed = false;

    _clear_log_lines( );
    state->container->frame.y = -2.0 - ZDJ_SCREEN_H;
}

static void _clear_log_lines( void ) {
    zdj_log_widget_state_t * state = (zdj_log_widget_state_t*)zdj_ui_get_log_widget( )->state;
    // printf( "_clear_log_lines: %p\n", state );
    zdj_log_line_t * _line = state->lines;
    zdj_log_line_t * _next_line = NULL;
    int iter = 0;
    while( _line && iter++ < 100 ) {
        // Remove the prev line's ref to this line and free it
        _next_line = _line->next;
        free( _line );
        _line = _next_line;
    }
    state->lines = NULL;
    state->line_count = 0;
}

void zdj_log_widget_append_line( char * str ) {
    // printf( "zdj_log_widget_append_line: %s\n", str );
    zdj_log_widget_state_t * state = (zdj_log_widget_state_t*)zdj_ui_get_log_widget( )->state;
    // If log widget is deployed, add a log line to the display
    if( state->deployed ) {
        // Make new line
        zdj_log_line_t * line = calloc( 1, sizeof( zdj_log_line_t ) );
        snprintf( line->buf, ZDJ_LOG_WIDGET_LINE_LEN, "%s", str );
        
        
        if( state->lines ) { 
            // Trim extra lines if we're over line_count
            if( state->line_count++ > 6 ) {
                zdj_log_line_t * _line = state->lines;
                zdj_log_line_t * _prev_line = state->lines;
                int iter = 0;
                while( _line && iter++ < 10 ) {
                    // Remove the prev line's ref to this line and free it
                    if( !_line->next ) { 
                        _prev_line->next = NULL;
                        free( _line );
                    }
                    _prev_line = _line;
                    _line = _line->next;
                }
            }

            // Prepend line to FILO
            line->next = state->lines;
            state->lines = line;
        } else {
            state->lines = line;
            line->next = NULL;
        }
    }
}
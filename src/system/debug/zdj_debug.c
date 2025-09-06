#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/system/debug/zdj_debug.h>

zdj_debug_state_t * zdj_debug_state = NULL;

static void _zdj_debug_update_layout( void );

void zdj_debug_init( void ) {
    if( !zdj_debug_state ) {
        pid_t pid = getpid( );
        zdj_debug_state = calloc( 1, sizeof( zdj_debug_state_t ) );
        zdj_debug_state->pid = pid;
        zdj_debug_state->show_ui = false;
        zdj_debug_state->ui_type = ZDJ_DEBUG_UI_MEMORY;
        zdj_debug_state->update_counter = 0;
        zdj_debug_state->update_duration = 20;
        zdj_view_clip_t * clip = calloc( 1, sizeof( zdj_view_clip_t ) );
        clip->src.w = 44;
        clip->src.h = 10;
        clip->dst.x = 84;
        clip->dst.w = clip->src.w;
        clip->dst.h = clip->src.h;
        clip->screen.x = 84;
        clip->screen.y = 0;
        zdj_debug_state->draw_clip = clip;
    }
}

void zdj_debug_ui_draw( void ) {
    if( zdj_debug_state && zdj_debug_state->show_ui ) {
        zdj_view_clip_t * clip = zdj_debug_state->draw_clip;
        if( zdj_debug_state->update_counter++ > zdj_debug_state->update_duration ) {
            zdj_debug_state->update_counter = 0;
            _zdj_debug_update_layout( );
        }
        // Draw mem line
        boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_BLACK );
        // Draw field 1
        if( zdj_debug_state->line_1 ) {
            zdj_debug_state->line_1->update_subview_clip( zdj_debug_state->line_1, clip );
            zdj_debug_state->line_1->draw( zdj_debug_state->line_1, zdj_debug_state->line_1->subview_clip );
        }
        // Draw field 2
        if( zdj_debug_state->line_2 ) {

        }
    }
}

void zdj_debug_ui_show( void ) {
    if( zdj_debug_state ) {
        zdj_debug_state->show_ui = true;
    }
}

void zdj_debug_ui_hide( void ) {
    if( zdj_debug_state ) {
        zdj_debug_state->show_ui = true;
    }
}

void zdj_debug_ui_toggle( void ) {
    if( zdj_debug_state ) {
        zdj_debug_state->show_ui = !zdj_debug_state->show_ui;
    }
}

void _zdj_debug_update_layout( void ) {
    // if( !zdj_debug_state ) { return; }

    // // Build mem size string
    // char cmd[ 256 ];
    // char mem[ 256 ];
    // snprintf( cmd, sizeof( cmd ), "cat /proc/%ld/status | grep 'VmSize*'", zdj_debug_state->pid );
    // char * p_res = zdj_fs_get_popen( cmd );
    // if( !p_res ) { return; }
    // snprintf( mem, sizeof( mem ), "%s | ", p_res );

    // free( p_res );

    // if( zdj_debug_state->line_1 ) { zdj_delete_view( zdj_debug_state->line_1 ); }
    // zdj_debug_state->line_1 = zdj_new_label_view( (char*)&mem, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    // zdj_label_state_t * label_state = (zdj_label_state_t*)zdj_debug_state->line_1->state;
    // label_state->debug = true;

    // // Build view stack string

    
    // if( zdj_debug_state->line_2 ) { zdj_delete_view( zdj_debug_state->line_2 ); }

    
}
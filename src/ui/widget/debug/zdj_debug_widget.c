#include <stdio.h>
#include <dirent.h>
#include <unistd.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/widget/debug/zdj_debug_widget.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _toggle( zdj_view_t * view );
static void _deploy( zdj_view_t * view );
static void _retract( zdj_view_t * view );

static void _draw_container( zdj_view_t * view, zdj_view_clip_t * clip );

static void _put_mem( char * str );

zdj_view_t * zdj_new_debug_widget( void ) {
    // printf( "zdj_new_volume_widget\n" );
    zdj_view_t * view = zdj_new_view( zdj_screen_rect( ) );
    view->type = ZDJ_VIEW_BASE;

    // Add a container view for animations/clipping
    zdj_view_t * container_view = zdj_new_view( &(zdj_rect_t){ ZDJ_SCREEN_W-50, ZDJ_SCREEN_H+2, 50, 11 } );
    zdj_add_subview( view, container_view );
    container_view->type = ZDJ_VIEW_BASE;
    container_view->draw = &_draw_container;

    zdj_set_anim( &container_view->in_anim, ZDJ_ANIM_DEBUG_WIDGET_SHOW );
    zdj_set_anim( &container_view->out_anim, ZDJ_ANIM_DEBUG_WIDGET_HIDE );

    // Add state
    zdj_debug_widget_state_t * state = calloc( 1, sizeof( zdj_debug_widget_state_t ) );
    view->state = state;
    container_view->state = state;
    state->container = container_view;
    state->update_counter = 0;
    state->toggle = &_toggle;
    
    return view;
}

static void _draw_container( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_debug_widget_state_t * state = (zdj_debug_widget_state_t*)view->state;
    // Draw box and border
    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_BLACK );

    if( state->update_counter++ > 100 ) {
        state->update_counter = 0;
        if( state->mem_label ){ zdj_remove_subview_of( view, state->mem_label ); }
        char str[ 64 ];
        _put_mem( str );
        state->mem_label = zdj_new_label_view( str, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
        zdj_add_subview( view, state->mem_label );
    }
}

static void _toggle( zdj_view_t * view ) {
    zdj_debug_widget_state_t * state = (zdj_debug_widget_state_t*)view->state;
    if( state->deployed ) {
        _retract( view );
    } else {
        _deploy( view );
    }
}

static void _deploy( zdj_view_t * view ) {
    zdj_debug_widget_state_t * state = (zdj_debug_widget_state_t*)view->state;
    state->deployed = true;
    // printf( "debug deploy: %p\n", state->container );

    ((anim_init_t)state->container->in_anim.init_fn)( 
        &state->container->in_anim, 
        state->container
    );
    state->container->anim = &state->container->in_anim;

    // printf( "debug deploy done\n" );
}

static void _retract( zdj_view_t * view ) {
    // printf( "debug retract\n" );
    zdj_debug_widget_state_t * state = (zdj_debug_widget_state_t*)view->state;
    state->deployed = false;

    ((anim_init_t)state->container->out_anim.init_fn)( 
        &state->container->out_anim, 
        state->container 
    );
    state->container->anim = &state->container->out_anim;
}

static void _put_mem( char * str ) {
    // Build mem size string
    char cmd[ 256 ];

    snprintf( cmd, sizeof( cmd ), "/proc/%d/statm", getpid( ) );
    FILE* fp = fopen( cmd, "r" );
    if( !fp ) { return; }

    long size = 0;
    long resident = 0;
    long shared = 0;
    long text = 0;
    long lib = 0;
    long data = 0;
    long dt = 0;

    if( fscanf( fp, 
                "%ld %ld %ld %ld %ld %ld %ld", 
                &size, &resident, &shared, &text, &lib, &data, &dt
               ) != 7
    ) {
        fclose( fp );
        return;
    }
    fclose( fp );

    long page_size_kb = sysconf( _SC_PAGE_SIZE ) / 1024;
    sprintf( str, "%ld KB", resident * page_size_kb );
}
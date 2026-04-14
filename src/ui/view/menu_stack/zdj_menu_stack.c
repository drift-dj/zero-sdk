#include <stdio.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/view/zdj_view_stack.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/menu_stack/zdj_menu_stack.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _handle_control( zdj_view_t * menu_stack, zdj_control_event_t * _event );
static void _handle_root_back( zdj_view_t * menu_view );
static void _deinit_state( zdj_view_t * menu_stack );

zdj_view_t * zdj_new_menu_stack( 
    zdj_rect_t * frame, 
    zdj_menu_stack_retract_cb_t retract_cb,
    void * retract_data 
) {
    zdj_view_t * menu_stack = zdj_new_view( frame );
    menu_stack->type = ZDJ_VIEW_MENU_STACK;
    menu_stack->handle_control_event = &_handle_control;
    menu_stack->deinit_state = &_deinit_state;
    zdj_set_anim( &menu_stack->in_anim, ZDJ_ANIM_MENU_STACK_SHOW );
    zdj_set_anim( &menu_stack->out_anim, ZDJ_ANIM_MENU_STACK_HIDE );

    menu_stack->frame.y = ZDJ_SCREEN_H+2;

    // Add a state instance
    zdj_menu_stack_state_t * state = malloc( sizeof( zdj_menu_stack_state_t ) );
    memset( state, 0, sizeof( zdj_menu_stack_state_t ) );
    menu_stack->state = state;
    state->retract_cb = retract_cb;
    state->retract_data = retract_data;
    state->is_enabled = true;

    return menu_stack;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_WHITE );
}

static void _handle_control( zdj_view_t * menu_stack, zdj_control_event_t * event ) {
    // printf( "_handle_control\n" );
    zdj_menu_stack_state_t * state = (zdj_menu_stack_state_t *)menu_stack->state;
    // Ignore events which have been blocked by layers above this one.
    if( event->blocked || !state->is_enabled ) { return; }

    // Capture a Nav long-press and retract( )
    if( event->id == ZDJ_UI_CONTROL_NAV_PRESS_1 && state->retract_cb ) {
        printf( "handling retract long press\n" );
        event->blocked = true;
        zdj_menu_stack_retract( menu_stack );
        return;
    }

    // Capture a Nav release if we're at the root menu and retract( )
    if( event->id == ZDJ_UI_CONTROL_NAV_RELEASE_0 && state->retract_cb ) {
        printf( "handling retract release: %p\n", state->retract_cb );
        event->blocked = true;
        zdj_menu_stack_retract( menu_stack );
        return;
    }

    // Send events down into the top menu
    // Note that menu_stack/subviews may be deleted during handle_control_event.
    // Be careful accessing them after calling handle_control_event.
    zdj_view_t * top_menu = zdj_view_stack_top_subview_of( menu_stack );
    top_menu->handle_control_event( top_menu, event );
}

static void _deinit_state( zdj_view_t * menu_stack ) {
    zdj_menu_stack_state_t * state = (zdj_menu_stack_state_t*)menu_stack->state;
    free( state );
    menu_stack->state = NULL;
}

void zdj_menu_stack_deploy( zdj_view_t * menu_stack ) {
    zdj_menu_stack_state_t * state = (zdj_menu_stack_state_t*)menu_stack->state;
    // Run in_anim
    // if( menu_stack->in_anim ) {
    //     menu_stack->in_anim->cb_fn = NULL;
    //     if( menu_stack->out_anim ) { menu_stack->out_anim->cb_fn = NULL; }
    //     ((anim_init_t)menu_stack->in_anim->init_fn)( menu_stack->in_anim, menu_stack );
    //     menu_stack->in_anim->view = menu_stack;
    //     menu_stack->anim = menu_stack->in_anim;
    // }
    menu_stack->in_anim.cb_fn = NULL;
    menu_stack->out_anim.cb_fn = NULL;
    ((anim_init_t)menu_stack->in_anim.init_fn)( &menu_stack->in_anim, menu_stack );
    menu_stack->in_anim.view = menu_stack;
    menu_stack->anim = &menu_stack->in_anim;

    // Activate control map/handling for top menu
    state->is_enabled = true;
    zdj_view_t * top_menu = zdj_view_stack_top_subview_of( menu_stack );
    zdj_activate_control_map( top_menu->map );
}

void zdj_menu_stack_retract( zdj_view_t * menu_stack ) {
    zdj_menu_stack_state_t * state = (zdj_menu_stack_state_t*)menu_stack->state;
    // Run out_anim
    ((anim_init_t)menu_stack->out_anim.init_fn)( &menu_stack->out_anim, menu_stack );
    menu_stack->out_anim.view = menu_stack;
    menu_stack->out_anim.cb_fn = NULL; // Delete subview after anim
    menu_stack->anim = &menu_stack->out_anim;
    
    // Disable control handling
    state->is_enabled = false;
    
    // Call retract_cb so front-end can activate its own control map, etc.
    if( state->retract_cb ){ state->retract_cb( state->retract_data ); }
}

void zdj_menu_stack_set_root_menu( zdj_view_t * menu_stack, zdj_view_t * root_menu ) {
    zdj_menu_view_state_t * root_menu_state = (zdj_menu_view_state_t*)root_menu->state;
    zdj_menu_header_view_state_t * root_header_state = NULL;
    if( root_menu_state->header_view ){ 
        root_header_state = (zdj_menu_header_view_state_t*)root_menu_state->header_view->state; 
    }

    // Capture the root menu's back button cb to invoke retract( )
    if( root_header_state ) { root_header_state->handle_back = &_handle_root_back; }
}

static void _handle_root_back( zdj_view_t * menu_view ) {
    // How to get a reference to root_menu's containing menu_stack?
}
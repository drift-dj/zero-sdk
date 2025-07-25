#include <stdlib.h>

#include <zerodj/ui/anim/zdj_anim.h>

void zdj_anim_init_debug_panel_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    // Set up start data
    zdj_point_t * start_point = malloc( sizeof( zdj_point_t ) );
    start_point->x = ZDJ_DEBUG_PANEL_WIDTH * -3;
    start_point->y = 0;
    anim->start_data = start_point;
    // Set up end data
    zdj_point_t * end_point = malloc( sizeof( zdj_point_t ) );
    end_point->x = 0;
    end_point->y = 0;
    anim->end_data = end_point;

    // Delay the in anim by a few frames to let the old menu's out animation happen.
    anim->frame = 0;
    anim->alive = true;
}

void zdj_anim_init_debug_panel_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    // Set up start data
    zdj_point_t * start_point = malloc( sizeof( zdj_point_t ) );
    start_point->x = 0;
    start_point->y = 0;
    anim->start_data = start_point;
    // Set up end data
    zdj_point_t * end_point = malloc( sizeof( zdj_point_t ) );
    end_point->x = ZDJ_DEBUG_PANEL_WIDTH * -3;
    end_point->y = 0;
    anim->end_data = end_point;

    // Delay the out anim by a few frames to give the button flash time to happen.
    anim->frame = 0;
    anim->alive = true;
}

void zdj_anim_update_debug_panel( zdj_anim_t * anim, zdj_view_t * view ) {
    // Gather start/end point refs
    zdj_point_t * start_point = (zdj_point_t*)anim->start_data;
    zdj_point_t * end_point = (zdj_point_t*)anim->end_data;

    if( anim->frame == anim->frames ) {
        // At anim end
        anim->alive = false;
        view->frame->x = end_point->x;
        view->frame->y = end_point->y;
        if( anim->cb_fn ){ ((anim_cb_t)anim->cb_fn)( anim->superview, anim->view ); }
    } else {
        anim->frame++;
        // Run animation update alogrithm
        if( anim->frame < 0 ) {
            // Before anim start
            view->frame->x = start_point->x;
            view->frame->y = start_point->y;
        } else {
            // During animation
            float coeff = anim->ease( (float)anim->frame, (float)anim->frames );
            view->frame->x = start_point->x + ( ( end_point->x - start_point->x ) * coeff );
            view->frame->y = start_point->y + ( ( end_point->y - start_point->y ) * coeff );
        }
    }
}

void zdj_anim_deinit_debug_panel( zdj_anim_t * anim ) {
    free( anim->start_data );
    free( anim->end_data );
    free( anim );
}
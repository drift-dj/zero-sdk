#include <stdlib.h>

#include <zerodj/ui/anim/zdj_anim.h>

void zdj_anim_init_dialog_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    anim->start_point.x = view->frame.x;
    anim->start_point.y = view->frame.y;
    anim->end_point.x = ZDJ_DIALOG_X;
    anim->end_point.y = ZDJ_DIALOG_Y;

    // Delay the in anim by a few frames to let the old menu's out animation happen.
    anim->frame = -12;
    anim->alive = true;
}

void zdj_anim_init_dialog_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    anim->start_point.x = view->frame.x;
    anim->start_point.y = view->frame.y;
    anim->end_point.x = ZDJ_DIALOG_X;
    anim->end_point.y = ZDJ_SCREEN_H+1;

    // Delay the out anim by a few frames to give the button flash time to happen.
    anim->frame = -5;
    anim->alive = true;
}

void zdj_anim_update_dialog( zdj_anim_t * anim, zdj_view_t * view ) {
    if( anim->frame == anim->frames ) {
        // At anim end
        anim->alive = false;
        view->frame.x = anim->end_point.x;
        view->frame.y = anim->end_point.y;
        if( anim->cb_fn ) { 
            // printf( "calling dialog exit cb\n" );
            ((anim_cb_t)anim->cb_fn)( anim->superview, anim->view ); 
        }
    } else {
        anim->frame++;
        // Run animation update alogrithm
        if( anim->frame < 0 ) {
            // Before anim start
            view->frame.x = anim->start_point.x;
            view->frame.y = anim->start_point.y;
        } else {
            // During animation
            float coeff = anim->ease( (float)anim->frame, (float)anim->frames );
            view->frame.x = anim->start_point.x + ( ( anim->end_point.x - anim->start_point.x ) * coeff );
            view->frame.y = anim->start_point.y + ( ( anim->end_point.y - anim->start_point.y ) * coeff );
        }
    }
}
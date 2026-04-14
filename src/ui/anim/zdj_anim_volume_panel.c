#include <stdlib.h>

#include <zerodj/ui/anim/zdj_anim.h>

void zdj_anim_init_volume_panel_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    anim->start_point.x = ZDJ_SCREEN_W + 2;
    anim->start_point.y = 5;
    anim->end_point.x = ZDJ_SCREEN_W - 14;
    anim->end_point.y = 5;
    anim->frame = 0;
    anim->frames = zdj_anim_show_hide_frames( );
    anim->alive = true;
}

void zdj_anim_init_volume_panel_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    anim->start_point.x = ZDJ_SCREEN_W - 14;
    anim->start_point.y = 5;
    anim->end_point.x = ZDJ_SCREEN_W + 2;
    anim->end_point.y = 5;
    anim->frame = 0;
    anim->frames = zdj_anim_show_hide_frames( );
    anim->alive = true;
}

void zdj_anim_update_volume_panel( zdj_anim_t * anim, zdj_view_t * view ) {
    if( anim->frame == anim->frames ) {
        // At anim end
        anim->alive = false;
        view->frame.x = anim->end_point.x;
        view->frame.y = anim->end_point.y;
        if( anim->cb_fn ){ ((anim_cb_t)anim->cb_fn)( anim->superview, anim->view ); }
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
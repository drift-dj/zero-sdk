#include <stdlib.h>

#include <zerodj/ui/anim/zdj_anim.h>


// void zdj_anim_init_panel_in_next( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
// void zdj_anim_init_panel_out_next( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
// void zdj_anim_init_panel_in_prev( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
// void zdj_anim_init_panel_out_prev( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn );
// void zdj_anim_update_panel( zdj_anim_t * anim, zdj_view_t * view );


void zdj_anim_init_panel_in_next( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    anim->start_point.x = ZDJ_SCREEN_W + 2;
    anim->start_point.y = 0;
    anim->end_point.x = 0;
    anim->end_point.y = 0;
    anim->frame = 0;
    anim->frames = zdj_anim_show_hide_frames( );
    anim->alive = true;
}

void zdj_anim_init_panel_out_next( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    anim->start_point.x = 0;
    anim->start_point.y = 0;
    anim->end_point.x = -129;
    anim->end_point.y = 0;
    anim->frame = 0;
    anim->frames = zdj_anim_show_hide_frames( );
    anim->alive = true;
}

void zdj_anim_init_panel_in_prev( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    anim->start_point.x = -129;
    anim->start_point.y = 0;
    anim->end_point.x = 0;
    anim->end_point.y = 0;
    anim->frame = 0;
    anim->frames = zdj_anim_show_hide_frames( );
    anim->alive = true;
}

void zdj_anim_init_panel_out_prev( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    anim->start_point.x = 0;
    anim->start_point.y = 0;
    anim->end_point.x = ZDJ_SCREEN_W + 2;
    anim->end_point.y = 0;
    anim->frame = 0;
    anim->frames = zdj_anim_show_hide_frames( );
    anim->alive = true;
}

void zdj_anim_init_panel_deploy( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    anim->start_point.x = 0;
    anim->start_point.y = ZDJ_SCREEN_H + 2;
    anim->end_point.x = 0;
    anim->end_point.y = 0;
    anim->frame = 0;
    anim->frames = zdj_anim_show_hide_frames( );
    anim->alive = true;
}

void zdj_anim_init_panel_retract( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    anim->start_point.x = 0;
    anim->start_point.y = 0;
    anim->end_point.x = 0;
    anim->end_point.y = ZDJ_SCREEN_H + 2;
    anim->frame = 0;
    anim->frames = zdj_anim_show_hide_frames( );
    anim->alive = true;
}

void zdj_anim_update_panel( zdj_anim_t * anim, zdj_view_t * view ) {
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
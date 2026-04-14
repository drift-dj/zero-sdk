#include <stdlib.h>

#include <zerodj/ui/anim/zdj_anim.h>

void zdj_anim_init_dj_deck_page_show( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    anim->start_point.x = (zdj_dj_deck_page_rect()->w + 2) * -1;
    anim->end_point.x = 0;

    // Delay the in anim by a few frames to let the old menu's out animation happen.
    anim->frame = zdj_ui_msec_to_frames( 50 ) * -1;
    anim->frames = zdj_ui_msec_to_frames( 120 );
    anim->alive = true;
}

void zdj_anim_init_dj_deck_page_hide( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    anim->start_point.x = 0;
    anim->end_point.x = (zdj_dj_deck_page_rect()->w + 2) * -1;

    // Delay the out anim by a few frames to give the button flash time to happen.
    anim->frame = 0;
    anim->frames = zdj_ui_msec_to_frames( 100 );
    anim->alive = true;
}

void zdj_anim_update_dj_deck_page( zdj_anim_t * anim, zdj_view_t * view ) {
    
    if( anim->frame == anim->frames ) {
        // At anim end
        anim->alive = false;
        view->frame.x = anim->end_point.x;
        // if( anim->cb_fn ) { ((anim_cb_t)anim->cb_fn)( anim->superview, anim->view ); }
    } else {
        anim->frame++;
        // Run animation update alogrithm
        if( anim->frame < 0 ) {
            // Before anim start
            view->frame.x = anim->start_point.x;
        } else {
            // During animation
            float coeff = anim->ease( (float)anim->frame, (float)anim->frames );
            view->frame.x = anim->start_point.x + ( ( anim->end_point.x - anim->start_point.x ) * coeff );
        }
    }
}
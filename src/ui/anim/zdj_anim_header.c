#include <stdlib.h>

#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>

typedef struct {
    float back_y;
    float title_x;
    float name_y;
} zdj_anim_header_data_t;

void zdj_anim_init_header_activate( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)view->state;

    // Note that we're hacking to different anims into a single x/y point
    anim->start_point.x = header_state->title_ticker->frame.x;
    anim->start_point.y = 1;

    anim->end_point.x = header_state->back_view->frame.w + 6;
    anim->end_point.y = 0;

    anim->frame = 0;
    anim->frames = 6;
    anim->alive = true;
}

void zdj_anim_init_header_deactivate( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)view->state;

    // Note that we're hacking to different anims into a single x/y point
    anim->start_point.x = header_state->title_ticker->frame.x; // <-- this doesn't look right
    anim->start_point.y = 0;

    anim->end_point.x = header_state->back_view->frame.w + 6;
    anim->end_point.y = 1;

    anim->frame = 0;
    anim->frames = 6;
    anim->alive = true;
}

void zdj_anim_update_header( zdj_anim_t * anim, zdj_view_t * view ) {
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)view->state;

    float coeff = anim->ease( (float)anim->frame, (float)anim->frames );
    float back_y = 8 * ( anim->start_point.y + ( ( anim->end_point.y - anim->start_point.y ) * coeff ) );
    float name_y = 8 - (8 * ( anim->start_point.y + ( ( anim->end_point.y - anim->start_point.y ) * coeff ) ));
    float title_x = anim->start_point.x + ( ( anim->end_point.x - anim->start_point.x ) * coeff );

    if( anim->frame == anim->frames ) {
        // At anim end
        anim->alive = false;
        header_state->back_view->frame.y = back_y;
        header_state->back_bg->frame.y = back_y;
        header_state->name_label->frame.y = name_y;
        header_state->title_ticker->frame.x = title_x;
        header_state->title_divider->frame.x = title_x - 4;
        if( anim->cb_fn ){ ((anim_cb_t)anim->cb_fn)( anim->superview, anim->view ); }
    } else {
        // Run animation update alogrithm
        anim->frame++;
        if( anim->frame < 0 ) {
            // Before anim start
            header_state->back_view->frame.y = back_y;
            header_state->back_bg->frame.y = back_y;
            header_state->name_label->frame.y = name_y;
            header_state->title_ticker->frame.x = title_x;
            header_state->title_divider->frame.x = title_x - 4;
        } else {
            // During animation
            header_state->back_view->frame.y = back_y;
            header_state->back_bg->frame.y = back_y;
            header_state->name_label->frame.y = name_y;
            header_state->title_ticker->frame.x = title_x;
            header_state->title_divider->frame.x = title_x - 4;
        }
    }
}
#include <stdlib.h>

#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>

typedef struct {
    float back_y;
    float title_x;
    float name_y;
} zdj_anim_header_data_t;

void zdj_anim_init_header_activate( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    // zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)view->state;

    // // Note that we're hacking two different anims into a single x/y point
    // anim->start_point.x = header_state->title_ticker->frame.x;
    // anim->start_point.y = 1;

    // anim->end_point.x = header_state->back_view->frame.w + 6;
    // anim->end_point.y = 0;

    // anim->frame = 0;
    // anim->frames = 6;
    // anim->alive = true;
}

void zdj_anim_init_header_deactivate( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    // zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)view->state;

    // // Note that we're hacking two different anims into a single x/y point
    // anim->start_point.x = header_state->title_ticker->frame.x; // <-- this doesn't look right
    // anim->start_point.y = 0;

    // // anim->end_point.x = header_state->back_view->frame.w + 6;
    // anim->end_point.x = header_state->name_label->frame.w + 6;
    // anim->end_point.y = 1;

    // anim->frame = 0;
    // anim->frames = 6;
    // anim->alive = true;
}

void zdj_anim_update_header( zdj_anim_t * anim, zdj_view_t * view ) {
    // zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)view->state;

    // float coeff = anim->ease( (float)anim->frame, (float)anim->frames );
    // float back_y = 7 * ( anim->start_point.y + ( ( anim->end_point.y - anim->start_point.y ) * coeff ) );
    // float name_y = 7 - (8 * ( anim->start_point.y + ( ( anim->end_point.y - anim->start_point.y ) * coeff ) ));
    // float title_x = anim->start_point.x + ( ( anim->end_point.x - anim->start_point.x ) * coeff );

    // if( anim->frame == anim->frames ) {
    //     // At anim end
    //     anim->alive = false;
    //     header_state->back_view->frame.y = back_y;
    //     header_state->back_bg->frame.y = back_y;
    //     header_state->name_label->frame.y = name_y;
    //     header_state->title_ticker->frame.x = title_x;
    //     header_state->title_divider->frame.x = title_x - 4;
    //     if( anim->cb_fn ){ ((anim_cb_t)anim->cb_fn)( anim->superview, anim->view ); }
    // } else {
    //     // Run animation update alogrithm
    //     anim->frame++;
    //     if( anim->frame < 0 ) {
    //         // Before anim start
    //         header_state->back_view->frame.y = back_y;
    //         header_state->back_bg->frame.y = back_y;
    //         header_state->name_label->frame.y = name_y;
    //         header_state->title_ticker->frame.x = title_x;
    //         header_state->title_divider->frame.x = title_x - 4;
    //     } else {
    //         // During animation
    //         header_state->back_view->frame.y = back_y;
    //         header_state->back_bg->frame.y = back_y;
    //         header_state->name_label->frame.y = name_y;
    //         header_state->title_ticker->frame.x = title_x;
    //         header_state->title_divider->frame.x = title_x - 4;
    //     }
    // }
}

//////////////////////////
// Header Back Animation
//////////////////////////

void zdj_anim_init_header_back_activate( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)view->state;
    
    anim->start_point.x = view->frame.x - 14;
    anim->start_point.y = 0;

    anim->end_point.x = 0;
    anim->end_point.y = 0;

    anim->frame = 0;
    anim->frames = zdj_ui_msec_to_frames( 40 );
    anim->alive = true;
}

void zdj_anim_init_header_back_deactivate( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)view->state;

    anim->start_point.x = 0;
    anim->start_point.y = 0;

    anim->end_point.x = view->frame.x - 14;
    anim->end_point.y = 0;

    anim->frame = 0;
    anim->frames = zdj_ui_msec_to_frames( 40 );
    anim->alive = true;
}

void zdj_anim_update_header_back( zdj_anim_t * anim, zdj_view_t * view ) {
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)view->state;

    float coeff = anim->ease( (float)anim->frame, (float)anim->frames );
    float x = anim->start_point.x + ( ( anim->end_point.x - anim->start_point.x ) * coeff );

    if( anim->frame == anim->frames ) {
        // At anim end
        anim->alive = false;
        header_state->back_view->frame.x = x;
        if( anim->cb_fn ){ ((anim_cb_t)anim->cb_fn)( anim->superview, anim->view ); }
    } else {
        // Run animation update alogrithm
        anim->frame++;
        if( anim->frame < 0 ) {
            // Before anim start
            header_state->back_view->frame.x = x;
        } else {
            // During animation
            header_state->back_view->frame.x = x;
        }
    }
}


//////////////////////////
// Header Close Animation
//////////////////////////

void zdj_anim_init_header_close_activate( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)view->state;

    anim->start_point.x = 0;
    anim->start_point.y = 6;

    anim->end_point.x = 0;
    anim->end_point.y = 0;

    anim->frame = 0;
    anim->frames = zdj_ui_msec_to_frames( 60 );
    anim->alive = true;
}

void zdj_anim_init_header_close_deactivate( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)view->state;

    anim->start_point.x = 0;
    anim->start_point.y = 0;

    anim->end_point.x = 0;
    anim->end_point.y = 6;

    anim->frame = 0;
    anim->frames = zdj_ui_msec_to_frames( 60 );
    anim->alive = true;
}

void zdj_anim_update_header_close( zdj_anim_t * anim, zdj_view_t * view ) {
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)view->state;

    float coeff = anim->ease( (float)anim->frame, (float)anim->frames );
    float y = anim->start_point.y + ( ( anim->end_point.y - anim->start_point.y ) * coeff );

    if( anim->frame == anim->frames ) {
        // At anim end
        anim->alive = false;
        header_state->back_view->frame.y = y;
        if( anim->cb_fn ){ ((anim_cb_t)anim->cb_fn)( anim->superview, anim->view ); }
    } else {
        // Run animation update alogrithm
        anim->frame++;
        if( anim->frame < 0 ) {
            // Before anim start
            header_state->back_view->frame.y = y;
        } else {
            // During animation
            header_state->back_view->frame.y = y;
        }
    }
}
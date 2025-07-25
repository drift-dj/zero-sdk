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
    zdj_anim_header_data_t * start_data;
    zdj_anim_header_data_t * end_data;

    // Set up start data
    if( !anim->start_data ) {
        anim->start_data = malloc( sizeof( zdj_anim_header_data_t ) );
    }
    start_data = (zdj_anim_header_data_t*)anim->start_data;
    start_data->back_y = 8;
    start_data->name_y = -1;
    start_data->title_x = header_state->title_ticker->frame->x;

    // Set up end data
    if( !anim->end_data ) {
        anim->end_data = malloc( sizeof( zdj_anim_header_data_t ) );
    }
    end_data = (zdj_anim_header_data_t*)anim->end_data;
    end_data->back_y = -1;
    end_data->name_y = -8;
    end_data->title_x = header_state->back_view->frame->w + 6;

    anim->frame = 0;
    anim->frames = 6;
    anim->alive = true;
}

void zdj_anim_init_header_deactivate( zdj_anim_t * anim, zdj_view_t * view, void * cb_fn ) {
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)view->state;
    zdj_anim_header_data_t * start_data;
    zdj_anim_header_data_t * end_data;

    // Set up start data
    if( !anim->start_data ) {
        anim->start_data = malloc( sizeof( zdj_anim_header_data_t ) );
    }
    start_data = (zdj_anim_header_data_t*)anim->start_data;
    start_data->back_y = -1;
    start_data->name_y = -8;
    start_data->title_x = header_state->title_ticker->frame->x;

    // Set up end data
    if( !anim->end_data ) {
        anim->end_data = malloc( sizeof( zdj_anim_header_data_t ) );
    }
    end_data = (zdj_anim_header_data_t*)anim->end_data;
    end_data->back_y = 8;
    end_data->name_y = -1;
    end_data->title_x = header_state->name_label->frame->w + 6;

    anim->frame = 0;
    anim->frames = 6;
    anim->alive = true;
}

void zdj_anim_update_header( zdj_anim_t * anim, zdj_view_t * view ) {
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)view->state;

    // Gather start/end point refs
    zdj_anim_header_data_t * start_data = (zdj_anim_header_data_t*)anim->start_data;
    zdj_anim_header_data_t * end_data = (zdj_anim_header_data_t*)anim->end_data;

    if( anim->frame == anim->frames ) {
        // At anim end
        anim->alive = false;
        header_state->back_view->frame->y = end_data->back_y;
        header_state->back_bg->frame->y = end_data->back_y;
        header_state->name_label->frame->y = end_data->name_y;
        header_state->title_ticker->frame->x = end_data->title_x;
        header_state->title_divider->frame->x = end_data->title_x - 4;
        if( anim->cb_fn ){ ((anim_cb_t)anim->cb_fn)( anim->superview, anim->view ); }
    } else {
        // Run animation update alogrithm
        anim->frame++;
        if( anim->frame < 0 ) {
            // Before anim start
            header_state->back_view->frame->y = start_data->back_y;
            header_state->back_bg->frame->y = start_data->back_y;
            header_state->name_label->frame->y = start_data->name_y;
            header_state->title_ticker->frame->x = start_data->title_x;
            header_state->title_divider->frame->x = start_data->title_x - 4;
        } else {
            // During animation
            float coeff = anim->ease( (float)anim->frame, (float)anim->frames );
            header_state->back_view->frame->y = start_data->back_y + ( ( end_data->back_y - start_data->back_y ) * coeff );
            header_state->back_bg->frame->y = start_data->back_y + ( ( end_data->back_y - start_data->back_y ) * coeff );
            header_state->name_label->frame->y =  start_data->name_y + ( ( end_data->name_y - start_data->name_y ) * coeff );
            header_state->title_ticker->frame->x = start_data->title_x + ( ( end_data->title_x - start_data->title_x ) * coeff );
            header_state->title_divider->frame->x = header_state->title_ticker->frame->x - 4;
        }
    }
    
}

void zdj_anim_deinit_header( zdj_anim_t * anim ) {
    free( anim->start_data );
    free( anim->end_data );
}
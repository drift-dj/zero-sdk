#include <stdlib.h>

#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>

typedef struct {
    float back_x;
    float title_x;
    float name_x;
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
    start_data->back_x = 0 - header_state->back_view->frame->w;
    start_data->name_x = header_state->name_label->frame->x;
    start_data->title_x = header_state->title_ticker->frame->x;

    // Set up end data
    if( !anim->end_data ) {
        anim->end_data = malloc( sizeof( zdj_anim_header_data_t ) );
    }
    end_data = (zdj_anim_header_data_t*)anim->end_data;
    end_data->back_x = 1;
    end_data->name_x = 0 - header_state->name_label->frame->w;
    end_data->title_x = header_state->back_view->frame->w + 8;

    anim->frame = 0;
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
    start_data->back_x = header_state->back_view->frame->x;
    start_data->name_x = 0 - header_state->name_label->frame->w;
    start_data->title_x = header_state->title_ticker->frame->x;

    // Set up end data
    if( !anim->end_data ) {
        anim->end_data = malloc( sizeof( zdj_anim_header_data_t ) );
    }
    end_data = (zdj_anim_header_data_t*)anim->end_data;
    end_data->back_x = 0 - header_state->back_view->frame->w;
    end_data->name_x = 1;
    end_data->title_x = header_state->name_label->frame->w + 8;

    anim->frame = 0;
    anim->alive = true;
}

void zdj_anim_update_header( zdj_anim_t * anim, zdj_view_t * view ) {
    zdj_menu_header_view_state_t * header_state = (zdj_menu_header_view_state_t*)view->state;

    // Gather start/end point refs
    zdj_anim_header_data_t * start_data = (zdj_anim_header_data_t*)anim->start_data;
    zdj_anim_header_data_t * end_data = (zdj_anim_header_data_t*)anim->end_data;

    if( anim->frame == anim->frames ) {
        // At anim end
        header_state->back_view->frame->x = end_data->back_x;
        header_state->name_label->frame->x = end_data->name_x;
        header_state->title_ticker->frame->x = end_data->title_x;
        header_state->title_divider->frame->x = header_state->title_ticker->frame->x - 4;
        if( anim->cb_fn ){ ((anim_cb_t)anim->cb_fn)( anim->superview, anim->view ); }
    } else {
        // Run animation update alogrithm
        anim->frame++;
        if( anim->frame < 0 ) {
            // Before anim start
            header_state->back_view->frame->x = start_data->back_x;
            header_state->name_label->frame->x = start_data->name_x;
            header_state->title_ticker->frame->x = start_data->title_x;
            header_state->title_divider->frame->x = header_state->title_ticker->frame->x - 4;
        } else {
            // During animation
            float coeff = anim->ease( (float)anim->frame, (float)anim->frames );
            header_state->back_view->frame->x = end_data->back_x + ( ( start_data->back_x - end_data->back_x ) * coeff );
            header_state->name_label->frame->x = end_data->name_x + ( ( start_data->name_x - end_data->name_x ) * coeff );
            header_state->title_ticker->frame->x = end_data->title_x + ( ( start_data->title_x - end_data->title_x ) * coeff );
            header_state->title_divider->frame->x = header_state->title_ticker->frame->x - 4;
        }
    }
    
}

void zdj_anim_deinit_header( zdj_anim_t * anim ) {
    free( anim->start_data );
    free( anim->end_data );
}
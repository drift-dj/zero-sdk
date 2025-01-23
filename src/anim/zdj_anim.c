#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include <zerodj/anim/zdj_anim.h>

int anim_id;
zdj_anim_state_t * zdj_anim_head;
zdj_anim_state_t * zdj_anim_tip( void );

float _zdj_ease_linear( float x );
float _zdj_ease_out_quart( float x );
float _zdj_ease_in_quart( float x );

void zdj_anim_init( void ) {
    zdj_anim_head = NULL;
}

void zdj_anim_update( void ) {
    zdj_anim_state_t * st = zdj_anim_head;
    while( st ) {
        if( st->alive ) {
            st->frame++;
            if( st->end_frame == -1 ) {
                st = st->next;
                continue;
            } else if( st->frame < st->start_frame ) {
                st->val = st->start_val;
            } else if( st->frame > st->end_frame ) {
                st->val = st->end_val;
                st->alive = false;
            } else {
                float coeff = ( float )( st->end_frame - st->frame ) / ( float )( st->end_frame - st->start_frame );
                float eased = st->easing_fn( coeff );
                st->val = st->end_val + ( ( st->start_val - st->end_val ) * eased );
            }
        }
        st = st->next;
    }
}

zdj_anim_state_t * zdj_anim_new_state( 
    int start_frame, 
    float start_val, 
    int end_frame, 
    float end_val, 
    zdj_anim_easing_t easing 
) {
    zdj_anim_state_t * ret = malloc( sizeof( zdj_anim_state_t ) );
    ret->id = anim_id++;
    ret->start_frame = start_frame;
    ret->start_val = start_val;
    ret->end_frame = end_frame;
    ret->end_val = end_val;
    ret->easing = easing;

    switch ( easing ) {
        case ZDJ_ANIM_EASEIN:
            ret->easing_fn = &_zdj_ease_in_quart;
            break;
        case ZDJ_ANIM_EASEOUT:
            ret->easing_fn = &_zdj_ease_out_quart;
            break;
        case ZDJ_ANIM_EASEINOUT:
            ret->easing_fn = &_zdj_ease_out_quart;
            break;    
        case ZDJ_ANIM_LINEAR:
            ret->easing_fn = &_zdj_ease_linear;
            break;
    }

    if( zdj_anim_head ) {
        zdj_anim_tip( )->next = ret;
    } else {
        zdj_anim_head = ret;
    }

    return ret;
}

bool zdj_anim_iscomplete( zdj_anim_state_t * state ) {
    return( state->frame >= state->end_frame );
}

void zdj_anim_delete( zdj_anim_state_t * state ) {
    // printf( "delete_anim_state: %p\n", state );
    zdj_anim_state_t * st = zdj_anim_head;
    zdj_anim_state_t * prev = zdj_anim_head;
    while( st ) {
        if( st == state ) {
            if( prev )
                prev->next = st->next;
            free( st );
            return;
        }
        prev = st;
        st = st->next;
    }
}

zdj_anim_state_t * zdj_anim_tip( void ) {
    zdj_anim_state_t * st = zdj_anim_head;
    while( st->next ) {
        st = st->next;
    }
    return st;
}

float _zdj_ease_linear( float x ) {
    return x;
}

float _zdj_ease_out_quart( float x ) {
    return x * x * x * x;
}

float _zdj_ease_in_quart( float x ) {
    return 1.0 - pow( 1.0 - x, 4 );
}
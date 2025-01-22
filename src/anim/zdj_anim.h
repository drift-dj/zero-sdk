#ifndef ZDJ_ANIM_H
#define ZDJ_ANIM_H

#include <stdbool.h>

#define ZDJ_ANIM_LENGTH_MENU_IN 50
#define ZDJ_ANIM_LENGTH_MENU_OUT 30
#define ZDJ_ANIM_LENGTH_MENU_BLINK_LENGTH 7
#define ZDJ_ANIM_LENGTH_MENU_BLINK_DELAY 30

typedef enum {
    ZDJ_ANIM_EASEOUT,
    ZDJ_ANIM_EASEIN,
    ZDJ_ANIM_EASEINOUT,
    ZDJ_ANIM_LINEAR
} zdj_anim_easing_t;

typedef struct {
    int id;
    int frame;
    int start_frame;
    float start_val;
    int end_frame;
    float end_val;
    float val;
    zdj_anim_easing_t easing;
    float ( *easing_fn )( float );
    bool alive;
    void * next;
} zdj_anim_state_t;
zdj_anim_state_t * zdj_anim_new_state( 
    int start_frame, 
    float start_val, 
    int end_frame, 
    float end_val, 
    zdj_anim_easing_t easing 
);
bool zdj_anim_iscomplete( zdj_anim_state_t * state );
void zdj_anim_delete( zdj_anim_state_t * state );

void zdj_anim_init( void );
void zdj_anim_update( void );

#endif
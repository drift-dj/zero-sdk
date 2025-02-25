// Copyright (c) 2025 Drift DJ Industries

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

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
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

#ifndef ZDJ_NOTIFY_WIDGET_H
#define ZDJ_NOTIFY_WIDGET_H

#include <zerodj/ui/anim/zdj_anim.h>

typedef struct {
    bool deployed;
    int deploy_counter;
    zdj_view_t * container;
    int line_count;
    zdj_view_t * label_1;
    zdj_view_t * label_2;
    zdj_view_t * label_3;
    float w;
    float h;
    zdj_anim_t * in_anim;
    zdj_anim_t * out_anim;
    void ( *toggle )( zdj_view_t* );
} zdj_notify_widget_state_t;

zdj_view_t * zdj_new_notify_widget( void );

void zdj_show_notify_widget( char * line_1, char * line_2, char * line_3 );


#endif
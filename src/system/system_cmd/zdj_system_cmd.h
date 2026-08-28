// Copyright (c) 2026 Drift DJ Industries

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

#ifndef ZDJ_SYSTEM_CMD_H
#define ZDJ_SYSTEM_CMD_H

#include <pthread.h>

typedef enum { 
    ZDJ_SYSTEM_CMD_STATUS_IDLE,
    ZDJ_SYSTEM_CMD_STATUS_SUCCESS,
    ZDJ_SYSTEM_CMD_STATUS_ERROR
} zdj_system_cmd_status_t;

typedef struct {
    char cmd[ 1024 ];
    bool async;
    pthread_t thread;
    void ( *cb ) ( void* );
    zdj_system_cmd_status_t status;
    int result;
    void * data;
} zdj_system_cmd_context_t;

void zdj_system_cmd( zdj_system_cmd_context_t * context );
void zdj_system_cmd_str( char * cmd_str );
void zdj_system_cmd_str_async( char * cmd_str, void(*cb)(void*) );

#endif

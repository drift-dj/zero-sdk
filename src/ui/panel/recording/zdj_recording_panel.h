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

#ifndef ZDJ_RECORDING_PANEL_H
#define ZDJ_RECORDING_PANEL_H

#include <zerodj/ui/anim/zdj_anim.h>

typedef enum {
    ZDJ_RECORD_PANEL_RETRACTED,
    ZDJ_RECORD_PANEL_MINI_METER,
    ZDJ_RECORD_PANEL_OPTIONS_VIEW
} zdj_recording_panel_deploy_state_t;

typedef struct {
    zdj_view_t * menu;
    bool view_needs_refresh;
    bool has_open_recording;
} zdj_recording_panel_state_t;

zdj_view_t * zdj_new_recording_panel( void );

#endif
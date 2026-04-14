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

#ifndef ZDJ_BATTERY_ICON_VIEW_H
#define ZDJ_BATTERY_ICON_VIEW_H

#include <SDL2/SDL.h>

#include <zerodj/ui/zdj_ui.h>

typedef enum {
    ZDJ_BATTERY_ICON_TYPE_SMALL,
    ZDJ_BATTERY_ICON_TYPE_MEDIUM
} zdj_battery_icon_type_t;

typedef struct {
    zdj_battery_icon_type_t type;
    zdj_view_t * bg;
    zdj_view_t * bar;
    double charge_val;
    FILE * fd;
} zdj_battery_icon_state_t;

zdj_view_t * zdj_new_battery_icon_view( zdj_battery_icon_type_t type );

#endif
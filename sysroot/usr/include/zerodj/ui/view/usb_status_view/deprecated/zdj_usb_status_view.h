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

#ifndef ZDJ_USB_STATUS_VIEW_H
#define ZDJ_USB_STATUS_VIEW_H

#include <stdbool.h>
#include <sys/inotify.h>

// int buf_len = 1 * ( sizeof(struct inotify_event ) + NAME_MAX + 1 );

    // int inotifyFd, wd;
    // char buf[ buf_len ] __attribute__ ((aligned(8)));
    // struct inotify_event * event;

    // inotifyFd = inotify_init( );
    // wd = inotify_add_watch( inotifyFd, ZDJ_USB_REQUEST_PATH, IN_CLOSE_WRITE );

typedef struct {
    void ( *handle_usb_status_exit )( zdj_view_t *, void *, bool );
    zdj_view_t * menu_view;
    bool needs_layout_update;
    bool transition;
    unsigned int transition_counter;
    char transition_title_1[ 128 ];
    char transition_title_2[ 128 ];
    int frame_counter;
} zdj_usb_status_view_state_t;

zdj_view_t * zdj_new_usb_status_view( void );
void zdj_usb_status_view_build_host_layout( zdj_view_t * view );
void zdj_usb_status_view_build_host_error_layout( zdj_view_t * view );
void zdj_usb_status_view_build_device_layout( zdj_view_t * view );
void zdj_usb_status_view_build_device_error_layout( zdj_view_t * view );
void zdj_usb_status_view_build_system_error_layout( zdj_view_t * view );
void zdj_usb_status_view_build_system_offline_layout( zdj_view_t * view );
void zdj_usb_status_view_build_processing_layout( zdj_view_t * view );

void zdj_usb_status_view_handle_host_mode_btn( zdj_view_t * view, zdj_control_event_t * _event );
void zdj_usb_status_view_handle_device_mode_btn( zdj_view_t * view, zdj_control_event_t * _event );
void zdj_usb_status_view_handle_offline_btn( zdj_view_t * view, zdj_control_event_t * _event );

#endif
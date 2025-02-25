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

#ifndef ZDJ_UI_H
#define ZDJ_UI_H

#include <stdbool.h>
#include <SDL2/SDL.h>

#define ZDJ_WHITE 0xFFFFFFFF
#define ZDJ_SDL_WHITE (SDL_Color){255,255,255,255}
#define ZDJ_MID_GRAY 0xFF999999
#define ZDJ_SDL_MID_GRAY (SDL_Color){127,127,127,255}
#define ZDJ_DK_GRAY 0xFF330000
#define ZDJ_BLACK 0xFF000000
#define ZDJ_SDL_BLACK (SDL_Color){0,0,0,255}

#define ZDJ_BLINK_LENGTH 20
#define ZDJ_BLINK_PERIOD 7
#define ZDJ_BLINK_DUTY 3

typedef struct {
    float x;
    float y;
} zdj_point_t;

typedef struct {
    float x;
    float y;
    float w;
    float h;
} zdj_rect_t;

typedef struct {
    char * tag;
    char * c_val;
    int i_val;
    float f_val;
    bool b_val;
    void * ptr;
} zdj_ui_data_t;

typedef enum {
    ZDJ_VERTICAL,
    ZDJ_HORIZONTAL
} zdj_ui_orient_t;

typedef struct {
    zdj_point_t screen;
    zdj_rect_t src;
    zdj_rect_t dst;
    zdj_point_t scroll_offset;
} zdj_view_clip_t;

typedef enum {
    ZDJ_FONT_6,
    ZDJ_FONT_6_CAPS,
    ZDJ_FONT_6_BOLD,
    ZDJ_FONT_9,
    ZDJ_FONT_9_BOLD,
    ZDJ_FONT_12,
    ZDJ_FONT_12_CAPS
} zdj_font_t;

typedef enum {
    ZDJ_JUSTIFY_LEFT,
    ZDJ_JUSTIFY_CENTER,
    ZDJ_JUSTIFY_RIGHT
} zdj_justify_t;

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} zdj_color_t;

typedef enum {
    ZDJ_VIEW_UNKNOWN,
    ZDJ_VIEW_BASE,
    ZDJ_VIEW_SCROLL,
    ZDJ_VIEW_MENU,
    ZDJ_VIEW_MENU_ITEM
} zdj_view_type_t;

typedef enum {
    ZDJ_UI_OKAY,
    ZDJ_UI_NOT_A_VIEW
} zdj_ui_err_t;

// zdj_view
typedef struct zdj_view_t {
    int id;
    zdj_view_type_t type;
    void ( *init )( struct zdj_view_t * );
    void ( *deinit )( struct zdj_view_t * );
    void ( *deinit_state )( struct zdj_view_t * );
    void ( *draw )( struct zdj_view_t *, zdj_view_clip_t * );
    void ( *update_subview_clip )( struct zdj_view_t *, zdj_view_clip_t * );
    void ( *handle_hmi_event )( struct zdj_view_t *, void * );
    struct zdj_view_t * next;
    struct zdj_view_t * prev;
    struct zdj_view_t * subviews;
    int subview_index;
    zdj_view_clip_t * subview_clip;
    zdj_rect_t * frame;
    bool is_visible;
    void * state;
} zdj_view_t;

extern uint32_t * zdj_ui_pixels;
extern SDL_Renderer* zdj_display_renderer;
extern zdj_rect_t * zdj_screen_rect_priv;
SDL_Renderer * zdj_renderer( void );
zdj_rect_t * zdj_screen_rect( void );
zdj_rect_t * zdj_modal_rect( void );
zdj_rect_t * zdj_menu_rect( void );
bool zdj_ui_intersect( zdj_rect_t * rect1, zdj_rect_t * rect2 );
SDL_Texture * zdj_ui_texture_from_bmp( char * filepath );

void zdj_ui_init( void );
void zdj_ui_deinit( void );
void zdj_ui_update( void );

void zdj_ui_start_events( void );
void zdj_ui_stop_events( void );

zdj_view_t * zdj_new_view( zdj_rect_t * frame );
void zdj_add_subview( zdj_view_t * view, zdj_view_t * subview );
void zdj_remove_subview( zdj_view_t * view, zdj_view_t * subview );
void zdj_pop_subview_of( zdj_view_t * view );
void zdj_remove_subviews_of( zdj_view_t * view ); // change to remove_subviews_of
void zdj_add_subview_below( zdj_view_t * view, zdj_view_t * target_subview, zdj_view_t * new_subview );
void zdj_add_bottom_subview_to( zdj_view_t * view, zdj_view_t * new_subview );
zdj_view_t * zdj_root_view( void );

void zdj_print_subviews_of( zdj_view_t * view );

#endif
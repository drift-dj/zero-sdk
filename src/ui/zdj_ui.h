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

#include <zerodj/controls/zdj_controls.h>

#define ZDJ_WHITE 0xFFFFFFFF
#define ZDJ_SDL_WHITE (SDL_Color){255,255,255,255}
#define ZDJ_MID_GRAY 0xFF999999
#define ZDJ_SDL_MID_GRAY (SDL_Color){127,127,127,255}
#define ZDJ_DK_GRAY 0xFF330000
#define ZDJ_BLACK 0xFF000000
#define ZDJ_SDL_BLACK (SDL_Color){0,0,0,255}

#define ZDJ_BLINK_LENGTH 10
#define ZDJ_BLINK_PERIOD 7
#define ZDJ_BLINK_DUTY 3

#define ZDJ_SCREEN_W 128
#define ZDJ_SCREEN_H 64

#define ZDJ_MODAL_X 5
#define ZDJ_MODAL_Y 3
#define ZDJ_MODAL_WIDTH 118
#define ZDJ_MODAL_HEIGHT 58

#define ZDJ_DIALOG_X 15
#define ZDJ_DIALOG_Y 9
#define ZDJ_DIALOG_W 100
#define ZDJ_DIALOG_H 48
 
#define ZDJ_MENU_X 15
#define ZDJ_MENU_Y 5
#define ZDJ_MENU_WIDTH 112
#define ZDJ_MENU_HEIGHT 59

#define ZDJ_DEBUG_PANEL_X 0
#define ZDJ_DEBUG_PANEL_Y 0
#define ZDJ_DEBUG_PANEL_WIDTH 100
#define ZDJ_DEBUG_PANEL_HEIGHT 20

#define ZDJ_PERF_PANEL_X 0
#define ZDJ_PERF_PANEL_Y 0
#define ZDJ_PERF_PANEL_WIDTH 100
#define ZDJ_PERF_PANEL_HEIGHT 50

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
    char c_val[ 32 ];
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
    float cur_x;
    float cur_y;
    float set_x;
    float set_y;
} zdj_scroll_offset_t;

typedef struct {
    zdj_point_t screen;
    zdj_rect_t src;
    zdj_rect_t dst;
    zdj_scroll_offset_t scroll_offset;
} zdj_view_clip_t;

typedef enum {
    ZDJ_FONT_6,
    ZDJ_FONT_6_CAPS,
    ZDJ_FONT_6_BOLD,
    ZDJ_FONT_9,
    ZDJ_FONT_9_BOLD,
    ZDJ_FONT_12,
    ZDJ_FONT_12_CAPS,
    ZDJ_FONT_18,
    ZDJ_FONT_18_CAPS
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
    ZDJ_ANIM_EASEOUT,
    ZDJ_ANIM_EASEIN,
    ZDJ_ANIM_EASEINOUT,
    ZDJ_ANIM_LINEAR
} zdj_anim_easing_t;

typedef enum {
    ZDJ_ANIM_NONE,
    ZDJ_ANIM_EMPTY,
    ZDJ_ANIM_VIEW_SHOW,
    ZDJ_ANIM_VIEW_HIDE,
    ZDJ_ANIM_MENU_SHOW,
    ZDJ_ANIM_MENU_HIDE,
    ZDJ_ANIM_MENU_STACK_SHOW,
    ZDJ_ANIM_MENU_STACK_HIDE,
    ZDJ_ANIM_MODAL_SHOW,
    ZDJ_ANIM_MODAL_HIDE,
    ZDJ_ANIM_HEADER_ACTIVATE,
    ZDJ_ANIM_HEADER_DEACTIVATE,
    ZDJ_ANIM_DIALOG_SHOW,
    ZDJ_ANIM_DIALOG_HIDE,
    ZDJ_ANIM_DJ_DECK_PAGE_SHOW,
    ZDJ_ANIM_DJ_DECK_PAGE_HIDE,
    ZDJ_ANIM_DEBUG_PANEL_SHOW,
    ZDJ_ANIM_DEBUG_PANEL_HIDE
} zdj_anim_type_t;

typedef struct {
    int frame;
    int frames;
    float val;
    void * start_data;
    void * end_data;
    bool alive;
    zdj_anim_type_t type;
    void * init_fn;
    void * update_fn;
    float ( *ease )( float, float );
    void * deinit_fn;
    struct zdj_view_t * superview;
    struct zdj_view_t * view;
    void * cb_fn;
} zdj_anim_t;

typedef enum {
    ZDJ_VIEW_UNKNOWN,
    ZDJ_VIEW_BASE,
    ZDJ_VIEW_SCROLL,
    ZDJ_VIEW_MENU,
    ZDJ_VIEW_MENU_ITEM,
    ZDJ_VIEW_MENU_SECTION,
    ZDJ_VIEW_MENU_HEADER,
    ZDJ_VIEW_MENU_STACK,
    ZDJ_VIEW_TEXT_INPUT,
    ZDJ_VIEW_TICKER,
    ZDJ_VIEW_LABEL,
    ZDJ_VIEW_ASSET,
    ZDJ_VIEW_SCROLLBAR,
    ZDJ_VIEW_MODAL,
    ZDJ_VIEW_BROWSER,
    ZDJ_VIEW_SOUNDCARD,
    ZDJ_VIEW_PROGRESS,
    ZDJ_VIEW_DIALOG,
    ZDJ_VIEW_WAVEFORM,
    ZDJ_VIEW_DJ_DECK_PAGE
} zdj_view_type_t;

typedef enum {
    ZDJ_UI_OKAY,
    ZDJ_UI_NOT_A_VIEW
} zdj_ui_err_t;

typedef void ( *handle_control_event_t )( struct zdj_view_t *, zdj_control_event_t * );

// zdj_view
typedef struct zdj_view_t {
    int id; // runtime-unique id
    int tag; // arbitrary tag (for front-end use)
    zdj_view_type_t type;
    void ( *init )( struct zdj_view_t * );
    void ( *deinit )( struct zdj_view_t * );
    void ( *deinit_state )( struct zdj_view_t * );
    void ( *draw )( struct zdj_view_t *, zdj_view_clip_t * );
    void ( *update_subview_clip )( struct zdj_view_t *, zdj_view_clip_t * );
    // control map will be activated on push/pop_to
    zdj_control_map_id_t map;
    handle_control_event_t handle_control_event;
    struct zdj_view_t * next;
    struct zdj_view_t * prev;
    struct zdj_view_t * subviews;
    int subview_count;
    zdj_view_clip_t * subview_clip;
    zdj_rect_t * frame;
    bool is_visible;
    zdj_anim_t * anim;
    zdj_anim_t * in_anim;
    zdj_anim_t * out_anim;
    bool is_deleting;
    void * state;
} zdj_view_t;

typedef void ( *anim_init_t )( zdj_anim_t*, zdj_view_t* );
typedef void ( *anim_deinit_t )( zdj_anim_t* );
typedef void ( *anim_update_t )( zdj_anim_t*, zdj_view_t* );
typedef void ( *anim_cb_t )( zdj_view_t*, zdj_view_t* );

extern uint32_t * zdj_ui_pixels;
extern SDL_Renderer* zdj_display_renderer;
extern zdj_view_t * zdj_delete_stack;
extern zdj_rect_t * zdj_screen_rect_priv;
extern int zdj_view_count;
extern int zdj_new_view_count;
SDL_Renderer * zdj_renderer( void );
zdj_rect_t * zdj_screen_rect( void );
zdj_rect_t * zdj_modal_rect( void );
zdj_rect_t * zdj_dialog_rect( void );
zdj_rect_t * zdj_menu_rect( void );
zdj_rect_t * zdj_menu_rect_sm( void );
zdj_rect_t * zdj_menu_rect_med( void );
zdj_rect_t * zdj_dj_deck_page_rect( void );
zdj_rect_t * zdj_debug_panel_rect( void );
zdj_rect_t * zdj_perf_panel_rect( void );
bool zdj_ui_intersect( zdj_rect_t * rect1, zdj_rect_t * rect2 );
SDL_Texture * zdj_ui_texture_from_bmp( char * filepath );


void zdj_ui_init( void );
void zdj_ui_deinit( void );
void zdj_ui_update( void );

void zdj_ui_start_events( void );
void zdj_ui_stop_events( void );

zdj_view_t * zdj_new_view( zdj_rect_t * frame );
void zdj_add_subview( zdj_view_t * view, zdj_view_t * subview );
void zdj_add_subview_behind( zdj_view_t * view, zdj_view_t * target_subview, zdj_view_t * new_subview );
void zdj_add_bottom_subview_to( zdj_view_t * view, zdj_view_t * new_subview );

void zdj_remove_subview_of( zdj_view_t * view, zdj_view_t * subview );
void zdj_remove_all_subviews_of( zdj_view_t * view );

bool zdj_subview_exists_in( zdj_view_t * view, zdj_view_t * target_subview );

void zdj_push_subview( zdj_view_t * view, zdj_view_t * subview, bool animated );
void zdj_push_subview_behind( zdj_view_t * view, zdj_view_t * target_subview, zdj_view_t * new_subview, bool animated );
void zdj_pop_subview_of( zdj_view_t * view, bool animated );
void zdj_pop_to_subview_of( zdj_view_t * view, zdj_view_t * target_subview, bool animated );
void zdj_pop_n_subviews_of( zdj_view_t * view, int count, bool animated );

void zdj_delete_view( zdj_view_t * view );

zdj_view_t * zdj_root_view( void );

void zdj_print_subviews_of( zdj_view_t * view );

#endif
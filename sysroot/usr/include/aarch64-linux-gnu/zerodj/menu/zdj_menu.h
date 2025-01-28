#ifndef ZDJ_MENU_H
#define ZDJ_MENU_H

#include <stdbool.h>
#include <SDL2/SDL.h>

#include <zerodj/anim/zdj_anim.h>

typedef enum {
    ZDJ_MI_SECTION_HEADER_LEFT,
    ZDJ_MI_SECTION_HEADER_RIGHT,
    ZDJ_MI_SIMPLE_LEFT,
    ZDJ_MI_SIMPLE_RIGHT,
    ZDJ_MI_TOGGLE_LEFT,
    ZDJ_MI_TOGGLE_RIGHT,
    ZDJ_MI_STATUS_LEFT,
    ZDJ_MI_STATUS_RIGHT,
    ZDJ_MI_ICON_LEFT,
    ZDJ_MI_ICON_RIGHT
} zdj_menu_item_layout_t;

typedef enum {
    ZDJ_MI_OPEN_SONG, // Items loads a song from the library
    ZDJ_MI_OPEN_SCREEN, // Item exits Nav menu and loads screen
    ZDJ_MI_OPEN_SUBMENU, // Item presents submenu
    ZDJ_MI_MODAL_WARN, // Item presents warning modal
    ZDJ_MI_MODAL_DROP, // Item presents a dropdown menu modal
    ZDJ_MI_MODAL_INPUT, // Item presents a text input modal
    ZDJ_MI_EXIT_SELECT, // Item causes menu to exit with a selection
    ZDJ_MI_TOGGLE_DATA, // Item toggles a data bool somewhere
    ZDJ_MI_CYCLE_DATA, // Item cycles among data points
} zdj_menu_item_action_t;

typedef struct {
    SDL_Texture * tex; // text texture
    SDL_Texture * tex_hi; // hilited text texture
    int tex_w; // width of text texture
    int tex_h; // width of text texture
    int clip_w; // on-screen pixel clip width
    int clip_h; // on-screen pixel clip height
    double scroll_rate; // speed of scroll
    double scroll_offset; // current x offset of scroll
} menu_item_scrolling_text_t;

typedef struct {
    zdj_menu_item_layout_t layout;
    int w; // total pixel width of item
    int h; // total pixel height of item
    int menu_x; // x offset within parent menu_view
    int menu_y; // y offset within parent menu_view
    int screen_x; // screen x after menu scroll/anim calc
    int screen_y; // screen y after menu scroll/anim calc
    menu_item_scrolling_text_t * title_scroll;
    menu_item_scrolling_text_t * subtitle_scroll;
} zdj_menu_item_view_t;

typedef struct {
    char * title;
    char * subtitle;
    bool is_blinking;
    int blink_timer;
    zdj_menu_item_action_t action;
    void * data; // data pointer type varies by action type
    zdj_menu_item_view_t * view;
} zdj_menu_item_t;

typedef enum {
    M_NAV, // Menu with a title, back/close btn, presented over bg mask
    M_EMBED, // Menu to be embedded within another view
    M_DROP // Dropdown menu
} zdj_zdj_menu_type_t;

typedef enum {
    M_BAR_BASIC,
    M_BAR_COUNT
} zdj_menu_bar_type_t;

// Menu can contain static or dynamic data.
// Static data is defined in xml and read in at run-time.
// Dynamic data is retrieved instantaneously by invoking a function.
typedef struct {
    char * title;
    zdj_menu_bar_type_t bar_type;
    int scroll_item_count; // selectable items in menu (excludes section headers).
    int total_item_count; // all items in menu (includes section headers).
    zdj_menu_item_t ( ** menu_data_fn )( void ); // dynamic menu data
    zdj_menu_item_t ** menu_data; // static menu data
    int scroll_index; // current selected item in menu
    float scroll_index_interp; // instantaneous interpolated scroll_index
    zdj_anim_state_t scroll_anim; // animation for interpolating scroll_index
} zdj_menu_t;

extern char ** zdj_menu_name_map;
extern zdj_menu_t ** zdj_menus;

void zdj_menu_draw( zdj_menu_t * menu );
void zdj_menu_load_data( char * path );
zdj_menu_t * zdj_menu_get_by_name( char * name );
char * zdj_menu_get_speech_str_for_item( zdj_menu_item_t * item );
// How to accesszdj_menus?
// How to turn multi-menu defs in xml into low-overhead refs in mem?
// Options
//      hash-table
//      problem of runtime-defined objects

// Maybe there's a mapping from the XML menu-id field to an array of [zdj_menu_t]
// zdj_menu_t * clock_menu = zdj_menu_get_by_name( "clock_ppqn_drop" );
// You'd only need to do this on menu open.  Once you have the ref, just pass it around.

#endif
#ifndef ZDJ_MENU_SECTION_VIEW_H
#define ZDJ_MENU_SECTION_VIEW_H

typedef enum {
    ZDJ_MENU_SECTION_VIEW_LAYOUT_UNKNOWN,
    ZDJ_MENU_SECTION_LAYOUT_DYNAMIC,
    ZDJ_MENU_SECTION_LAYOUT_BASIC_R,
    ZDJ_MENU_SECTION_LAYOUT_BASIC_L
} zdj_menu_section_view_layout_t;

typedef struct {
    zdj_view_t * title_ticker;
} zdj_menu_section_view_state_t;

zdj_view_t * zdj_new_menu_section( char * title );

void zdj_menu_section_static_draw( zdj_view_t * view, zdj_view_clip_t * clip );

#endif
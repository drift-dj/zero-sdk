#ifndef ZDJ_MENU_HEADER_VIEW_H
#define ZDJ_MENU_HEADER_VIEW_H

typedef enum {
    ZDJ_MENU_HEADER_STYLE_NONE,
    ZDJ_MENU_HEADER_STYLE_NORMAL
} zdj_menu_view_header_style_t;

typedef enum {
    ZDJ_MENU_HEADER_BACK_STYLE_NONE,
    ZDJ_MENU_HEADER_BACK_STYLE_BACK,
    ZDJ_MENU_HEADER_BACK_STYLE_CANCEL,
    ZDJ_MENU_HEADER_BACK_STYLE_X
} zdj_menu_view_header_back_style_t;

typedef struct {
    zdj_menu_view_header_style_t style;
    char * name;
    char * title;
    zdj_view_t * name_label;
    zdj_view_t * title_ticker;
    zdj_view_t * title_divider;
    bool has_valid_display;
    bool is_hilite;
    bool is_blinking;
    int blink_timer;
    zdj_menu_view_header_back_style_t back_style;
    bool has_back;
    bool show_back;
    bool hide_back;
    void ( *handle_back )( zdj_view_t * );
    zdj_view_t * back_view;
} zdj_menu_header_view_state_t;

zdj_view_t * zdj_new_menu_header( 
    char * name, 
    char * title, 
    zdj_menu_view_header_style_t style, 
    zdj_menu_view_header_back_style_t back_btn_style 
);

#endif
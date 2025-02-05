#ifndef ZDJ_UI_H
#define ZDJ_UI_H

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
    zdj_rect_t * frame;
    void ** items;
    int item_count;
    zdj_point_t * scroll_point;
} zdj_scrollview_t;

typedef enum {
    ZDJ_SCROLLVIEW_MENU_ITEM,
    ZDJ_SCROLLVIEW_MENU_HEADER,
    ZDJ_SCROLLVIEW_LIBRARY_ITEM,
    ZDJ_SCROLLVIEW_LIBRARY_HEADER,
    ZDJ_SCROLLVIEW_DAW_CHANNEL_ITEM,
    ZDJ_SCROLLVIEW_PERF_REPORT_ROW_ITEM
} zdj_scrollview_item_type_t;

typedef struct {
    char * str;
} zdj_ticker_str_t;

typedef struct zdj_view_t {
    int id;
    void ( *draw )( struct zdj_view_t *, zdj_rect_t * );
    struct zdj_view_t * up;
    struct zdj_view_t * down;
    int stack_index;
    void ( *handle_hmi_event )( struct zdj_view_t *, void * );
    void ( *deinit )( struct zdj_view_t * );
} zdj_view_t;

void zdj_view_stack_init( void );
void zdj_view_stack_deinit( void );
void zdj_view_stack_update( void );

#endif
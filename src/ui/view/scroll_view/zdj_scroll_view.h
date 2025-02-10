#ifndef ZDJ_SCROLL_VIEW_H
#define ZDJ_SCROLL_VIEW_H

#include <SDL2/SDL.h>

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    zdj_point_t scroll_content; // total scrollable size
    zdj_point_t scroll_offset; // current scroll position
    zdj_point_t scroll_ratio; // scroll_offset / scroll_content ratio
    zdj_point_t scroll_window_ratio; // frame / scroll_content ratio (for scrollbars)
} zdj_scroll_view_state_t;

zdj_view_t * zdj_new_scroll_view( 
    zdj_rect_t * frame
);
void zdj_free_scroll_view( zdj_view_t * ticker_view );

void zdj_scroll_view_to_point( zdj_view_t * scroll_view, zdj_point_t * point );
void zdj_scroll_view_to_ratio( zdj_view_t * scroll_view, zdj_point_t * ratio );
void zdj_scroll_view_by_point( zdj_view_t * scroll_view, zdj_point_t * point );
void zdj_scroll_view_by_ratio( zdj_view_t * scroll_view, zdj_point_t * ratio );
#endif
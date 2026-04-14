#include <stdlib.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>

#include "zdj_progress_bar_view.h"


static void _draw_normal( zdj_view_t * view, zdj_view_clip_t * clip );
static void _draw_wait( zdj_view_t * view, zdj_view_clip_t * clip );
static void _draw_combo( zdj_view_t * view, zdj_view_clip_t * clip );
static void _deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_progress_bar_view( zdj_rect_t * frame, zdj_progress_bar_view_type_t type ) {
    zdj_view_t * progress_bar = zdj_new_view( frame );
    progress_bar->type = ZDJ_VIEW_PROGRESS;
    switch ( type ) {
        case ZDJ_PROGRESS_BAR_VIEW_NORMAL: progress_bar->draw = &_draw_normal; break;
        case ZDJ_PROGRESS_BAR_VIEW_WAIT: progress_bar->draw = &_draw_wait; break;
        case ZDJ_PROGRESS_BAR_VIEW_COMBO: progress_bar->draw = &_draw_combo; break;
    }
    progress_bar->deinit_state = &_deinit_state;

    zdj_progress_bar_view_state_t * state = calloc( 1, sizeof( zdj_progress_bar_view_state_t ) );
    state->has_valid_display = false;
    state->val = 0.0f;
    state->type = type;
    state->crawl_anim_offset = 0;
    progress_bar->state = state;

    state->bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_SM_HATCH_TEX ], NULL );
    state->bg->frame.w = 0;
    state->bg->frame.h = progress_bar->frame.h;
    zdj_add_subview( progress_bar, state->bg );

    state->wait_crawl = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ], NULL );
    state->wait_crawl->frame.y = -2;
    state->wait_crawl->frame.w = 0;
    state->wait_crawl->frame.h = progress_bar->frame.h + 2;
    zdj_add_subview( progress_bar, state->wait_crawl );

    state->bar = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_WHITE ], NULL );
    state->bar->frame.w = 0;
    state->bar->frame.h = progress_bar->frame.h;
    zdj_add_subview( progress_bar, state->bar );

    state->bar_border = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BLACK ], NULL );
    state->bar_border->frame.w = 1;
    state->bar_border->frame.h = progress_bar->frame.h;
    zdj_add_subview( progress_bar, state->bar_border );

    return progress_bar;
}

static void _draw_normal( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_progress_bar_view_state_t * state = (zdj_progress_bar_view_state_t*)view->state;
    state->bg->frame.w = view->frame.w;
    state->wait_crawl->frame.w = 0;
    state->bar->frame.w = round(state->val * view->frame.w);
    state->bar_border->frame.x = state->bar->frame.w;
}

static void _draw_wait( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_progress_bar_view_state_t * state = (zdj_progress_bar_view_state_t*)view->state;
    state->bg->frame.w = view->frame.w;
    state->bar->frame.w = 0;
    state->bar_border->frame.x = -2;

    state->wait_crawl->frame.w = view->frame.w+8;
    state->crawl_anim_offset++;
    state->crawl_anim_offset %= 8;
    state->wait_crawl->frame.x = state->crawl_anim_offset * -1;
}

static void _draw_combo( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_progress_bar_view_state_t * state = (zdj_progress_bar_view_state_t*)view->state;
    state->bg->frame.w = view->frame.w;
    state->bar->frame.w = round(state->val * view->frame.w);
    state->bar_border->frame.x = state->bar->frame.w;

    state->crawl_anim_offset++;
    state->crawl_anim_offset %= 8;

    state->wait_crawl->frame.w = view->frame.w+8;
    state->wait_crawl->frame.x = state->crawl_anim_offset * -1;
}

static void _deinit_state( zdj_view_t * view ) {
    zdj_progress_bar_view_state_t * state = (zdj_progress_bar_view_state_t*)view->state;
    free( state );
    view->state = NULL;
}
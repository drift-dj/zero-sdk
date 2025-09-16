#include <stdlib.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>

#include "zdj_progress_bar_view.h"

static void _zdj_progress_bar_view_draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_progress_bar_view_deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_progress_bar_view( zdj_rect_t * frame, zdj_progress_bar_view_type_t type ) {
    zdj_view_t * progress_bar = zdj_new_view( frame );
    progress_bar->type = ZDJ_VIEW_PROGRESS;
    progress_bar->draw = &_zdj_progress_bar_view_draw;
    progress_bar->deinit_state = &_zdj_progress_bar_view_deinit_state;

    zdj_progress_bar_view_state_t * progress_state = calloc( 1, sizeof( zdj_progress_bar_view_state_t ) );
    progress_state->has_valid_display = false;
    progress_state->val = 0.0f;
    progress_state->type = type;
    progress_state->wait_crawl_view = NULL;
    progress_state->crawl_anim_offset = 0;
    progress_bar->state = progress_state;

    return progress_bar;
}

void _zdj_progress_bar_view_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_progress_bar_view_state_t * progress_state = (zdj_progress_bar_view_state_t*)view->state;
    
    if( progress_state->type == ZDJ_PROGRESS_BAR_VIEW_WAIT ) {
        if( !progress_state->has_valid_display ) {
            // Add a wait crawl
            zdj_view_t * crawl = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_ALERT_STRIP ], NULL );
            crawl->frame.w = view->frame.w+8;
            crawl->frame.h = view->frame.h-1;
            crawl->frame.y = 1;
            progress_state->wait_crawl_view = crawl;

            zdj_add_subview( view, crawl );
        }
        // Update the wait crawl anim
        progress_state->crawl_anim_offset++;
        progress_state->crawl_anim_offset %= 8;
        progress_state->wait_crawl_view->frame.x = progress_state->crawl_anim_offset * -1;
    } else {
        if( !progress_state->has_valid_display ) {
            // Remove all subviews in case we're switching from wait > normal
            zdj_remove_all_subviews_of( view );
        }

        // Draw the progress bar
        float bar_w = progress_state->val * clip->dst.w;
        boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y+2, clip->dst.x+bar_w, clip->dst.y+clip->dst.h-1, ZDJ_WHITE );
    }

    // Draw the border
    roundedRectangleColor( zdj_renderer( ), clip->dst.x-2, clip->dst.y, clip->dst.x+clip->dst.w+1, clip->dst.y+clip->dst.h+1, 2, ZDJ_WHITE );
}

void _zdj_progress_bar_view_deinit_state( zdj_view_t * view ) {
    zdj_progress_bar_view_state_t * state = (zdj_progress_bar_view_state_t*)view->state;
    free( state );
    view->state = NULL;
}
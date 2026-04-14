#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/knob_view/zdj_knob_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );

static void _init_knob_unity( zdj_view_t * view );
static void _draw_knob_unity( zdj_view_t * view, zdj_view_clip_t * clip );

static void _init_knob_middle( zdj_view_t * view );
static void _draw_knob_middle( zdj_view_t * view, zdj_view_clip_t * clip );

static void _init_knob_filt_bi( zdj_view_t * view );
static void _draw_knob_filt_bi( zdj_view_t * view, zdj_view_clip_t * clip );

static void _deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_knob_view( zdj_knob_type_t type, zdj_rect_t * frame ) {
    zdj_knob_state_t * knob_state = calloc( 1, sizeof( zdj_knob_state_t ) );
    knob_state->frame.x = frame->x;
    knob_state->frame.y = frame->y;
    knob_state->frame.w = frame->w;
    knob_state->frame.h = frame->h;
    knob_state->type = type;

    zdj_view_t * knob_view = zdj_new_view( frame );
    knob_view->type = ZDJ_VIEW_KNOB;
    knob_view->draw = &_draw;
    knob_view->deinit_state = &_deinit_state;
    knob_view->state = (void*)knob_state;

    switch ( knob_state->type ) {
        case ZDJ_KNOB_TYPE_DET_UNITY: _init_knob_unity( knob_view ); break;
        case ZDJ_KNOB_TYPE_DET_MIDDLE: _init_knob_middle( knob_view ); break;
        case ZDJ_KNOB_TYPE_FILT_BI: _init_knob_filt_bi( knob_view ); break;
        default: break;
    }

    return knob_view;
}

void zdj_knob_view_set_val( zdj_view_t * knob_view, double val ) {
    zdj_knob_state_t * knob_state = (zdj_knob_state_t*)knob_view->state;
    switch ( knob_state->type ) {
        case ZDJ_KNOB_TYPE_DET_UNITY: 
            knob_state->val = fmin( val, 1.0 );
            break;
        case ZDJ_KNOB_TYPE_DET_MIDDLE: 
            if( val < -1.0 ) {
                knob_state->val = -1.0;
            } else if( val > 1.0 ) {
                knob_state->val = 1.0;
            } else {
                knob_state->val = val;
            }
            break;
        case ZDJ_KNOB_TYPE_FILT_BI: 
            knob_state->val = val;
            break;
        default: break;
    }
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_knob_state_t * knob_state = (zdj_knob_state_t*)view->state;
    switch ( knob_state->type ) {
        case ZDJ_KNOB_TYPE_DET_UNITY: _draw_knob_unity( view, clip ); break;
        case ZDJ_KNOB_TYPE_DET_MIDDLE: _draw_knob_middle( view, clip ); break;
        case ZDJ_KNOB_TYPE_FILT_BI: _draw_knob_filt_bi( view, clip ); break;
        default: break;
    }
}

static void _init_knob_unity( zdj_view_t * view ) {
    zdj_knob_state_t * state = (zdj_knob_state_t*)view->state;

    state->bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DJ_KNOB_BG ], NULL );
    state->bg->frame.w = view->frame.w;
    state->bg->frame.h = view->frame.h;
    zdj_add_subview( view, state->bg );

    state->tape = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DJ_KNOB_VAL ], NULL );
    state->tape->frame.w = 0;
    state->tape->frame.h = view->frame.h;
    zdj_add_subview( view, state->tape );

    state->detent = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DJ_KNOB_DETENT ], NULL );
    state->detent->frame.x = round( view->frame.w * 0.7 );
    state->detent->frame.h = view->frame.h;
    zdj_add_subview( view, state->detent );

    state->edge = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BLACK ], NULL );
    state->edge->frame.w = 1.0;
    state->edge->frame.h = view->frame.h;
    zdj_add_subview( view, state->edge );
}

static void _draw_knob_unity( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_knob_state_t * state = (zdj_knob_state_t*)view->state;

    state->tape->frame.w = view->frame.w * state->val;
    state->edge->frame.x = floor( state->tape->frame.w );
}


static void _init_knob_middle( zdj_view_t * view ) {

}

static void _draw_knob_middle( zdj_view_t * view, zdj_view_clip_t * clip ) {
    
}

static void _init_knob_filt_bi( zdj_view_t * view ) {
    zdj_knob_state_t * state = (zdj_knob_state_t*)view->state;

    state->bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DJ_KNOB_BG ], NULL );
    state->bg->frame.w = view->frame.w;
    state->bg->frame.h = view->frame.h;
    zdj_add_subview( view, state->bg );

    state->tape = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DJ_KNOB_VAL ], NULL );
    state->tape->frame.w = 0;
    state->tape->frame.h = view->frame.h;
    zdj_add_subview( view, state->tape );

    // state->edge = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BLACK ], NULL );
    // state->edge->frame.w = 1.0;
    // state->edge->frame.h = view->frame.h;
    // zdj_add_subview( view, state->edge );
}

static void _draw_knob_filt_bi( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_knob_state_t * state = (zdj_knob_state_t*)view->state;

    zdj_remove_subview_of( view, state->tape );

    // Hi-pass
    if( state->val > 0.01 ) {
        state->tape = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DJ_KNOB_FILT_HP_VAL ], NULL );
        state->tape->frame.x = round(state->val * view->frame.w);

    // Lo-pass
    } else if( state->val < -0.01 ) {
        state->tape = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DJ_KNOB_FILT_LP_VAL ], NULL );
        state->tape->frame.x = round((state->val * view->frame.w) - (state->tape->frame.w - view->frame.w));

    // Offline
    } else {
        state->tape = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DJ_X ], NULL );
        state->tape->frame.x = (view->frame.w / 2) - 4;
        
    }
    
    
    zdj_add_subview( view, state->tape );
}

static void _deinit_state( zdj_view_t * view ) {
    zdj_knob_state_t * state = (zdj_knob_state_t*)view->state;
    free( state );
    view->state = NULL;
}
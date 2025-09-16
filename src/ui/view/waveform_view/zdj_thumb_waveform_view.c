#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/analysis/waveform/zdj_waveform.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/waveform_view/zdj_waveform_view.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_thumb_waveform_view( zdj_rect_t * frame, zdj_library_song_t * song ) {
    // printf( "zdj_new_thumb_waveform_view\n" );
    
    // Build a waveform node for the thumbnail
    char filepath[ 512 ];
    snprintf( filepath, sizeof( filepath ), "%s/%s", 
        ZDJ_LIBRARY_THUMB_WAVEFORM_DIR,
        song->entity_id
    );
    zdj_pipeline_node_t * waveform_node = zdj_new_thumbnail_waveform( filepath, frame->w );
    if( !waveform_node ) { return NULL; }
    zdj_waveform_state_t * waveform_node_state = (zdj_waveform_state_t*)waveform_node->state;

    // Build view
    zdj_view_t * view = zdj_new_view( frame );
    view->type = ZDJ_VIEW_WAVEFORM;
    view->draw = &_draw;
    view->deinit_state = &_deinit_state;

    zdj_waveform_view_state_t * state = calloc( 1, sizeof( zdj_waveform_view_state_t ) );
    view->state = state;
    state->waveform_node = waveform_node;

    // Make a new texture instance for drawing
    state->waveform_tex = SDL_CreateTexture(
        zdj_renderer( ),
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        frame->w, 
        frame->h
    );

    // Render the thumbnail points into the texture
    SDL_SetRenderTarget(zdj_renderer( ), state->waveform_tex );
    waveform_node_state->render( waveform_node, frame );
    SDL_SetRenderTarget(zdj_renderer( ), NULL);

    // printf( "zdj_new_thumb_waveform_view done\n" );
    return view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_DK_GRAY );
    // boxColor( zdj_renderer( ), clip->src.x, clip->src.y, clip->src.x+clip->src.w, clip->src.y+clip->src.h, ZDJ_MID_GRAY );

    zdj_waveform_view_state_t * view_state = (zdj_waveform_view_state_t*)view->state;

    // Draw the texture
    SDL_Rect s = { 0 + clip->src.x, 0 + clip->src.y, clip->src.w, clip->src.h };
    SDL_Rect d = { clip->dst.x, clip->dst.y, clip->dst.w, clip->dst.h };
    SDL_RenderCopy( zdj_renderer( ), view_state->waveform_tex, &s, &d );
}

static void _deinit_state( zdj_view_t * view ) {
    zdj_waveform_view_state_t * view_state = (zdj_waveform_view_state_t*)view->state;
    // Clean up mem
    SDL_DestroyTexture( view_state->waveform_tex );
}
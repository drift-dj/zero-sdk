#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/analysis/waveform/zdj_waveform.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/waveform_view/zdj_waveform_view.h>

static void _zdj_live_waveform_view_draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_live_waveform_view_deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_live_waveform_view( zdj_rect_t * frame, zdj_soundcard_node_t * node ) {
    zdj_view_t * view = zdj_new_view( frame );
    view->type = ZDJ_VIEW_WAVEFORM;
    view->draw = &_zdj_live_waveform_view_draw;
    view->deinit_state = &_zdj_live_waveform_view_deinit_state;

    zdj_live_waveform_view_state_t * state = calloc( 1, sizeof( zdj_live_waveform_view_state_t ) );
    view->state = state;
    state->node = node;

    // Get a reference to the soundcard's scope pipeline node.
    state->pipe = zdj_soundcard->scope_waveform;
    zdj_live_waveform_set_point_count( state->pipe, frame->w / 2);
    zdj_live_waveform_set_scale( state->pipe, 400 );
    // Set the soundcard to send sample data to the scope node.
    zdj_soundcard->scope_node_name = node->name;

    return view;
}

void _zdj_live_waveform_view_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    // boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_BLACK );

    // printf( "_zdj_live_waveform_view_draw: %f\n", view->frame.h );

    zdj_live_waveform_view_state_t * view_state = (zdj_live_waveform_view_state_t*)view->state;
    zdj_live_waveform_state_t * waveform_state = (zdj_live_waveform_state_t*)view_state->pipe->state;

    waveform_state->render( view_state->pipe );

    // Make a new texture instance for drawing
    SDL_Texture * waveform_tex = SDL_CreateTexture(
        zdj_renderer( ),
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        view->frame.w, 
        view->frame.h
    );

    // Grab the renderer for our texture
    SDL_SetRenderTarget(zdj_renderer( ), waveform_tex);

    for( int i=0; i<waveform_state->point_count-1; i++ ) {
        if( fabs(waveform_state->render_buf[ i ]) > 0.01 ) {
            aalineColor( 
                zdj_renderer( ), 
                i*2, 
                (waveform_state->render_buf[ i ] * (view->frame.h/2)) + (view->frame.h/2),
                i*2+1,
                (waveform_state->render_buf[ i+1 ] * (view->frame.h/2)) + (view->frame.h/2),
                ZDJ_WHITE
            );
        }
    }

    // Release the renderer
    SDL_SetRenderTarget(zdj_renderer( ), NULL);

    // Draw the texture
    SDL_Rect s = { 0 + clip->src.x, 0 + clip->src.y, clip->src.w, clip->src.h };
    SDL_Rect d = { clip->dst.x, clip->dst.y, clip->dst.w, clip->dst.h };
    SDL_RenderCopy( zdj_renderer( ), waveform_tex, &s, &d );

    // Clean up mem
    SDL_DestroyTexture( waveform_tex );
}

void _zdj_live_waveform_view_deinit_state( zdj_view_t * view ) {

}
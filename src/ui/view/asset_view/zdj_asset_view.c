#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>

void _zdj_asset_view_draw( zdj_view_t * view, zdj_view_clip_t * clip );
void _zdj_asset_view_deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_asset_view( SDL_Rect * rect, SDL_Texture * tex ) {
    zdj_view_t * view = zdj_new_view( &(zdj_rect_t){0,0,rect->w,rect->h} );
    view->draw = _zdj_asset_view_draw;
    view->deinit_state = _zdj_asset_view_deinit_state;

    // Build the texture_view state instance
    zdj_asset_view_state_t * state = calloc( 1, sizeof( zdj_asset_view_state_t ) );
    if( tex ) {
        state->tex = tex;
    } else {
        state->tex = zdj_asset_atlas( );
    }
    state->asset_rect.x = rect->x;
    state->asset_rect.y = rect->y;
    state->asset_rect.w = rect->w;
    state->asset_rect.h = rect->h;
    view->state = state;

    return view;
}

void _zdj_asset_view_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_asset_view_state_t * state = (zdj_asset_view_state_t*)view->state;
    // Debug BG
    // boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_MID_GRAY );
    
    // Draw the texture
    SDL_Rect s = { state->asset_rect.x + clip->src.x, state->asset_rect.y + clip->src.y, clip->src.w, clip->src.h };
    SDL_Rect d = { clip->dst.x, clip->dst.y, clip->dst.w, clip->dst.h };
    SDL_RenderCopy( zdj_renderer( ), state->tex, &s, &d );
}

void _zdj_asset_view_deinit_state( zdj_view_t * view ) {
    zdj_asset_view_state_t * state = (zdj_asset_view_state_t*)view->state;
    // SDL_DestroyTexture( state->tex );
    free( state );
    view->state = NULL;
}
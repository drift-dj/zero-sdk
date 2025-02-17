#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/texture_view/zdj_texture_view.h>

void _zdj_texture_view_draw( zdj_view_t * view, zdj_view_clip_t * clip );
void _zdj_texture_view_deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_texture_view( SDL_Texture * tex, int tex_w, int tex_h ) {
    zdj_view_t * view = zdj_new_view( &(zdj_rect_t){0,0,tex_w,tex_h} );
    view->draw = _zdj_texture_view_draw;
    view->deinit_state = _zdj_texture_view_deinit_state;

    // Build the texture_view state instance
    zdj_texture_view_state_t * state = calloc( 1, sizeof( zdj_texture_view_state_t ) );
    state->tex = tex;
    state->tex_w = tex_w;
    state->tex_h = tex_h;
    view->state = state;

    return view;
}


void _zdj_texture_view_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_texture_view_state_t * state = (zdj_texture_view_state_t*)view->state;

    // Debug BG
    // boxColor( zdj_renderer( ), d.x, d.y, d.x+d.w, d.y+d.h, ZDJ_MID_GRAY );
    
    // Draw the texture
    SDL_Rect s = { clip->src.x, clip->src.y, clip->src.w, clip->src.h };
    SDL_Rect d = { clip->dst.x, clip->dst.y, clip->dst.w, clip->dst.h };
    SDL_RenderCopy( zdj_renderer( ), state->tex, &s, &d );
}

void _zdj_texture_view_deinit_state( zdj_view_t * view ) {
    zdj_texture_view_state_t * state = (zdj_texture_view_state_t*)view->state;
    SDL_DestroyTexture( state->tex );
    free( state );
    view->state = NULL;
}
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
    view->type = ZDJ_VIEW_ASSET;
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
    state->clip_to_frame = true;
    view->state = state;

    return view;
}

zdj_view_t * zdj_new_asset_view_at( SDL_Rect * rect, SDL_Texture * tex, zdj_point_t * screen_coords ) {
    zdj_view_t * view = zdj_new_view( &(zdj_rect_t){screen_coords->x,screen_coords->y,rect->w,rect->h} );
    view->type = ZDJ_VIEW_ASSET;
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
    state->clip_to_frame = true;
    view->state = state;

    return view;
}

zdj_view_t * zdj_new_noclip_asset_view( SDL_Rect * rect, SDL_Texture * tex ) {
    zdj_view_t * view = zdj_new_view( &(zdj_rect_t){0,0,rect->w,rect->h} );
    view->type = ZDJ_VIEW_ASSET;
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
    state->clip_to_frame = false;
    view->state = state;

    return view;
}

void _zdj_asset_view_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_asset_view_state_t * state = (zdj_asset_view_state_t*)view->state;
    // Debug BG
    // boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_MID_GRAY );
    
    SDL_Rect s;
    SDL_Rect d;
    if( state->clip_to_frame ) {
        // Observe clip dims
        // s = { state->asset_rect.x + clip->src.x, state->asset_rect.y + clip->src.y, clip->src.w, clip->src.h };
        s.x = state->asset_rect.x + clip->src.x;
        s.y = state->asset_rect.y + clip->src.y;
        s.w = clip->src.w;
        s.h = clip->src.h;
        // d = { clip->dst.x, round(clip->dst.y), clip->dst.w, clip->dst.h };
        d.x = clip->dst.x;
        d.y = round(clip->dst.y);
        d.w = clip->dst.w;
        d.h = clip->dst.h;
    } else {
        // Ignore clip dims
        s.x = state->asset_rect.x;
        s.y = state->asset_rect.y;
        s.w = state->asset_rect.w;
        s.h = state->asset_rect.h;
        d.x = clip->screen.x; 
        d.y = round(clip->screen.y);
        d.w = state->asset_rect.w;
        d.h = state->asset_rect.h;
    }
    SDL_RenderCopy( zdj_renderer( ), state->tex, &s, &d );
}

// Not that we don't destroy the asset's texture since it's the global asset tex.
void _zdj_asset_view_deinit_state( zdj_view_t * view ) {
    zdj_asset_view_state_t * state = (zdj_asset_view_state_t*)view->state;
    free( state );
    view->state = NULL;
}
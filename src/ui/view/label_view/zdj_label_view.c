#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/font/zdj_font.h>
#include <zerodj/ui/view/zdj_view_stack.h>

void _zdj_label_view_draw( zdj_view_t * view, zdj_view_clip_t * clip );
void _zdj_label_view_deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_label_view( 
    char * str,
    zdj_font_t font,
    zdj_justify_t justify,
    SDL_Color color
) {
    // Build the label_view state instance
    zdj_label_state_t * label_state = calloc( 1, sizeof( zdj_label_state_t ) );
    label_state->str = strdup( str );
    label_state->justify = justify;

    // Build type texture
    TTF_Font * ttf_font = zdj_font( font );
    if( ttf_font ) {
        // Build the texture/text size from rendered string
        SDL_Surface *surface;
        surface = TTF_RenderText_Solid( ttf_font, label_state->str, color );
        label_state->tex = SDL_CreateTextureFromSurface( zdj_renderer( ), surface );
        label_state->tex_w = surface->w;
        label_state->tex_h = surface->h;
        SDL_FreeSurface( surface );
    } else {
        // Missing font will draw in the error texture
    }

    // Build the label's view
    zdj_view_t * label_view = zdj_new_view( &(zdj_rect_t){0,0,label_state->tex_w,label_state->tex_h} );
    label_view->draw = &_zdj_label_view_draw;
    label_view->deinit_state = &_zdj_label_view_deinit_state;
    label_view->state = (void*)label_state;

    return label_view;
}


void _zdj_label_view_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_label_state_t * label_state = (zdj_label_state_t*)view->state;

    // Debug BG
    // boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_MID_GRAY );
    
    // Draw the label
    SDL_Rect s = { clip->src.x, clip->src.y, clip->src.w, clip->src.h };
    SDL_Rect d = { clip->dst.x, clip->dst.y, clip->dst.w, clip->dst.h };
    SDL_RenderCopy( zdj_renderer( ), label_state->tex, &s, &d );
}

void _zdj_label_view_deinit_state( zdj_view_t * view ) {
    zdj_label_state_t * state = (zdj_label_state_t*)view->state;
    free( state->str );
    SDL_DestroyTexture( state->tex );
    free( state );
    view->state = NULL;
}
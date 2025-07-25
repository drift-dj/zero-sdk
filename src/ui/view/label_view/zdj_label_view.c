#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/font/zdj_font.h>
#include <zerodj/ui/view/zdj_view_stack.h>

void _zdj_label_view_draw( zdj_view_t * view, zdj_view_clip_t * clip );
void _zdj_label_view_draw_vert( zdj_view_t * view, zdj_view_clip_t * clip );
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
    label_view->type = ZDJ_VIEW_LABEL;
    label_view->draw = &_zdj_label_view_draw;
    label_view->deinit_state = &_zdj_label_view_deinit_state;
    label_view->state = (void*)label_state;

    return label_view;
}

void _zdj_label_view_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_label_state_t * label_state = (zdj_label_state_t*)view->state;    
    
    
    // Draw the label
    SDL_Rect s = { clip->src.x, clip->src.y, clip->src.w, clip->src.h };
    SDL_Rect d = { clip->dst.x, clip->dst.y, clip->dst.w, clip->dst.h };
    SDL_RenderCopy( zdj_renderer( ), label_state->tex, &s, &d );

    // if( label_state->debug ) {
        // Debug BG
        // boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_MID_GRAY );
    //     printf( "label draw s:%1.0f,%1.0f,%1.0f,%1.0f d:%1.0f,%1.0f,%1.0f,%1.0f\n",
    //         clip->src.x, clip->src.y, clip->src.w, clip->src.h,
    //         clip->dst.x, clip->dst.y, clip->dst.w, clip->dst.h 
    //     );
    // }
}


zdj_view_t * zdj_new_label_vert_view( 
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
        label_state->tex_w = surface->h;
        label_state->tex_h = surface->w;
        SDL_FreeSurface( surface );
    } else {
        // Missing font will draw in the error texture
    }

    // Build the label's view
    zdj_view_t * label_view = zdj_new_view( &(zdj_rect_t){0,0,label_state->tex_w,label_state->tex_h} );
    // zdj_view_t * label_view = zdj_new_view( &(zdj_rect_t){0,0,label_state->tex_h,label_state->tex_w} );
    label_view->type = ZDJ_VIEW_LABEL;
    label_view->draw = &_zdj_label_view_draw_vert;
    label_view->deinit_state = &_zdj_label_view_deinit_state;
    label_view->state = (void*)label_state;

    return label_view;
}

void _zdj_label_view_draw_vert( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_label_state_t * label_state = (zdj_label_state_t*)view->state;    
    
    
    // Draw the label
    SDL_Rect s = { clip->src.x, clip->src.y, clip->src.w, clip->src.h };
    SDL_Rect d = { clip->dst.x, clip->dst.y, clip->dst.w, clip->dst.h };
    // SDL_Rect s = { clip->src.y, clip->src.x, clip->src.h, clip->src.w };
    // SDL_Rect d = { clip->dst.x, clip->dst.y, clip->dst.w, clip->dst.h };
    // SDL_RenderCopy( zdj_renderer( ), label_state->tex, &s, &d );
    SDL_Point roto_point = { .x=0, .y=0 };
    SDL_RenderCopyEx( zdj_renderer( ), label_state->tex, &s, &d, 270, &roto_point, SDL_FLIP_NONE );

    // if( label_state->debug ) {
        // Debug BG
        // boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, ZDJ_MID_GRAY );
    //     printf( "label draw s:%1.0f,%1.0f,%1.0f,%1.0f d:%1.0f,%1.0f,%1.0f,%1.0f\n",
    //         clip->src.x, clip->src.y, clip->src.w, clip->src.h,
    //         clip->dst.x, clip->dst.y, clip->dst.w, clip->dst.h 
    //     );
    // }
}


void _zdj_label_view_deinit_state( zdj_view_t * view ) {
    zdj_label_state_t * state = (zdj_label_state_t*)view->state;
    free( state->str );
    SDL_DestroyTexture( state->tex );
    free( state );
    view->state = NULL;
}


// SDL_Texture * roto_small_string( 
//     SDL_Renderer * renderer, 
//     char * str, 
//     int x,
//     int y,
//     double angle, 
//     SDL_Rect * result_rect 
// ) {
//     int text_width;
//     int text_height;
//     SDL_Surface *surface;
//     surface = TTF_RenderText_Solid( very_small_font, str, white_text_color );
//     SDL_Texture * texture = SDL_CreateTextureFromSurface( renderer, surface );
//     text_width = surface->w;
//     text_height = surface->h;
//     SDL_FreeSurface(surface);
//     font_scratch_rect->x = x;
//     font_scratch_rect->y = y;
//     font_scratch_rect->w = text_width;
//     font_scratch_rect->h = text_height;
//     SDL_Point roto_point = { .x=0, .y=0 };
//     SDL_RenderCopyEx( renderer, texture, NULL, font_scratch_rect, angle, &roto_point, SDL_FLIP_NONE );
//     SDL_DestroyTexture( texture );
// }
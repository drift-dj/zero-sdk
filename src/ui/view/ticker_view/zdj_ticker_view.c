#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/texture_view/zdj_texture_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/font/zdj_font.h>
#include <zerodj/ui/view/zdj_view_stack.h>

void _zdj_ticker_view_draw( zdj_view_t * view, zdj_view_clip_t * clip );
void _zdj_ticker_view_deinit_state( zdj_view_t * view );

// Ticker displays a single line of text, scrolling within a clipping frame.
// If the text is shorter than the clipping frame, it does not scroll.
zdj_view_t * zdj_new_ticker_view( 
    char * str,
    zdj_font_t font,
    zdj_justify_t justify,
    SDL_Color color
) {
    // Build the ticker's view
    zdj_view_t * view = zdj_new_view( NULL );
    view->draw = &_zdj_ticker_view_draw;
    view->deinit_state = &_zdj_ticker_view_deinit_state;

    // Build the ticker_view state instance
    zdj_ticker_state_t * state = calloc( 1, sizeof( zdj_ticker_state_t ) );
    state->scroll_offset = 0;
    state->scroll_rate = 0.3;
    state->str = strdup( str );
    state->justify = justify;
    view->state = (void*)state;

    // Build type texture
    TTF_Font * ttf_font = zdj_font( font );
    if( ttf_font ) {
        // We need to double the string, because a scrolling marquee
        // requires the beginning and end of the string to appear a 
        // few pixels from eachother to acheive seemless scrolling.
        char double_str[ 1024 ];
        snprintf( double_str, sizeof( double_str ), "%s - %s",
            str,
            str
        );

        // Build the texture/text size from rendered string
        SDL_Surface *surface;
        surface = TTF_RenderText_Solid( ttf_font, double_str, color );

        // Because we've duplicated the input string, we need to cut the width in half.
        state->single_text_w = (surface->w / 2)-3;
        state->double_text_w = surface->w;
        state->text_h = surface->h;
        
        // Add the text texture view
        state->texture_view = zdj_new_texture_view( 
            SDL_CreateTextureFromSurface( zdj_renderer( ), surface ),
            surface->w,
            surface->h
        );
        zdj_add_subview( view, state->texture_view );

        SDL_FreeSurface( surface );
    } else {
        // Missing font will draw in the error texture
        // ticker_state->tex = zdj_ui_asset_error_tex( );
        // ticker_state->tex_w = 4;
        // ticker_state->tex_h = 4;
    }

    return view;
}

void _zdj_ticker_view_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_ticker_state_t * state = (zdj_ticker_state_t*)view->state;

    // Figure out if we're scrolling - based on view width vs. width of string
    bool is_scrolling = (view->frame->w < state->single_text_w);

    if( is_scrolling ) {
        state->texture_view->frame->w = state->double_text_w;
        state->text_w = view->frame->w;

        // Update scroll_offset
        state->scroll_offset += state->scroll_rate;
        if( state->scroll_offset > state->single_text_w ) {
            state->scroll_offset -= state->single_text_w + 6;
        }
        state->texture_view->frame->x = state->scroll_offset * -1;
    } else {
        state->text_w = state->single_text_w;

        // Adjust texture_view frame within view's frame based on justify setting.
        if( state->justify == ZDJ_JUSTIFY_LEFT ) {
            state->texture_view->frame->x = 0;
        } else if( state->justify == ZDJ_JUSTIFY_RIGHT ) {
            state->texture_view->frame->x = view->frame->w - state->single_text_w;
        }
        state->texture_view->frame->w = state->single_text_w;
    }
}

void _zdj_ticker_view_deinit_state( zdj_view_t * view ) {
    zdj_ticker_state_t * state = (zdj_ticker_state_t*)view->state;
    free( state->str );
    free( state );
    view->state = NULL;
}
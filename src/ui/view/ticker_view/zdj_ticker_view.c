#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/font/zdj_font.h>
#include <zerodj/ui/view/zdj_view_stack.h>

void draw_ticker_view( zdj_view_t * ticker_view, zdj_view_clip_t * clip );

// Ticker displays a single line of text, scrolling within a clipping frame.
// If the text is shorter than the clipping frame, it does not scroll.
zdj_view_t * new_ticker_view( 
    char * str,
    zdj_font_t font,
    zdj_justify_t justify,
    SDL_Color color,
    zdj_rect_t * frame
) {
    // Build the ticker_view state instance
    zdj_ticker_state_t * ticker_state = malloc( sizeof( zdj_ticker_state_t ) );
    ticker_state->scroll_offset = 0;
    ticker_state->scroll_rate = 0.3;
    ticker_state->str = strdup( str );
    ticker_state->justify = justify;

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
        ticker_state->tex = SDL_CreateTextureFromSurface( zdj_renderer( ), surface );
        ticker_state->tex_w = surface->w;
        ticker_state->tex_h = surface->h;
        // Because we've duplicated the input string, we need to cut the width in half.
        ticker_state->string_w = (ticker_state->tex_w / 2)+3;
        SDL_FreeSurface( surface );
    } else {
        // Missing font will draw in the error texture
        ticker_state->tex = zdj_ui_asset_error_tex( );
        ticker_state->tex_w = 4;
        ticker_state->tex_h = 4;
    }

    // Build the ticker's view
    zdj_view_t * ticker_view = zdj_view_stack_new_view( );
    ticker_view->draw = &draw_ticker_view;
    ticker_view->state = (void*)ticker_state;

    // Build frame based on string texture metrics and input rect.
    zdj_rect_t * f = calloc( 1, sizeof( zdj_rect_t ) );
    ticker_view->frame = f;
    // Set vertical frame
    f->h = ticker_state->tex_h;

    // Set horizontal frame
    // We double the string on input to make scrolling work,
    // so we must cut it in half here.
    if( ticker_state->string_w > frame->w ) {
        // Only scroll if string is wider than frame
        ticker_state->is_scrolling = true;
        f->w = frame->w;
    } else {
        // A narrow string - we constrain the frame width to string width
        ticker_state->is_scrolling = false;
        f->w = ticker_state->string_w;
    }
    
    return ticker_view;
}


void draw_ticker_view( zdj_view_t * ticker_view, zdj_view_clip_t * clip ) {
    zdj_ticker_state_t * ticker_state = (zdj_ticker_state_t*)ticker_view->state;

    SDL_Rect s;
    if( ticker_state->is_scrolling ) {
        // Update scroll offset based on scroll rate
        ticker_state->scroll_offset += ticker_state->scroll_rate;
        if( ticker_state->scroll_offset > ticker_state->string_w ) {
            ticker_state->scroll_offset -= ticker_state->string_w;
        }
        // If we are scrolling, we first need to clip into the text texture based
        // on scroll offset.
        s.x = ticker_state->scroll_offset;
        s.y = 0;
        s.w = ticker_view->frame->w;
        s.h = ticker_view->frame->h;
        // We then need to clip that clipping rect using clip->src.
        s.x += clip->src.x;
        s.y += clip->src.y;
        s.w = clip->src.w;
        s.h = clip->src.h;
    } else {
        // If we're not scrolling, just clip into the text texture based on clip->src.
        s.x = clip->src.x;
        s.y = clip->src.y;
        s.w = clip->src.w;
        s.h = clip->src.h;
    }

    // Dest rect comes directly from zdj_view clipping geometry
    SDL_Rect d = { clip->dst.x, clip->dst.y, clip->dst.w, clip->dst.h };

    // Draw the ticker
    SDL_RenderCopy( zdj_renderer( ), ticker_state->tex, &s, &d );
}

void free_ticker_view( zdj_view_t * ticker_view ) {
    zdj_ticker_state_t * ticker_state = (zdj_ticker_state_t*)ticker_view->state;
    free( ticker_state->str );
    SDL_DestroyTexture( ticker_state->tex );
    free( ticker_state );
}
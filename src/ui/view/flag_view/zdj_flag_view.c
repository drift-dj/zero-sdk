#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/flag_view/zdj_flag_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/font/zdj_font.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip );

static void _draw_cue_mini( zdj_view_t * view, zdj_view_clip_t * clip );
static void _draw_cue_norm( zdj_view_t * view, zdj_view_clip_t * clip );
static void _draw_cue_loop( zdj_view_t * view, zdj_view_clip_t * clip );
static void _draw_bar( zdj_view_t * view, zdj_view_clip_t * clip );
static void _draw_text( zdj_view_t * view, zdj_view_clip_t * clip );

static void _deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_flag_view( 
    zdj_flag_type_t type,
    char * str
) {
    // Build the flag_view state instance
    zdj_flag_state_t * flag_state = calloc( 1, sizeof( zdj_flag_state_t ) );
    strcpy( flag_state->str, str );
    flag_state->type = type;

    // Build type texture
    TTF_Font * ttf_font = zdj_font( ZDJ_FONT_6_CAPS );
    if( ttf_font ) {
        // Build the texture/text size from rendered string
        SDL_Surface *surface;
        if( type == ZDJ_FLAG_TYPE_BAR ) { 
            surface = TTF_RenderText_Solid( ttf_font, flag_state->str, ZDJ_SDL_WHITE );
        } else {
            surface = TTF_RenderText_Solid( ttf_font, flag_state->str, ZDJ_SDL_BLACK );
        }
        flag_state->tex = SDL_CreateTextureFromSurface( zdj_renderer( ), surface );
        flag_state->tex_w = surface->w;
        flag_state->tex_h = surface->h;
        SDL_FreeSurface( surface );
    } else {
        // Missing font will draw in the error texture
    }

    // Build the label's view
    zdj_view_t * flag_view = zdj_new_view( &(zdj_rect_t){0,0,flag_state->tex_w,flag_state->tex_h} );
    flag_view->type = ZDJ_VIEW_LABEL;
    flag_view->draw = &_draw;
    flag_view->deinit_state = &_deinit_state;
    flag_view->state = (void*)flag_state;

    return flag_view;
}

static void _draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_flag_state_t * flag_state = (zdj_flag_state_t*)view->state;  
    
    switch ( flag_state->type ) {
        case ZDJ_FLAG_TYPE_CUE_MINI: _draw_cue_mini( view, clip ); break;
        case ZDJ_FLAG_TYPE_CUE_NORM: _draw_cue_norm( view, clip ); break;
        case ZDJ_FLAG_TYPE_CUE_LOOP: _draw_cue_loop( view, clip ); break;
        case ZDJ_FLAG_TYPE_BAR: _draw_bar( view, clip ); break;
        default: break;
    }
}

static void _draw_cue_mini( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_flag_state_t * flag_state = (zdj_flag_state_t*)view->state; 
    
    float flag_w = flag_state->tex_w;
    
    // Draw the background
    boxColor( zdj_renderer( ), clip->dst.x+1, clip->dst.y, clip->dst.x+flag_w-1, clip->dst.y+clip->dst.h-1, ZDJ_WHITE );
    
    // Draw the label
    SDL_Rect s = { clip->src.x, clip->src.y, clip->src.w, clip->src.h };
    SDL_Rect d = { clip->dst.x+4, clip->dst.y-1, clip->dst.w, clip->dst.h };
    SDL_RenderCopy( zdj_renderer( ), flag_state->tex, &s, &d );

    lineColor( zdj_renderer( ), clip->dst.x, clip->dst.y+1, clip->dst.x, clip->dst.y+clip->dst.h+1, ZDJ_WHITE );
    lineColor( zdj_renderer( ), clip->dst.x+flag_w, clip->dst.y+1, clip->dst.x+flag_w, clip->dst.y+clip->dst.h-2, ZDJ_WHITE );

    lineColor( zdj_renderer( ), clip->dst.x-1, clip->dst.y+1, clip->dst.x-1, clip->dst.y+clip->dst.h, ZDJ_BLACK );
    lineColor( zdj_renderer( ), clip->dst.x+1, clip->dst.y+7, clip->dst.x+flag_w-1, clip->dst.y+7, ZDJ_BLACK );
    lineColor( zdj_renderer( ), clip->dst.x+flag_w+1, clip->dst.y+1, clip->dst.x+flag_w+1, clip->dst.y+5, ZDJ_BLACK );

    pixelColor( zdj_renderer( ), clip->dst.x+flag_w, clip->dst.y+6, ZDJ_BLACK );
    pixelColor( zdj_renderer( ), clip->dst.x, clip->dst.y+8, ZDJ_BLACK );
}

static void _draw_cue_norm( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_flag_state_t * state = (zdj_flag_state_t*)view->state; 

    zdj_remove_all_subviews_of( view ); 

    zdj_view_t * cue_label = zdj_new_label_view( 
        state->str, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK 
    );
    zdj_label_state_t * label_state = (zdj_label_state_t*)cue_label->state;

    view->frame.w = label_state->tex_w + 7;

    // Add BG
    zdj_view_t * cue_l = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_CUEPOINT_L ], NULL );
    zdj_add_subview( view, cue_l );
    cue_l->frame.w = label_state->tex_w+5;

    zdj_view_t * cue_r = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_CUEPOINT_R ], NULL );
    zdj_add_subview( view, cue_r );
    cue_r->frame.x = label_state->tex_w+5;

    // Add type icon
    zdj_view_t * icon = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_PLAY_SM ], NULL );
    zdj_add_subview( view, icon );
    icon->frame.x = 2;
    icon->frame.y = 3;
    // Add label
    zdj_add_subview( view, cue_label );
    cue_label->frame.x = 6;
}

static void _draw_cue_loop( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_flag_state_t * state = (zdj_flag_state_t*)view->state; 

    zdj_remove_all_subviews_of( view ); 

    zdj_view_t * cue_label = zdj_new_label_view( 
        state->str, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK 
    );
    zdj_label_state_t * label_state = (zdj_label_state_t*)cue_label->state;

    view->frame.w = label_state->tex_w + 9;

    // Add BG
    zdj_view_t * cue_l = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_CUEPOINT_L ], NULL );
    zdj_add_subview( view, cue_l );
    cue_l->frame.w = label_state->tex_w+7;

    zdj_view_t * cue_r = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_CUEPOINT_R ], NULL );
    zdj_add_subview( view, cue_r );
    cue_r->frame.x = label_state->tex_w+7;

    // Add type icon
    zdj_view_t * icon = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_LOOP_SM ], NULL );
    zdj_add_subview( view, icon );
    icon->frame.x = 2;
    icon->frame.y = 3;
    // Add label
    zdj_add_subview( view, cue_label );
    cue_label->frame.x = 8;
}

static void _draw_bar( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_flag_state_t * state = (zdj_flag_state_t*)view->state; 

    zdj_remove_all_subviews_of( view ); 

    zdj_view_t * label = zdj_new_label_view( 
        state->str, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE 
    );
    zdj_label_state_t * label_state = (zdj_label_state_t*)label->state;

    // Add BG
    zdj_view_t * flag_l = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BG_FLAG_L ], NULL );
    zdj_add_subview( view, flag_l );
    flag_l->frame.y = 1;

    zdj_view_t * flag_c = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BG_FLAG_C ], NULL );
    zdj_add_subview( view, flag_c );
    flag_c->frame.x = 7;
    flag_c->frame.w = label_state->tex_w-1;

    zdj_view_t * flag_r = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BG_FLAG_R ], NULL );
    zdj_add_subview( view, flag_r );
    flag_r->frame.x = label_state->tex_w+7-1;

    // Add label
    zdj_add_subview( view, label );
    label->frame.x = 7;
}

static void _deinit_state( zdj_view_t * view ) {
    zdj_flag_state_t * state = (zdj_flag_state_t*)view->state;
    SDL_DestroyTexture( state->tex );
    free( state );
    view->state = NULL;
}
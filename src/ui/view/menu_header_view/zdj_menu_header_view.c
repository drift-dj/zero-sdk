#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_header_view/zdj_menu_header_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

void _zdj_menu_header_draw( zdj_view_t * view, zdj_view_clip_t * clip );
void _zdj_menu_header_update_layout( zdj_view_t * view, zdj_view_clip_t * clip );
void _zdj_menu_header_deinit_state( zdj_view_t * view );

// Parse a static XML node into a menu_item_view.
zdj_view_t * zdj_new_menu_header( 
    char * name, 
    char * title, 
    zdj_menu_view_header_style_t style, 
    zdj_menu_view_header_back_style_t back_btn_style 
) {
    // Make view
    zdj_view_t * menu_header = zdj_new_view( NULL );
    menu_header->type = ZDJ_VIEW_MENU_HEADER;
    menu_header->draw = &_zdj_menu_header_draw;
    menu_header->deinit_state = &_zdj_menu_header_deinit_state;

    // Add back btn show/hide anims
    // menu_header->in_anim = zdj_new_anim( ZDJ_ANIM_HEADER_ACTIVATE );
    // menu_header->out_anim = zdj_new_anim( ZDJ_ANIM_HEADER_DEACTIVATE );
    zdj_set_anim( &menu_header->in_anim, ZDJ_ANIM_HEADER_ACTIVATE );
    zdj_set_anim( &menu_header->out_anim, ZDJ_ANIM_HEADER_DEACTIVATE );
    
    // Build state
    zdj_menu_header_view_state_t * state = calloc( 1, sizeof( zdj_menu_header_view_state_t ) );
    menu_header->state = state;
    strcpy( state->name, name );
    strcpy( state->title, title );
    state->style = style;
    state->back_style = back_btn_style;
    state->has_back = (back_btn_style > ZDJ_MENU_HEADER_BACK_STYLE_NONE) ? true : false;
    state->show_back = false;
    state->hide_back = false;
    state->back_hidden = true;
    state->has_valid_display = false;
    
    return menu_header;
}

void _zdj_menu_header_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_menu_header_view_state_t * state = (zdj_menu_header_view_state_t*)view->state;

    // BG
    boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h-1, ZDJ_WHITE );

    if( !state->has_valid_display ) {
        _zdj_menu_header_update_layout( view, clip );
    }

    // If show_back is requested by menu system, animate back button into view
    if( state->show_back && state->back_hidden ) {
        // activate header in_anim
        // if( view->in_anim ) {
        //     ((anim_init_t)view->in_anim->init_fn)( view->in_anim, view );
        //     view->anim = view->in_anim;
        // }
        ((anim_init_t)view->in_anim.init_fn)( &view->in_anim, view );
        view->anim = &view->in_anim;
        state->show_back = false;
        state->back_hidden = false;
    }
    
    // If hide_back is requested by menu system, animate back button out of view
    if( state->hide_back && !state->back_hidden ) {
        // activate header out_anim
        // if( view->out_anim ) {
        //     ((anim_init_t)view->out_anim->init_fn)( view->out_anim, view );
        //     view->anim = view->out_anim;
        // }
        ((anim_init_t)view->out_anim.init_fn)( &view->out_anim, view );
        view->anim = &view->out_anim;
        state->hide_back = false;
        state->back_hidden = true;
    }

    // Run the blink cycle
    if( state->is_blinking ) {
        if( state->blink_timer++ > ZDJ_BLINK_LENGTH ) {
            // exit blink state after time
            state->is_blinking = false;
            state->blink_timer = 0;
        } else {
            // blink on a duty cycle
            if( state->blink_timer % ZDJ_BLINK_PERIOD > ZDJ_BLINK_DUTY ) {
                boxColor( zdj_renderer( ), clip->dst.x, clip->dst.y, clip->dst.x+clip->dst.w, clip->dst.y+clip->dst.h, 0xFFFF0000 );
            }
        }
    }
}

void _zdj_menu_header_deinit_state( zdj_view_t * view ) {
    zdj_menu_header_view_state_t * state = (zdj_menu_header_view_state_t*)view->state;
    free( state );
    view->state = NULL;
}

void _zdj_menu_header_update_layout( zdj_view_t * header, zdj_view_clip_t * clip ) {
    zdj_menu_header_view_state_t * state = (zdj_menu_header_view_state_t*)header->state;

    // Remove all subviews
    zdj_remove_all_subviews_of( header ); 

    // Setup name label
    zdj_view_t * name_label = zdj_new_label_view( state->name, ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    name_label->frame.y = -1;
    state->name_label = name_label;
    zdj_add_subview( header, name_label );

    // Setup title divider
    // zdj_view_t * title_divider = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DOT_PIPE_HI ], NULL );
    zdj_view_t * title_divider = zdj_new_asset_view( &(SDL_Rect){13,81,1,6}, NULL );
    title_divider->frame.y = -1;
    title_divider->frame.x = state->name_label->frame.w + 2;
    state->title_divider = title_divider;
    zdj_add_subview( header, title_divider );

    // Setup title ticker
    zdj_view_t * title_ticker = zdj_new_ticker_view( state->title, ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    state->title_ticker = title_ticker;
    zdj_add_subview( header, title_ticker );
    state->title_ticker->frame.y = -1;
    state->title_ticker->frame.h = 10;

    // Setup back button if needed
    if( state->has_back ) {
        state->back_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BLACK ], NULL );
        zdj_add_subview( header, state->back_bg );

        zdj_view_t * back_view;
        switch ( state->back_style ) {
        case ZDJ_MENU_HEADER_BACK_STYLE_BACK:
            back_view = zdj_new_label_view( "<BACK", ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
            break;
        case ZDJ_MENU_HEADER_BACK_STYLE_CANCEL:
            back_view = zdj_new_label_view( "<CANCEL", ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
            break;
        case ZDJ_MENU_HEADER_BACK_STYLE_EXIT:
            back_view = zdj_new_label_view( "<EXIT", ZDJ_FONT_6_CAPS, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
            break;
        default:
            break;
        }
        back_view->frame.x = 2;
        back_view->frame.y = 10;
        state->back_view = back_view;
        zdj_add_subview( header, back_view );

        state->back_bg->frame.x = 0;
        state->back_bg->frame.y = 10;
        state->back_bg->frame.w = back_view->frame.w + 5;
        state->back_bg->frame.h = 10;
    }

    state->name_label->frame.x = 1;
    state->title_divider->frame.x = state->name_label->frame.w + 2;
    state->title_divider->frame.y = 0;
    state->title_ticker->frame.x = state->title_divider->frame.x + 2;
    state->title_ticker->frame.w = header->frame.w - state->title_ticker->frame.x;

    state->has_valid_display = true;
}
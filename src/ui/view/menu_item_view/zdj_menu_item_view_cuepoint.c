#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>

void zdj_menu_item_cuepoint_init_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    view->frame.h = 10;

    // Clear out the normal/hilite views' subviews
    if( state->hilite_view ) { 
        zdj_remove_all_subviews_of( state->hilite_view ); 
    } else {
        state->hilite_view = zdj_new_view( NULL );
        zdj_add_subview( view, state->hilite_view );
        state->hilite_view->frame.w = view->frame.w;
        state->hilite_view->frame.h = view->frame.h;
    }
    if( state->normal_view ) { 
        zdj_remove_all_subviews_of( state->normal_view );
    } else {
        state->normal_view = zdj_new_view( NULL );
        zdj_add_subview( view, state->normal_view );
        state->normal_view->frame.w = view->frame.w;
        state->normal_view->frame.h = view->frame.h;
    }

    // Setup normal view
    zdj_view_t * cue_label = zdj_new_label_view( 
        state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK 
    );
    zdj_label_state_t * label_state = (zdj_label_state_t*)cue_label->state;
    
    zdj_view_t * cue_l = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_CUEPOINT_L ], NULL );
    zdj_add_subview( state->normal_view, cue_l );
    cue_l->frame.w = label_state->tex_w+1;

    zdj_view_t * cue_r = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_CUEPOINT_R ], NULL );
    zdj_add_subview( state->normal_view, cue_r );
    cue_r->frame.x = label_state->tex_w+1;

    zdj_add_subview( state->normal_view, cue_label );
    cue_label->frame.x = 2;
    

    // Setup hilite view
    zdj_view_t * cue_label_hi = zdj_new_label_view( 
        state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE
    );

    zdj_view_t * cue_l_hi = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_CUEPOINT_L_HI ], NULL );
    zdj_add_subview( state->hilite_view, cue_l_hi );
    cue_l_hi->frame.w = label_state->tex_w+1;

    zdj_view_t * cue_r_hi = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_CUEPOINT_R_HI ], NULL );
    zdj_add_subview( state->hilite_view, cue_r_hi );
    cue_r_hi->frame.x = label_state->tex_w+1;

    zdj_add_subview( state->hilite_view, cue_label_hi );
    cue_label_hi->frame.x = 2;

    state->needs_layout_init = false;
}
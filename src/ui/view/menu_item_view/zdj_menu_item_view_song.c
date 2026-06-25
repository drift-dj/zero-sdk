#include <stdio.h>
#include <string.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/deck/zdj_deck_manager.h>
#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/ticker_view/zdj_ticker_view.h>
#include <zerodj/ui/view/progress_bar_view/zdj_progress_bar_view.h>

static void _draw_base( zdj_view_t * view );
static void _draw_deck_1( zdj_view_t * view );
static void _draw_deck_2( zdj_view_t * view );
static void _draw_edit( zdj_view_t * view );
static void _draw_move_item( zdj_view_t * view );
static void _draw_delete( zdj_view_t * view );
static void _draw_add_to_list( zdj_view_t * view );

static void _enter_edit_mode( zdj_view_t * item );
static void _exit_edit_mode( zdj_view_t * item );

static SDL_Rect * _get_key_asset( zdj_library_key_t key_enum ) {
    // printf( "_get_key_asset: %d\n", key_enum );
    switch ( key_enum ) {
        case ZDJ_LIBRARY_KEY_A: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_A ];
        case ZDJ_LIBRARY_KEY_AM: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_AM ];
        case ZDJ_LIBRARY_KEY_AB: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_AB ];
        case ZDJ_LIBRARY_KEY_ABM: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_ABM ];
        case ZDJ_LIBRARY_KEY_B: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_B ];
        case ZDJ_LIBRARY_KEY_BM: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_BM ];
        case ZDJ_LIBRARY_KEY_BB: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_BB ];
        case ZDJ_LIBRARY_KEY_BBM: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_BBM ];
        case ZDJ_LIBRARY_KEY_C: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_C ];
        case ZDJ_LIBRARY_KEY_CM: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_CM ];
        case ZDJ_LIBRARY_KEY_D: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_D ];
        case ZDJ_LIBRARY_KEY_DM: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_DM ];
        case ZDJ_LIBRARY_KEY_DB: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_DB ];
        case ZDJ_LIBRARY_KEY_DBM: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_DBM ];
        case ZDJ_LIBRARY_KEY_E: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_E ];
        case ZDJ_LIBRARY_KEY_EM: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_EM ];
        case ZDJ_LIBRARY_KEY_EB: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_EB ];
        case ZDJ_LIBRARY_KEY_EBM: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_EBM ];
        case ZDJ_LIBRARY_KEY_F: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_F ];
        case ZDJ_LIBRARY_KEY_FM: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_FM ];
        case ZDJ_LIBRARY_KEY_FS: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_FS ];
        case ZDJ_LIBRARY_KEY_FSM: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_FSM ];
        case ZDJ_LIBRARY_KEY_G: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_G ];
        case ZDJ_LIBRARY_KEY_GM: return &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_GM ];
        default: return NULL;
    }
}

void zdj_menu_item_song_import_init_layout( zdj_view_t * view ) {
    // printf( "zdj_menu_item_song_import_init_layout\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    // Add title ticker
    state->normal_view = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE );
    zdj_add_subview( view, state->normal_view );
    state->normal_view->frame.x = ZDJ_MENU_ITEM_MARGIN_L;
    state->normal_view->frame.w = view->frame.w - ZDJ_MENU_ITEM_MARGIN_L - ZDJ_MENU_ITEM_MARGIN_R;
    state->normal_view->frame.h = view->frame.h;

    state->needs_layout_init = false;

    // printf( "zdj_menu_item_song_import_init_layout done\n" );
}

void zdj_menu_item_song_import_update_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    state->normal_view->frame.w = view->frame.w - 31;

    if( !state->hilite_view ) {
        state->hilite_view = zdj_new_progress_bar_view( &(zdj_rect_t){ view->frame.w-28,2,27,5 }, ZDJ_PROGRESS_BAR_VIEW_NORMAL );
        zdj_add_subview( view, state->hilite_view );
    }

    zdj_library_song_t * song = (zdj_library_song_t*)state->data.ptr;
    if( state->hilite_view && song ) {
        zdj_progress_bar_view_state_t * progress_state = (zdj_progress_bar_view_state_t*)state->hilite_view->state;
        progress_state->val = song->analysis_progress;
    }
    
    state->needs_layout_update = false;
}


/////////////
// Songs
/////////////
void zdj_menu_item_song_init_layout( zdj_view_t * view ) {
    // printf( "zdj_menu_item_song_init_layout\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

    state->enter_edit_mode = _enter_edit_mode;
    state->exit_edit_mode = _exit_edit_mode;
    state->scroll_to_exit_edit_mode = true;

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

    view->frame.h = 8;
    
    // Setup normal view
    zdj_view_t * title_norm = zdj_new_label_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( state->normal_view, title_norm );
    title_norm->frame.x = 1;
    title_norm->frame.y = -1;

    // Setup hilite view
    zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_L ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg );
    hilite_bg->frame.y = 0;
    hilite_bg->frame.w = view->frame.w;
    hilite_bg->frame.h = 8;
    zdj_view_t * hilite_bg_r = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_R ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg_r );
    hilite_bg_r->frame.y = 0;
    hilite_bg_r->frame.x = view->frame.w-2;
    hilite_bg_r->frame.h = 8;

    zdj_view_t * title_ticker_hilite = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    zdj_ticker_state_t * ticker_state = (zdj_ticker_state_t*)title_ticker_hilite->state;
    zdj_add_subview( state->hilite_view, title_ticker_hilite );
    title_ticker_hilite->frame.x = 1;
    title_ticker_hilite->frame.y = -1;
    title_ticker_hilite->frame.w = view->frame.w;
    title_ticker_hilite->frame.h = view->frame.h;

    // Add key/bpm
    zdj_view_t * xtra_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BLACK ], NULL );
    zdj_add_subview( state->hilite_view, xtra_bg );
    hilite_bg->frame.y = 0;
    hilite_bg->frame.h = 8;

    float xtra_x = view->frame.w;

    // We're using i_val as a bitfield here.
    // Clearly the menu item data needs a re-architecting...
    bool show_divider = false;
    bool show_key = state->data.i_val & 0x1;
    bool show_camelot = (state->data.i_val >> 1) & 0x1;
    bool show_bpm = (state->data.i_val >> 2) & 0x1;
    bool show_error = (state->data.i_val >> 3) & 0x1;
    int key_enum = (state->data.i_val >> 8) & 0xFF;
    int bpm = state->data.i_val >> 16;
    // printf( "init song item: sk:%d sb:%d sc:%d\n", 
    //     show_key, show_bpm, show_camelot  
    // );
    if( show_error ) {
        show_divider = true;

        zdj_view_t * alert = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_ALERT ], NULL );
        zdj_add_subview( state->hilite_view, alert );
        alert->frame.x = view->frame.w - alert->frame.w;
        alert->frame.y = 1;
        xtra_x = xtra_x - (alert->frame.w + 1);
    }

    if( show_camelot ) { 
        if( show_divider ) {
            zdj_view_t * spacer = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DJ_KNOB_BG ], NULL );
            zdj_add_subview( state->hilite_view, spacer );
            spacer->frame.w = 1;
            spacer->frame.y = 1;
            spacer->frame.x = xtra_x - 2;
            xtra_x = xtra_x - 3;
        }
        
        zdj_library_camelot_t camelot = zdj_library_get_camelot( 
            zdj_deck_manager_get_current_key( ), key_enum 
        );
        show_divider = true;

        if( camelot == ZDJ_LIBRARY_CAMELOT_EQUAL ) {
            zdj_view_t * cam = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_EQUAL ], NULL );
            zdj_add_subview( state->hilite_view, cam );
            cam->frame.x = xtra_x - cam->frame.w;
            xtra_x = xtra_x - cam->frame.w;
        } else if ( camelot == ZDJ_LIBRARY_CAMELOT_PLUS ) {
            zdj_view_t * cam = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_PLUS ], NULL );
            zdj_add_subview( state->hilite_view, cam );
            cam->frame.x = xtra_x - cam->frame.w;
            xtra_x = xtra_x - cam->frame.w;
        } else if ( camelot == ZDJ_LIBRARY_CAMELOT_PLUS_PLUS ) {
            zdj_view_t * cam = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_PLUS ], NULL );
            zdj_add_subview( state->hilite_view, cam );
            cam->frame.x = xtra_x - cam->frame.w;
            xtra_x = xtra_x - cam->frame.w;
            cam = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_PLUS ], NULL );
            zdj_add_subview( state->hilite_view, cam );
            cam->frame.x = xtra_x - cam->frame.w;
            xtra_x = xtra_x - cam->frame.w;
        } else if ( camelot == ZDJ_LIBRARY_CAMELOT_MINUS ) {
            zdj_view_t * cam = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_MINUS ], NULL );
            zdj_add_subview( state->hilite_view, cam );
            cam->frame.x = xtra_x - cam->frame.w;
            xtra_x = xtra_x - cam->frame.w;
        } else if ( camelot == ZDJ_LIBRARY_CAMELOT_MINUS_MINUS ) {
            zdj_view_t * cam = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_MINUS ], NULL );
            zdj_add_subview( state->hilite_view, cam );
            cam->frame.x = xtra_x - cam->frame.w;
            xtra_x = xtra_x - cam->frame.w;
            cam = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_MINUS ], NULL );
            zdj_add_subview( state->hilite_view, cam );
            cam->frame.x = xtra_x - cam->frame.w;
            xtra_x = xtra_x - cam->frame.w;
        } else if ( camelot == ZDJ_LIBRARY_CAMELOT_TILDA ) {
            zdj_view_t * cam = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_TILDA ], NULL );
            zdj_add_subview( state->hilite_view, cam );
            cam->frame.x = xtra_x - cam->frame.w;
            xtra_x = xtra_x - cam->frame.w;
        } else if ( camelot == ZDJ_LIBRARY_CAMELOT_DELTA ) {
            zdj_view_t * cam = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_KEY_DELTA ], NULL );
            zdj_add_subview( state->hilite_view, cam );
            cam->frame.x = xtra_x - cam->frame.w;
            xtra_x = xtra_x - cam->frame.w;
        } else {
            show_divider = false;
        }
        
        
    }
    if( show_key ) { 
        if( show_divider ) {
            zdj_view_t * spacer = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DJ_KNOB_BG ], NULL );
            zdj_add_subview( state->hilite_view, spacer );
            spacer->frame.w = 1;
            spacer->frame.y = 1;
            spacer->frame.x = xtra_x - 2;
            xtra_x = xtra_x - 3;
        }
        // Show key
        SDL_Rect * key_rect = _get_key_asset( key_enum );
        if( key_rect ) {
            zdj_view_t * key = zdj_new_asset_view( key_rect, NULL );
            zdj_add_subview( state->hilite_view, key );
            xtra_x = xtra_x - key->frame.w;
            key->frame.h = 8;
            key->frame.x = xtra_x;
            show_divider = true;
        } else {
            show_divider = false;
        }
    }
    
    if( show_bpm ) {
        if( show_divider ) {
            zdj_view_t * spacer = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_DJ_KNOB_BG ], NULL );
            zdj_add_subview( state->hilite_view, spacer );
            spacer->frame.w = 1;
            spacer->frame.y = 1;
            spacer->frame.x = xtra_x - 2;
            xtra_x = xtra_x - 3;
        }
        char str[ 8 ];
        sprintf( str, "%3d", bpm );
        zdj_view_t * bpm_label = zdj_new_label_view( str, ZDJ_FONT_6_BOLD, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
        xtra_x = xtra_x - bpm_label->frame.w;
        zdj_add_subview( state->hilite_view, bpm_label );
        bpm_label->frame.x = xtra_x;
        bpm_label->frame.y = -1;
    }

    xtra_bg->frame.x = xtra_x - 2;
    xtra_bg->frame.w = view->frame.w - xtra_bg->frame.x;

    state->needs_layout_init = false;
}

void zdj_menu_item_song_update_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    int _edit_option_index = (int)round(state->edit_option_index);
    // printf( "zdj_menu_item_song_update_layout: %d\n", _edit_option_index );
    switch( _edit_option_index ) {
        case 0:
            break;
        case 1:
            state->edit_action = ZDJ_MENU_ITEM_ACTION_DONE;
            _draw_base( view );
            break;
        case 2:
        case 3:
            state->edit_action = ZDJ_MENU_ITEM_ACTION_LEAD_DECK_1;
            _draw_deck_1( view );
            break;
        case 4:
        case 5:
            state->edit_action = ZDJ_MENU_ITEM_ACTION_LEAD_DECK_2;
            _draw_deck_2( view );
            break;
        case 6:
        case 7:
            state->edit_action = ZDJ_MENU_ITEM_ACTION_EDIT;
            _draw_edit( view );
            break;
        case 8:
        case 9:
            state->edit_action = ZDJ_MENU_ITEM_ACTION_ADD_TO_PLAYLIST;
            _draw_add_to_list( view );
            break;
        case 10:
            state->edit_action = ZDJ_MENU_ITEM_ACTION_DONE;
            _draw_base( view );
            break;
        case 11:
            break;
    }

    state->needs_layout_update = false;
}


void zdj_menu_item_song_playlist_update_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    int _edit_option_index = (int)round(state->edit_option_index);
    // printf( "zdj_menu_item_song_playlist_update_layout: %d\n", _edit_option_index );
    switch( _edit_option_index ) {
        case 0:
            break;
        case 1:
            state->edit_action = ZDJ_MENU_ITEM_ACTION_DONE;
            _draw_base( view );
            break;
        case 2:
        case 3:
            state->edit_action = ZDJ_MENU_ITEM_ACTION_LEAD_DECK_1;
            _draw_deck_1( view );
            break;
        case 4:
            state->edit_action = ZDJ_MENU_ITEM_ACTION_LEAD_DECK_2;
            _draw_deck_2( view );
            break;
        case 5:
            state->edit_action = ZDJ_MENU_ITEM_ACTION_EDIT;
            _draw_edit( view );
            break;
        case 6:
            state->edit_action = ZDJ_MENU_ITEM_ACTION_START_MOVE;
            _draw_move_item( view );
            break;
        case 7:
        case 8:
            state->edit_action = ZDJ_MENU_ITEM_ACTION_DELETE;
            _draw_delete( view );
            break;
        case 9:
            state->edit_action = ZDJ_MENU_ITEM_ACTION_DONE;
            _draw_base( view );
            break;
        case 10:
            break;
    }

    state->needs_layout_update = false;
}


static void _draw_base( zdj_view_t * view ) {
    // printf( "_draw_base\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_remove_all_subviews_of( state->hilite_view );
    zdj_remove_all_subviews_of( state->normal_view );

    // Setup normal view
    zdj_view_t * title_ticker_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( state->normal_view, title_ticker_norm );
    title_ticker_norm->frame.x = 1;
    title_ticker_norm->frame.y = -1;
    title_ticker_norm->frame.w = view->frame.w;
    title_ticker_norm->frame.h = view->frame.h;
    
    // Setup hilite view
    zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_L ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg );
    hilite_bg->frame.y = 0;
    hilite_bg->frame.w = view->frame.w;
    hilite_bg->frame.h = 8;

    zdj_view_t * title_ticker_hilite = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    zdj_ticker_state_t * ticker_state = (zdj_ticker_state_t*)title_ticker_hilite->state;
    zdj_add_subview( state->hilite_view, title_ticker_hilite );
    title_ticker_hilite->frame.x = 1;
    title_ticker_hilite->frame.y = -1;
    title_ticker_hilite->frame.w = view->frame.w;
    title_ticker_hilite->frame.h = view->frame.h;
}

static void _draw_deck_1( zdj_view_t * view ) {
    // printf( "_draw_deck_1\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_remove_all_subviews_of( state->hilite_view );
    zdj_remove_all_subviews_of( state->normal_view );

    // Setup normal view
    zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_L ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg );
    hilite_bg->frame.y = 0;
    hilite_bg->frame.w = view->frame.w;
    hilite_bg->frame.h = 8;

    zdj_view_t * title_ticker_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    zdj_add_subview( state->hilite_view, title_ticker_norm );
    title_ticker_norm->frame.x = 1;
    title_ticker_norm->frame.y = -1;
    title_ticker_norm->frame.w = view->frame.w;
    title_ticker_norm->frame.h = view->frame.h;

    zdj_view_t * icon= zdj_new_noclip_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_DECK_1 ], NULL );
    icon->frame.x = -12;
    icon->frame.y = -4;
    zdj_add_subview( state->hilite_view, icon );
}

static void _draw_deck_2( zdj_view_t * view ) {
    // printf( "_draw_deck_2\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_remove_all_subviews_of( state->hilite_view );
    zdj_remove_all_subviews_of( state->normal_view );

    // Setup normal view
    zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_L ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg );
    hilite_bg->frame.y = 0;
    hilite_bg->frame.w = view->frame.w;
    hilite_bg->frame.h = 8;

    zdj_view_t * title_ticker_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    zdj_add_subview( state->hilite_view, title_ticker_norm );
    title_ticker_norm->frame.x = 1;
    title_ticker_norm->frame.y = -1;
    title_ticker_norm->frame.w = view->frame.w;
    title_ticker_norm->frame.h = view->frame.h;

    zdj_view_t * icon= zdj_new_noclip_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_DECK_2], NULL );
    icon->frame.x = -13;
    icon->frame.y = -4;
    zdj_add_subview( state->hilite_view, icon );
}

static void _draw_edit( zdj_view_t * view ) {
    // printf( "_draw_edit_edit\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_remove_all_subviews_of( state->hilite_view );
    zdj_remove_all_subviews_of( state->normal_view );

    // Setup normal view
    zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_L ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg );
    hilite_bg->frame.y = 0;
    hilite_bg->frame.w = view->frame.w;
    hilite_bg->frame.h = 8;

    zdj_view_t * title_ticker_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    zdj_add_subview( state->hilite_view, title_ticker_norm );
    title_ticker_norm->frame.x = 1;
    title_ticker_norm->frame.y = -1;
    title_ticker_norm->frame.w = view->frame.w;
    title_ticker_norm->frame.h = view->frame.h;

    zdj_view_t * btn = zdj_new_noclip_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_EDIT ], NULL );
    btn->frame.x = -13;
    btn->frame.y = -3;
    zdj_add_subview( state->hilite_view, btn );
}

static void _draw_move_item( zdj_view_t * view ) {
    // printf( "_draw_move_2\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_remove_all_subviews_of( state->hilite_view );
    zdj_remove_all_subviews_of( state->normal_view );

    // Setup normal view
    zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_L ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg );
    hilite_bg->frame.y = 0;
    hilite_bg->frame.w = view->frame.w;
    hilite_bg->frame.h = 8;

    zdj_view_t * title_ticker_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    zdj_add_subview( state->hilite_view, title_ticker_norm );
    title_ticker_norm->frame.x = 1;
    title_ticker_norm->frame.y = -1;
    title_ticker_norm->frame.w = view->frame.w;
    title_ticker_norm->frame.h = view->frame.h;

    // zdj_view_t * btn = zdj_new_noclip_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_DECK_1 ], NULL );
    // btn->frame.x = -13;
    // btn->frame.y = -3;
    zdj_view_t * btn = zdj_new_noclip_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_MOVE ], NULL );
    btn->frame.x = -9;
    btn->frame.y = -3;
    zdj_add_subview( state->hilite_view, btn );
}

static void _draw_delete( zdj_view_t * view ) {
    // printf( "_draw_delete\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_remove_all_subviews_of( state->hilite_view );
    zdj_remove_all_subviews_of( state->normal_view );

    // Setup normal view
    zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_L ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg );
    hilite_bg->frame.y = 0;
    hilite_bg->frame.w = view->frame.w;
    hilite_bg->frame.h = 8;

    zdj_view_t * title_ticker_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    zdj_add_subview( state->hilite_view, title_ticker_norm );
    title_ticker_norm->frame.x = 1;
    title_ticker_norm->frame.y = -1;
    title_ticker_norm->frame.w = view->frame.w;
    title_ticker_norm->frame.h = view->frame.h;

    // zdj_view_t * btn = zdj_new_noclip_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_DECK_2 ], NULL );
    zdj_view_t * btn = zdj_new_noclip_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_DELETE ], NULL );
    btn->frame.x = -12;
    btn->frame.y = -3;
    zdj_add_subview( state->hilite_view, btn );
}

static void _draw_add_to_list( zdj_view_t * view ) {
    // printf( "_draw_delete\n" );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_remove_all_subviews_of( state->hilite_view );
    zdj_remove_all_subviews_of( state->normal_view );

    // Setup normal view
    zdj_view_t * hilite_bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_L ], NULL );
    zdj_add_subview( state->hilite_view, hilite_bg );
    hilite_bg->frame.y = 0;
    hilite_bg->frame.w = view->frame.w;
    hilite_bg->frame.h = 8;

    zdj_view_t * title_ticker_norm = zdj_new_ticker_view( state->title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_BLACK );
    zdj_add_subview( state->hilite_view, title_ticker_norm );
    title_ticker_norm->frame.x = 1;
    title_ticker_norm->frame.y = -1;
    title_ticker_norm->frame.w = view->frame.w;
    title_ticker_norm->frame.h = view->frame.h;

    zdj_view_t * btn = zdj_new_noclip_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_LIB_ADD_TO_PLAYLIST ], NULL );
    btn->frame.x = -9;
    btn->frame.y = -3;
    zdj_add_subview( state->hilite_view, btn );
}

static void _enter_edit_mode( zdj_view_t * item ) {
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;

    // Update layout to first edit option state
    item_state->edit_active = true;
    item_state->needs_layout_update = true;

    // Set edit option index = current deck
    zdj_deck_station_t station = zdj_deck_manager_get_recent_playback_station( );
    // printf( "enter edit mode station %d map %d\n", station, zdj_control_active_map );
    if( station == ZDJ_DECK_STATION_2 ) {
        item_state->edit_option_index = 4.0f;
    } else {
        // default to deck 1
        item_state->edit_option_index = 3.0f;
    }
}

static void _exit_edit_mode( zdj_view_t * item ) {
    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;
    
    // Update layout to normal state
    item_state->edit_active = false;
    item_state->needs_layout_init = true;    
    item_state->edit_option_index = 1.0;
    item_state->edit_action = ZDJ_MENU_ITEM_ACTION_SELECT;
}
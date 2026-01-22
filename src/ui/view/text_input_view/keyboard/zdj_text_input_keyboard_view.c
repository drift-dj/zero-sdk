#include <SDL2/SDL.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/menu_view/zdj_menu_view.h>
#include <zerodj/ui/view/menu_item_view/zdj_menu_item_view.h>
#include <zerodj/ui/view/text_input_view/keyboard/zdj_text_input_keyboard_view.h>

#define ZDJ_TEXT_INPUT_VIEW_COLUMN_WIDTH 6
#define ZDJ_TEXT_INPUT_VIEW_ROW_HEIGHT 9

static int _help_key_index;
static int _space_key_index;
static int _shift_key_index;
static int _backspace_key_index;
static int _insert_key_index;
static int _cancel_key_index;
static int _okay_key_index;

static zdj_view_t * _shift_key;
static bool _shift_key_active;

static zdj_keyboard_key_t * _get_current_keyboard_key( zdj_view_t * keyboard_menu );
static void _init_key_layout( zdj_view_t * view );
static void _init_space_key_layout( zdj_view_t * view );
static void _init_shift_key_layout( zdj_view_t * view );
static void _init_backspace_key_layout( zdj_view_t * view );
static void _init_insert_key_layout( zdj_view_t * view );
static void _add_key_item( zdj_view_t * menu, zdj_keyboard_key_t * key, int row, int col );


zdj_view_t * zdj_new_text_input_keyboard_view( void ) {
    // Build the keyboard menu view
    zdj_rect_t menu_rect = {0, 0, ZDJ_MODAL_WIDTH, ZDJ_SCREEN_H-22};
    zdj_view_t * menu = zdj_new_menu_view( ZDJ_VERTICAL, &menu_rect);
    menu->frame.w = ZDJ_MODAL_WIDTH;
    menu->frame.y = 22;
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)menu->state;
    menu_state->scroll_enabled = false;
    _shift_key_active = false;

    _add_key_item( menu, &keyboard_keys_en_us[ 0 ][ 0 ], 0, 0 );
    _add_key_item( menu, &keyboard_keys_en_us[ 0 ][ 1 ], 0, 1 );
    _add_key_item( menu, &keyboard_keys_en_us[ 0 ][ 2 ], 0, 2 );
    _add_key_item( menu, &keyboard_keys_en_us[ 0 ][ 3 ], 0, 3 );
    _add_key_item( menu, &keyboard_keys_en_us[ 0 ][ 4 ], 0, 4 );
    _add_key_item( menu, &keyboard_keys_en_us[ 0 ][ 5 ], 0, 5 );
    _add_key_item( menu, &keyboard_keys_en_us[ 0 ][ 6 ], 0, 6 );
    _add_key_item( menu, &keyboard_keys_en_us[ 0 ][ 7 ], 0, 7 );
    _add_key_item( menu, &keyboard_keys_en_us[ 0 ][ 8 ], 0, 8 );
    _add_key_item( menu, &keyboard_keys_en_us[ 0 ][ 9 ], 0, 9 );
    _add_key_item( menu, &keyboard_keys_en_us[ 0 ][ 10 ], 0, 10 );
    _add_key_item( menu, &keyboard_keys_en_us[ 0 ][ 11 ], 0, 11 );
    _add_key_item( menu, &keyboard_keys_en_us[ 0 ][ 12 ], 0, 12 );

    _add_key_item( menu, &keyboard_keys_en_us[ 1 ][ 0 ], 1, 0 );
    _add_key_item( menu, &keyboard_keys_en_us[ 1 ][ 1 ], 1, 1 );
    _add_key_item( menu, &keyboard_keys_en_us[ 1 ][ 2 ], 1, 2 );
    _add_key_item( menu, &keyboard_keys_en_us[ 1 ][ 3 ], 1, 3 );
    _add_key_item( menu, &keyboard_keys_en_us[ 1 ][ 4 ], 1, 4 );
    _add_key_item( menu, &keyboard_keys_en_us[ 1 ][ 5 ], 1, 5 );
    _add_key_item( menu, &keyboard_keys_en_us[ 1 ][ 6 ], 1, 6 );
    _add_key_item( menu, &keyboard_keys_en_us[ 1 ][ 7 ], 1, 7 );
    _add_key_item( menu, &keyboard_keys_en_us[ 1 ][ 8 ], 1, 8 );
    _add_key_item( menu, &keyboard_keys_en_us[ 1 ][ 9 ], 1, 9 );
    _add_key_item( menu, &keyboard_keys_en_us[ 1 ][ 10 ], 1, 10 );
    _add_key_item( menu, &keyboard_keys_en_us[ 1 ][ 11 ], 1, 11 );
    _add_key_item( menu, &keyboard_keys_en_us[ 1 ][ 12 ], 1, 12 );

    _add_key_item( menu, &keyboard_keys_en_us[ 2 ][ 0 ], 2, 0 );
    _add_key_item( menu, &keyboard_keys_en_us[ 2 ][ 1 ], 2, 1 );
    _add_key_item( menu, &keyboard_keys_en_us[ 2 ][ 2 ], 2, 2 );
    _add_key_item( menu, &keyboard_keys_en_us[ 2 ][ 3 ], 2, 3 );
    _add_key_item( menu, &keyboard_keys_en_us[ 2 ][ 4 ], 2, 4 );
    _add_key_item( menu, &keyboard_keys_en_us[ 2 ][ 5 ], 2, 5 );
    _add_key_item( menu, &keyboard_keys_en_us[ 2 ][ 6 ], 2, 6 );
    _add_key_item( menu, &keyboard_keys_en_us[ 2 ][ 7 ], 2, 7 );
    _add_key_item( menu, &keyboard_keys_en_us[ 2 ][ 8 ], 2, 8 );
    _add_key_item( menu, &keyboard_keys_en_us[ 2 ][ 9 ], 2, 9 );

    _add_key_item( menu, &keyboard_keys_en_us[ 3 ][ 0 ], 3, 0 );
    _add_key_item( menu, &keyboard_keys_en_us[ 3 ][ 1 ], 3, 1 );
    _add_key_item( menu, &keyboard_keys_en_us[ 3 ][ 2 ], 3, 2 );
    _add_key_item( menu, &keyboard_keys_en_us[ 3 ][ 3 ], 3, 3 );
    _add_key_item( menu, &keyboard_keys_en_us[ 3 ][ 4 ], 3, 4 );
    _add_key_item( menu, &keyboard_keys_en_us[ 3 ][ 5 ], 3, 5 );
    _add_key_item( menu, &keyboard_keys_en_us[ 3 ][ 6 ], 3, 6 );
    _add_key_item( menu, &keyboard_keys_en_us[ 3 ][ 7 ], 3, 7 );
    _add_key_item( menu, &keyboard_keys_en_us[ 3 ][ 8 ], 3, 8 );
    _add_key_item( menu, &keyboard_keys_en_us[ 3 ][ 9 ], 3, 9 );

    // Add space key
    _space_key_index = menu_state->item_count;
    zdj_view_t * space_key = zdj_new_asset_menu_item( ZDJ_UI_ASSET_SPACE_KEY, ZDJ_UI_ASSET_SPACE_KEY_HI, false );
    zdj_menu_item_view_state_t * space_state = (zdj_menu_item_view_state_t*)space_key->state;
    space_key->frame.x = 100;
    space_key->frame.y = 2;
    space_key->frame.w = 8;
    space_key->frame.h = 5;
    zdj_menu_view_add_item( menu, space_key );

    // Add shift key
    _shift_key_index = menu_state->item_count;
    _shift_key = zdj_new_menu_item( "shift", ZDJ_MENU_ITEM_LAYOUT_CUSTOM );
    zdj_menu_item_view_state_t * shift_state = (zdj_menu_item_view_state_t*)_shift_key->state;
    shift_state->init_layout = _init_shift_key_layout;
    shift_state->handles_hmi = true;
    _shift_key->frame.x = 110;
    _shift_key->frame.y = 1;
    _shift_key->frame.w = 5;
    _shift_key->frame.h = 6;
    zdj_menu_view_add_item( menu, _shift_key );

    // Add backspace key
    _backspace_key_index = menu_state->item_count;
    zdj_view_t * backspace_key = zdj_new_asset_menu_item( ZDJ_UI_ASSET_BACKSPACE_KEY, ZDJ_UI_ASSET_BACKSPACE_KEY_HI, false );
    backspace_key->frame.x = 96;
    backspace_key->frame.y = 9;
    backspace_key->frame.w = 9;
    backspace_key->frame.h = 7;
    zdj_menu_view_add_item( menu, backspace_key );

    // Add insert key
    _insert_key_index = menu_state->item_count;
    zdj_view_t * insert_key = zdj_new_asset_menu_item( ZDJ_UI_ASSET_INSERT_KEY, ZDJ_UI_ASSET_INSERT_KEY_HI, false );
    insert_key->frame.x = 107;
    insert_key->frame.y = 9;
    insert_key->frame.w = 9;
    insert_key->frame.h = 7;
    zdj_menu_view_add_item( menu, insert_key );

    // Add cancel btn
    _cancel_key_index = menu_state->item_count;
    zdj_view_t * cancel_key = zdj_new_menu_item( "Cancel", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
    zdj_menu_item_view_state_t * cancel_state = (zdj_menu_item_view_state_t*)cancel_key->state;
    cancel_key->frame.x = 91;
    cancel_key->frame.y = 17;
    cancel_key->frame.w = 26;
    cancel_key->frame.h = 8;
    zdj_menu_view_add_item( menu, cancel_key );

    // Add okay btn
    _okay_key_index = menu_state->item_count;
    zdj_view_t * ok_key = zdj_new_menu_item( "Okay", ZDJ_MENU_ITEM_LAYOUT_BASIC_R );
    zdj_menu_item_view_state_t * ok_state = (zdj_menu_item_view_state_t*)ok_key->state;
    ok_key->frame.x = 94;
    ok_key->frame.y = 25;
    ok_key->frame.w = 23;
    ok_key->frame.h = 8;
    zdj_menu_view_add_item( menu, ok_key );    

    return menu;
}

int zdj_text_input_keyboard_get_current_char( zdj_view_t * keyboard_menu ) {
    // printf( "zdj_text_input_keyboard_get_current_char\n" );
    // Get key at scroll_index
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)keyboard_menu->state;
    
    zdj_keyboard_key_t * key = _get_current_keyboard_key( keyboard_menu );
    if( key ) {
        return key->chars[ key->cur_char ].ascii_char;
    }
    
    if ( menu_state->scroll_index == _space_key_index ){
        return ' ';
    } else {
        return -1;
    }
}

void zdj_text_input_keyboard_set_current_char( zdj_view_t * keyboard_menu, char c ) {
    // printf( "zdj_text_input_keyboard_set_current_char %c\n", c );
    // Find index for key with matching char
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)keyboard_menu->state;
    zdj_keyboard_key_t * key = NULL;


    // If char is space, set menu chrome 'space' key
    if( c == ' ' ) {
        zdj_menu_view_set_scroll_index( keyboard_menu, _space_key_index );
        return;
    }
    
    for( int row=0; row<ZDJ_TEXT_INPUT_KEYBOARD_ROW_COUNT; row++ ) {
        for( int col=0; col<ZDJ_TEXT_INPUT_KEYBOARD_COLUMN_COUNT; col++ ) {
            key = &keyboard_keys_en_us[ row ][ col ];
            for( int key_ind=0; key_ind<ZDJ_TEXT_INPUT_KEYBOARD_KEY_COUNT; key_ind++ ) {
                if( key->chars[ key_ind ].ascii_char == c ) {
                    key->cur_char = key_ind;
                    int menu_index = (row * ZDJ_TEXT_INPUT_KEYBOARD_COLUMN_COUNT) + col;
                    zdj_menu_view_set_scroll_index( keyboard_menu, menu_index );
                    // Invalidate key menu item layout in case cur_char changed
                    zdj_view_t * item = zdj_menu_view_item_at_scroll_index( keyboard_menu, menu_index );
                    zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;
                    item_state->needs_layout_init = true;
                }
            }
        }
    }
}

void zdj_text_input_keyboard_set_item_index( zdj_view_t * keyboard_menu, int index ) {
    // printf( "zdj_text_input_keyboard_set_item_index %d\n", index );
    zdj_menu_view_set_scroll_index( keyboard_menu, index );
}

zdj_keyboard_chrome_item_t zdj_text_input_keyboard_get_current_chrome( 
    zdj_view_t * keyboard_menu 
) {
    // printf( "zdj_text_input_keyboard_get_current_chrome\n" );
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)keyboard_menu->state;

    if( menu_state->scroll_index == _help_key_index ) {
        return ZDJ_KEYBOARD_CHROME_HELP;
    } else if( menu_state->scroll_index == _space_key_index ) {
        return  ZDJ_KEYBOARD_CHROME_SPACE;
    } else if( menu_state->scroll_index == _shift_key_index ) {
        return ZDJ_KEYBOARD_CHROME_SHIFT;
    } else if( menu_state->scroll_index == _backspace_key_index ) {
        return ZDJ_KEYBOARD_CHROME_BACKSPACE;
    } else if( menu_state->scroll_index == _insert_key_index ) {
        return ZDJ_KEYBOARD_CHROME_INSERT;
    } else if( menu_state->scroll_index == _cancel_key_index ) {
        return ZDJ_KEYBOARD_CHROME_CANCEL;
    } else if( menu_state->scroll_index == _okay_key_index ) {
        return ZDJ_KEYBOARD_CHROME_OKAY;
    } else {
        return ZDJ_KEYBOARD_CHROME_NONE;
    }
}

void zdj_text_input_keyboard_set_current_chrome( 
    zdj_view_t * keyboard_menu, 
    zdj_keyboard_chrome_item_t item 
) {
    // printf( "zdj_text_input_keyboard_set_current_chrome\n" );
    switch( item ) {
        case ZDJ_KEYBOARD_CHROME_HELP:
            break;
        case ZDJ_KEYBOARD_CHROME_SPACE:
            zdj_menu_view_set_scroll_index( keyboard_menu, _space_key_index );
            break;
        case ZDJ_KEYBOARD_CHROME_SHIFT:
            zdj_menu_view_set_scroll_index( keyboard_menu, _shift_key_index );
            break;
        case ZDJ_KEYBOARD_CHROME_BACKSPACE:
            zdj_menu_view_set_scroll_index( keyboard_menu, _backspace_key_index );
            break;
        case ZDJ_KEYBOARD_CHROME_INSERT:
            zdj_menu_view_set_scroll_index( keyboard_menu, _insert_key_index );
            break;
        case ZDJ_KEYBOARD_CHROME_CANCEL:
            zdj_menu_view_set_scroll_index( keyboard_menu, _cancel_key_index );
            break;
        case ZDJ_KEYBOARD_CHROME_OKAY:
            zdj_menu_view_set_scroll_index( keyboard_menu, _okay_key_index );
            break;
        default:
            break;
    }
    
}

void zdj_text_input_keyboard_select_next_key_char( zdj_view_t * keyboard_menu ) {
    // printf( "zdj_text_input_keyboard_select_next_key_char\n" );
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)keyboard_menu->state;
    zdj_keyboard_key_t * key = _get_current_keyboard_key( keyboard_menu );
    if( key && (key->cur_char < key->char_count-1) ) {
        key->cur_char++;

        zdj_view_t * item = zdj_menu_view_item_at_scroll_index( keyboard_menu, menu_state->scroll_index );
        zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;
        item_state->needs_layout_init = true;
    }
}

void zdj_text_input_keyboard_select_prev_key_char( zdj_view_t * keyboard_menu ) {
    // printf( "zdj_text_input_keyboard_select_prev_key_char\n" );
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)keyboard_menu->state;
    zdj_keyboard_key_t * key = _get_current_keyboard_key( keyboard_menu );
    if( key && (key->cur_char > 0) ) {
        key->cur_char--;

        zdj_view_t * item = zdj_menu_view_item_at_scroll_index( keyboard_menu, menu_state->scroll_index );
        zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;
        item_state->needs_layout_init = true;
    }
}

void zdj_text_input_keyboard_activate_shift_key( zdj_view_t * keyboard_menu ) {
    zdj_menu_item_view_state_t * shift_state = (zdj_menu_item_view_state_t*)_shift_key->state;
    shift_state->needs_layout_init = true;
    _shift_key_active = true;

    // Move all char keys 1 char to the right.
    // (Captialize whichever char is currently selected.)
    zdj_keyboard_key_t * key = NULL;
    for( int row=0; row<2; row++ ) { // only hit top 2 rows of char keys
        for( int col=0; col<ZDJ_TEXT_INPUT_KEYBOARD_COLUMN_COUNT; col++ ) {
            key = &keyboard_keys_en_us[ row ][ col ];
            if( key->cur_char < key->char_count - 1 ) {
                key->cur_char++;
                int key_index = (row * ZDJ_TEXT_INPUT_KEYBOARD_COLUMN_COUNT) + col;
                zdj_view_t * item = zdj_menu_view_item_at_scroll_index( keyboard_menu, key_index );
                zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;
                item_state->needs_layout_init = true;
            }
        }
    }
}

void zdj_text_input_keyboard_deactivate_shift_key( zdj_view_t * keyboard_menu ) {
    zdj_menu_item_view_state_t * shift_state = (zdj_menu_item_view_state_t*)_shift_key->state;
    shift_state->needs_layout_init = true;
    _shift_key_active = false;

    // Move all char keys 1 char to the left.
    // (Un-captialize whichever char is currently selected.)
    zdj_keyboard_key_t * key = NULL;
    for( int row=0; row<2; row++ ) { // only hit top 2 rows of char keys
        for( int col=0; col<ZDJ_TEXT_INPUT_KEYBOARD_COLUMN_COUNT; col++ ) {
            key = &keyboard_keys_en_us[ row ][ col ];
            if( key->cur_char > 0 ) {
                key->cur_char--;
                int key_index = (row * ZDJ_TEXT_INPUT_KEYBOARD_COLUMN_COUNT) + col;
                zdj_view_t * item = zdj_menu_view_item_at_scroll_index( keyboard_menu, key_index );
                zdj_menu_item_view_state_t * item_state = (zdj_menu_item_view_state_t*)item->state;
                item_state->needs_layout_init = true;
            }
        }
    }
}

static zdj_keyboard_key_t * _get_current_keyboard_key( zdj_view_t * keyboard_menu ) {
    zdj_menu_view_state_t * menu_state = (zdj_menu_view_state_t*)keyboard_menu->state;
    
    zdj_keyboard_key_t * key = NULL;
    if( menu_state->scroll_index < 13 ) {
        return &keyboard_keys_en_us[ 0 ][ menu_state->scroll_index ];
    } else if( menu_state->scroll_index < 26 ) {
        return &keyboard_keys_en_us[ 1 ][ menu_state->scroll_index - 13 ];
    } else if( menu_state->scroll_index < 36 ) {
        return &keyboard_keys_en_us[ 2 ][ menu_state->scroll_index - 26 ];
    } else if( menu_state->scroll_index < 46 ) {
        return &keyboard_keys_en_us[ 3 ][ menu_state->scroll_index - 36 ];
    } else {
        return NULL;
    }
}

void _init_key_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;
    zdj_keyboard_key_t * key = state->data.ptr;
    char title[ 2 ] = { key->chars[ key->cur_char ].ascii_char, '\0' };

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
    zdj_view_t * char_label = zdj_new_label_view( title, ZDJ_FONT_6, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    zdj_add_subview( state->normal_view, char_label );
    char_label->frame.x = (ZDJ_TEXT_INPUT_VIEW_COLUMN_WIDTH/2) - (char_label->frame.w/2) + 1;
    char_label->frame.y = -1;
    
    // Setup hilite view
    zdj_view_t * bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_L ], NULL );
    bg->frame.x = 1;
    // bg->frame.y = 1;
    bg->frame.w = ZDJ_TEXT_INPUT_VIEW_COLUMN_WIDTH-1;
    zdj_add_subview( state->hilite_view, bg );
    zdj_view_t * bg_r = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_HILITE_7_R ], NULL );
    // bg->frame.y = 1;
    bg_r->frame.x = ZDJ_TEXT_INPUT_VIEW_COLUMN_WIDTH-1;
    zdj_add_subview( state->hilite_view, bg_r );

    zdj_view_t * char_label_hi = zdj_new_label_view( title, ZDJ_FONT_6, ZDJ_JUSTIFY_CENTER, ZDJ_SDL_BLACK );
    zdj_add_subview( state->hilite_view, char_label_hi );
    char_label_hi->frame.x = (ZDJ_TEXT_INPUT_VIEW_COLUMN_WIDTH/2) - (char_label->frame.w/2) + 1;
    char_label_hi->frame.y = -1;

    state->needs_layout_init = false;
}

void _init_shift_key_layout( zdj_view_t * view ) {
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)view->state;

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
    
    if( _shift_key_active ) {
        // Setup normal view
        zdj_view_t * key = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_SHIFT_KEY_HI ], NULL );
        zdj_add_subview( state->normal_view, key );
        // Setup hilite view
        zdj_view_t * key_hi = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_SHIFT_KEY ], NULL );
        zdj_add_subview( state->hilite_view, key_hi );
    } else {
        // Setup normal view
        zdj_view_t * key = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_SHIFT_KEY ], NULL );
        zdj_add_subview( state->normal_view, key );
        // Setup hilite view
        zdj_view_t * key_hi = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_SHIFT_KEY_HI ], NULL );
        zdj_add_subview( state->hilite_view, key_hi );
    }
    
    state->needs_layout_init = false;
}

void _add_key_item( zdj_view_t * menu, zdj_keyboard_key_t * key, int row, int col ) {
    char title[ 2 ] = { key->chars[ key->cur_char ].ascii_char, '\0' };
    zdj_view_t * item = zdj_new_menu_item( title, ZDJ_MENU_ITEM_LAYOUT_CUSTOM );
    zdj_menu_item_view_state_t * state = (zdj_menu_item_view_state_t*)item->state;
    state->init_layout = _init_key_layout;
    state->data.ptr = key;
    item->state = state;
    item->frame.x = 1 + (col * ZDJ_TEXT_INPUT_VIEW_COLUMN_WIDTH);
    item->frame.y = (row * ZDJ_TEXT_INPUT_VIEW_ROW_HEIGHT) + 1;
    item->frame.w = ZDJ_TEXT_INPUT_VIEW_COLUMN_WIDTH;
    item->frame.h = ZDJ_TEXT_INPUT_VIEW_ROW_HEIGHT;
    state->handles_hmi = true;
    zdj_menu_view_add_item( menu, item );
}
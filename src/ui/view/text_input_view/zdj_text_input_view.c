#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>


#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/anim/zdj_anim.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/text_input_view/zdj_text_input_view.h>
#include <zerodj/ui/view/text_input_view/buffer/zdj_text_input_buffer_view.h>
#include <zerodj/ui/view/text_input_view/keyboard/zdj_text_input_keyboard_view.h>
#include <zerodj/ui/view/zdj_view_stack.h>

static void _zdj_text_input_view_draw( zdj_view_t * view, zdj_view_clip_t * clip );
static void _zdj_text_input_view_handle_control( zdj_view_t * view, zdj_control_event_t * _event );
static void _zdj_text_input_view_deinit_state( zdj_view_t * view );

static void _zdj_text_input_view_next_char( zdj_view_t * view );
static void _zdj_text_input_view_prev_char( zdj_view_t * view );
static void _zdj_text_input_view_update_keyboard_char( zdj_view_t * view );

static void _zdj_text_input_view_handle_key_release( zdj_view_t * view );

zdj_view_t * zdj_new_text_input_view( zdj_text_input_callback_t cb, char * input ) {
    zdj_view_t * view = zdj_new_view( &(zdj_rect_t){0,0,ZDJ_MODAL_WIDTH+1,ZDJ_MODAL_HEIGHT} );
    view->type = ZDJ_VIEW_TEXT_INPUT;
    view->draw = &_zdj_text_input_view_draw;
    view->handle_control_event = _zdj_text_input_view_handle_control;
    view->deinit_state = &_zdj_text_input_view_deinit_state;
    view->frame->x = ZDJ_MODAL_X+1;
    view->frame->y = ZDJ_SCREEN_H;
    view->in_anim = zdj_new_anim( ZDJ_ANIM_MODAL_SHOW );
    view->out_anim = zdj_new_anim( ZDJ_ANIM_MODAL_HIDE );

    zdj_text_input_view_state_t * view_state = calloc( 1, sizeof( zdj_text_input_view_state_t * ) );
    view_state->input_str = strdup( input );
    view_state->cb = cb;
    view_state->shift_key_active = false;
    view->state = view_state;

    zdj_view_t * bg = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_BLACK ], NULL );
    zdj_add_subview( view, bg );

    // Add keyboard menu
    zdj_view_t * keyboard_menu = zdj_new_text_input_keyboard_view( );
    view_state->keyboard_menu = keyboard_menu;
    zdj_add_subview( view, keyboard_menu );
    

    // Add Input buffer view
    zdj_view_t * input_buffer = zdj_new_text_input_buffer_view( input );
    view_state->input_buffer = input_buffer;
    zdj_add_subview( view, input_buffer );

    return view;
}

void _zdj_text_input_view_draw( zdj_view_t * view, zdj_view_clip_t * clip ) {
    zdj_text_input_view_state_t * view_state = (zdj_text_input_view_state_t*)view->state;
    zdj_text_input_buffer_view_state_t * input_buffer_state = (zdj_text_input_buffer_view_state_t*)view_state->input_buffer->state;

    // Update input buffer based on keyboard menu's selected key
    int keyboard_char = zdj_text_input_keyboard_get_current_char( view_state->keyboard_menu );
    if( keyboard_char != input_buffer_state->cursor_char ) {
        input_buffer_state->cursor_char = keyboard_char;
        input_buffer_state->has_valid_layout = false;
    }
}

void _zdj_text_input_view_handle_control( zdj_view_t * view, zdj_control_event_t * _event ) {
    zdj_control_event_t * e = (zdj_control_event_t *)_event;
    zdj_text_input_view_state_t * view_state = (zdj_text_input_view_state_t*)view->state;
    zdj_text_input_buffer_view_state_t * input_buffer_state = (zdj_text_input_buffer_view_state_t*)view_state->input_buffer->state;

    // Ignore events which have been blocked by layers above this one.
    if( e->blocked ) { return; }

    // Jog Wheel
    if( e->id == ZDJ_UI_CONTROL_JOG_ADJUST_0 ) {
        // Scroll thru keyboard l->r + t->b -> cmd keys t->b.
        view_state->keyboard_menu->handle_control_event( view_state->keyboard_menu, _event );
        // Reset input buffer char to value in str if we're over menu chrome.
    } else if( e->id == ZDJ_UI_CONTROL_JOG_RELEASE_0 ) {
        _zdj_text_input_view_handle_key_release( view );
    } else if( e->id == ZDJ_UI_CONTROL_TONE_1_ADJUST_0 ) { 
        printf( "tone 1 adjust\n" );
        // Scroll input cursor 1 char right/left.
        if( e->i_val > 0 ) {
            _zdj_text_input_view_next_char( view );
        } else if( e->i_val < 0 ) {
            _zdj_text_input_view_prev_char( view );
        }
    } else if( e->id == ZDJ_UI_CONTROL_TONE_1_RELEASE_0 ) {
        _zdj_text_input_view_handle_key_release( view );
    } else if( e->id == ZDJ_UI_CONTROL_TONE_2_ADJUST_0 ) { 
            printf( "tone 2 adjust\n" ); 
            // Scroll keyboard to next/prev key
            // Make a fake event to send into menu
            zdj_control_event_t * fake_e = calloc( 1, sizeof( zdj_control_event_t ) );
            fake_e->id = ZDJ_UI_CONTROL_JOG_ADJUST_0;
            fake_e->i_val = e->i_val;
            view_state->keyboard_menu->handle_control_event( view_state->keyboard_menu, fake_e );
            free( fake_e );
    } else if( e->id == ZDJ_UI_CONTROL_TONE_2_RELEASE_0 ) {
        _zdj_text_input_view_handle_key_release( view );
    } else if( e->id == ZDJ_UI_CONTROL_TONE_3_ADJUST_0 ) { 
            // printf( "tone 3 adjust\n" );
        //     // Scroll keyboard/cmd selection 1 column up/down.
        // } else if( e->type == ZDJ_HMI_EVENT_RELEASE ) {
        //     // Select char at cursor and move cursor to next char.
        //     input_buffer_state->str[ input_buffer_state->cursor_index ] = zdj_text_input_keyboard_get_current_char( view_state->keyboard_menu );
        //     _zdj_text_input_view_next_char( view );
    } else if( e->id == ZDJ_UI_CONTROL_HOTCUE_RELEASE_0 ) {
        // Delete char at cursor and move cursor to prev char.
    
    } else if( e->id == ZDJ_UI_CONTROL_PLAY_RELEASE_0 ) {
        printf( "play btn\n" ); 
        _zdj_text_input_view_handle_key_release( view );

    } else if( e->id == ZDJ_UI_CONTROL_FN_1_RELEASE_0 ) {
        printf( "fn 1 btn\n" );
        _zdj_text_input_view_prev_char( view );
    } else if( e->id == ZDJ_UI_CONTROL_FN_2_RELEASE_0 ) {
        printf( "fn 2 btn\n" );
    } else if( e->id == ZDJ_UI_CONTROL_FN_3_RELEASE_0 ) {
        printf( "fn 3 btn\n" );
        _zdj_text_input_view_next_char( view );
    } else if( e->id == ZDJ_UI_CONTROL_NAV_PRESS_0 ) {
        printf( "nav btn\n" );
        // Scroll-to+blink cancel button if not already there.
        // Exit w/cancel action if cancel is selected.
        zdj_keyboard_chrome_item_t current_chrome = zdj_text_input_keyboard_get_current_chrome( view_state->keyboard_menu );
        if( current_chrome == ZDJ_KEYBOARD_CHROME_CANCEL ) {
            view_state->cb( ZDJ_TEXT_INPUT_ACTION_CANCEL, NULL );
        } else {
            zdj_text_input_keyboard_set_current_chrome( view_state->keyboard_menu, ZDJ_KEYBOARD_CHROME_CANCEL );
        }
    }

    // Prevent views/menus below this one from getting events
    // Any events will be passed to subviews from this func.
    e->blocked = true;
}

void _zdj_text_input_view_deinit_state( zdj_view_t * view ) {
    zdj_text_input_view_state_t * state = (zdj_text_input_view_state_t*)view->state;
    // free( state->input_str );
    free( state );
    view->state = NULL;
}

static void _zdj_text_input_view_next_char( zdj_view_t * view ) {
    zdj_text_input_view_state_t * view_state = (zdj_text_input_view_state_t*)view->state;
    zdj_text_input_buffer_view_state_t * input_buffer_state = (zdj_text_input_buffer_view_state_t*)view_state->input_buffer->state;
    zdj_view_t * keyboard_menu = view_state->keyboard_menu;

    if( input_buffer_state->cursor_index < strlen( input_buffer_state->str ) ) {
        // Move cursor 1 char forward in buffer.
        input_buffer_state->cursor_index++;
        input_buffer_state->has_valid_layout = false;
        // Set keyboard to cursor char
        _zdj_text_input_view_update_keyboard_char( view );
    }
}

static void _zdj_text_input_view_prev_char( zdj_view_t * view ) {
    zdj_text_input_view_state_t * view_state = (zdj_text_input_view_state_t*)view->state;
    zdj_text_input_buffer_view_state_t * input_buffer_state = (zdj_text_input_buffer_view_state_t*)view_state->input_buffer->state;
    // zdj_view_t * keyboard_menu = view_state->keyboard_menu;

    if( input_buffer_state->cursor_index > 0 ) {
        // Move cursor 1 char back in buffer.
        input_buffer_state->cursor_index--;
        input_buffer_state->has_valid_layout = false;
        // Set keyboard to cursor char
        _zdj_text_input_view_update_keyboard_char( view );
    }
}

static void _zdj_text_input_view_update_keyboard_char( zdj_view_t * view ) {
    zdj_text_input_view_state_t * view_state = (zdj_text_input_view_state_t*)view->state;
    zdj_text_input_buffer_view_state_t * input_buffer_state = (zdj_text_input_buffer_view_state_t*)view_state->input_buffer->state;
    zdj_view_t * keyboard_menu = view_state->keyboard_menu;

    // Set keyboard to cursor char
    char c;
    if( input_buffer_state->cursor_index == strlen( input_buffer_state->str ) ) {
        c = 'a';
    } else {
        c = input_buffer_state->str[ input_buffer_state->cursor_index ];
    }
    zdj_text_input_keyboard_set_current_char( keyboard_menu, c );
}

void _zdj_text_input_view_handle_key_release( zdj_view_t * view ) {
    zdj_text_input_view_state_t * view_state = (zdj_text_input_view_state_t*)view->state;
    zdj_text_input_buffer_view_state_t * input_buffer_state = (zdj_text_input_buffer_view_state_t*)view_state->input_buffer->state;
    
    int current_char = zdj_text_input_keyboard_get_current_char( 
        view_state->keyboard_menu
     );
    zdj_keyboard_chrome_item_t current_chrome = zdj_text_input_keyboard_get_current_chrome( 
        view_state->keyboard_menu
    );

    if( current_char > -1 ) {
        // Select char at cursor and move cursor to next char.
        input_buffer_state->str[ input_buffer_state->cursor_index ] = current_char;
        _zdj_text_input_view_next_char( view );
    } else if( current_chrome ) {
        // If keyboard has chrome item selected, current char will be -1.
        // Handle chrome item relase.
        switch ( current_chrome ) {
            case ZDJ_KEYBOARD_CHROME_HELP:
                
                break;
            case ZDJ_KEYBOARD_CHROME_SPACE:
                input_buffer_state->str[ input_buffer_state->cursor_index ] = ' ';
                _zdj_text_input_view_next_char( view );
                break;
            case ZDJ_KEYBOARD_CHROME_SHIFT:
                if( view_state->shift_key_active ) {
                    zdj_text_input_keyboard_deactivate_shift_key( view_state->keyboard_menu );
                    view_state->shift_key_active = false;
                } else {
                    zdj_text_input_keyboard_activate_shift_key( view_state->keyboard_menu );
                    view_state->shift_key_active = true;
                }
                break;
            case ZDJ_KEYBOARD_CHROME_BACKSPACE:
                zdj_text_input_buffer_backspace( view_state->input_buffer );
                input_buffer_state->has_valid_layout = false;
                break;
            case ZDJ_KEYBOARD_CHROME_INSERT:
                zdj_text_input_buffer_insert( view_state->input_buffer );
                input_buffer_state->has_valid_layout = false;
                break;
            case ZDJ_KEYBOARD_CHROME_CANCEL:
                view_state->cb( ZDJ_TEXT_INPUT_ACTION_CANCEL, NULL );
                break;
            case ZDJ_KEYBOARD_CHROME_OKAY:
                view_state->cb( ZDJ_TEXT_INPUT_ACTION_OKAY, input_buffer_state->str );
                break;
            default:
                break;
        }
    }
}
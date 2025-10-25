#include <stdlib.h>
#include <math.h>
#include <string.h>

#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/zdj_ui.h>
#include <zerodj/ui/asset/zdj_ui_asset.h>
#include <zerodj/ui/font/zdj_font.h>
#include <zerodj/ui/view/asset_view/zdj_asset_view.h>
#include <zerodj/ui/view/label_view/zdj_label_view.h>
#include <zerodj/ui/view/text_input_view/buffer/zdj_text_input_buffer_view.h>

#define ZDJ_TEXT_INPUT_BUFFER_MARGIN 1
#define ZDJ_TEXT_INPUT_CURSOR_W 25
#define ZDJ_TEXT_INPUT_BUFFER_W ZDJ_SCREEN_W - (ZDJ_TEXT_INPUT_BUFFER_MARGIN*2) - ZDJ_TEXT_INPUT_CURSOR_W

static void _zdj_text_input_draw( zdj_view_t * input_view, zdj_view_clip_t * clip );
void _zdj_text_input_buffer_view_deinit_state( zdj_view_t * view );

zdj_view_t * zdj_new_text_input_buffer_view( char * input_str ) {
    zdj_rect_t buffer_rect = {0, 0, ZDJ_SCREEN_W, 22};
    zdj_view_t * buffer_view = zdj_new_view( &buffer_rect);
    buffer_view->draw = &_zdj_text_input_draw;
    buffer_view->deinit_state = &_zdj_text_input_buffer_view_deinit_state;
    zdj_text_input_buffer_view_state_t * buffer_state = calloc( 1, sizeof( zdj_text_input_buffer_view_state_t ) );
    buffer_state->cursor_index = strlen( input_str );
    buffer_state->has_valid_layout = false;
    buffer_state->cursor_counter = 0;
    buffer_view->state = buffer_state;
    // buffer_state->str = strdup( input_str );
    strcpy( buffer_state->str, input_str );
    return buffer_view;
}

void zdj_text_input_buffer_backspace( zdj_view_t * input_buffer ) {
    zdj_text_input_buffer_view_state_t * buffer_state = (zdj_text_input_buffer_view_state_t*)input_buffer->state;
    if( buffer_state->cursor_index >= strlen( buffer_state->str ) ) {
        // Move cursor back a char if we're at end of string.
        buffer_state->cursor_index--;
    }
    // Copy chars after cursor to cursor index.
    strcpy( &buffer_state->str[ buffer_state->cursor_index ], &buffer_state->str[ buffer_state->cursor_index + 1 ] );
}

void zdj_text_input_buffer_insert( zdj_view_t * input_buffer ) {
    zdj_text_input_buffer_view_state_t * buffer_state = (zdj_text_input_buffer_view_state_t*)input_buffer->state;
    // Copy cur_char at cur_char + 1
    // Set cur_char to ' '
    strcpy( &buffer_state->str[ buffer_state->cursor_index + 1 ], &buffer_state->str[ buffer_state->cursor_index ] );
    buffer_state->str[ buffer_state->cursor_index ] = ' ';
}

void _zdj_text_input_draw( zdj_view_t * input_view, zdj_view_clip_t * clip ) {
    zdj_text_input_buffer_view_state_t * input_state = (zdj_text_input_buffer_view_state_t*)input_view->state;

    // Skip if no layout update is needed.
    if( input_state->has_valid_layout ) { return; }

    // Drop current text views
    zdj_remove_all_subviews_of( input_view );
    input_state->left_label = NULL;
    input_state->right_label = NULL;
    input_state->cursor_label = NULL;

    // Build labels and calculate dimensions
    int left_w = 0;
    int right_w = 0;
    int cursor_w = 0;
    char right_str[ 512 ] = { '\0' };
    char left_str[ 512 ] = { '\0' };
    if( input_state->cursor_index == 0 ) {
        left_w = 0;
        input_state->left_label = NULL;
        if( strlen( input_state->str ) > 0 ) {
            strcpy( right_str, &input_state->str[ 1 ] );
            input_state->right_label = zdj_new_label_view( (char*)&right_str, ZDJ_FONT_12, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
            right_w = input_state->right_label->frame.w;
        } else {
            right_w = 0;
            input_state->right_label = NULL;
        }
    } else if( input_state->cursor_index == strlen( input_state->str ) ) {
        input_state->left_label = zdj_new_label_view( input_state->str, ZDJ_FONT_12, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE );
        left_w = input_state->left_label->frame.w;
        right_w = 0;
        input_state->right_label = NULL;
    } else {
        // Split string at cursor
        strncpy( left_str, input_state->str, input_state->cursor_index );
        left_str[ input_state->cursor_index ] = '\0';
        
        if( input_state->cursor_index < ( strlen( input_state->str ) - 1) ) {
            strcpy( right_str, &input_state->str[ input_state->cursor_index + 1 ] );
        } else {
            right_str[ 0 ] = ' ';
            right_str[ 1 ] = '\0';
        }

        input_state->left_label = zdj_new_label_view( 
            (char*)&left_str, 
            ZDJ_FONT_12, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE 
        );
        left_w = input_state->left_label->frame.w;
        
        input_state->right_label = zdj_new_label_view( 
            (char*)&right_str, 
            ZDJ_FONT_12, ZDJ_JUSTIFY_RIGHT, ZDJ_SDL_WHITE 
        );
        right_w = input_state->right_label->frame.w;
    }

    // Add cursor label
    if( input_state->cursor_char > 0 ) {
        char cursor_title[ 2 ] = {input_state->cursor_char, '\0'};
        input_state->cursor_label = zdj_new_label_view( cursor_title, ZDJ_FONT_18, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    } else {
        char cursor_title[ 2 ] = {' ', '\0'};
        input_state->cursor_label = zdj_new_label_view( cursor_title, ZDJ_FONT_18, ZDJ_JUSTIFY_LEFT, ZDJ_SDL_WHITE );
    }
    cursor_w = input_state->cursor_label->frame.w;

    // Draw labels + cursor
    if( left_w + right_w + cursor_w < ZDJ_TEXT_INPUT_BUFFER_W ) {
        // Simple case - width of text is narrower than screen.
        // Just position everything from left to right.
        if( input_state->left_label ) {
            input_state->left_label->frame.x = ZDJ_TEXT_INPUT_BUFFER_MARGIN;
            input_state->left_label->frame.y = 3;
            zdj_add_subview( input_view, input_state->left_label );
        }
        
        if( input_state->right_label ) {
            input_state->right_label->frame.x = left_w + ZDJ_TEXT_INPUT_CURSOR_W + ZDJ_TEXT_INPUT_BUFFER_MARGIN;
            input_state->right_label->frame.y = 3;
            zdj_add_subview( input_view, input_state->right_label );
        }

        zdj_view_t * cursor_frame = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_TEXT_INPUT_CURSOR ], NULL );
        cursor_frame->frame.x = left_w + ZDJ_TEXT_INPUT_BUFFER_MARGIN + 1;
        cursor_frame->frame.y = 1;
        zdj_add_subview( input_view, cursor_frame );

        input_state->cursor_label->frame.x = left_w + ZDJ_TEXT_INPUT_BUFFER_MARGIN + 13-(cursor_w/2);
        input_state->cursor_label->frame.y = -3;
        zdj_add_subview( input_view, input_state->cursor_label );
    } else {
        // Complex case - cursor works like a scrollbar.
        // Left edge = index 0, right edge = last char in buffer + 1.

        // Width is window w - margin - cursor w
        float scroll_w = (float)ZDJ_TEXT_INPUT_BUFFER_W;
        float scroll_coeff = (float)input_state->cursor_index / (float)strlen( input_state->str );
        int cursor_x = (scroll_w * scroll_coeff) + ZDJ_TEXT_INPUT_BUFFER_MARGIN;

        if( input_state->left_label ) {
            input_state->left_label->frame.x = cursor_x - input_state->left_label->frame.w - 2;
            input_state->left_label->frame.y = 3;
            zdj_add_subview( input_view, input_state->left_label );
        }
        
        if( input_state->right_label ) {
            input_state->right_label->frame.x = cursor_x + ZDJ_TEXT_INPUT_CURSOR_W + 2;
            input_state->right_label->frame.y = 3;
            zdj_add_subview( input_view, input_state->right_label );
        }

        zdj_view_t * cursor_frame = zdj_new_asset_view( &zdj_ui_assets[ ZDJ_UI_ASSET_TEXT_INPUT_CURSOR ], NULL );
        cursor_frame->frame.x = cursor_x;
        cursor_frame->frame.y = 1;
        zdj_add_subview( input_view, cursor_frame );

        input_state->cursor_label->frame.x = cursor_x + 13-(cursor_w/2);
        input_state->cursor_label->frame.y = -3;
        zdj_add_subview( input_view, input_state->cursor_label );
    }
    
    input_state->has_valid_layout = true;
}

void _zdj_text_input_buffer_view_deinit_state( zdj_view_t * view ) {
    zdj_text_input_buffer_view_state_t * state = (zdj_text_input_buffer_view_state_t*)view->state;
    free( state );
    view->state = NULL;
}
// Copyright (c) 2025 Drift DJ Industries

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ZDJ_TEXT_INPUT_KEYBOARD_VIEW_H
#define ZDJ_TEXT_INPUT_KEYBOARD_VIEW_H

#include <SDL2/SDL.h>

#include <zerodj/ui/zdj_ui.h>

#define ZDJ_TEXT_INPUT_KEYBOARD_ROW_COUNT 4
#define ZDJ_TEXT_INPUT_KEYBOARD_COLUMN_COUNT 13
#define ZDJ_TEXT_INPUT_KEYBOARD_KEY_COUNT 8

typedef struct {
    int rows;
    int cols;
} zdj_keyboard_layout_t;

typedef enum {
    ZDJ_KEYBOARD_CHROME_NONE,
    ZDJ_KEYBOARD_CHROME_HELP,
    ZDJ_KEYBOARD_CHROME_SPACE,
    ZDJ_KEYBOARD_CHROME_SHIFT,
    ZDJ_KEYBOARD_CHROME_BACKSPACE,
    ZDJ_KEYBOARD_CHROME_INSERT,
    ZDJ_KEYBOARD_CHROME_CANCEL,
    ZDJ_KEYBOARD_CHROME_OKAY
} zdj_keyboard_chrome_item_t;

typedef struct { 
    char ascii_char;
} zdj_keyboard_char_t;

typedef struct {
    int char_count;
    int cur_char;
    zdj_keyboard_char_t chars[ ZDJ_TEXT_INPUT_KEYBOARD_KEY_COUNT ];
    bool is_hilite;
} zdj_keyboard_key_t;

static zdj_keyboard_key_t keyboard_keys_en_us[ ZDJ_TEXT_INPUT_KEYBOARD_ROW_COUNT ][ ZDJ_TEXT_INPUT_KEYBOARD_COLUMN_COUNT ] = {
    {
        { 2, 0, { {'a'}, {'A'} }, false },
        { 2, 0, { {'b'}, {'B'} }, false },
        { 2, 0, { {'c'}, {'C'} }, false },
        { 2, 0, { {'d'}, {'D'} }, false },
        { 2, 0, { {'e'}, {'E'} }, false },
        { 2, 0, { {'f'}, {'F'} }, false },
        { 2, 0, { {'g'}, {'G'} }, false },
        { 2, 0, { {'h'}, {'H'} }, false },
        { 2, 0, { {'i'}, {'I'} }, false },
        { 2, 0, { {'j'}, {'J'} }, false },
        { 2, 0, { {'k'}, {'K'} }, false },
        { 2, 0, { {'l'}, {'L'} }, false },
        { 2, 0, { {'m'}, {'M'} }, false }
    },
    {
        { 2, 0, { {'n'}, {'N'} }, false },
        { 2, 0, { {'o'}, {'O'} }, false },
        { 2, 0, { {'p'}, {'P'} }, false },
        { 2, 0, { {'q'}, {'Q'} }, false },
        { 2, 0, { {'r'}, {'R'} }, false },
        { 2, 0, { {'s'}, {'S'} }, false },
        { 2, 0, { {'t'}, {'T'} }, false },
        { 2, 0, { {'u'}, {'U'} }, false },
        { 2, 0, { {'v'}, {'V'} }, false },
        { 2, 0, { {'w'}, {'W'} }, false },
        { 2, 0, { {'x'}, {'X'} }, false },
        { 2, 0, { {'y'}, {'Y'} }, false },
        { 2, 0, { {'z'}, {'Z'} }, false }
    },
    {
        { 1, 0, { {'1'} }, false },
        { 1, 0, { {'2'} }, false },
        { 1, 0, { {'3'} }, false },
        { 1, 0, { {'4'} }, false },
        { 1, 0, { {'5'} }, false },
        { 1, 0, { {'6'} }, false },
        { 1, 0, { {'7'} }, false },
        { 1, 0, { {'8'} }, false },
        { 1, 0, { {'9'} }, false },
        { 1, 0, { {'0'} }, false },
        { 0, 0, { 0 }, false },
        { 0, 0, { 0 }, false },
        { 0, 0, { 0 }, false }
    },
    {
        { 1, 0, { {'!'} }, false },
        { 1, 0, { {'?'} }, false },
        { 1, 0, { {'#'} }, false },
        { 1, 0, { {'$'} }, false },
        { 1, 0, { {'%'} }, false },
        { 1, 0, { {'&'} }, false },
        { 1, 0, { {'+'} }, false },
        { 1, 0, { {'-'} }, false },
        { 1, 0, { {'='} }, false },
        { 1, 0, { {'.'} }, false },
        { 0, 0, { 0 }, false },
        { 0, 0, { 0 }, false },
        { 0, 0, { 0 }, false }
    },
};

zdj_view_t * zdj_new_text_input_keyboard_view( void );
int zdj_text_input_keyboard_get_current_char( zdj_view_t * keyboard_menu );
void zdj_text_input_keyboard_set_current_char( zdj_view_t * keyboard_menu, char c );
void zdj_text_input_keyboard_set_item_index( zdj_view_t * keyboard_menu, int index );
zdj_keyboard_chrome_item_t zdj_text_input_keyboard_get_current_chrome( 
    zdj_view_t * keyboard_menu 
);
void zdj_text_input_keyboard_set_current_chrome( 
    zdj_view_t * keyboard_menu, 
    zdj_keyboard_chrome_item_t item 
);
void zdj_text_input_keyboard_select_next_key_char( zdj_view_t * keyboard_menu );
void zdj_text_input_keyboard_select_prev_key_char( zdj_view_t * keyboard_menu );
void zdj_text_input_keyboard_activate_shift_key( zdj_view_t * keyboard_menu );
void zdj_text_input_keyboard_deactivate_shift_key( zdj_view_t * keyboard_menu );
#endif
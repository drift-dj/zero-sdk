#include <stdio.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/ui/asset/zdj_ui_asset.h>

SDL_Texture * _zdj_asset_atlas;

SDL_Rect zdj_ui_assets[ ZDL_UI_ASSET_COUNT ] = {
    { 0,80,4,4 }, // ZDJ_UI_ASSET_ERROR_TEX
    { 0,0,128,64 }, // ZDJ_UI_ASSET_DOT_BG
    { 0,74,128,1 }, // ZDJ_UI_ASSET_NAR_H_DIV
    { 0,75,128,1 }, // ZDJ_UI_ASSET_NAR_H_DIV_HI
    { 0,76,128,1 }, // ZDJ_UI_ASSET_MID_H_DIV
    { 0,77,128,1 }, // ZDJ_UI_ASSET_MID_H_DIV_HI
    { 0,78,128,1 }, // ZDJ_UI_ASSET_WIDE_H_DIV
    { 0,79,128,1 }, // ZDJ_UI_ASSET_WIDE_H_DIV_HI
    { 0,64,128,9 }, // ZDJ_UI_ASSET_ALERT_STRIP
    { 6,82,5,1 }, // ZDJ_UI_ASSET_DOT_DASH
    { 6,81,5,1 }, // ZDJ_UI_ASSET_DOT_DASH_HI
    { 13,81,1,5 }, // ZDJ_UI_ASSET_DOT_PIPE
    { 14,81,5,1 }, // ZDJ_UI_ASSET_DOT_PIPE_HI
    { 17,80,8,7 }, // ZDJ_UI_ASSET_FOLDER
    { 26,80,8,7 }, // ZDJ_UI_ASSET_FOLDER_HI
    { 40,82,4,3 }, // ZDJ_UI_ASSET_CHECKMARK
    { 35,82,4,3 }, // ZDJ_UI_ASSET_CHECKMARK_HI
    { 49,82,3,3 }, // ZDJ_UI_ASSET_PLUS
    { 45,82,3,3 }, // ZDJ_UI_ASSET_PLUS_HI
    { 52,81,10,6 }, // ZDJ_UI_ASSET_ADD_MUSIC
    { 60,81,10,6 }, // ZDJ_UI_ASSET_ADD_MUSIC_HI
    { 68,82,7,5 }, // ZDJ_UI_ASSET_INSTALL
    { 76,82,7,5 }, // ZDJ_UI_ASSET_INSTALL_HI
    { 6,85,5,5 }, // ZDJ_UI_ASSET_TOGGLE_OFF,
    { 0,85,5,5 }, // ZDJ_UI_ASSET_TOGGLE_ON,
    { 6,91,5,5 }, // ZDJ_UI_ASSET_TOGGLE_OFF_HI,
    { 0,91,5,5 }, // ZDJ_UI_ASSET_TOGGLE_ON_HI,
    { 36,104,3,7 }, // ZDJ_UI_ASSET_EXCLAIM_SM,
    { 32,97,7,7 }, // ZDJ_UI_ASSET_SNOOZE,
    { 13,89,12,7 }, // ZDJ_UI_ASSET_DIR_UP
    { 27,89,12,7 }, // ZDJ_UI_ASSET_DIR_UP_HI
    { 13,89,5,5 }, // ZDJ_UI_ASSET_UP,
    { 27,89,5,5 }, // ZDJ_UI_ASSET_UP_HI,
    { 41,89,12,7 }, // ZDJ_UI_ASSET_DIR_ADD
    { 55,89,12,7 }, // ZDJ_UI_ASSET_DIR_ADD_HI
    { 40,97,13,7 }, // ZDJ_UI_ASSET_DIR_SELECT,
    { 54,97,13,7 }, // ZDJ_UI_ASSET_DIR_SELECT_HI,
    { 85,81,15,9 }, // ZDJ_UI_ASSET_ZERO
    { 101,81,15,9 }, // ZDJ_UI_ASSET_ZERO_HI
    { 85,91,12,10 }, // ZDJ_UI_ASSET_USB
    { 98,91,12,10 }, // ZDJ_UI_ASSET_USB_HI
    { 69,90,7,6 }, // ZDJ_UI_ASSET_ALERT,
    { 77,90,7,6 }, // ZDJ_UI_ASSET_ALERT_HI,
    { 118,92,10,10 }, // ZDJ_UI_ASSET_BIG_PLUS,
    { 118,81,10,10 }, // ZDJ_UI_ASSET_BIG_PLUS_HI,
    { 101,103,13,9 }, // ZDJ_UI_ASSET_LIBRARY,
    { 115,103,13,9 }, // ZDJ_UI_ASSET_LIBRARY_HI,
    { 78,97,5,6 }, // ZDJ_UI_ASSET_SHIFT_KEY,
    { 72,97,5,6 }, // ZDJ_UI_ASSET_SHIFT_KEY_HI,
    { 44,105,9,7 }, // ZDJ_UI_ASSET_INSERT_KEY,
    { 54,105,9,7 }, // ZDJ_UI_ASSET_INSERT_KEY_HI,
    { 72,106,8,5 }, // ZDJ_UI_ASSET_SPACE_KEY,
    { 64,106,8,5 }, // ZDJ_UI_ASSET_SPACE_KEY_HI,
    { 90,104,9,7 }, // ZDJ_UI_ASSET_BACKSPACE_KEY,
    { 81,104,9,7 }, // ZDJ_UI_ASSET_BACKSPACE_KEY_HI,
    { 0,97,17,11 }, // ZDJ_UI_ASSET_ZERO_REBOOT,
    { 0,122,128,2 }, // ZDJ_UI_ASSET_UNDERLINE,
    { 0,119,128,2 }, // ZDJ_UI_ASSET_UNDERLINE_HI,
    { 0,125,21,21 }, // ZDJ_UI_ASSET_TEXT_INPUT_CURSOR,
    { 19,125,2,21 }, // ZDJ_UI_ASSET_TEXT_INPUT_CURSOR_R,
    { 22,125,19,17 }, // ZDJ_UI_ASSET_BOX_1,
    { 42,125,19,17 }, // ZDJ_UI_ASSET_DOTTED_BOX_1,
    { 73,126,10,9 }, // ZDJ_UI_ASSET_SPLICE,
    { 73,136,10,9 }, // ZDJ_UI_ASSET_SPLICE_HI,
    { 85,127,7,8 }, // ZDJ_UI_ASSET_ADD_CUEPOINT,
    { 85,138,7,8 }, // ZDJ_UI_ASSET_ADD_CUEPOINT_HI,
    { 93,126,2,10 }, // ZDJ_UI_ASSET_CUEPOINT_L,
    { 95,125,32,9 }, // ZDJ_UI_ASSET_CUEPOINT_C,
    { 127,126,2,7 }, // ZDJ_UI_ASSET_CUEPOINT_R,
    { 93,136,2,10 }, // ZDJ_UI_ASSET_CUEPOINT_L_HI,
    { 95,136,32,9 }, // ZDJ_UI_ASSET_CUEPOINT_C_HI,
    { 127,137,2,7 }, // ZDJ_UI_ASSET_CUEPOINT_R_HI,
    { 0,147,119,20 }, // ZDJ_UI_ASSET_BIG_ACTION_L,
    { 119,147,9,20 }, // ZDJ_UI_ASSET_BIG_ACTION_R,
    { 0,168,127,7 }, // ZDJ_UI_ASSET_HILITE_7_L,
    { 128,168,1,7 }, // ZDJ_UI_ASSET_HILITE_7_R,
    { 0,177,127,8 }, // ZDJ_UI_ASSET_HILITE_8_L,
    { 128,177,1,8 }, // ZDJ_UI_ASSET_HILITE_8_R,
    { 0,186,127,9 }, // ZDJ_UI_ASSET_HILITE_9_L,
    { 128,186,1,9 }, // ZDJ_UI_ASSET_HILITE_9_R,
    { 0,196,127,10 }, // ZDJ_UI_ASSET_HILITE_10_L,
    { 128,196,1,10 }, // ZDJ_UI_ASSET_HILITE_10_R,
    { 0,196,127,11 }, // ZDJ_UI_ASSET_HILITE_11_L,
    { 128,196,1,11 }, // ZDJ_UI_ASSET_HILITE_11_R,
    { 0,219,127,12 }, // ZDJ_UI_ASSET_HILITE_12_L,
    { 128,219,1,12 }, // ZDJ_UI_ASSET_HILITE_12_R,
    { 3,237,29,21 }, // ZDJ_UI_ADDET_APP_DJ,
    { 4,257,35,20 }, // ZDJ_UI_ADDET_APP_LIB,
    { 5,278,36,19 }, // ZDJ_UI_ADDET_APP_CFG,
    { 129,0,128,64 }, // ZDJ_UI_ASSET_WHITE
    { 130,65,3,65 }, // ZDJ_UI_ADDET_NAR_V_DIV,
    { 131,65,1,65 }, // ZDJ_UI_ADDET_NAR_V_LINE,
    { 148,102,4,10 }, // ZDJ_UI_ASSET_MIXER_OUT_DIV,
    { 155,107,3,5 }, // ZDJ_UI_ASSET_MIXER_IN_DIV,
    { 155,73,4,10 }, // ZDJ_UI_ASSET_MIXER_BUS_DIV,
    { 164,65,1,47 },// ZDJ_UI_ASSET_MIXER_DIV,
    { 166,65,9,47 }, // ZDJ_UI_ASSET_MIXER_MONO_METER,
    { 176,65,11,47 }, // ZDJ_UI_ASSET_MIXER_STEREO_METER,
    { 188,65,12,5 }, // ZDJ_UI_ASSET_MIXER_ANA_IO_12,
    { 188,71,12,5 }, // ZDJ_UI_ASSET_MIXER_ANA_IO_34,
    { 187,77,8,5 }, // ZDJ_UI_ASSET_MIXER_ANA_IO_1,
    { 187,83,8,5 }, // ZDJ_UI_ASSET_MIXER_ANA_IO_2,
    { 187,89,8,5 }, // ZDJ_UI_ASSET_MIXER_ANA_IO_3,
    { 187,95,8,5 }, // ZDJ_UI_ASSET_MIXER_ANA_IO_4,
    { 202,65,13,5 }, // ZDJ_UI_ASSET_MIXER_USB_IO_12,
    { 202,71,13,5 }, // ZDJ_UI_ASSET_MIXER_USB_IO_34,
    { 202,78,8,5 }, // ZDJ_UI_ASSET_MIXER_USB_IO_1,
    { 202,83,8,5 }, // ZDJ_UI_ASSET_MIXER_USB_IO_2,
    { 202,89,8,5 }, // ZDJ_UI_ASSET_MIXER_USB_IO_3,
    { 202,95,8,5 }, // ZDJ_UI_ASSET_MIXER_USB_IO_4,
    { 217,65,12,5 }, // ZDJ_UI_ASSET_MIXER_AUX_12,
    { 217,71,12,5 }, // ZDJ_UI_ASSET_MIXER_AUX_34,
    { 217,78,7,5 }, // ZDJ_UI_ASSET_MIXER_AUX_1,
    { 217,83,7,5 }, // ZDJ_UI_ASSET_MIXER_AUX_2,
    { 217,89,7,5 }, // ZDJ_UI_ASSET_MIXER_AUX_3,
    { 217,95,7,5 }, // ZDJ_UI_ASSET_MIXER_AUX_4,
    { 232,65,11,5 }, // ZDJ_UI_ASSET_MIXER_LR_BUS,
    { 245,65,11,5 }, // ZDJ_UI_ASSET_MIXER_CUE_BUS,
    { 233,71,9,5 }, // ZDJ_UI_ASSET_MIXER_ANNOT_BUS,
    { 245,71,11,5 }, // ZDJ_UI_ASSET_MIXER_RECORD_BUS,
    { 230,78,5,5 }, // ZDJ_UI_ASSET_MIXER_CLOCK_1,
    { 230,83,5,5 }, // ZDJ_UI_ASSET_MIXER_CLOCK_2,
    { 230,89,5,5 }, // ZDJ_UI_ASSET_MIXER_CLOCK_3,
    { 230,95,5,5 }, // ZDJ_UI_ASSET_MIXER_CLOCK_4,
    { 236,78,5,5 }, // ZDJ_UI_ASSET_MIXER_CV_1,
    { 236,83,5,5 }, // ZDJ_UI_ASSET_MIXER_CV_2,
    { 236,89,5,5 }, // ZDJ_UI_ASSET_MIXER_CV_3,
    { 236,95,5,5 }, // ZDJ_UI_ASSET_MIXER_CV_4,
    { 242,78,9,5 }, // ZDJ_UI_ASSET_MIXER_MIDI_1,
    { 242,83,9,5 }, // ZDJ_UI_ASSET_MIXER_MIDI_2,
    { 242,89,9,5 }, // ZDJ_UI_ASSET_MIXER_MIDI_3,
    { 242,95,9,5 }, // ZDJ_UI_ASSET_MIXER_MIDI_4,
    { 147,112,11,3 }, // ZDJ_UI_ASSET_MIXER_CV_BASELINE,
    { 147,114,11,5 }, // ZDJ_UI_ASSET_MIXER_CV_VALUE,
    { 148,119,7,3 }, // ZDJ_UI_ASSET_MIXER_CLOCK_PULSE,
    { 158,113,7,7 }, // ZDJ_UI_ASSET_MIXER_CLOCK_COUNT_1,
    { 166,113,7,7 }, // ZDJ_UI_ASSET_MIXER_CLOCK_COUNT_2,
    { 174,113,7,7 }, // ZDJ_UI_ASSET_MIXER_CLOCK_COUNT_3,
    { 182,113,7,7 }, // ZDJ_UI_ASSET_MIXER_CLOCK_COUNT_4,
    { 148,123,3,7 }, // ZDJ_UI_ASSET_MIXER_FADER_BODY,
    { 151,124,1,5 }, // ZDJ_UI_ASSET_MIXER_FADER_EDGE,
    { 152,126,7,1 }, // ZDJ_UI_ASSET_MIXER_FADER_INDEX,
    { 159,120,9,13 }, // ZDJ_UI_ASSET_MIXER_NO_CNX,
    { 130,133,16,5 }, // ZDJ_UI_ASSET_MIXER_ADD_BUS_HI,
    { 130,139,15,5 }, // ZDJ_UI_ASSET_MIXER_ADD_CLK_HI,
    { 130,145,12,5 }, // ZDJ_UI_ASSET_MIXER_ADD_CV_HI,
    { 130,151,19,5 }, // ZDJ_UI_ASSET_MIXER_ADD_MIDI_HI,
    { 130,158,16,5 }, // ZDJ_UI_ASSET_MIXER_ADD_BUS,
    { 130,164,15,5 }, // ZDJ_UI_ASSET_MIXER_ADD_CLK,
    { 130,170,12,5 }, // ZDJ_UI_ASSET_MIXER_ADD_CV,
    { 130,176,19,5 }, // ZDJ_UI_ASSET_MIXER_ADD_MIDI,
    { 130,182,16,6 }, // ZDJ_UI_ASSET_MIXER_ADD_BUS_BTN_HI,
    { 130,189,16,5 }, // ZDJ_UI_ASSET_MIXER_ADD_LOAD_BTN_HI,
    { 130,195,16,5 }, // ZDJ_UI_ASSET_MIXER_ADD_SAVE_BTN_HI,
    { 130,202,16,6 }, // ZDJ_UI_ASSET_MIXER_ADD_BUS_BTN,
    { 130,208,16,5 }, // ZDJ_UI_ASSET_MIXER_ADD_LOAD_BTN,
    { 130,214,16,5 }, // ZDJ_UI_ASSET_MIXER_ADD_SAVE_BTN,
    { 149,131,7,5 }, // ZDJ_UI_ASSET_MIXER_DETAIL,
    { 149,138,7,5 }, // ZDJ_UI_ASSET_MIXER_DETAIL_HI,
    { 169,121,11,42 }, // ZDJ_UI_ASSET_MIXER_MUTE_STEREO,
    { 183,121,7,42 }, // ZDJ_UI_ASSET_MIXER_MUTE_MUTE,
    { 257,0,128,64 } // ZDJ_UI_ASSET_BLACK
};

int zdj_ui_asset_init( void ) {
    _zdj_asset_atlas = zdj_ui_texture_from_bmp( "/root/res/zero_atlas-32bit.bmp" );

    if( !_zdj_asset_atlas ) { return 1; } else { return 0; }
}

SDL_Texture * zdj_asset_atlas( void ) {
    if( _zdj_asset_atlas ) { return _zdj_asset_atlas; } else {
        // // It's probably bad if this case is called, but it's here for safety.
        // return zdj_ui_texture_from_bmp( "/root/res/zero_atlas-32bit.bmp" );
        zdj_ui_asset_init( );
        return _zdj_asset_atlas;
    }
}

void zdj_ui_asset_draw( zdj_ui_asset_t asset, zdj_rect_t * dest ) {
    SDL_RenderCopy( zdj_renderer( ), _zdj_asset_atlas, &zdj_ui_assets[asset], &(SDL_Rect){dest->x,dest->y,dest->w,dest->h} );
}
#include <stdlib.h>
#include <stdio.h>

#include <zerodj/controls/zdj_controls.h>

void zdj_activate_control_map( zdj_control_map_id_t map_id ) {
    // printf( "zdj_activate_control_map: %d\n", map_id );
    zdj_deactivate_all_controls( );
    switch ( map_id ) {
        case ZDJ_CONTROL_MAP_MENU_BASE:
            zdj_activate_control( ZDJ_DECK_CONTROL_LR_VOL );
            zdj_activate_control( ZDJ_UI_CONTROL_JOG_ADJUST_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_JOG_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_NAV_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_1_PRESS_1 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_2_PRESS_1 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_3_PRESS_1 );
            break;
        case ZDJ_CONTROL_MAP_MENU_DJ_ROOT:
            zdj_activate_control( ZDJ_DECK_CONTROL_LR_VOL );
            zdj_activate_control( ZDJ_UI_CONTROL_JOG_ADJUST_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_JOG_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_NAV_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_1_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_2_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_3_RELEASE_0 );
            break;

        case ZDJ_CONTROL_MAP_TEXT_INPUT:
            zdj_activate_control( ZDJ_DECK_CONTROL_LR_VOL );
            zdj_activate_control( ZDJ_UI_CONTROL_JOG_ADJUST_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_JOG_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_NAV_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_1_PRESS_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_2_PRESS_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_3_PRESS_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_1_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_2_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_3_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_TONE_1_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_TONE_2_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_TONE_3_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_TONE_1_ADJUST_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_TONE_2_ADJUST_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_TONE_3_ADJUST_0 );
            break;
        case ZDJ_CONTROL_MAP_SDK_TEST:
            zdj_activate_control( ZDJ_DECK_CONTROL_LR_VOL );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_PLAY_PAUSE );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_HOTCUE_START );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_HOTCUE_END );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_SCRUB );
            zdj_activate_control( ZDJ_UI_CONTROL_NAV_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_1_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_2_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_3_RELEASE_0 );
            break;

        case ZDJ_CONTROL_MAP_SOUNDCARD:
            zdj_activate_control( ZDJ_DECK_CONTROL_LR_VOL );
            zdj_activate_control( ZDJ_UI_CONTROL_JOG_ADJUST_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_JOG_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_NAV_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_1_PRESS_1 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_2_PRESS_1 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_3_PRESS_1 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_1_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_2_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_3_RELEASE_0 );
            break;

        case ZDJ_CONTROL_MAP_LIB_EDIT_SONG:
            zdj_activate_control( ZDJ_DECK_CONTROL_LR_VOL );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_PLAY_PAUSE );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_HOTCUE_START );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_HOTCUE_END );
            // zdj_activate_control( ZDJ_DECK_1_CONTROL_SCRUB );
            zdj_activate_control( ZDJ_UI_CONTROL_JOG_ADJUST_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_JOG_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_NAV_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_1_PRESS_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_1_PRESS_1 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_2_PRESS_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_3_PRESS_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_1_RELEASE_2 );
            zdj_activate_control( ZDJ_UI_CONTROL_TONE_1_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_TONE_2_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_TONE_3_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_TONE_1_ADJUST_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_TONE_2_ADJUST_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_TONE_3_ADJUST_0 );
            break;
        case ZDJ_CONTROL_MAP_LIB_EDIT_CUEPOINT:
        case ZDJ_CONTROL_MAP_LIB_EDIT_BEATGRID:
            zdj_activate_control( ZDJ_DECK_CONTROL_LR_VOL );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_PLAY_PAUSE );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_HOTCUE_START );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_HOTCUE_END );
            zdj_activate_control( ZDJ_UI_CONTROL_JOG_ADJUST_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_JOG_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_NAV_RELEASE_0 );
            break;

        case ZDJ_CONTROL_MAP_STATION_1_EMPTY:
            zdj_activate_control( ZDJ_DECK_CONTROL_LR_VOL );
            zdj_activate_control( ZDJ_UI_CONTROL_NAV_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_2_PRESS_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_3_PRESS_0 );
            break;
        case ZDJ_CONTROL_MAP_STATION_1_MOM_EQ:
            zdj_activate_control( ZDJ_DECK_CONTROL_LR_VOL );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_PLAY_PAUSE );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_HOTCUE_START );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_HOTCUE_END );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_SCRUB );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_EQ_LO );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_EQ_MID );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_EQ_HI );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_1_RELEASE_0 ); 
            zdj_activate_control( ZDJ_UI_CONTROL_FN_1_RELEASE_1 );
            break;
        case ZDJ_CONTROL_MAP_STATION_1_EQ:
            zdj_activate_control( ZDJ_DECK_CONTROL_LR_VOL );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_PLAY_PAUSE );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_HOTCUE_START );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_HOTCUE_END );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_SCRUB );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_EQ_LO );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_EQ_MID );
            zdj_activate_control( ZDJ_DECK_1_CONTROL_EQ_HI );
            zdj_activate_control( ZDJ_UI_CONTROL_NAV_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_1_RELEASE_0 ); 
            zdj_activate_control( ZDJ_UI_CONTROL_FN_2_PRESS_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_3_PRESS_0 );

            // Shift play/shift fn1
            zdj_activate_control( ZDJ_UI_CONTROL_PLAY_RELEASE_2 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_1_RELEASE_2 );
            break;
        // ZDJ_CONTROL_MAP_STATION_1_TRIM,
        // ZDJ_CONTROL_MAP_STATION_1_LOOP,
        // ZDJ_CONTROL_MAP_STATION_1_SYNC,


        case ZDJ_CONTROL_MAP_STATION_2_EMPTY:
            zdj_activate_control( ZDJ_DECK_CONTROL_LR_VOL );
            zdj_activate_control( ZDJ_UI_CONTROL_NAV_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_1_PRESS_0 ); 
            zdj_activate_control( ZDJ_UI_CONTROL_FN_2_PRESS_0 );
            break;
        case ZDJ_CONTROL_MAP_STATION_2_MOM_EQ:
            zdj_activate_control( ZDJ_DECK_CONTROL_LR_VOL );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_PLAY_PAUSE );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_HOTCUE_START );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_HOTCUE_END );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_SCRUB );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_EQ_LO );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_EQ_MID );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_EQ_HI );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_3_RELEASE_0 ); 
            zdj_activate_control( ZDJ_UI_CONTROL_FN_3_RELEASE_1 );
            break;
        case ZDJ_CONTROL_MAP_STATION_2_EQ:
            zdj_activate_control( ZDJ_DECK_CONTROL_LR_VOL );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_PLAY_PAUSE );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_HOTCUE_START );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_HOTCUE_END );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_SCRUB );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_EQ_LO );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_EQ_MID );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_EQ_HI );
            zdj_activate_control( ZDJ_UI_CONTROL_NAV_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_1_PRESS_0 ); 
            zdj_activate_control( ZDJ_UI_CONTROL_FN_2_PRESS_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_3_RELEASE_0 );
        // ZDJ_CONTROL_MAP_STATION_2_TRIM,
        // ZDJ_CONTROL_MAP_STATION_2_LOOP,
        // ZDJ_CONTROL_MAP_STATION_2_SYNC,
        
        case ZDJ_CONTROL_MAP_STATION_EXT_MOM_EQ:
            zdj_activate_control( ZDJ_DECK_CONTROL_LR_VOL );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_PLAY_PAUSE );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_HOTCUE_START );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_HOTCUE_END );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_SCRUB );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_EQ_LO );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_EQ_MID );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_EQ_HI );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_1_RELEASE_0 ); 
            zdj_activate_control( ZDJ_UI_CONTROL_FN_2_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_3_RELEASE_0 );
        case ZDJ_CONTROL_MAP_STATION_EXT_EQ:
            zdj_activate_control( ZDJ_DECK_CONTROL_LR_VOL );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_PLAY_PAUSE );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_HOTCUE_START );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_HOTCUE_END );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_SCRUB );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_EQ_LO );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_EQ_MID );
            zdj_activate_control( ZDJ_DECK_2_CONTROL_EQ_HI );
            zdj_activate_control( ZDJ_UI_CONTROL_NAV_RELEASE_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_1_PRESS_0 ); 
            zdj_activate_control( ZDJ_UI_CONTROL_FN_2_PRESS_0 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_3_PRESS_0 );

            // Shift play/shift fn1
            zdj_activate_control( ZDJ_UI_CONTROL_PLAY_RELEASE_2 );
            zdj_activate_control( ZDJ_UI_CONTROL_FN_1_RELEASE_2 );
            break;
        // ZDJ_CONTROL_MAP_STATION_EXT_TRIM,
    }

    // Re-activate special control handler
    if( zdj_special_control_handler ) {
        zdj_activate_control( zdj_special_control_handler->id );
    }
}
#ifndef ZDJ_M7_H
#define ZDJ_M7_H

typedef enum {
    ZDJ_M7_FLUSH_HMI,
    ZDJ_M7_UPDATE_DISPLAY,
    ZDJ_M7_ACTIVATE_HMI,
    ZDJ_M7_DEACTIVATE_HMI,
    ZDJ_M7_BOOT_SENTINEL,
    ZDJ_M7_SHOW_DEVELOPER_SCREEN,
    ZDJ_M7_SHOW_TERMINAL_SCREEN,
    ZDJ_M7_SHOW_DRIVE_SCREEN,
    ZDJ_M7_SHOW_ERROR_SCREEN
} zdj_m7_message_t;

int zdj_m7( void );
void zdj_m7_msg( zdj_m7_message_t msg );

#endif
#ifndef STATE_H
#define STATE_H

#include <zerodj/system/installer/zdj_installer.h>
#include <zerodj/ui/zdj_ui.h>

typedef enum {
    UI_PHASE_INIT,
    UI_PHASE_IN_PROGRESS,
    UI_PHASE_COMPLETE,
    UI_PHASE_ERROR
} ui_phase_t;

typedef struct {
    ui_phase_t phase;
    zdj_view_t * menu;
    zdj_view_t * progress_bar;
    zdj_view_t * done_button;
    zdj_installer_t * installer;
    char registry_name[ 64 ];
    char display_name[ 64 ];
    bool is_update;
    char current_version_string[ 64 ];
    char installer_version_string[ 64 ];
    char manifest_totals[ 64 ];
    char size_suffix[ 16 ];
    int file_count;
    float file_size;
    bool install_started;
    bool install_complete;
    bool install_error;
    float install_percent;
} ui_state_t;
extern ui_state_t * ui_state;

#endif
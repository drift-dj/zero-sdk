#ifndef CUEPOINT_EDIT_H
#define CUEPOINT_EDIT_H

#include <zerodj/health/zdj_health_type.h>
// #include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/dj/zdj_deck_dj.h>
#include <zerodj/ui/zdj_ui.h>

typedef struct {
    zdj_library_song_t * song;
    zdj_library_cuepoint_t * cuepoint;
    zdj_view_t * flag_view;
    zdj_view_t * zoom_status;
    zdj_view_t * time;
    zdj_view_t * beat;
    zdj_view_t * zoom;
    zdj_view_t * waveform;
    zdj_view_t * beatgrid;
    zdj_view_t * header;
    zdj_view_t * loop_view;
    zdj_view_t * loop_icon;
    zdj_view_t * loop_label;
    zdj_deck_t * deck;
    bool snap_to_bg;
    zdj_dj_deck_state_t * deck_state;
    double zoom_val;
    void (*cb)( void );
} cuepoint_edit_view_state_t;

zdj_view_t * new_cuepoint_edit_view( zdj_library_song_t * song, char * cuepoint_eid, zdj_deck_t * deck, void (*cb)( void ) );

#endif
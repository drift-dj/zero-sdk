#ifndef BEATGRID_EDIT_H
#define BEATGRID_EDIT_H

#include <zerodj/health/zdj_health_type.h>
// #include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/dj/zdj_deck_dj.h>
#include <zerodj/ui/zdj_ui.h>

typedef struct {
    zdj_library_song_t * song;
    zdj_view_t * zoom_label;
    zdj_view_t * start_time;
    zdj_view_t * bpm;
    double bpm_val;
    zdj_view_t * waveform;
    zdj_view_t * beatgrid;
    zdj_view_t * header;
    zdj_deck_t * deck;
    zdj_dj_deck_state_t * deck_state;
    double zoom_val;
} beatgrid_edit_view_state_t;

zdj_view_t * new_beatgrid_edit_view( zdj_library_song_t * song, zdj_deck_t * deck );

#endif
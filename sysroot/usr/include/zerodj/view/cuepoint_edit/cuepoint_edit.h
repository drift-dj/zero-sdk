#ifndef CUEPOINT_EDIT_H
#define CUEPOINT_EDIT_H

#include <zerodj/health/zdj_health_type.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/lib/zdj_deck_lib.h>
#include <zerodj/ui/zdj_ui.h>

typedef struct {
    zdj_library_song_t * song;
    zdj_view_t * time;
    zdj_view_t * beat;
    zdj_view_t * zoom;
    zdj_view_t * waveform;
    zdj_view_t * header;
    zdj_deck_t * deck;
    zdj_lib_deck_state_t * deck_state;
    double zoom_val;
} cuepoint_edit_view_state_t;

zdj_view_t * new_cuepoint_edit_view( zdj_library_song_t * song, zdj_deck_t * deck );

#endif
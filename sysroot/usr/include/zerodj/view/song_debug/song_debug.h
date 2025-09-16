#ifndef SONG_DEBUG_H
#define SONG_DEBUG_H

#include <zerodj/health/zdj_health_type.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/signal/deck/lib/zdj_deck_lib.h>
#include <zerodj/ui/zdj_ui.h>

typedef struct {
    zdj_library_song_t * song;
    zdj_view_t * pcm_addr;
    zdj_view_t * label_0;
    zdj_view_t * label_1;
    zdj_view_t * label_2;
    zdj_view_t * label_3;
    zdj_view_t * label_4;
    zdj_view_t * label_5;
    zdj_view_t * label_6;
    zdj_view_t * label_7;
    zdj_deck_t * deck;
    zdj_lib_deck_state_t * deck_state;
    double scale;
} song_debug_view_state_t;

extern song_debug_view_state_t * song_debug_view_state;

zdj_view_t * new_song_debug_view( zdj_library_song_t * song, zdj_deck_t * deck );

#endif
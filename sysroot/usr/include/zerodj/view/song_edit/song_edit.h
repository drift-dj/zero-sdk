#ifndef SONG_EDIT_H
#define SONG_EDIT_H

#include <zerodj/health/zdj_health_type.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/ui/zdj_ui.h>

typedef struct {
    zdj_library_song_t * song;
    zdj_view_t * menu;
    zdj_deck_t * deck;
} song_edit_view_state_t;

extern song_edit_view_state_t * song_edit_view_state;

zdj_view_t * new_song_edit_view( zdj_library_song_t * song );

#endif
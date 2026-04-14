#ifndef SONG_EDIT_H
#define SONG_EDIT_H

#include <zerodj/health/zdj_health_type.h>
#include <zerodj/signal/deck/zdj_deck.h>
#include <zerodj/ui/zdj_ui.h>

typedef struct {
    zdj_library_song_t * song;
    zdj_view_t * menu;
    zdj_deck_t * deck;
    bool display_needs_refresh;
    void (*cb)( void );
} song_edit_view_state_t;

extern song_edit_view_state_t * song_edit_view_state;

void refresh_song_edit_view( void );
zdj_view_t * new_song_edit_view( zdj_library_song_t * song, zdj_deck_station_t station, void (*cb)( void ) );
zdj_view_t * new_song_edit_playlists_menu( char * library_entity_id, zdj_library_song_t * song );
zdj_view_t * new_song_edit_key_menu( char * library_entity_id, zdj_library_song_t * song );

#endif
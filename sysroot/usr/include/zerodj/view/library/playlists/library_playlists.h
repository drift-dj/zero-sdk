#ifndef LIBRARY_PLAYLIST_MENU_H
#define LIBRARY_PLAYLIST_MENU_H

#include <zerodj/ui/zdj_ui.h>

zdj_view_t * new_library_playlist_add_song_menu( void (*cb)(char*) );
zdj_view_t * new_library_playlist_menu( char * library_entity_id, zdj_library_playlist_t * playlist );
zdj_view_t * new_library_playlists_menu( char * library_entity_id );

void refresh_playlists_menu( void );

#endif
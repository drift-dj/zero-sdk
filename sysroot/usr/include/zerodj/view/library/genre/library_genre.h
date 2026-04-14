#ifndef LIBRARY_GENRES_MENU_H
#define LIBRARY_GENRES_MENU_H

#include <zerodj/ui/zdj_ui.h>

zdj_view_t * new_library_genre_menu( char * library_entity_id, char * genre );
zdj_view_t * new_library_genres_menu( char * library_entity_id );

#endif
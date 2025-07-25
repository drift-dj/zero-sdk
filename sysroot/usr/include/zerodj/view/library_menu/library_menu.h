#ifndef LIBRARY_MENU_H
#define LIBRARY_MENU_H

#include <zerodj/health/zdj_health_type.h>
#include <zerodj/ui/zdj_ui.h>

extern zdj_view_t * main_menu_stack;

void push_library_root_menu( char * entity_id );
void push_library_edit_menu( void );
void push_library_artists_menu( char * library_entity_id );
void push_library_genres_menu( char * library_entity_id );
void push_library_settings_menu( char * entity_id );

#endif
#ifndef LIBRARY_SELECT_VIEW_H
#define LIBRARY_SELECT_VIEW_H

extern zdj_view_t * library_select_menu_view;

zdj_view_t * new_library_select_view( void );
void reload_library_select_menu_items( zdj_view_t * menu_view );

#endif
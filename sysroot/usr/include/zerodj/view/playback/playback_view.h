#ifndef PLAYBACK_VIEW_H
#define PLAYBACK_VIEW_H

#include <zerodj/ui/zdj_ui.h>

zdj_view_t * new_playback_view( void );
handle_control_event_t playback_control_handler_for_map( zdj_control_map_id_t map );

void select_song_for_playback(  zdj_deck_station_t station, zdj_library_song_t * song );

void handle_init_map( zdj_view_t * view, zdj_control_event_t * event );

void switch_station_1_ui( zdj_control_map_id_t map );
void load_station_1_deck_ui( zdj_deck_t * deck );
void unload_station_1_deck_ui( zdj_deck_t * deck );

void handle_station_1_empty_map( zdj_view_t * view, zdj_control_event_t * event );
void handle_station_1_map( zdj_view_t * view, zdj_control_event_t * event );
void handle_station_1_mom_eq_map( zdj_view_t * view, zdj_control_event_t * event );


void switch_station_ext_ui( zdj_control_map_id_t map );
void load_station_ext_deck_ui( zdj_deck_t * deck );
void unload_station_ext_deck_ui( zdj_deck_t * deck );

void handle_station_ext_map( zdj_view_t * view, zdj_control_event_t * event );
void handle_station_ext_mom_eq_map( zdj_view_t * view, zdj_control_event_t * event );


void switch_station_2_ui( zdj_control_map_id_t map );
void load_station_2_deck_ui( zdj_deck_t * deck );
void unload_station_2_deck_ui( zdj_deck_t * deck );

void handle_station_2_empty_map( zdj_view_t * view, zdj_control_event_t * event );
void handle_station_2_map( zdj_view_t * view, zdj_control_event_t * event );
void handle_station_2_mom_eq_map( zdj_view_t * view, zdj_control_event_t * event );


void switch_playback_map( zdj_control_map_id_t map );

void deck_frame_anim( zdj_anim_t * anim, zdj_view_t * view );
void animate_deck_page_out( zdj_view_t * view );
void animate_deck_page_in( zdj_view_t * view );

#endif
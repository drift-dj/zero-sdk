#ifndef PLAYBACK_VIEW_H
#define PLAYBACK_VIEW_H

#include <zerodj/ui/zdj_ui.h>

zdj_view_t * new_playback_view( void );
handle_control_event_t playback_control_handler_for_map( zdj_control_map_id_t map );

void handle_init_map( zdj_view_t * view, zdj_control_event_t * event );

void handle_station_1_empty_map( zdj_view_t * view, zdj_control_event_t * event );
void handle_station_1_map( zdj_view_t * view, zdj_control_event_t * event );
void handle_station_1_mom_eq_map( zdj_view_t * view, zdj_control_event_t * event );

void handle_station_ext_map( zdj_view_t * view, zdj_control_event_t * event );
void handle_station_ext_mom_eq_map( zdj_view_t * view, zdj_control_event_t * event );

void handle_station_2_empty_map( zdj_view_t * view, zdj_control_event_t * event );
void handle_station_2_map( zdj_view_t * view, zdj_control_event_t * event );
void handle_station_2_mom_eq_map( zdj_view_t * view, zdj_control_event_t * event );

void switch_playback_map( zdj_control_map_id_t map );

void deck_frame_anim( zdj_anim_t * anim, zdj_view_t * view );
void animate_deck_page_out( zdj_view_t * view );
void animate_deck_page_in( zdj_view_t * view );

#endif
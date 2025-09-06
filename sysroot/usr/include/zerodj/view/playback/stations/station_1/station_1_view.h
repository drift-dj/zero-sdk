#ifndef STATION_1_VIEW_H
#define STATION_1_VIEW_H

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    zdj_view_t * eq_page;
    zdj_view_t * trim_page;
    zdj_view_t * loop_page;
    zdj_view_t * sync_page;
} station_1_view_state_t;

zdj_view_t * new_station_1_view( void );

void switch_station_1_ui( zdj_control_map_id_t map );

#endif
#ifndef STATION_EXT_VIEW_H
#define STATION_EXT_VIEW_H

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    zdj_view_t * eq_page;
    zdj_view_t * trim_page;
} station_ext_view_state_t;

zdj_view_t * new_station_ext_view( void );

void switch_station_ext_ui( zdj_control_map_id_t map );

#endif
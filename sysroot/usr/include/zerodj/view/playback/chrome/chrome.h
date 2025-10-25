#ifndef CHROME_H
#define CHROME_H

#include <zerodj/ui/zdj_ui.h>

typedef struct {
    double station_1_val;
    zdj_view_t * station_1_label;
    double station_2_val;
    zdj_view_t * station_2_label;
    double station_ext_val;
    zdj_view_t * station_ext_label;
    double root_val;
    zdj_view_t * root_label;
} zdj_bpm_view_state_t;

zdj_view_t * new_bpm_view( void );

#endif
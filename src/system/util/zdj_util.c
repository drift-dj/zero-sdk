#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>

#include <zerodj/system/util/zdj_util.h>

void zdj_util_put_hmsm_str_for_sample( int sample, int sample_rate, char * str ) {
    // TODO: Add Hours
    int mins = sample / sample_rate / 60;
    int secs = (sample / sample_rate) - (mins * 60);
    double secf = ((double)sample / (double)sample_rate);
    int msec = (int)((secf - secs) * 1000.0);

    snprintf( str, -1, "%d:%02d.%03d", 
        mins,
        secs,
        msec
    );
}

void zdj_util_put_msm_str_for_sample( int sample, int sample_rate, char * str ) {
    int mins = sample / sample_rate / 60;
    int secs = (sample / sample_rate) - (mins * 60);
    double secf = ((double)sample / (double)sample_rate);
    int msec = (int)((secf - secs) * 1000.0);

    snprintf( str, -1, "%d:%02d.%03d", 
        mins,
        secs,
        msec
    );
}
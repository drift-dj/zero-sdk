#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <sys/sem.h>
#include <math.h>
#include <time.h>

#include <zerodj/system/perf/zdj_perf.h>
#include <zerodj/system/thread/zdj_thread.h>

static bool _zdj_perf_enabled;
static uint32_t _zdj_perf_tag_max;

static zdj_perf_tag_t * _zdj_perf_hmi_scan_tags;
static int32_t _zdj_perf_hmi_scan_tag_index;

static zdj_perf_tag_t * _zdj_perf_hmi_process_tags;
static int32_t _zdj_perf_hmi_process_tag_index;

static zdj_perf_tag_t * _zdj_perf_ui_tags;
static int32_t _zdj_perf_ui_tag_index;

static zdj_perf_tag_t * _zdj_perf_soundcard_fast_cycle_tags;
static int32_t _zdj_perf_soundcard_fast_cycle_tag_index;

static zdj_perf_tag_t * _zdj_perf_soundcard_slow_cycle_tags;
static int32_t _zdj_perf_soundcard_slow_cycle_tag_index;

uint64_t zdj_perf_time( void ) {
    struct timespec ts;
    clock_gettime( CLOCK_MONOTONIC, &ts );
    return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

zdj_error_type_t zdj_perf_init( uint32_t tag_count ) {
    _zdj_perf_tag_max = tag_count-1;

    _zdj_perf_hmi_scan_tags = calloc( tag_count, sizeof( zdj_perf_tag_t ) );
    _zdj_perf_hmi_scan_tag_index = 0;

    _zdj_perf_hmi_process_tags = calloc( tag_count, sizeof( zdj_perf_tag_t ) );
    _zdj_perf_hmi_process_tag_index = 0;

    _zdj_perf_ui_tags = calloc( tag_count, sizeof( zdj_perf_tag_t ) );
    _zdj_perf_ui_tag_index = 0;

    _zdj_perf_soundcard_fast_cycle_tags = calloc( tag_count, sizeof( zdj_perf_tag_t ) );
    _zdj_perf_soundcard_fast_cycle_tag_index = 0;

    _zdj_perf_soundcard_slow_cycle_tags = calloc( tag_count, sizeof( zdj_perf_tag_t ) );
    _zdj_perf_soundcard_slow_cycle_tag_index = 0;
}

zdj_error_type_t zdj_enable_perf( void ) {
    _zdj_perf_enabled = true;
}

zdj_error_type_t zdj_disable_perf( void ) {
    _zdj_perf_enabled = false;
}

zdj_error_type_t zdj_reset_perf( void ) {
    _zdj_perf_hmi_scan_tag_index = 0;
    _zdj_perf_hmi_process_tag_index = 0;
    _zdj_perf_ui_tag_index = 0;
    _zdj_perf_soundcard_fast_cycle_tag_index = 0;
    _zdj_perf_soundcard_slow_cycle_tag_index = 0;
}

bool zdj_perf_enabled( void ) {
    return _zdj_perf_enabled;
}

// Return the next available tag for a given thread.
// Always return the last tag in the array once we max out.
zdj_perf_tag_t * zdj_new_perf_tag_for_thread( zdj_system_thread_t thread ) {
    uint32_t ind = _zdj_perf_tag_max;
    switch ( thread ) {
        case ZDJ_SYSTEM_THREAD_CONTROL:
            if( _zdj_perf_hmi_scan_tag_index < _zdj_perf_tag_max ) { ind = _zdj_perf_hmi_scan_tag_index; }
            _zdj_perf_hmi_scan_tag_index++;
            return &_zdj_perf_hmi_scan_tags[ ind ];
        case ZDJ_SYSTEM_THREAD_UI:
            if( _zdj_perf_ui_tag_index < _zdj_perf_tag_max ) { ind = _zdj_perf_ui_tag_index; }
            _zdj_perf_ui_tag_index++;
            return &_zdj_perf_ui_tags[ ind ];
        case ZDJ_SYSTEM_THREAD_AUDIO_BUF:
            if( _zdj_perf_soundcard_fast_cycle_tag_index < _zdj_perf_tag_max ) { ind = _zdj_perf_soundcard_fast_cycle_tag_index; }
            _zdj_perf_soundcard_fast_cycle_tag_index++;
            return &_zdj_perf_soundcard_fast_cycle_tags[ ind ];
    }
}

zdj_perf_report_t * zdj_new_perf_report( void ) {
    zdj_perf_report_t * report = calloc( 1, sizeof( zdj_perf_report_t ) );
    return report;
}


zdj_perf_report_line_t * zdj_perf_report_line_for_name( 
    zdj_perf_report_t * report,
    zdj_perf_tag_name_t name 
) {
    zdj_perf_report_line_t * search_line = report->lines;
    zdj_perf_report_line_t * found_line = NULL;
    // Loop thru all lines in report for matching name
    while( search_line ) {
        if( search_line->name == name ) {
            found_line = search_line;
            search_line = NULL;
            continue;
        }
        search_line = search_line->next;
    }
   
    if( !found_line ) {
        // If none found, make a new one
        found_line = calloc( 1, sizeof( zdj_perf_report_line_t ) );
        found_line->name = name;
        // insert new line at front of linked list
        found_line->next = report->lines;
        report->lines = found_line;
        report->line_count++;

    }
    return found_line;
}

zdj_error_type_t zdj_perf_report_add_tag( 
    zdj_perf_report_t * report, 
    zdj_perf_tag_t * tag,
    zdj_perf_tag_t * next_tag
) {
    // Retreive or make a line for this tag's name
    zdj_perf_report_line_t * line = zdj_perf_report_line_for_name(
        report, 
        tag->name
    );
    // Tabulate tag data into line
    line->count++;

    // Add duration data if end has a value
    if( tag->end ) {
        uint64_t dur = tag->end - tag->start;
        // Min dur
        line->min_dur = fmin( dur, line->min_dur );
        // Max dur
        line->max_dur = fmax( dur, line->max_dur );
        // Avg dur
        // line->avg_dur = line->avg_dur + ( dur - line->avg_dur ) / line->count;
        float avg = ( (line->count-1) * (float)line->avg_dur + (float)dur ) / (float)line->count;
        line->avg_dur = (uint64_t)avg;
    }

    uint64_t cad = next_tag->start - tag->start;
    // Min cadence
    if( line->min_cadence == 0 ) { line->min_cadence = cad; }
    line->min_cadence = fmin( cad, line->min_cadence );
    // Max cadence
    line->max_cadence = fmax( cad, line->max_cadence );
    // Avg cadence
    float avg = ( (line->count-1) * (float)line->avg_cadence + (float)cad ) / (float)line->count;
    line->avg_cadence = (uint32_t)avg;
}

zdj_perf_report_t * zdj_perf_make_cycle_timing_report( void ) {
    zdj_perf_report_t * report = zdj_new_perf_report( );
    // Loop thru each tag array, looking for the specified tags and tallying report lines.
    // Skip the last index in the array, as it may contain overflow tags.
    int i, len;

    len = fmin( _zdj_perf_hmi_scan_tag_index-1, _zdj_perf_tag_max-2 );
    if( len > 0 ) {
        for( i=0; i<len; i++ ) { 
            zdj_perf_report_add_tag( 
                report, 
                &_zdj_perf_hmi_scan_tags[ i ], 
                &_zdj_perf_hmi_scan_tags[ i+1 ] 
            ); 
        }
    }

    len = fmin( _zdj_perf_hmi_process_tag_index-1, _zdj_perf_tag_max-2 );
    if( len > 0 ) {
        for( i=0; i<len; i++ ) { 
            zdj_perf_report_add_tag( 
                report, 
                &_zdj_perf_hmi_process_tags[ i ],
                &_zdj_perf_hmi_process_tags[ i+1 ] 
            ); 
        }
    }

    len = fmin( _zdj_perf_ui_tag_index-1, _zdj_perf_tag_max-2 );
    if( len > 0 ) {
        for( i=0; i<len; i++ ) { 
            zdj_perf_report_add_tag( 
                report, 
                &_zdj_perf_ui_tags[ i ],
                &_zdj_perf_ui_tags[ i+1 ] 
            ); 
        }
    }

    len = fmin( _zdj_perf_soundcard_fast_cycle_tag_index-1, _zdj_perf_tag_max-2 );
    if( len > 0 ) {
        for( i=0; i<len; i++ ) { 
            zdj_perf_report_add_tag( 
                report, 
                &_zdj_perf_soundcard_fast_cycle_tags[ i ],
                &_zdj_perf_soundcard_fast_cycle_tags[ i+1 ]
            ); 
        }
    }

    len = fmin( _zdj_perf_soundcard_slow_cycle_tag_index-1, _zdj_perf_tag_max-2 );
    if( len > 0 ) {
        for( i=0; i<len; i++ ) { 
            zdj_perf_report_add_tag( 
                report, 
                &_zdj_perf_soundcard_slow_cycle_tags[ i ],
                &_zdj_perf_soundcard_slow_cycle_tags[ i+1 ] 
            ); 
        }
    }

    return report;
}
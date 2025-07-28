#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <sys/sem.h>
#include <math.h>
#include <time.h>

#include <zerodj/signal/pipeline/perf/zdj_pipeline_perf.h>

uint64_t zdj_perf_time( void ) {
    struct timespec ts;
    clock_gettime( CLOCK_MONOTONIC, &ts );
    return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

zdj_pipeline_perf_report_t * zdj_pipeline_new_perf_report( void ) {
    zdj_pipeline_perf_report_t * report = calloc( 1, sizeof( zdj_pipeline_perf_report_t ) );
    return report;
}

zdj_pipeline_perf_report_line_t * zdj_pipeline_perf_report_line_for_name( 
    zdj_pipeline_perf_report_t * report,
    zdj_pipeline_perf_tag_name_t name 
) {
    zdj_pipeline_perf_report_line_t * search_line = report->lines;
    zdj_pipeline_perf_report_line_t * found_line = NULL;
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
        found_line = calloc( 1, sizeof( zdj_pipeline_perf_report_line_t ) );
        found_line->name = name;
        // insert new line at front of linked list
        found_line->next = report->lines;
        report->lines = found_line;
        report->line_count++;

    }
    return found_line;
}

zdj_error_type_t zdj_pipeline_perf_report_add_tags( 
    zdj_pipeline_perf_report_t * report, 
    zdj_pipeline_perf_state_t * perf_state 
) {
    // Loop thru all tags in state, adding each to a matching report line's data
    for( int i=0; i<perf_state->tag_count; i++ ) {
        
        // Retreive or make a line for this tag's name
        zdj_pipeline_perf_report_line_t * line = zdj_pipeline_perf_report_line_for_name(
            report, 
            perf_state->tags[ i ].name
        );
        // Tabulate tag data into line
        line->count++;

        // Add duration data if end has a value
        if( perf_state->tags[ i ].end ) {
            uint64_t dur = perf_state->tags[ i ].end - perf_state->tags[ i ].start;
            // Min dur
            line->min_dur = fmin( dur, line->min_dur );
            // Max dur
            line->max_dur = fmax( dur, line->max_dur );
            // Avg dur
            // line->avg_dur = line->avg_dur + ( dur - line->avg_dur ) / line->count;
            float avg = ( (line->count-1) * (float)line->avg_dur + (float)dur ) / (float)line->count;
            line->avg_dur = (uint64_t)avg;
        }

        if( i < (perf_state->tag_count - 1) ) {
            uint64_t cad = perf_state->tags[ i+1 ].start - perf_state->tags[ i ].start;
            // Min cadence
            if( line->min_cadence == 0 ) { line->min_cadence = cad; }
            line->min_cadence = fmin( cad, line->min_cadence );
            // Max cadence
            line->max_cadence = fmax( cad, line->max_cadence );
            // Avg cadence
            float avg = ( (line->count-1) * (float)line->avg_cadence + (float)cad ) / (float)line->count;
            line->avg_cadence = (uint32_t)avg;
        }
    }
}
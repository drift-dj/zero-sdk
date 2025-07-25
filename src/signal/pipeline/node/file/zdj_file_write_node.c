#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/file/zdj_file_write_node.h>

static void _zdj_file_write_node_update_wait( zdj_pipeline_node_t * node );
static void _zdj_file_write_node_deinit_state( zdj_pipeline_node_t * node );
zdj_error_type_t _zdj_file_write_node_open( zdj_pipeline_node_t * node );
zdj_error_type_t _zdj_file_write_node_close( zdj_pipeline_node_t * node );

zdj_pipeline_node_t * zdj_new_file_write_node( 
    char * filepath,
    void * header,
    size_t header_size 
) {
    // Make node
    zdj_pipeline_node_t * node = zdj_new_pipeline_node( );
    node->deinit_state = &_zdj_file_write_node_deinit_state;
    node->update_wait = &_zdj_file_write_node_update_wait;
    node->open = &_zdj_file_write_node_open;
    node->close = &_zdj_file_write_node_close;

    zdj_file_write_node_state_t * state = calloc( 1, sizeof( zdj_file_write_node_state_t ) );
    node->state = state;
    state->filepath = strdup( filepath );
    state->header = NULL;
    state->header_size = 0;
    if( header ) {
        state->header = header;
        state->header_size = header_size;
    }
    state->bytes_written = 0;

    return node;
}

zdj_error_type_t _zdj_file_write_node_open( zdj_pipeline_node_t * node ) {
    zdj_file_write_node_state_t * state = (zdj_file_write_node_state_t*)node->state;

    // Open and error check
    state->fd = fopen( state->filepath, "w" );
    if( !state->fd ) {
        printf( "failed to open: %s\n", state->filepath );
        return ZDJ_ERROR_MISSING;
    }
    // Write n bytes to file header;
    if( state->header ) {
        fseek( state->fd, 0, SEEK_SET );
        fwrite( state->header, state->header_size, 1, state->fd );
    }

    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t _zdj_file_write_node_close( zdj_pipeline_node_t * node ) {
    zdj_file_write_node_state_t * state = (zdj_file_write_node_state_t*)node->state;

    // Write header
    if( state->header ) {
        fseek( state->fd, 0, SEEK_SET );
        fwrite( state->header, state->header_size, 1, state->fd );
    }

    // Close
    fclose( state->fd );

    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t zdj_file_write_node_write( 
    zdj_pipeline_node_t * node, 
    void * addr, 
    size_t len 
) {
    zdj_file_write_node_state_t * state = (zdj_file_write_node_state_t*)node->state;
    // int8_t val = *(int8_t*)addr;
    // printf( "writing %d/%d\n", val, len );
    fwrite( addr, len, 1, state->fd );

    return ZDJ_ERROR_OKAY;
}

void _zdj_file_write_node_update_wait( zdj_pipeline_node_t * node ) {
    // source_node's window will have valid data between back_infill_index->fwd_infill_index.
    // Write everything between those addresses.
    // size_t len = window->fwd_infill_index - window->back_infill_index;
    // if( window->type == ZDJ_PIPELINE_WINDOW_TYPE_LINKED_LIST ) {
    //     // Data write has to visit each data node in the linked list.
    //     // Get data ref at back_infill_index.
    //     for( int i=0; i<=len; i++ ) {
    //         // Write point.
    //         // Get next data.
    //     }
    // } else if( window->type == ZDJ_PIPELINE_WINDOW_TYPE_FLOAT_BUF ) {
    //     // Buffer write is just an addr + length.
    //     float * addr = &window->float_buf[ window->back_infill_index ];
    //     fwrite( addr, len, sizeof( float ), node_state->fd );
    // } else if( window->type == ZDJ_PIPELINE_WINDOW_TYPE_VOID_BUF ) {
    //     // Buffer write is just an addr + length.
    //     void * addr = &window->void_buf[ window->back_infill_index ];
    //     fwrite( addr, len, window->void_buf_size, node_state->fd );
    // }
}

void _zdj_file_write_node_deinit_state( zdj_pipeline_node_t * node ) {

}
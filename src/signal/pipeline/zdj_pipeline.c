#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <sys/sem.h>

#include <zerodj/signal/pipeline/zdj_pipeline.h>

static void _deinit_node( zdj_pipeline_node_t * node );

zdj_pipeline_node_t * zdj_new_pipeline_node( void ) {
    zdj_pipeline_node_t * node = calloc( 1, sizeof( zdj_pipeline_node_t ) );
    node->window_state = calloc( 1, sizeof( zdj_pipeline_window_state_t ) );
    node->window_state->back_valid_index = -1;
    node->window_state->fwd_valid_index = -1;
    node->deinit = &_deinit_node;
    return node;
}

static void _deinit_node( zdj_pipeline_node_t * node ) {
    // printf( "deinit node\n" );
    if( node->deinit_state ) { node->deinit_state( node ); }
    if( node->window_state ) { free( node->window_state ); }
    free( node );
    // printf( "deinit node done\n" );
}

// Helper function to format valid indexes for further processing.
// If a window contains valid data, report the start and end indexes
// of the valid data (in the window's buffer/linked list space).
// Return true if valid data is found, false otherwise.
bool zdj_pipeline_window_state_get_valid_indexes( 
    zdj_pipeline_window_state_t * window_state, 
    zdj_pipeline_window_state_valid_data_t * valid_data 
) {
    // Check to see if both valid indexes are pinned to one side of the window - a no-valid-data state.
    if( 
        (window_state->fwd_valid_index == -1 && window_state->back_valid_index == -1) ||
        (window_state->fwd_valid_index == window_state->len && window_state->back_valid_index == window_state->len)
    ) {
        return false;
    }

    valid_data->data_start = ( window_state->back_valid_index == -1 ) ? 
        0 : 
        window_state->back_valid_index;
    
    valid_data->data_end = ( window_state->fwd_valid_index == window_state->len ) ? 
        window_state->len-1 : 
        window_state->fwd_valid_index;

    return true;
}

// Move and clip a window's data_valid indexes by a given offset.
// When moving an abstract window, we attempt to avoid re-loading data we already have.
// For example, if a window spanning 10 indexes is moved by 2 indexes, we infill 2 new 
// indexes from source, and move the remaining 8 indexes over by 2 indexes.
// If we move beyond the window's current span, all data in the buffer would
// be invalid.
zdj_error_type_t zdj_pipeline_window_state_move( zdj_pipeline_window_state_t * window_state, int offset ) {
    int new_back_valid_index = window_state->back_valid_index - offset;
    if( (int)new_back_valid_index > (int)window_state->len ) { new_back_valid_index = window_state->len; }
    else if( new_back_valid_index < 0 ) { new_back_valid_index = -1; }
    window_state->back_valid_index = new_back_valid_index;

    int new_fwd_valid_index = window_state->fwd_valid_index - offset;
    if( (int)new_fwd_valid_index > (int)window_state->len ) { new_fwd_valid_index = window_state->len; }
    else if( new_fwd_valid_index < 0 ) { new_fwd_valid_index = -1; }
    window_state->fwd_valid_index = new_fwd_valid_index;

    return ZDJ_ERROR_OKAY;
}

// Clear a window's data_valid indexes.
// When a node finds a window with no data_valid, it should attempt to refill the
// entire span.
zdj_error_type_t zdj_pipeline_window_state_reset( zdj_pipeline_window_state_t * window_state, uint32_t ext_address ) {
    // window_state->ext_ref_addr = ext_address;
    // window_state->fwd_valid_index = -1;
    // window_state->back_valid_index = -1;

    return ZDJ_ERROR_OKAY;
}

// Resize and clip a window's span and data_valid indexes.
zdj_error_type_t zdj_pipeline_window_state_resize( 
    zdj_pipeline_window_state_t * window_state,
    uint32_t back_len, 
    uint32_t fwd_len
) {
    // // Clip/offset back_valid_index against new back_len val.
    // int new_back_offset = back_len - window_state->back_len;
    // if( window_state->back_valid_index != -1 && window_state->back_valid_index != window_state->len ) {
    //     window_state->back_valid_index -= new_back_offset;
    // }

    // // Preserve the offset from ref_addr to fwd_valid index
    // int fwd_valid_offset = window_state->fwd_valid_index - window_state->int_ref_addr_index;

    // // Offset ref_addr index against new back_len val.
    // window_state->int_ref_addr_index -= new_back_offset;

    // // Offset fwd_valid index against new ref_addr index.
    // if( window_state->fwd_valid_index != -1 && window_state->fwd_valid_index != window_state->len ) {
    //     window_state->fwd_valid_index = window_state->int_ref_addr_index + fwd_valid_offset;
    // }

    // // Set new len values
    // window_state->back_len = back_len;
    // window_state->fwd_len = fwd_len;
    // window_state->len = back_len + fwd_len;

    // // Clip fwd_valid index to new len
    // if( window_state->fwd_valid_index >= window_state->len ) {
    //     window_state->fwd_valid_index = window_state->len - 1;
    // }

    return ZDJ_ERROR_OKAY;
}

bool zdj_pipeline_window_contains_address_range( 
    zdj_pipeline_window_state_t * window_state, 
    uint32_t start, 
    uint32_t end 
) {
    // return (window_state->ext_ref_addr - window_state->back_len > start) &&
    //        (window_state->ext_ref_addr + window_state->fwd_len < end);
}

void zdj_pipeline_window_print( zdj_pipeline_window_state_t * window_state ) {
    // printf( "win: %p bv_i:%d fv_i:%d l:%d ref:%d\n", 
    //     window_state,
    //     window_state->back_valid_index, 
    //     window_state->fwd_valid_index,
    //     window_state->len,
    //     window_state->int_ref_addr_index
    // );
}
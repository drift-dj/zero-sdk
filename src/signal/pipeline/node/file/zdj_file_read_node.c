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
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/signal/pipeline/node/file/zdj_file_read_node.h>

static void _zdj_file_read_node_update_wait( zdj_pipeline_node_t * node );
static void _zdj_file_read_node_deinit_state( zdj_pipeline_node_t * node );
static zdj_error_type_t _zdj_file_read_node_move_window( zdj_pipeline_node_t * node, double _offset );
static zdj_error_type_t _zdj_file_read_node_reset_window( zdj_pipeline_node_t * node, double _address );
zdj_error_type_t _zdj_file_read_node_open( zdj_pipeline_node_t * node );
zdj_error_type_t _zdj_file_read_node_close( zdj_pipeline_node_t * node );


zdj_pipeline_node_t * zdj_new_file_read_node( 
    char * filepath, 
    size_t cache_len,
    size_t cache_size, 
    size_t header_size 
) {
    // Make node
    zdj_pipeline_node_t * node = zdj_new_pipeline_node( );
    node->deinit_state = &_zdj_file_read_node_deinit_state;
    node->update_wait = &_zdj_file_read_node_update_wait;
    node->open = &_zdj_file_read_node_open;
    node->close = &_zdj_file_read_node_close;
    node->move_window = &_zdj_file_read_node_move_window;
    node->reset_window = &_zdj_file_read_node_reset_window;

    // Set up the window based on cache size
    zdj_pipeline_window_state_resize( 
        node->window_state,
        0,
        cache_len
    );

    zdj_file_read_node_state_t * state = calloc( 1, sizeof( zdj_file_read_node_state_t ) );
    node->state = state;
    strcpy( state->filepath, filepath );

    state->header = calloc( 1, header_size );
    state->header_size = header_size;
    state->at_eof = false;

    state->cache = calloc( cache_len, cache_size );
    state->cache_len = cache_len;
    state->cache_size = cache_size;
    
    return node;
}

zdj_error_type_t _zdj_file_read_node_open( zdj_pipeline_node_t * node ) {
    zdj_file_read_node_state_t * state = (zdj_file_read_node_state_t*)node->state;

    // Open and error check
    state->fd = fopen( state->filepath, "r" );
    if( !state->fd ) {
        printf( "failed to open: %s\n", state->filepath );
        return ZDJ_ERROR_MISSING;
    }

    // Alloc and read n bytes into state's file_header;
    if( state->header_size ) {
        fseek( state->fd, 0, SEEK_SET );
        fread( state->header, state->header_size, 1, state->fd );
    }

    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t _zdj_file_read_node_close( zdj_pipeline_node_t * node ) {
    zdj_file_read_node_state_t * state = (zdj_file_read_node_state_t*)node->state;
    fclose( state->fd );
    return ZDJ_ERROR_OKAY;
}

void _zdj_file_read_node_update_wait( zdj_pipeline_node_t * node ) {  
    zdj_file_read_node_state_t * state = (zdj_file_read_node_state_t*)node->state;  

    uint32_t read_len;
    uint32_t seek_target;
    uint32_t cache_target_index;

    // If reverse span needs data...
    if( node->window_state->back_valid_index < node->window_state->len &&
        node->window_state->back_valid_index > 0 ) {
        // Reverse span is partially infilled already.
        // Read up to back_valid_index.
        read_len = node->window_state->back_valid_index;
    } else {
        // Reverse span has no infill.
        // Read up to ref_addr_index
        read_len = node->window_state->int_ref_addr_index;
    }
    // Seek fd to beginning of window span (observing header offset).
    // We always start reading at the beginning of cache.
    seek_target = node->window_state->ext_ref_addr - node->window_state->int_ref_addr_index + state->header_size;
    // Read back infill into cache and update valid state.
    fseek( state->fd, seek_target, SEEK_SET );
    fread( state->cache, state->cache_size, read_len, state->fd );
    node->window_state->back_valid_index = 0;

    // printf( "reading %d bytes from %d to back_infill @ %d\n", read_len*state->cache_size, seek_target, 0 );

    // If forward span needs data...
    if( node->window_state->fwd_valid_index < node->window_state->len &&
        node->window_state->fwd_valid_index > 0 ) {
        // Forward span is partially infilled already.
        // Read from to fwd_valid_index to window->len.
        read_len = node->window_state->len - node->window_state->fwd_valid_index;
        seek_target = node->window_state->ext_ref_addr - node->window_state->int_ref_addr_index + node->window_state->fwd_valid_index + state->header_size;
        cache_target_index = node->window_state->fwd_valid_index;
    } else {
        // Forward span has no infill.
        // Read from ref_addr_index to end of cache
        read_len = node->window_state->len - node->window_state->int_ref_addr_index;
        seek_target = node->window_state->ext_ref_addr + state->header_size;
        cache_target_index = node->window_state->int_ref_addr_index;
    }
    // Read forward infill into cache and update valid/at_eof state
    fseek( state->fd, seek_target, SEEK_SET );
    int br = fread( &state->cache[ cache_target_index ], state->cache_size, read_len, state->fd );
    
    if( br < read_len ) { 
        // If we hit end-of-file, flag state and zero out missing indexes in cache.
        state->at_eof = true;
        int zero_fill_start = cache_target_index + br;
        memset( &state->cache[ zero_fill_start ], 0, state->cache_len - zero_fill_start );
    } else { 
        state->at_eof = false; 
    }
    node->window_state->fwd_valid_index = node->window_state->len - 1;

    // printf( "reading %zu bytes from %d to fwd_infill @ %d\n", read_len*state->cache_size, seek_target, cache_target_index );
}

void _zdj_file_read_node_deinit_state( zdj_pipeline_node_t * node ) {
    zdj_file_read_node_state_t * state = (zdj_file_read_node_state_t*)node->state;
    if( !state ) { return; }
    if( state->cache ) { free( state->cache ); }
    if( state->cache ) { free( state->header ); }
    if( state->cache ) { free( state->filepath ); }
    free( state );
}

zdj_error_type_t _zdj_file_read_node_move_window( zdj_pipeline_node_t * node, double _offset ) {
    zdj_file_read_node_state_t * node_state = (zdj_file_read_node_state_t*)node->state;

    int offset = round( _offset );
    // If we're moving formward in the file, make sure EOF flag hasn't been set.
    if( offset > 0 && node_state->at_eof ) { return ZDJ_ERROR_OKAY; }
    
    // Move is aware of an abstract file header.  
    // Header size/type is set by upstream node.
    // Clip offset to end of header
    if( (int)(node->window_state->ext_ref_addr + offset) < (int)node_state->header_size ) {
        if( (int)node->window_state->ext_ref_addr < (int)node_state->header_size ) {
            // If window ref is still within the header, forward to header end.
            offset = node_state->header_size - node->window_state->ext_ref_addr;
        } else {
            // If window reg is outside the header, clip offset to header end.
            offset = node->window_state->ext_ref_addr - node_state->header_size;
        }
    }
    node->window_state->ext_ref_addr += offset;
    // printf( "pre-move offset: %d - ", offset );
    // zdj_pipeline_window_print( node->window_state );
    zdj_pipeline_window_state_move( node->window_state, offset );
    // printf( "post-mode win: " );
    // zdj_pipeline_window_print( node->window_state );

    // If move is within cache boundary, memmove pre-existing data into place.
    zdj_pipeline_window_state_valid_data_t valid_data;
    bool valid = zdj_pipeline_window_state_get_valid_indexes( node->window_state, &valid_data );
    // printf( "valid: %d, start:%u, end:%u\n", valid, valid_data.data_start, valid_data.data_end );
    if( valid ) {
        // Build memmove by reversing offset of clipped valid_data coords.
        int dest_index = valid_data.data_start;
        int src_index = valid_data.data_start+offset;
        char * dest = &node_state->cache[ dest_index ];
        char * src = &node_state->cache[ src_index ];
        size_t size = valid_data.data_end - valid_data.data_start;
        // printf( "memmove - dest[%d] src[%d] size:%lu\n", dest_index, src_index, size );
        memmove( dest, src, size );
    }

    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t _zdj_file_read_node_reset_window( zdj_pipeline_node_t * node, double _address ) {
    int64_t address = round( _address );
    zdj_pipeline_window_state_reset( node->window_state, address );
}

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavcodec/codec_id.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_raw_node.h>

// #define ZDJ_DECODE_INFILL_SAMPLE_LIMIT 256
// #define ZDJ_DECIDE_DEFAULT_INFILL_SIZE 2048

#define ZDJ_DECODE_NODE_FWD_INFILL 2048
#define ZDJ_DECODE_NODE_BACK_INFILL 2048

static void _zdj_decode_node_update_wait( zdj_pipeline_node_t * node );
static void _zdj_decode_node_update_async( zdj_pipeline_node_t * node );
static void _zdj_decode_node_deinit_state( zdj_pipeline_node_t * node );
static zdj_error_type_t _zdj_decode_node_move_window( zdj_pipeline_node_t * node, int offset );
static zdj_error_type_t _zdj_decode_node_reset_window( zdj_pipeline_node_t * node, uint32_t address );
static zdj_error_type_t _zdj_decode_node_resize_window( 
    zdj_pipeline_node_t * node, 
    uint32_t back_infill_targ, 
    uint32_t fwd_infill_targ 
);

static zdj_pipeline_node_t * _zdj_raw_node_for_address( 
    zdj_pipeline_node_t * decode_node, 
    uint32_t address
);
static zdj_error_type_t _zdj_add_raw_node_for_address( 
    zdj_pipeline_node_t * decode_node, 
    uint32_t address 
);

zdj_pipeline_node_t * zdj_new_decode_node( 
    zdj_library_song_t * song,
    size_t win_fwd,
    size_t win_back
) {
    printf( "zdj_new_decode_node\n" );

    // Make node
    zdj_pipeline_node_t * node = zdj_new_pipeline_node( );
    node->deinit_state = &_zdj_decode_node_deinit_state;
    node->update_wait = &_zdj_decode_node_update_wait;
    node->move_window = &_zdj_decode_node_move_window;
    node->reset_window = &_zdj_decode_node_reset_window;

    // Set up the window
    zdj_pipeline_window_state_resize( 
        node->window_state,
        win_back,
        win_fwd
    );

    // Add state
    zdj_decode_node_state_t * state = calloc( 1, sizeof( zdj_decode_node_state_t ) );
    node->state = state;
    state->status = ZDJ_PCM_INIT;
    state->win_fwd_infill = win_fwd;
    state->win_back_infill = win_back;

    // Setup inital raw node
    // How to calculate raw_node's frame count from decode_node's window size?
    zdj_pipeline_node_t * raw_node = zdj_new_decode_raw_node( song, 0, 10 );

    return node;
}

void _zdj_decode_node_update_wait( zdj_pipeline_node_t * node ) {
    // Check for loop/beat skip conditions and create new raw nodes

    // Fill window forward infill from raw node if needed.

    // Fill window back infill from raw node if needed.
}

void _zdj_decode_node_update_async( zdj_pipeline_node_t * node ) {
    // Release thread to begin multi-cycle update
}

void _zdj_decode_node_deinit_state( zdj_pipeline_node_t * node ) {

}

zdj_error_type_t _zdj_decode_node_move_window( zdj_pipeline_node_t * node, int offset ) {
    // Move reference address(es) by offset.
    // Update loop/skip states.
    // Create/groom any raw nodes as required.
    // Update window data management.
}

zdj_error_type_t _zdj_decode_node_reset_window( zdj_pipeline_node_t * node, uint32_t address ) {
    // Clear and set reference address state.
    // Create loop/skip states as needed.
    // Create raw nodes as required.
    // Update window data management.
}

zdj_error_type_t _zdj_decode_node_resize_window( 
    zdj_pipeline_node_t * node, 
    uint32_t back_infill_targ, 
    uint32_t fwd_infill_targ 
) {

}
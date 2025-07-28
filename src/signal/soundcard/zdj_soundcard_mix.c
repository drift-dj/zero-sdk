#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/buffer/zdj_audio_buffer_node.h>
#include <zerodj/signal/pipeline/node/audio/io/zdj_io_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
static zdj_error_type_t _zdj_soundcard_clear_mix_buffer( 
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_t * node
);

zdj_error_type_t zdj_soundcard_mix_input( zdj_soundcard_t * soundcard, zdj_soundcard_node_t * node ) {  
    // If this node has already run its mix/dsp cycle, we're done.
    // This happens when multiple nodes have input to from a single node.
    // We don't need to do recursion/dsp on a node twice.
    if( node->mix_complete ) { return ZDJ_ERROR_OKAY; }

    // If this node links outside the graph, fill the sig buf before accumulating other inputs
    if( node->get_edge_data ) {
        // Invoke the CB to fill the data_pipe pipe with data
        node->get_edge_data( node->data_pipe );
    }
    
    // Recurse into linked inputs and accumulate to data_pipe buffer.
    if( node->input_link_count ) {
        // If there are input links, recurse and accumulate.
        for( int i=0; i<node->input_link_count; i++ ) {
            // Recursively mix/dsp the input node.
            zdj_soundcard_node_t * input_node = zdj_soundcard_get_node_for_name( soundcard, node->input_links[ i ].source_node );
            zdj_soundcard_mix_input( soundcard, input_node );

            // Accumulate the input node's samples into this node's buffer.
            zdj_soundcard_accumulate_node( soundcard, input_node, node );

            // Accumulate this node's buffer into the meter.
            node->meter_pipe->update_wait( node->meter_pipe );
        }
    }

    // Apply any configured DSP to the mixed buffer.
    if( node->dsp ) { node->dsp( node ); }

    // Mark this node's mix as complete.
    // Requests from other linked nodes will skip this node's
    // recursion and just accumulate its buffer into thers.
    node->mix_complete = true;
}

// Add samples from one node to another.  
// Observe stereo/pan behavior for each node.
zdj_error_type_t zdj_soundcard_accumulate_node(
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_t * input_node,
    zdj_soundcard_node_t * node
) {
    // printf( "accum: %s <- %s\n", 
    //     zdj_soundcard_node_name[ node->name ],
    //     zdj_soundcard_node_name[ input_node->name ] 
    // );
    // Build an accumulator map to copy input node's samples into this node's buffer.
    zdj_soundcard_accumulate_map_t map;

    // Figure out nodes' mapping
    map.dest_channel_stride = ( node->stereo ) ? 2 : 1;
    map.dest_channel_count = ( node->stereo ) ? 2 : 1;
    map.dest_channel_offset = 0;
    map.source_channel_stride = ( input_node->stereo ) ? 2 : 1;
    map.source_channel_count = ( input_node->stereo ) ? 2 : 1;
    map.source_channel_offset = 0;

    // Stride thru buffers, mixing samples
    float * source_buf = input_node->data_pipe->get_data( input_node->data_pipe );
    float * dest_buf = node->data_pipe->get_data( node->data_pipe );
    double source_sample, dest_sample, new_dest_sample;
    // printf( "node:%s %p\n", zdj_soundcard_node_name[ node->name ], node->data_pipe );
    for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
        int source_index = i*map.source_channel_stride+map.source_channel_offset;
        int dest_index = i*map.dest_channel_stride+map.dest_channel_offset;
        
        // Add the sample and clip it to float min/max
        source_sample = source_buf[ source_index ];
        dest_sample = dest_buf[ dest_index ];
        new_dest_sample = source_sample+dest_sample;
        if( new_dest_sample > 1.0f ) {
            dest_buf[ dest_index ] = 1.0f;
        } else if( new_dest_sample < -1.0f ) {
            dest_buf[ dest_index ] = -1.0f;
        } else {
            dest_buf[ dest_index ] = new_dest_sample;
        }
    }
}

zdj_error_type_t _zdj_soundcard_clear_mix_buffer( 
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_t * node
) {
    float * data = node->data_pipe->get_data( node->data_pipe );
    if( node->stereo ) {
        memset( data, 0, sizeof( float ) * 2 * ZDJ_SOUNDCARD_BUF_LEN );
    } else {
        memset( data, 0, sizeof( float ) * ZDJ_SOUNDCARD_BUF_LEN );
    }
}
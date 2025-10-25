#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/buffer/zdj_audio_buffer_node.h>
#include <zerodj/signal/pipeline/node/analysis/meter/zdj_meter_node.h>
#include <zerodj/signal/pipeline/node/analysis/waveform/zdj_waveform.h>
#include <zerodj/signal/pipeline/node/audio/io/zdj_io_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

zdj_error_type_t _zdj_soundcard_meter_analog_input_node(
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_t * node
);

zdj_error_type_t zdj_soundcard_clear_buffer( 
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_t * node 
) {
    // Clear signal data buffer
    if( node->data_pipe && node->data_pipe->get_data ) {
        float * buffer = node->data_pipe->get_data( node->data_pipe );
        memset( buffer, 0, ZDJ_SOUNDCARD_BUF_LEN * sizeof( float ) * (node->stereo+1) );
    }
    node->mix_complete = false;
}

zdj_error_type_t zdj_soundcard_mix_input( zdj_soundcard_t * soundcard, zdj_soundcard_node_t * node ) {  
    
    // printf( "zdj_soundcard_mix_input: %s\n", zdj_soundcard_node_name[ node->name ] );
    // If this node has already run its mix/dsp cycle, we're done.
    // This happens when multiple nodes have input to from a single node.
    // We don't need to do recursion/dsp on a node twice.
    if( node->mix_complete ) { return ZDJ_ERROR_OKAY; }

    // If this node has input from outside the graph, fill the sig buf before accumulating.
    if( node->get_edge_input_data && node->edge_input_link ) {
        // Invoke the CB to fill the data_pipe pipe with data
        node->get_edge_input_data( node->edge_input_link, node->data_pipe, node->stereo );
    }

    // Recurse into linked inputs and accumulate to data_pipe buffer.
    if( node->input_link_count ) {
        // If there are input links, recurse and accumulate.
        for( int i=0; i<node->input_link_count; i++ ) {
            // Recursively mix/dsp the input node.
            zdj_soundcard_node_t * input_node = zdj_soundcard_get_node_for_name( soundcard, node->input_links[ i ].source_node );
            zdj_soundcard_mix_input( soundcard, input_node );

            // Accumulate the input node's samples into this node's buffer + meter.
            // Note that we can't meter an edge node - there are no input links to an edge node.
            zdj_soundcard_accumulate_node( soundcard, input_node, node );
        }
    } else if ( zdj_soundcard_node_name_is_analog_input( node->name ) ) {
        // Analog inputs have no input linkage - manually update metering.
        _zdj_soundcard_meter_analog_input_node( soundcard, node );
    } else {
        // Reset meter
        if( node->meter_pipe ) {
            zdj_meter_node_reset( node->meter_pipe );
        }
    }

    // Push final mixed buffer back to any linked output edge node.
    if( node->push_edge_output_data && node->edge_output_link ) {
        node->push_edge_output_data( node->edge_output_link, node->data_pipe, node->stereo );
    }

    // Apply any configured DSP to the mixed buffer.
    if( node->dsp ) { node->dsp( node ); }

    // If soundcard's o-scope is looking at this node, push samples to waveform pipe.
    if( soundcard->scope_node_name == node->name ) {
        // printf( "soundcard pushing %s to scope\n", zdj_soundcard_node_name[ node->name ] );
        zdj_live_waveform_state_t * scope_state = (zdj_live_waveform_state_t*)soundcard->scope_waveform->state;
        scope_state->handle_soundcard_node_push( soundcard->scope_waveform, node->data_pipe, node->stereo );
    }

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

    zdj_audio_buffer_node_state_t * source_buf_state = (zdj_audio_buffer_node_state_t*)input_node->data_pipe->state;

    float * source_buf = input_node->data_pipe->get_data( input_node->data_pipe );
    float * dest_buf = node->data_pipe->get_data( node->data_pipe );

    // Build an accumulator map to copy input node's samples into this node's buffer.
    zdj_soundcard_accumulate_map_t map;

    // Figure out nodes' mapping
    // If dest == stereo && source == stereo
    //      source/dest.stride=2, source/dest.count=2, source/dest.offset=0
    // If dest == stereo && source == mono
    //      source.stride=1 dest.stride=2, source.count=1 dest.count=2, source/dest.offset=0
    //      modulate source amplitude via pan for each dest channel
    // If dest == mono && source == stereo
    //      source.stride=2 dest.stride=1, source/dest.count=1, source/dest.offset=0
    // If dest == mono && source == mono
    //      source/dest.stride=1, source/dest.count=1, source/dest.offset=0

    bool accum_pan = false;
    if( node->stereo && input_node->stereo ) {
        // printf( "accum: st %s/%p <- st %s/%p\n", 
        //     zdj_soundcard_node_name[ node->name ], dest_buf,
        //     zdj_soundcard_node_name[ input_node->name ], source_buf
        // );
        map.dest_channel_stride = 2;
        map.dest_channel_count = 2;
        map.dest_channel_offset = 0;
        map.source_channel_stride = 2;
        map.source_channel_count = 2;
        map.source_channel_offset = 0;
    } else if ( node->stereo && !input_node->stereo ) {
        map.dest_channel_stride = 2;
        map.dest_channel_count = 2;
        map.dest_channel_offset = 0;
        map.source_channel_stride = 1;
        map.source_channel_count = zdj_soundcard_node_name_is_analog_input( input_node->name ) ? 2 : 1;
        map.source_channel_offset = zdj_soundcard_node_name_is_right_channel( input_node->name ) ? 0 : 1;
        map.source_pan = input_node->pan;
        // printf( "accum: st %s <- mo %s:%d/%d\n", 
        //     zdj_soundcard_node_name[ node->name ],
        //     zdj_soundcard_node_name[ input_node->name ],
        //     map.source_channel_count,
        //     map.source_channel_offset
        // );
        accum_pan = true;
    } else if ( !node->stereo && input_node->stereo ) {
        // printf( "accum: mo %s <- st %s\n", 
        //     zdj_soundcard_node_name[ node->name ],
        //     zdj_soundcard_node_name[ input_node->name ] 
        // );
        map.dest_channel_stride = 1;
        map.dest_channel_count = 1;
        map.dest_channel_offset = 0;
        map.source_channel_stride = 2;
        map.source_channel_count = 1;
        map.source_channel_offset = 0;
    } else if ( !node->stereo && !input_node->stereo ) {
        map.dest_channel_stride = 1;
        map.dest_channel_count = 1;
        map.dest_channel_offset = 0;
        map.source_channel_stride = 1;
        map.source_channel_count = zdj_soundcard_node_name_is_analog_input( input_node->name ) ? 2 : 1;
        map.source_channel_offset = zdj_soundcard_node_name_is_right_channel( input_node->name ) ? 0 : 1;
        // printf( "accum: mo %s <- mo %s:%d/%d\n", 
        //     zdj_soundcard_node_name[ node->name ],
        //     zdj_soundcard_node_name[ input_node->name ],
        //     map.source_channel_count,
        //     map.source_channel_offset
        // );
    }


    // Stride thru buffers, mixing samples, capturing metering data, and pushing waveform data
    float source_sample, dest_sample, new_dest_sample;
    int source_index, dest_index;
    float meter_val[ 2 ] = { 0 };
    // printf( "node:%s %p\n", zdj_soundcard_node_name[ node->name ], node->data_pipe );
    for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {

        // Select each destination channel
        for( int c=0; c<map.dest_channel_count; c++ ) {
            // Map source channels to destination channels
            if( map.source_channel_count == 1 ) {
                // Mono source channel - copy to mono or stereo dest channels
                source_index = i*map.source_channel_stride+map.source_channel_offset;
            } else {
                // Stereo source channel - copy to mono or stereo dest channels
                source_index = i*map.source_channel_stride+c+map.source_channel_offset;
            }
            dest_index = i*map.dest_channel_stride+c+map.dest_channel_offset;

            // Add the samples and clip to min/max ( -1.0->1.0 )
            source_sample = source_buf[ source_index ];
            dest_sample = dest_buf[ dest_index ];
            // Include pan for mono->stereo accum
            if( accum_pan ) {
                new_dest_sample = source_sample+dest_sample;
            } else {
                new_dest_sample = source_sample+dest_sample;
            }
            

            if( new_dest_sample > 1.0f ) {
                dest_buf[ dest_index ] = 1.0f;
            } else if( new_dest_sample < -1.0f ) {
                dest_buf[ dest_index ] = -1.0f;
            } else {
                dest_buf[ dest_index ] = new_dest_sample;
            }

            // Add sample to meter value
            if( c < 2 ) { meter_val[ c ] += fabs( new_dest_sample ); }
        }
    }

    // Select meter pipe - output nodes have special meter sourcing
    zdj_meter_node_state_t * meter_pipe_state;
    if( zdj_soundcard_node_name_is_analog_output( node->name ) ) {
        zdj_soundcard_node_t * source_node = zdj_soundcard_get_node_for_name (
            soundcard,
            node->input_links->source_node
        );
        meter_pipe_state = (zdj_meter_node_state_t*)source_node->meter_pipe->state;

        // printf( "meter: %s <- %s: %1.3f\n", 
        //     zdj_soundcard_node_name[ node->name ],
        //     zdj_soundcard_node_name[ source_node->name ],
        //     meter_val[ 0 ] / ZDJ_SOUNDCARD_BUF_LEN
        // );
        
    } else {
        meter_pipe_state = (zdj_meter_node_state_t*)node->meter_pipe->state;
    }

    // Average meter val + set in meter pipe
    if( meter_pipe_state && meter_pipe_state->add_frame ) { 
        meter_val[ 0 ] /= ZDJ_SOUNDCARD_BUF_LEN;
        meter_val[ 1 ] /= ZDJ_SOUNDCARD_BUF_LEN;
        meter_pipe_state->add_frame( node->meter_pipe, meter_val[ 0 ], meter_val[ 1 ] ); 
    }
}


// Analog input nodes need a special metering update cycle.
// They don't pass thru the accumulate cycle since they have no input linkage,
// so their meter pipes don't get updated unless we explicitly do it here.
zdj_error_type_t _zdj_soundcard_meter_analog_input_node(
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_t * node
) {
    zdj_soundcard_node_t * output_node = zdj_soundcard_get_node_for_name(
        soundcard,
        node->output_links->dest_node
    );
    

    // For Analog input nodes, we're actually metering the node to which it is linked.
    float * buf = node->data_pipe->get_data( node->data_pipe );

    // printf( "meter analog input: %s -> %s: %p\n", 
    //     zdj_soundcard_node_name[ node->name ],
    //     zdj_soundcard_node_name[ output_node->name ],
    //     buf
    // );

    float meter_val[ 2 ] = { 0 };
    for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
        if( node->stereo ) {
            if( zdj_soundcard_node_name_is_audio( output_node->name ) ) {
                meter_val[ 0 ] += fabs( buf[ i*2 ] );
                meter_val[ 1 ] += fabs( buf[ i*2+1 ] );
            } else if( zdj_soundcard_node_name_is_cv( output_node->name ) ) {
                meter_val[ 0 ] += buf[ i*2 ];
                meter_val[ 1 ] += buf[ i*2+1 ];
            }
            // printf( "meter analog stereo input: %s -> %s: %1.3f\n", 
            //     zdj_soundcard_node_name[ node->name ],
            //     zdj_soundcard_node_name[ output_node->name ],
            //     meter_val[ 0 ]
            // );
        } else {
            int channel;
            if( zdj_soundcard_node_name_is_audio( output_node->name ) ) {
                channel = zdj_soundcard_node_name_is_right_channel( node->name ) ? 0 : 1;
                meter_val[ 0 ] += fabs( buf[ i*2+channel ] );
            } else if( zdj_soundcard_node_name_is_cv( output_node->name ) ) {
                meter_val[ 0 ] += buf[ i*2 ];
            }
            // printf( "meter analog mono input: %s -> %s: %d - %1.3f\n", 
            //     zdj_soundcard_node_name[ node->name ],
            //     zdj_soundcard_node_name[ output_node->name ],
            //     channel,
            //     meter_val[ 0 ]
            // );
        }
    }

    zdj_meter_node_state_t * meter_pipe_state = (zdj_meter_node_state_t*)output_node->meter_pipe->state;

    // Average meter val + set in meter pipe
    if( meter_pipe_state && meter_pipe_state->add_frame ) { 
        meter_val[ 0 ] /= ZDJ_SOUNDCARD_BUF_LEN;
        meter_val[ 1 ] /= ZDJ_SOUNDCARD_BUF_LEN;
        meter_pipe_state->add_frame( node->meter_pipe, meter_val[ 0 ], meter_val[ 1 ] ); 
    }
}
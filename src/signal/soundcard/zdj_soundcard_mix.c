#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include <zerodj/signal/deck/zdj_deck_manager.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/buffer/zdj_audio_buffer_node.h>
#include <zerodj/signal/pipeline/node/analysis/meter/zdj_meter_node.h>
#include <zerodj/signal/pipeline/node/analysis/waveform/zdj_waveform.h>
#include <zerodj/signal/pipeline/node/audio/io/zdj_io_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

static void _meter_node(
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_t * node,
    zdj_soundcard_accumulate_map_t * map
);
static zdj_error_type_t _meter_analog_input_node(
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_t * node
);
static void _put_map(
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_t * input_node,
    zdj_soundcard_node_t * node,
    zdj_soundcard_accumulate_map_t * map
);
static void _process_dsp( zdj_soundcard_node_t * node );
static void _process_analog_out_gain( zdj_soundcard_node_t * node );

zdj_error_type_t zdj_soundcard_clear_buffer( 
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_t * node 
) {
    // Clear signal data buffer
    if( node->data_pipe && node->data_pipe->get_data ) {
        float * buffer = node->data_pipe->get_data( node->data_pipe );
        if( zdj_soundcard_node_name_is_analog_input( node->name ) ||
            zdj_soundcard_node_name_is_analog_output( node->name ) 
        ) {
            memset( buffer, 0, ZDJ_SOUNDCARD_BUF_LEN * sizeof( float ) * 2 );
        } else {
            memset( buffer, 0, ZDJ_SOUNDCARD_BUF_LEN * sizeof( float ) * (node->stereo+1) );
        }
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

    zdj_soundcard_accumulate_map_t map;
    memset(&map, 0, sizeof( zdj_soundcard_accumulate_map_t ) );

    // Recurse into linked inputs and accumulate to data_pipe buffer.
    if( node->input_link_count ) {
        // If there are input links, recurse and accumulate.
        for( int i=0; i<node->input_link_count; i++ ) {
            // Recursively mix/dsp the input node.
            zdj_soundcard_node_t * input_node = zdj_soundcard_get_node_for_name( soundcard, node->input_links[ i ].source_node );
            zdj_soundcard_mix_input( soundcard, input_node );
            _put_map( soundcard, input_node, node, &map );

            // Accumulate the input node's samples into this node's buffer + meter.
            // Note that we can't meter an edge node - there are no input links to an edge node.
            if( !zdj_soundcard_node_name_is_analog_input( input_node->name ) ) {
                zdj_soundcard_accumulate_node( soundcard, input_node, node, &map );
            } else if( !input_node->stereo ) {
                zdj_soundcard_accumulate_node( soundcard, input_node, node, &map );
            } else if( !zdj_soundcard_node_name_is_right_channel( input_node->name ) ) {
                // Avoid accumulating right channel of stereo input node
                zdj_soundcard_accumulate_node( soundcard, input_node, node, &map );
            }
        }
    } else if ( zdj_soundcard_node_name_is_analog_input( node->name ) ||
                zdj_soundcard_node_name_is_usb_input( node->name ) 
    ) {
        // Inputs have no input linkage - manually update metering.
        if( !node->stereo ) {
            _meter_analog_input_node( soundcard, node );
        } else if( !zdj_soundcard_node_name_is_right_channel( node->name ) ) {
            // Avoid accumulating right channel of stereo input node
            _meter_analog_input_node( soundcard, node );
        }
        
    } else {
        // Reset meter
        if( node->meter_pipe ) {
            zdj_meter_node_reset( node->meter_pipe );
        }
    }

    // Apply any configured DSP to the mixed buffer.
    if( node->dsp_dto ) { _process_dsp( node ); }

    // Apply signal-type gain to output nodes
    if( zdj_soundcard_node_name_is_analog_output( node->name ) ) {
        _process_analog_out_gain( node );
    }

    // Push final mixed/DSP'd buffer back to any linked output edge node.
    if( node->push_edge_output_data && node->edge_output_link ) {
        node->push_edge_output_data( node->edge_output_link, node->data_pipe, node->stereo );
    }

    // Push buffer into meter
    if( zdj_soundcard_node_name_is_clock( node->name ) ) {
        // _meter_clock
    } else if( !zdj_soundcard_node_name_is_analog_input( node->name ) ) {
        _meter_node( soundcard, node, &map );
    }

    // If this node has an active live waveform, push samples
    if( node->live_waveform_node && node->live_waveform_is_active ) {
        zdj_live_waveform_state_t * waveform_state = (zdj_live_waveform_state_t*)node->live_waveform_node->state;
        waveform_state->handle_soundcard_node_push( node->live_waveform_node, node->data_pipe, node->stereo );
    }

    // Mark this node's mix as complete.
    // Requests from other linked nodes will skip this node's
    // recursion and just accumulate its buffer into theirs.
    node->mix_complete = true;
}

static void _process_dsp( zdj_soundcard_node_t * node ) {
    float * buf = node->data_pipe->get_data( node->data_pipe );
    if( !node->dsp_dto ) { return; }
    int channel_count = node->stereo+1;
    // If muted, copy zeros into the buffer and return
    if( node->dsp_dto->mute ) {
        memset( buf, 0, ZDJ_SOUNDCARD_BUF_LEN * sizeof( float ) * channel_count );
        return;
    }
    // Process Pan, Gain, Crossfader Gain (if linked)
    // Skip Analog In nodes since they're wonky
    if( !zdj_soundcard_node_name_is_analog_input( node->name ) ) {
        for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
            buf[ i*channel_count ] *= node->dsp_dto->gain;
            if( channel_count > 1 ) {  
                buf[ (i*channel_count)+1 ] *= node->dsp_dto->gain;
            }
        }
    }
    // If there are extra DSP stages enabled, process those
    if( node->dsp_dto->has_stages ) {
        for( int i=0; i<8; i++ ) {
            if( node->dsp_dto->stages[ i ].fn ) {
                node->dsp_dto->stages[ i ].fn( &node->dsp_dto->stages[ i ], buf, channel_count );
            }
        }
    }
}

static void _process_analog_out_gain( zdj_soundcard_node_t * node ) {
    // If set to rail-to-rail just return
    if( node->signal_type == ZDJ_SOUNDCARD_SIGNAL_AUDIO_RAIL_TO_RAIL ) { return; }

    float * buf = node->data_pipe->get_data( node->data_pipe );
    int channel_count = node->stereo+1;
    float gain;
    switch ( node->signal_type ) {
        case ZDJ_SOUNDCARD_SIGNAL_MINUS_10_DBV:
            gain = ZDJ_SOUNDCARD_SIGNAL_MINUS_10_DBV_GAIN;
            break;
        case ZDJ_SOUNDCARD_SIGNAL_CON_0_DBV:
            gain = ZDJ_SOUNDCARD_SIGNAL_CON_0_DBV_GAIN;
            break;
        case ZDJ_SOUNDCARD_SIGNAL_PRO_PLUS_4_DBU:
            gain = ZDJ_SOUNDCARD_SIGNAL_PRO_PLUS_4_DBU_GAIN;
            break;
        case ZDJ_SOUNDCARD_SIGNAL_HEADPHONE_LOW:
            gain = ZDJ_SOUNDCARD_SIGNAL_HEADPHONE_LOW_GAIN;
            break;
        case ZDJ_SOUNDCARD_SIGNAL_HEADPHONE_HI:
            gain = ZDJ_SOUNDCARD_SIGNAL_HEADPHONE_HI_GAIN;
            break;
        case ZDJ_SOUNDCARD_SIGNAL_AUDIO_RAIL_TO_RAIL:
            gain = ZDJ_SOUNDCARD_SIGNAL_AUDIO_RAIL_TO_RAIL_GAIN;
            break;
    }
    if( node->stereo ) {
        for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
            buf[ i*2 ] *= gain;
            // if( channel_count > 1 ) {  
                buf[ (i*2)+1 ] *= gain;
            // }
        }
    } else if( !node->stereo && !zdj_soundcard_node_name_is_right_channel( node->name ) ) {
        // Apply gain to left channel only -- note that output buffers are 2 channels, whether
        // they're stereo linked or not.
        for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
            buf[ i*2 ] *= gain;
        }
    } else if( !node->stereo && zdj_soundcard_node_name_is_right_channel( node->name ) ) {
        // Apply gain to left channel only -- note that output buffers are 2 channels, whether
        // they're stereo linked or not.
        for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
            buf[ i*2+1 ] *= gain;
        }
    }
}


// Need to create a mapping for the accumulator to 
// know how to accum mono/stereo node channels together.
static void _put_map(
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_t * input_node,
    zdj_soundcard_node_t * node,
    zdj_soundcard_accumulate_map_t * map
) {
    bool node_is_ana_output = zdj_soundcard_node_name_is_analog_output( node->name );
    bool node_is_right_channel = zdj_soundcard_node_name_is_right_channel( node->name );
    bool input_node_is_ana_input = zdj_soundcard_node_name_is_analog_input( input_node->name );

    map->accum_pan = false;

    if( node_is_ana_output && input_node->stereo  ) {
        // If given output node is right channel, assume it isn't stereo
        // and set up to copy left channel of input to right channel of node
        map->dest_channel_stride = 2;
        map->dest_channel_count = (!node->stereo || node_is_right_channel) ? 1 : 2;
        map->dest_channel_offset = (node_is_right_channel) ? 1 : 0;
        map->source_channel_stride = 2;
        map->source_channel_count = (!node->stereo || node_is_right_channel) ? 1 : 2;
        map->source_channel_offset = 0;
    } else if( node_is_ana_output && !input_node->stereo ) {
        
        map->dest_channel_stride = 2;
        map->dest_channel_count = 1;
        map->dest_channel_offset = zdj_soundcard_node_name_is_right_channel( node->name ) ? 1 : 0;
        map->source_channel_stride = 1;
        map->source_channel_count = 1;
        map->source_channel_offset = 0;
    } else if( input_node_is_ana_input && input_node->stereo && node->stereo ) {
        map->dest_channel_stride = 2;
        map->dest_channel_count = 2;
        map->dest_channel_offset = 0;
        map->source_channel_stride = 2;
        map->source_channel_count = 2;
        map->source_channel_offset = 0;
    } else if( input_node_is_ana_input && !input_node->stereo && node->stereo ) {
        map->dest_channel_stride = 2;
        map->dest_channel_count = 2;
        map->dest_channel_offset = 0;
        map->source_channel_stride = 2; // buffer is always stereo even if input is split to mono
        map->source_channel_count = 1;
        map->source_channel_offset = zdj_soundcard_node_name_is_right_channel( input_node->name ) ? 1 : 0;
        // Add pan here
        map->accum_pan = true;
    } else if( input_node_is_ana_input && input_node->stereo && !node->stereo ) {
        map->dest_channel_stride = 1;
        map->dest_channel_count = 1;
        map->dest_channel_offset = 0;
        map->source_channel_stride = 2;
        map->source_channel_count = 2;
        map->source_channel_offset = 0;
    } else if( input_node_is_ana_input && !input_node->stereo && !node->stereo ) {
        map->dest_channel_stride = 1;
        map->dest_channel_count = 1;
        map->dest_channel_offset = 0;
        map->source_channel_stride = 2;
        map->source_channel_count = 1;
        map->source_channel_offset = zdj_soundcard_node_name_is_right_channel( node->name ) ? 1 : 0;
    } else if(node->stereo && input_node->stereo ) {
        map->dest_channel_stride = 2;
        map->dest_channel_count = 2;
        map->dest_channel_offset = 0;
        map->source_channel_stride = 2;
        map->source_channel_count = 2;
        map->source_channel_offset = 0;
    } else if ( node->stereo && !input_node->stereo ) {
        map->dest_channel_stride = 2;
        map->dest_channel_count = 2;
        map->dest_channel_offset = 0;
        map->source_channel_stride = 1;
        map->source_channel_count = 1;
        map->source_channel_offset = 0;
        map->accum_pan = true;
    } else if ( !node->stereo && input_node->stereo ) {
        map->dest_channel_stride = 1;
        map->dest_channel_count = 1;
        map->dest_channel_offset = 0;
        map->source_channel_stride = 2;
        map->source_channel_count = 1;
        map->source_channel_offset = 0;
    } else if ( !node->stereo && !input_node->stereo ) {
        map->dest_channel_stride = 1;
        map->dest_channel_count = 1;
        map->dest_channel_offset = 0;
        map->source_channel_stride = 1;
        map->source_channel_count = 1;
        map->source_channel_offset = 0;
    }
}

// Add samples from one node to another.  
// Observe stereo/pan behavior for each node.
zdj_error_type_t zdj_soundcard_accumulate_node(
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_t * input_node,
    zdj_soundcard_node_t * node,
    zdj_soundcard_accumulate_map_t * map
) {
    zdj_audio_buffer_node_state_t * source_buf_state = (zdj_audio_buffer_node_state_t*)input_node->data_pipe->state;
    float input_node_gain = 1.0f;

    float * source_buf;
    if( input_node->name == ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1 || 
        input_node->name == ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3 ||
        input_node->name == ZDJ_SOUNDCARD_NODE_NAME_USB_IN_1
    ) { 
        zdj_soundcard_node_t * left_node = zdj_soundcard_node_get_stereo_partner_node( input_node );
        source_buf = left_node->data_pipe->get_data( left_node->data_pipe );
        input_node_gain = input_node->dsp_dto->gain;
    } else if( input_node->name == ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_0 || 
        input_node->name == ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_2 ||
        input_node->name == ZDJ_SOUNDCARD_NODE_NAME_USB_IN_0
    ) {
        source_buf = input_node->data_pipe->get_data( input_node->data_pipe );
        input_node_gain = input_node->dsp_dto->gain;
    } else {
        source_buf = input_node->data_pipe->get_data( input_node->data_pipe );
    }

    float * dest_buf;
    // If dest node is analog out right channel, we need to grab a ref to the left buffer
    // because that's where the samples are.  This is dubm.
    if( node->name == ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1 || 
        node->name == ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_3 ||
        node->name == ZDJ_SOUNDCARD_NODE_NAME_USB_OUT_1
    ) { 
        zdj_soundcard_node_t * left_node = zdj_soundcard_node_get_stereo_partner_node( node );
        dest_buf = left_node->data_pipe->get_data( left_node->data_pipe );
    } else {
        dest_buf = node->data_pipe->get_data( node->data_pipe );
    }

    // Stride thru buffers, mixing samples, capturing metering data, and pushing waveform data
    float source_sample, dest_sample, dest_sample_l, dest_sample_r;
    float new_dest_sample, new_dest_sample_l, new_dest_sample_r;
    float pan_coeff_l, pan_coeff_r, pan_coeff;
    int source_index, dest_index;
    float meter_val[ 2 ] = { 0 };

    if( map->accum_pan ) {
        pan_coeff_l = ((input_node->dsp_dto->pan*-1.0) / 2.0) + 0.5;
        pan_coeff_r = (input_node->dsp_dto->pan / 2.0) + 0.5;
    }

    for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
        // Select each destination channel
        for( int c=0; c<map->dest_channel_count; c++ ) {
            // Map source channels to destination channels
            if( map->source_channel_count == 1 ) {
                // Mono source channel - copy to mono or stereo dest channels
                source_index = (i*map->source_channel_stride) + map->source_channel_offset;
            } else {
                // Stereo source channel - copy to mono or stereo dest channels
                source_index = (i*map->source_channel_stride) + c+map->source_channel_offset;
            }
            
            // Add the samples
            source_sample = source_buf[ source_index ] * input_node_gain;
            
            if( map->accum_pan ) {
                // Panned mix
                // If accum_pan is true, we know dest_buf is stereo.
                if( !node->stereo ) { 
                    printf( "panning into a mono buffer !!!" ); 
                } else {
                    dest_index = i*map->dest_channel_stride+c+map->dest_channel_offset;
                    pan_coeff = (c == 0) ? pan_coeff_l : pan_coeff_r;
                    dest_sample = dest_buf[ dest_index ];
                    new_dest_sample = (source_sample*pan_coeff)+dest_sample;
                    dest_buf[ dest_index ] = new_dest_sample;
                }
            } else {
                // Non-panned mix
                dest_index = i*map->dest_channel_stride+c+map->dest_channel_offset;
                dest_sample = dest_buf[ dest_index ];
                new_dest_sample = source_sample+dest_sample;
                dest_buf[ dest_index ] = new_dest_sample;
            }
        }
    }
}

static void _meter_node(
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_t * node,
    zdj_soundcard_accumulate_map_t * map
) {
    float meter_val[ 2 ] = { 0 };
    int index;
    int channel_count = node->stereo+1;
    float sample;
    float * buf = node->data_pipe->get_data( node->data_pipe );

    // printf( "node:%s %p\n", zdj_soundcard_node_name[ node->name ], node->data_pipe );
    for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {

        // Select each destination channel
        for( int c=0; c<channel_count; c++ ) {
            index = (i*channel_count)+c;
            sample = buf[ index ];
            // Add sample to meter value
            if( c < 2 ) { meter_val[ c ] += fabs( sample ); }
        }
    }

    // Select meter pipe - output nodes have special meter sourcing
    zdj_meter_node_state_t * meter_pipe_state = NULL;
    if( zdj_soundcard_node_name_is_analog_output( node->name ) ) {
        zdj_soundcard_node_t * source_node = zdj_soundcard_get_node_for_name (
            soundcard,
            node->input_links->source_node
        );
        if( source_node ) {
            meter_pipe_state = (zdj_meter_node_state_t*)source_node->meter_pipe->state;
        }
        
    } else {
        meter_pipe_state = (zdj_meter_node_state_t*)node->meter_pipe->state;
    }

    // Average meter val + set in meter pipe
    if( meter_pipe_state && meter_pipe_state->add_frame ) { 
        meter_val[ 0 ] /= ZDJ_SOUNDCARD_BUF_LEN;
        meter_val[ 1 ] /= ZDJ_SOUNDCARD_BUF_LEN;
        meter_pipe_state->add_frame( node->meter_pipe, meter_val[ 0 ], meter_val[ 1 ] ); 

        // Set clip
        if( meter_val[ 0 ] > 0.9 ) { 
            meter_pipe_state->has_ol_0_0 = true;
            meter_pipe_state->timer_ol_0_0 = 100;
            // Catch record bus clip to trigger UI alert
            if( node->name == ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS ) {
                // Catch record bus clip to Force the OL meter to appear
                zdj_deck_manager( )->control_change_flags[ ZDJ_DECK_CONTROL_RECORD_VOL ] = true;
            }
        }
        if( meter_pipe_state->timer_ol_0_0 > 0 ){ meter_pipe_state->timer_ol_0_0--; }
        else { meter_pipe_state->has_ol_0_0 = false; }

        if( meter_val[ 1 ] > 0.9 ) { 
            meter_pipe_state->has_ol_0_1 = true;
            meter_pipe_state->timer_ol_0_1 = 100;
            // Catch record bus clip to trigger UI alert
            if( node->name == ZDJ_SOUNDCARD_NODE_NAME_RECORD_BUS ) {
                // Catch record bus clip to Force the OL meter to appear
                zdj_deck_manager( )->control_change_flags[ ZDJ_DECK_CONTROL_RECORD_VOL ] = true;
            }
        }
        if( meter_pipe_state->timer_ol_0_1 > 0 ){ meter_pipe_state->timer_ol_0_1--; }
        else { meter_pipe_state->has_ol_0_1 = false; }
    }
}

// Analog input nodes need a special metering update cycle.
// They don't pass thru the accumulate cycle since they have no input linkage,
// so their meter pipes don't get updated unless we explicitly do it here.
static zdj_error_type_t _meter_analog_input_node(
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_t * node
) {
    zdj_soundcard_node_t * output_node = zdj_soundcard_get_node_for_name(
        soundcard,
        node->output_links->dest_node
    );

    // If the input node has no linkage, don't meter.
    if( !output_node ) { return ZDJ_ERROR_OKAY; }

    // For input node right channels, we need the left channel's data pipe
    float * buf;
    if( node->name == ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_1 || 
        node->name == ZDJ_SOUNDCARD_NODE_NAME_ANALOG_IN_3 ||
        node->name == ZDJ_SOUNDCARD_NODE_NAME_USB_IN_1
    ) {
        zdj_soundcard_node_t * left_node = zdj_soundcard_node_get_stereo_partner_node( node );
        buf = left_node->data_pipe->get_data( left_node->data_pipe );
    } else {
        buf = node->data_pipe->get_data( node->data_pipe );
    }

    float meter_val[ 2 ] = { 0 };
    if( node->stereo ) {
        for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
            meter_val[ 0 ] += fabs( buf[ i*2 ] );
            meter_val[ 1 ] += fabs( buf[ i*2+1 ] );
        }
    } else {
        for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
            int channel = zdj_soundcard_node_name_is_right_channel( node->name ) ? 1 : 0;
            meter_val[ 0 ] += fabs( buf[ i*2+channel ] );
        }
    }

    // Apply gain
    meter_val[ 0 ] *= node->dsp_dto->gain;
    meter_val[ 1 ] *= node->dsp_dto->gain;

    // Average meter val + set in meter pipe
    zdj_meter_node_state_t * meter_pipe_state = (zdj_meter_node_state_t*)node->meter_pipe->state;
    if( meter_pipe_state && meter_pipe_state->add_frame ) { 
        meter_val[ 0 ] /= ZDJ_SOUNDCARD_BUF_LEN;
        meter_val[ 1 ] /= ZDJ_SOUNDCARD_BUF_LEN;
        meter_pipe_state->add_frame( node->meter_pipe, meter_val[ 0 ], meter_val[ 1 ] ); 
    }
}
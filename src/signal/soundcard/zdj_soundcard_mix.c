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
            // if( node->name == ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_1){ printf( "%d links\n", node->input_link_count ); }
            // Recursively mix/dsp the input node.
            zdj_soundcard_node_t * input_node = zdj_soundcard_get_node_for_name( soundcard, node->input_links[ i ].source_node );
            zdj_soundcard_mix_input( soundcard, input_node );

            _put_map( soundcard, input_node, node, &map );
            // 
            // Accumulate the input node's samples into this node's buffer + meter.
            // Note that we can't meter an edge node - there are no input links to an edge node.
            zdj_soundcard_accumulate_node( soundcard, input_node, node, &map );
        }
    } else if ( zdj_soundcard_node_name_is_analog_input( node->name ) ||
                zdj_soundcard_node_name_is_usb_input( node->name ) 
    ) {
        // Inputs have no input linkage - manually update metering.
        _meter_analog_input_node( soundcard, node );
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
    } else {
        _meter_node( soundcard, node, &map );
    }

    // If soundcard's o-scope is looking at this node, push samples to waveform pipe.
    if( soundcard->scope_node_name == node->name ) {
        // printf( "soundcard pushing %s to scope\n", zdj_soundcard_node_name[ node->name ] );
        zdj_live_waveform_state_t * scope_state = (zdj_live_waveform_state_t*)soundcard->scope_waveform->state;
        scope_state->handle_soundcard_node_push( soundcard->scope_waveform, node->data_pipe, node->stereo );
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
    for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
        buf[ i*channel_count ] *= node->dsp_dto->gain;
        if( channel_count > 1 ) {  
            buf[ (i*channel_count)+1 ] *= node->dsp_dto->gain;
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

        // if( node->name == ZDJ_SOUNDCARD_NODE_NAME_ANALOG_OUT_0 ) {
        //     printf( "ana out->stereo: %d\n", node->stereo );
        // } 

        map->dest_channel_stride = 2;
        map->dest_channel_count = (!node->stereo || node_is_right_channel) ? 1 : 2;
        map->dest_channel_offset = (node_is_right_channel) ? 1 : 0;
        map->source_channel_stride = 2;
        map->source_channel_count = (!node->stereo || node_is_right_channel) ? 1 : 2;
        map->source_channel_offset = 0;

        // printf( "accum: st %s:%d/%d/%d <- mo %s:%d/%d/%d\n", 
        //     zdj_soundcard_node_name[ node->name ],
        //     map->dest_channel_stride,
        //     map->dest_channel_count,
        //     map->dest_channel_offset,
        //     zdj_soundcard_node_name[ input_node->name ],
        //     map->source_channel_stride,
        //     map->source_channel_count,
        //     map->source_channel_offset
        // );
    } else if( node_is_ana_output && !input_node->stereo ) {
        
        map->dest_channel_stride = 2;
        map->dest_channel_count = 1;
        map->dest_channel_offset = zdj_soundcard_node_name_is_right_channel( node->name ) ? 1 : 0;
        map->source_channel_stride = 1;
        map->source_channel_count = 1;
        map->source_channel_offset = 0;
        // printf( "accum: st %s:%d/%d/%d <- mo %s:%d/%d/%d\n", 
        //     zdj_soundcard_node_name[ node->name ],
        //     map->dest_channel_stride,
        //     map->dest_channel_count,
        //     map->dest_channel_offset,
        //     zdj_soundcard_node_name[ input_node->name ],
        //     map->source_channel_stride,
        //     map->source_channel_count,
        //     map->source_channel_offset
        // );
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
        map->source_channel_stride = 2;
        map->source_channel_count = 1;
        map->source_channel_offset = zdj_soundcard_node_name_is_right_channel( input_node->name ) ? 1 : 0;
        // Add pan here
        // map->source_pan = input_node->pan;
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
        // printf( "accum: st %s <- st %s\n", 
        //     zdj_soundcard_node_name[ node->name ],
        //     zdj_soundcard_node_name[ input_node->name ]
        // );
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
        // map->source_pan = input_node->pan;
        // if( is_output_node ) {
        //     printf( "accum: st %s <- mo %s:%d/%d\n", 
        //         zdj_soundcard_node_name[ node->name ],
        //         zdj_soundcard_node_name[ input_node->name ],
        //         map->source_channel_count,
        //         map->source_channel_offset
        //     );
        // }
        map->accum_pan = true;
    } else if ( !node->stereo && input_node->stereo ) {
        // printf( "accum: mo %s <- st %s\n", 
        //     zdj_soundcard_node_name[ node->name ],
        //     zdj_soundcard_node_name[ input_node->name ] 
        // );
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
        // printf( "accum: mo %s <- mo %s:%d/%d\n", 
        //     zdj_soundcard_node_name[ node->name ],
        //     zdj_soundcard_node_name[ input_node->name ],
        //     map->source_channel_count,
        //     map->source_channel_offset
        // );
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
    // printf( "accum: %s <- %s\n", 
    //     zdj_soundcard_node_name[ node->name ],
    //     zdj_soundcard_node_name[ input_node->name ] 
    // );

    zdj_audio_buffer_node_state_t * source_buf_state = (zdj_audio_buffer_node_state_t*)input_node->data_pipe->state;

    float * source_buf = input_node->data_pipe->get_data( input_node->data_pipe );

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
    float pan_coeff_l, pan_coeff_r;
    int source_index, dest_index;
    float meter_val[ 2 ] = { 0 };

    if( map->accum_pan ) {
        pan_coeff_l = ((input_node->dsp_dto->pan*-1.0) / 2.0) + 0.5;
        pan_coeff_r = (input_node->dsp_dto->pan / 2.0) + 0.5;
        // printf( "%s->%s pan %1.3f [%1.3f<>%1.3f]\n", 
        //     zdj_soundcard_node_name[ input_node->name ], 
        //     zdj_soundcard_node_name[ node->name ], 
        //     input_node->dsp_dto->pan, pan_coeff_l, pan_coeff_r 
        // );
    }


    // if( input_node->name == ZDJ_SOUNDCARD_NODE_NAME_USB_IN_0 ) {
    //     printf( "usb in 0[ 5 ]: %f\n", source_buf[ 5 ] );
    // }


    for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {

        // Select each destination channel
        for( int c=0; c<map->dest_channel_count; c++ ) {
            // Map source channels to destination channels
            if( map->source_channel_count == 1 ) {
                // Mono source channel - copy to mono or stereo dest channels
                source_index = i*map->source_channel_stride+map->source_channel_offset;
            } else {
                // Stereo source channel - copy to mono or stereo dest channels
                source_index = i*map->source_channel_stride+c+map->source_channel_offset;
            }
            
            // Add the samples
            source_sample = source_buf[ source_index ];
            
            if( map->accum_pan ) {
                // Panned mix
                // If accum_pan is true, we know dest_buf has 2 channels.
                if( !node->stereo ) { 
                    printf( "panning into a mono buffer !!!" ); 
                } else {
                    dest_sample_l = dest_buf[ i*2 ];
                    dest_sample_r = dest_buf[ i*2+1 ];

                    new_dest_sample_l = (source_sample*pan_coeff_l)+dest_sample_l;
                    dest_buf[ i*2 ] = new_dest_sample_l;

                    new_dest_sample_r = (source_sample*pan_coeff_r)+dest_sample_r;
                    dest_buf[ i*2+1 ] = new_dest_sample_r;
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
    // printf( "_meter_node 0: %s\n", zdj_soundcard_node_name[ node->name ] );
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

        // printf( "meter: %s <- %s: %1.3f\n", 
        //     zdj_soundcard_node_name[ node->name ],
        //     zdj_soundcard_node_name[ source_node->name ],
        //     meter_val[ 0 ] / ZDJ_SOUNDCARD_BUF_LEN
        // );
        
    } else {
        meter_pipe_state = (zdj_meter_node_state_t*)node->meter_pipe->state;
        // printf( "meter %s: %1.3f\n", 
        //     zdj_soundcard_node_name[ node->name ],
        //     meter_val[ 0 ] / ZDJ_SOUNDCARD_BUF_LEN
        // );
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

    // printf( "_meter_node done\n" );
}

// Analog input nodes need a special metering update cycle.
// They don't pass thru the accumulate cycle since they have no input linkage,
// so their meter pipes don't get updated unless we explicitly do it here.
static zdj_error_type_t _meter_analog_input_node(
    zdj_soundcard_t * soundcard, 
    zdj_soundcard_node_t * node
) {
    // printf( "_meter_analog_input_node: %s\n", zdj_soundcard_node_name[ node->name ] );
    zdj_soundcard_node_t * output_node = zdj_soundcard_get_node_for_name(
        soundcard,
        node->output_links->dest_node
    );

    // For Analog input nodes, we're actually metering the node to which it is linked.
    float * buf = node->data_pipe->get_data( node->data_pipe );

    // printf( "meter analog input: %s -> %s: %p %f\n", 
    //     zdj_soundcard_node_name[ node->name ],
    //     zdj_soundcard_node_name[ output_node->name ],
    //     buf, buf[ 0 ]
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
    // printf( "_meter_analog_input_node done\n" );
}
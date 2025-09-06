#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>

#include <libavformat/avformat.h>
#include <libavcodec/packet.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavcodec/codec_id.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_decode_node.h>

static void _update_wait( zdj_pipeline_node_t * node );
static void _deinit_state( zdj_pipeline_node_t * node );

static zdj_error_type_t _move_window( zdj_pipeline_node_t * node, int offset );
static zdj_error_type_t _reset_window( zdj_pipeline_node_t * node, uint32_t address );
static zdj_error_type_t _resize_window( 
    zdj_pipeline_node_t * node, 
    uint32_t back_infill_targ, 
    uint32_t fwd_infill_targ 
);

static void _add_packet_before( AVFormatContext * fmt_ctx, zdj_decode_packet_t * packet );
static zdj_decode_packet_t * _add_packet_after( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet );
static void _deinit_packet( zdj_decode_packet_t * packet );

static void _move_discon_seq_window_forward( zdj_pipeline_node_t * node, int offset );
static void _move_discon_seq_window_backward( zdj_pipeline_node_t * node, int offset );
static void _add_fwd_discon_seq( zdj_pipeline_node_t * node );
static void _add_back_discon_seq( zdj_pipeline_node_t * node );

zdj_pipeline_node_t * zdj_new_decode_node( 
    zdj_library_song_t * song,
    uint64_t address,
    size_t back_sample_count,
    size_t fwd_sample_count
) {
    // Make node
    zdj_pipeline_node_t * node = zdj_new_pipeline_node( );
    node->deinit_state = &_deinit_state;
    node->update_wait = &_update_wait;
    node->move_window = &_move_window;
    node->reset_window = &_reset_window;

    // Add state
    zdj_decode_node_state_t * state = calloc( 1, sizeof( zdj_decode_node_state_t ) );
    node->state = state;
    state->status = ZDJ_DECODE_NODE_INIT;
    state->song = song;
    state->win_back_sample_count = back_sample_count;
    state->win_fwd_sample_count = fwd_sample_count;
    state->win_sample_count = back_sample_count + fwd_sample_count;
    state->channel_count = state->song->audio->av_channel_count;
    state->song_pcm_duration = state->song->audio->duration;

    state->decode_mono_addr.i_val = 0;
    state->decode_mono_addr.space = ZDJ_PIPELINE_ADDRESS_DECODE_MONOSPACE;

    // Alloc output buffer - sample_count * 4 is a bit of a mystery - malloc error if it's less. 
    state->out_buffer = calloc( state->win_sample_count * 4, sizeof( float ) );

    // Use the open command to build the Format Context
    int res = avformat_open_input( &state->fmt_ctx, strdup( song->audio->filepath ), NULL, NULL );
    if( res != 0 ) {
        printf( "avformat_open_input failed\n" );
    }

    res = avformat_find_stream_info( state->fmt_ctx, NULL );
    if( res != 0 ) {
        printf( "avformat_find_stream_info failed\n" );
    }

    // Build a Codec Context for decoding.
    AVCodec * codec = avcodec_find_decoder( state->song->audio->av_codec_id );
    state->codec_ctx = avcodec_alloc_context3( codec );
    state->codec_ctx->channels = state->song->audio->av_channel_count;
    // state->codec_ctx->request_sample_fmt = AV_SAMPLE_FMT_FLT; // Keep... in case.
    res = avcodec_open2( state->codec_ctx, codec, NULL );

    // Init/add first discontinuity sequence
    // Create inital seq/packet, then call move_seq with zero offset.
    // Forces seq window to fill forward/backward from initial packet.
    state->discon_seq = calloc( 1, sizeof( zdj_decode_discon_seq_t ) );
    state->discon_seq->packet_set = _add_packet_after( node, NULL );
    _move_discon_seq_window_forward( node, 0 );
    // _move_discon_seq_window_backward( node, 0 );

    // // Debug dump format
    // av_dump_format( state->fmt_ctx, 0, song->audio->filepath, 0 );

    return node;
}

float _dn_ts1_p = 0;
float _dn_ts1_f = 945;
float _dn_ts2_p = 0;
float _dn_ts2_f = 300;

// Execute a copy/accumulate of all of node's discon_seqs to the out_buffer.
static void _update_wait( zdj_pipeline_node_t * node ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    zdj_decode_discon_seq_t * seq = state->discon_seq;

    // printf( "_update_wait: %s / %d\n", state->song->audio->filepath, state->highest_out_write );

    // Clear out_buffer - count is empirical to avoid crash.
    // Figure out why * 4 is needed.  Don't alter without testing.
    memset( state->out_buffer, 0, state->win_sample_count * 4 * sizeof( float ) );
    
    int seq_count = 0;
    int packet_count = 0;
    int accum_tally = 0;
    // Run thru discon_seqs, mapping packet samples thru accumulator to out_buffer.
    while( seq ) {
        seq_count++;
        zdj_decode_packet_t * packet = seq->packet_set;
        while( packet ) {
            packet_count++;
            if( packet->accum ) { 
                accum_tally += packet->accum( node, packet ); 
            }
            // If we're decoding, update the analysis progress
            state->song->analysis_progress = state->head_percent;
            packet = packet->next;
        }
        seq = seq->next;
    }

    state->debug_seq_count = seq_count;
    state->debug_packet_count = packet_count;
    state->debug_accum_tally = accum_tally;

    // Write test sine to out_buffer
    // _dn_ts1_p = zdj_signal_gen_sine( 
    //     _dn_ts1_f, 
    //     _dn_ts1_p, 
    //     state->win_sample_count,
    //     state->out_buffer,
    //     state->channel_count,
    //     0,
    //     3000.0
    // );
    // _dn_ts2_p = zdj_signal_gen_sine( 
    //     _dn_ts2_f, 
    //     _dn_ts2_p, 
    //     state->win_sample_count,
    //     state->out_buffer,
    //     state->channel_count,
    //     1,
    //     3000.0
    // );
}

static void _deinit_state( zdj_pipeline_node_t * node ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    if( state->fmt_ctx ) { avformat_close_input( &state->fmt_ctx ); }
    if( state->codec_ctx ) { avcodec_free_context( &state->codec_ctx ); }
    if( state->out_buffer ) { free( state->out_buffer ); }
    if( state ) { node->state = NULL; free( state );  }
}

static zdj_error_type_t _move_window( zdj_pipeline_node_t * node, int offset ) {
    zdj_decode_node_state_t * state = (zdj_decode_node_state_t*)node->state;
    // printf( "_move_window: %s\n", state->song->audio->filepath );

    // For each existing discon_seq, decode new packets to fill window offset.
    zdj_decode_discon_seq_t * seq = state->discon_seq;
    while( seq ) {
        if( offset > 0 ) {
            _move_discon_seq_window_forward( node, offset );
        } else if( offset < 0 ) {
            _move_discon_seq_window_backward( node, offset );
        }
        seq = seq->next;
    }
    if( offset > 0 ) {
        // If window moves forward, but current discon_seq stack doesn't fill
        // output_buffer, one or more new discon_seqs are required to handle
        // the current discontinuity state.  
        // Add discon_seqs until out buffer is full.
        // Imagine a 5-sample loop with a 12-sample forward window move.
        // 3 new discon_seqs would need to be added to fill window.
        while( state->win_fwd_valid_sample < state->win_sample_count ) {
            _add_fwd_discon_seq( node );
        }
    } else if( offset < 0 ) {
        // If window moves backward, but current discon_seq stack doesn't go back
        // that far, keep adding discon_seqs until it does.
        // while( state->win_back_valid_sample > 0 ) {
        //     _add_back_discon_seq( node );
        // }
    }

    // Update the monotonic address
    state->decode_mono_addr.i_val += offset;
}


static zdj_error_type_t _reset_window( zdj_pipeline_node_t * node, uint32_t address ) {
    // Clear and set reference address state.
    // Create loop/skip states as needed.
    // Create raw nodes as required.
    // Update window data management.
    // Reset monotonic address to zero.
}

static zdj_error_type_t _resize_window( 
    zdj_pipeline_node_t * node, 
    uint32_t back_infill_targ, 
    uint32_t fwd_infill_targ 
) {

}

// An external node, a Pitch TSM node for ex. is asking for this node's current mono address.
// Fill in the referenced addr based on this node's current mono address.
void zdj_decode_node_capture_mono_addr( zdj_pipeline_node_t * node, zdj_pipeline_addr_t * addr ) {
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;
    addr->i_val = node_state->decode_mono_addr.i_val;
}

// Transform a set of TSM interpolation coords by the difference between the 
// given mono address and this node's current mono address.
void zdj_decode_node_xform_tsm_coords_for_captured_mono_addr( 
    zdj_pipeline_node_t * node,
    double * start_coord,
    double * end_coord,
    zdj_pipeline_addr_t * mono_addr
) {
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;
    double offset = mono_addr->i_val - node_state->decode_mono_addr.i_val;
    *start_coord += offset;
    *end_coord += offset;
}

// Discontinuity / Sample / Frame layout example
// --------------------------------------------------------------------------------
// discon_seq #1
//       discon interior -> |---------------------|
//  [ ] [ ] [ ] [-] [-] [-] [*] [*] [*] [*] [*] [*] [-] [-] [-] [ ] [ ] [ ] [ ] [ ]
//              |---------|    <- discon edge ->    |---------|
//  [ packet          ] [ packet          ] [ packet          ] [ packet          ]
// 
//  discon_seq #2
//  [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [-] [-] [-] [*] [*] [*] [*] [*] [*] [-] [-]
//  discon_seq #3
//  [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [ ] [-] [-] [-] [*] [*]
static void _move_discon_seq_window_forward( zdj_pipeline_node_t * node, int offset ) {
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;
    zdj_decode_discon_seq_t * seq = node_state->discon_seq;
    zdj_decode_packet_t * first_packet = seq->packet_set;
    zdj_decode_packet_t * packet;

    // Run thru packet_set and move windows.
    packet = first_packet;
    while ( packet ) {
        // Move packet out_buf clipping targets based on new offset.
        packet->discon_in_buf_addr.i_val -= offset;
        packet->xfade_in_buf_addr.i_val -= offset;
        packet->discon_out_buf_addr.i_val -= offset;
        packet->xfade_out_buf_addr.i_val -= offset;
        packet = packet->next;
    }

    // Run thru packet set, adding new packets to fill right window edge
    packet = first_packet;
    while ( packet ) {
        // Look at last packet in seq - add a new packet if it doesn't cover out_buf
        if( !packet->next ) {
            // printf( "%s - last packet: %p %ld/%ld - %ld\n", 
            //     node_state->song->audio->filepath,
            //     packet, 
            //     packet->discon_in_buf_addr.i_val, 
            //     packet->discon_out_buf_addr.i_val,
            //     node_state->win_sample_count
            // );
            // Store the latest out_buffer address present across all seqs in node.
            node_state->win_fwd_valid_sample = fmax( 
                node_state->win_fwd_valid_sample, packet->discon_out_buf_addr.i_val 
            );

            // Don't add packets beyond eof
            if( !packet->has_eof &&
                packet->discon_out_buf_addr.i_val < (int64_t)node_state->win_sample_count 
            ) {
                _add_packet_after( node, packet );
            }
        }

        // This calculation requires much more nuance.
        // Find a better arch for this as scrubbing comes online.
        if( packet->has_eof ) { node_state->at_eof = true; } // <- temporary

        packet = packet->next;
    }

    // Run thru packet set, looking for the new root packet for this seq.
    // Based on the earliest xfade_out_buf_addr which falls within the window.
    // A window move may jump by multiple packets.
    packet = first_packet;
    while ( packet ) {
        if( packet->xfade_out_buf_addr.i_val > 0 ) {
            // Just grab first packet which enters left edge of window and exit.
            seq->packet_set = packet;
            packet = NULL;
        } else {
            packet = packet->next;
        }
    }
    
    // Run thru packet set, removing packets beyond left window edge
    packet = first_packet;
    while ( packet ) {
        // Packet has moved off left edge of window...
        if( packet->xfade_out_buf_addr.i_val < 0 ) {
            // Hold on to next packet ref if we're deleting this one
            zdj_decode_packet_t * next_packet = packet->next;
            _deinit_packet( packet );
            packet = next_packet;
        } else {
            packet = NULL; // Exit early once we're inside the window
        }
    }
}

static void _move_discon_seq_window_backward( zdj_pipeline_node_t * node, int offset ) {
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;
    zdj_decode_discon_seq_t * seq = node_state->discon_seq;
    zdj_decode_packet_t * first_packet = seq->packet_set;
    zdj_decode_packet_t * packet;
    // Run forward thru packet_set and move windows.
    packet = first_packet;
    while ( packet ) {
        // Move packet out_buf targets based on new offset.
        packet->discon_in_buf_addr.i_val += offset;
        packet->discon_out_buf_addr.i_val += offset;
        packet = packet->next;
    }
}

static void _add_fwd_discon_seq( zdj_pipeline_node_t * node ) {

}

static void _add_back_discon_seq( zdj_pipeline_node_t * node ) {

}

static void _add_packet_before( AVFormatContext * fmt_ctx, zdj_decode_packet_t * packet ) {
    // Calculate seek target from packet address
    // Build a timestamp by stepping back from packet by packet width
    // av_seek_frame( fmt_ctx, 0, seek_timestamp, AVSEEK_FLAG_ANY | AVSEEK_FLAG_BACKWARD );
    
    // // Create packet and set params
    // AVPacket * packet = av_packet_alloc( );
    // int res = av_read_frame( state->fmt_ctx, packet );
    // if( res != 0 ) {
    //     if( state->fmt_ctx->pb->eof_reached ) {
    //         state->at_eof = true;
    //     } else {
    //         state->at_eof = true;
    //         printf( "av_read_frame failed\n" );
    //     }
    // }
}

// add_packet_after is called when first creating a new seq.
// In that case, there will be no existing packet to add this to.
// Just return the newly created packet and let the window move fn handle it.
static zdj_decode_packet_t * _add_packet_after( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet ) {
    zdj_decode_node_state_t * node_state = (zdj_decode_node_state_t*)node->state;

    // Calculate seek target from packet address
    // Build a timestamp at the next address after packet
    // av_seek_frame( node->fmt_ctx, 0, node_state., AVSEEK_FLAG_ANY );

    bool at_eof = false;

    // Create packet/frame and set params
    AVPacket * av_packet = av_packet_alloc( );
    AVFrame * av_frame = av_frame_alloc( );
    int res;

    // uint8_t parse_buf[ 2048 ];
    // res = av_parser_parse2( 
    //     node_state->parser_ctx, node_state->codec_ctx, &av_packet->data, &av_packet->size, parse_buf, 
    //     sizeof( parse_buf ), AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0 
    // );
    
    // Fill the Packet with compressed data
    res = av_read_frame( node_state->fmt_ctx, av_packet );
    if( res != 0 ) {
        if( node_state->fmt_ctx->pb->eof_reached ) {
            at_eof = true;
        } else {
            at_eof = true; // <- temporary
            printf( "av_read_frame failed\n" );
        }
    }

    // MP3s seem to have trouble sending the first packet or two.
    // Retry a few times before just giving up.
    bool exit = false;
    int attempt = 0;
    while ( !exit ) {
        // Push the compressed packet data into the decoder
        res = avcodec_send_packet( node_state->codec_ctx, av_packet );
        if ( res >= 0 ) { 
            exit = true;
            continue;
        }
        printf( "%s send_packet failed: %s\n", 
            node_state->song->audio->filepath, 
            av_err2str( res ) 
        );
        if( attempt > 2 ) {
            node_state->song->audio->has_libav_error = true;
            node_state->song->audio->libav_error = res;
            node_state->song->has_error = true;
            return NULL; 
        }
        // Attempt to read another frame to see if things improve
        attempt++;
        av_read_frame( node_state->fmt_ctx, av_packet );
    }

    // Pull the decompressed samples from the decoder into the packet's data buffer
    int decoded_frame_count = 0;
    res = 0;
    res = avcodec_receive_frame( node_state->codec_ctx, av_frame );
    if ( res == AVERROR( EAGAIN ) || res == AVERROR_EOF ) {
        // Handle decoding error
        printf( "decoding error: %s\n", av_err2str( res ) );
        // break;
    } else if ( res < 0 ) {
        printf( "other error: %s\n", av_err2str( res ) );
        // continue;
    } else {
        decoded_frame_count = av_frame->nb_samples;
    }
    
    zdj_decode_packet_t * new_packet = calloc( 1, sizeof( zdj_decode_packet_t ) );
    new_packet->av_packet = av_packet;
    new_packet->av_frame = av_frame;
    new_packet->av_codec_id = node_state->song->audio->av_codec_id;
    new_packet->av_frame_sample_count = decoded_frame_count;

    // Handle MP3s which have a different timebase for... reasons.
    int codec_timebase_factor = ( new_packet->av_codec_id == AV_CODEC_ID_MP3 ) ? 320 : 1;
    
    // Set up discontinuity params -- currently just play the whole packet
    new_packet->is_contiguous = true; // Add check for discon
    new_packet->has_eof = at_eof;
    new_packet->discon_in_sample = 0;

    // If this is the first packet in the seq, start at timestamp 0.
    // uint64_t prev_packet_out_addr = ( packet ) ? packet->discon_out_buf_addr.i_val : 0;
    new_packet->discon_out_sample = decoded_frame_count;
    // new_packet->discon_in_buf_addr.i_val = prev_packet_out_addr + 1;
    new_packet->discon_in_buf_addr.i_val = new_packet->av_packet->pts / codec_timebase_factor;
    new_packet->discon_out_buf_addr.i_val = new_packet->discon_in_buf_addr.i_val + decoded_frame_count;

    // Temporary until we have finished the seek frame work
    // new_packet->packet_pcm_addr.i_val = new_packet->discon_in_buf_addr.i_val;
    new_packet->packet_pcm_addr.i_val = new_packet->av_packet->pts / codec_timebase_factor;

    // Temporary until we have finished discon work
    // Set up discon xfade indexes
    new_packet->xfade_in_buf_addr.i_val = new_packet->discon_in_buf_addr.i_val;
    new_packet->xfade_out_buf_addr.i_val = new_packet->discon_out_buf_addr.i_val;

    // Set up the out_buffer accumulator based on sample data format
    switch ( av_frame->format ) {
        case AV_SAMPLE_FMT_U8: new_packet->accum = &zdj_decode_node_accum_u8; break;
        case AV_SAMPLE_FMT_S16: new_packet->accum = &zdj_decode_node_accum_s16; break;
        case AV_SAMPLE_FMT_S32: new_packet->accum = &zdj_decode_node_accum_s32; break;
        case AV_SAMPLE_FMT_S64: new_packet->accum = &zdj_decode_node_accum_s64; break;
        case AV_SAMPLE_FMT_FLT: new_packet->accum = &zdj_decode_node_accum_flt; break;
        case AV_SAMPLE_FMT_DBL: new_packet->accum = &zdj_decode_node_accum_dbl; break;
        case AV_SAMPLE_FMT_U8P: new_packet->accum = &zdj_decode_node_accum_u8p; break;
        case AV_SAMPLE_FMT_S16P: new_packet->accum = &zdj_decode_node_accum_s16p; break;
        case AV_SAMPLE_FMT_S32P: new_packet->accum = &zdj_decode_node_accum_s32p; break;
        case AV_SAMPLE_FMT_S64P: new_packet->accum = &zdj_decode_node_accum_s64p; break;
        case AV_SAMPLE_FMT_FLTP: new_packet->accum = &zdj_decode_node_accum_fltp; break;
        case AV_SAMPLE_FMT_DBLP: new_packet->accum = &zdj_decode_node_accum_dblp; break;
        default: new_packet->accum = NULL; break;
    }
    
    // Feed a test sine wave into the packet decode data
    // new_packet->accum = &zdj_decode_node_accum_test_sine;

    // Packet will be null if this is the first packet in a seq
    if( packet ) {
        // Insert new packet if we're mid-sequence
        if( packet->next ) { 
            packet->next->prev = new_packet;
            new_packet->next = packet->next; 
        }
        packet->next = new_packet;
        new_packet->prev = packet;
    } else {
        new_packet->next = NULL;
        new_packet->prev = NULL;
    }

    // Only used when creating a new seq with no packets.
    return new_packet;
}

static void _deinit_packet( zdj_decode_packet_t * packet ) {
    // printf( "_deinit_packet\n" );
    // Stitch surrounding packets together if we're attached to other packets
    if( packet->prev ) {
        packet->prev->next = packet->next;
    }
    if( packet->next ) {
        packet->next->prev = packet->prev;
    }
    packet->next = NULL;
    packet->prev = NULL;
    av_packet_unref( packet->av_packet );
    av_frame_unref( packet->av_frame );
    free( packet );
}
// Copyright (c) 2025 Drift DJ Industries

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef ZDJ_DECODE_NODE_H
#define ZDJ_DECODE_NODE_H

#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavcodec/codec_id.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>

typedef enum {
    ZDJ_DECODE_NODE_INIT,
    ZDJ_DECODE_NODE_READY,
    ZDJ_DECODE_NODE_RUNNING,
    ZDJ_DECODE_NODE_DONE,
    ZDJ_DECODE_NODE_IDLE
} zdj_decode_node_status_t;

typedef struct zdj_decode_packet_t {
    // void * buffer;
    int ( *accum )( zdj_pipeline_node_t *, struct zdj_decode_packet_t * );

    AVPacket * av_packet; // libav's holder for a chunk of raw, undecoded data
    AVFrame * av_frame; // libav's holder for a chunk of decoded PCM data
    int av_frame_sample_count; // Count of frames decoded into AVFrame

    zdj_pipeline_addr_t packet_pcm_addr; // PCM address of first sample in decoded frame
    bool is_contiguous; // no discontinuity present in this packet
    bool has_eof;
    bool has_sof;

    // MP3s from FFMpeg have a weird timebase.
    // MP3 timestamps are all in 44100*320 time.
    // When seeking and calculating frame indexes, factor in this 320 value.  
    enum AVCodecID av_codec_id;
    bool has_mp3_timebase;

    // Discon in/out points don't include xfade. 
    // They specify the exact PCM sample at which loops/skips start/end.
    // xfade values are a separate calculation based on length of loop/skip.
    // Loops are a minimum of 2 frames long.

    // Sample in av_packet buffer at which loop starts, or 0 if this is a contiguous frame.
    int discon_in_sample;
    // Sample in decode node out_buffer at which discon_in_sample will be written.
    zdj_pipeline_addr_t discon_in_buf_addr;
    zdj_pipeline_addr_t xfade_in_buf_addr; 

    // Sample in av_packet buffer at which loop ends, or packet->len if this is a contiguous frame.
    int discon_out_sample;
    // Sample in decode node out_buffer at which discon_out_sample will be written.
    zdj_pipeline_addr_t discon_out_buf_addr; 
    zdj_pipeline_addr_t xfade_out_buf_addr; 

    int discon_sample_count; // samples in packet, accounting for discon if present.
    
    // Xfade params - for calculating crossfade coeffs during accumulate if discon is present.
    // bool contains_xfade;
    // int discon_in_xfade_start; // can be negative if xfade starts in prev frame.
    // int discon_in_xfade_len; // min( loop_len - 1, 128 )
    // int discon_out_xfade_start; // can be negative if xfade starts in prev frame.
    // int discon_out_xfade_len;
    int discon_xfade_sample_count;
    
    struct zdj_decode_packet_t * next;
    struct zdj_decode_packet_t * prev;
} zdj_decode_packet_t;

typedef struct zdj_decode_discon_seq_t {
    zdj_decode_packet_t * packet_set;
    struct zdj_decode_discon_seq_t * next;
    struct zdj_decode_discon_seq_t * prev;
} zdj_decode_discon_seq_t;

typedef struct {
    zdj_library_song_t * song;
    zdj_decode_node_status_t status;

    // Indexes of out_buffer define an address space: ZDJ_PIPELINE_ADDRESS_DECODE_WIN.
    // It is the mapping link between ZDJ_PIPELINE_ADDRESS_FILE_PCM and
    // higher-level address spaces, like ZDJ_PIPELINE_ADDRESS_GLOBAL_PCM.
    float * out_buffer;
    
    // discon_seq joins together 2 purposes:
    // 1 - hold decoded sample data from libav decode process.
    // 2 - serve as the address data source while copying discontinuities (loops/skips)
    //     from the originial decoded sample data to the out_buffer.
    zdj_decode_discon_seq_t * discon_seq;
    // It's a linked-list of containers for sets of decoded sample packets.
    // Starting at first discon_seq, samples are copied from the seq's packet_set
    // into the out_buffer.  If we reach the end of the packet_set before
    // we reach the end of the out_buffer, we advance to the next packet_set, etc. 

    // In a loop state which is shorter than decode_win, filling the
    // decode buffer requires multiple copies of the same loop data, with
    // smooth crossfades of variable length across their in/out points.

    // discon_seqs links are ordered chronologically from first to last.
    // The first seq contains the earliest samples to be mapped to the decode buffer,
    // the last seq contains the latest samples to be mapped to the decode buffer.

    // This complexity supports a window that can move both forward and backward
    // by an arbitrary number of samples.
    // AND will bound the output to an active loop in both directions.
 
    // Moving the window by 2 samples may be covered by data in existing packets, 
    // however, if the window move crosses a packet boundary, it may require 
    // decoding new packets from source file.
    // Consider that source samples for loop/skip crossfades factor into the above.

    AVFormatContext * fmt_ctx;
    AVCodecContext * codec_ctx;
    AVCodecParserContext * parser_ctx;
    // AVRational timebase;
    bool at_sof;
    bool at_eof;
    
    // Monotonically increasing reference addr...
    // If a decode window contains discontinuities, how does an external system 
    // know where to find specific samples after a window_move operation?
    // This addr mapping serves that purpose.
    // Ex. a TSM pitch node knows it needs to interpolate starting from a previous
    // decode out_buffer index to a new decode out_buf index based on playback rate.
    // The decode window will have moved an arbitrary distance/direction since the last call, 
    // so how do we know where that previous decode out_buffer index 
    // should point after the window has moved?
    zdj_pipeline_addr_t decode_mono_addr; 

    // Example of how the 3 decode spaces move against eachother.
    // Below we have a discon running which is 3 samples long, starting a sample 5.
    // As you scrub backward, the song_decode_mono_addr continues to decrease,
    // the samples in out_buffer are shifted right during the move_window() call,
    // and the song PCM samples are filled in, continuing the discontinuity pattern.
    //    [0] [1] [2] [3] [4] [5] [6] [7] [8] [9] [10][11]  <-decode out_buffer index
    // -> [-4][-3][-2][-1][0] [1] [2] [3] [4] [5] [6] [7]   <-decode monotonic space
    // |  [7] [5] [6] [7] [5] [6] [7] [5] [6] [7] [5] [6]   <-song PCM sample
    // ^ decode_mono_addr
    // 
    // The sample number of the original decoded audio stream where the "head" sits.
    // This maps down thru the discon_seq stack to find the seq with live (non-fade in/out)
    // samples which contains the current win_head_sample, then finds the pcm sample number
    // of that specific sample.
    zdj_pipeline_addr_t head_pcm_addr;

    // Total count of stereo sample frames in song
    double song_pcm_duration; 

    float head_percent; // 0 -> 1, percent of head position in file
    double head_sec;
    int channel_count;

    // We're speaking in sample frames here to keep channel count out of vocabulary.
    
    // Layout example:
    // win_back_sample_count = 2;
    // win_fwd_sample_count = 5;
    // win_sample_count = 7;
    // 
    //          head sample
    //          v
    // [ ] [ ] [ ] [ ] [ ] [ ] [ ]
    //  0   1   2   3   4   5   6 
    // NOTE these are internal buffer indexes, not PCM addresses.
    int win_head_sample;
    size_t win_back_sample_count;
    size_t win_fwd_sample_count;
    size_t win_sample_count;
    int win_fwd_valid_sample;
    int win_back_valid_sample;

    int debug_seq_count;
    int debug_packet_count;
    int debug_accum_tally;
    int debug_accum_src_st_samp;
    int debug_accum_dest_st_samp;
    int debug_accum_dest_en_samp;
} zdj_decode_node_state_t;

zdj_pipeline_node_t * zdj_new_decode_node( 
    zdj_library_song_t * song,
    uint64_t address,
    size_t back_sample_count,
    size_t fwd_sample_count
);

void zdj_decode_node_capture_mono_addr( zdj_pipeline_node_t * node, zdj_pipeline_addr_t * addr );
void zdj_decode_node_xform_tsm_coords_for_captured_mono_addr( 
    zdj_pipeline_node_t * node,
    double * start_coord,
    double * end_coord,
    zdj_pipeline_addr_t * mono_addr
);

int zdj_decode_node_accum_u8( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet );
int zdj_decode_node_accum_s16( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet );
int zdj_decode_node_accum_s32( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet );
int zdj_decode_node_accum_s64( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet );
int zdj_decode_node_accum_flt( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet );
int zdj_decode_node_accum_dbl( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet );
int zdj_decode_node_accum_u8p( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet );
int zdj_decode_node_accum_s16p( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet );
int zdj_decode_node_accum_s32p( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet );
int zdj_decode_node_accum_s64p( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet );
int zdj_decode_node_accum_fltp( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet );
int zdj_decode_node_accum_dblp( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet );


int zdj_decode_node_accum_test_sine( zdj_pipeline_node_t * node, zdj_decode_packet_t * packet );


#endif
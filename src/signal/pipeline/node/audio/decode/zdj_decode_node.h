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

typedef enum {
    ZDJ_DECODE_PACKET_TYPE_UNKNOWN,
    ZDJ_DECODE_PACKET_TYPE_START_INERT,
    ZDJ_DECODE_PACKET_TYPE_END_INERT,
    ZDJ_DECODE_PACKET_TYPE_NORMAL,
    ZDJ_DECODE_PACKET_TYPE_DISCON,
    ZDJ_DECODE_PACKET_TYPE_START_BUMPER,
    ZDJ_DECODE_PACKET_TYPE_END_BUMPER
} zdj_decode_packet_type_t;

typedef enum {
    ZDJ_PACKET_DIRECTION_FWD,
    ZDJ_PACKET_DIRECTION_BACK,
} zdj_decode_node_packet_direction_t;

typedef struct zdj_decode_packet_t {
    zdj_decode_packet_type_t type;

    AVPacket * av_packet; // libav's holder for a chunk of raw, undecoded data
    AVFrame * av_frame; // libav's holder for a chunk of decoded PCM data
    int av_frame_sample_count; // Count of frames decoded into AVFrame

    int64_t packet_pcm_addr; // PCM address of first sample in decoded frame
    int64_t packet_decode_addr; // Decode-space address of first sample in decoded frame

    bool has_eof;
    bool has_sof;
    bool has_decode_error;

    // Discon extents
    // These declare a packet to be the end of a single discon's decoded layer.
    // One packet can be both, if for ex. a short loop inside a single packet.
    bool is_back_extent;
    bool is_fwd_extent;

    // MP3s from FFMpeg have a weird timebase.
    // MP3 timestamps are all in 44100*320 time.
    // When seeking and calculating frame indexes, factor in this 320 value.  
    enum AVCodecID av_codec_id;
    int av_timebase_factor;

    // Addresses are in decode space.
    // See layer anatomy diagram for meaning of core, lead_in, lead_out.
    int64_t core_start_addr;
    int64_t core_end_addr;
    uint64_t core_sample_count; // sample count disregarding lead_in/out
    
    // Be very careful.  Lead-in/lead-out will likely extend beyond bounds of this packet.
    // Be sure to clip lead-in/out xfades to packet bounds.
    int64_t lead_in_start_addr;
    int64_t lead_in_end_addr;

    int64_t lead_out_start_addr;
    int64_t lead_out_end_addr;
    
    struct zdj_decode_packet_t * next;
    struct zdj_decode_packet_t * prev;
} zdj_decode_packet_t;

typedef enum {
    ZDJ_DECODE_DISCON_NONE,
    ZDJ_DECODE_DISCON_SKIP,
    ZDJ_DECODE_DISCON_LOOP,
    ZDJ_DECODE_DISCON_INERT,
    ZDJ_DECODE_DISCON_BUMPER
} zdj_decode_discon_type_t;

typedef struct {
    zdj_decode_discon_type_t type;
    int64_t depart_pcm_addr;
    int64_t depart_decode_addr;
    int64_t dest_pcm_addr;
    // int64_t dest_decode_addr;
} zdj_decode_layer_discon_t;

typedef struct zdj_decode_layer_t {
    // During layer init, we need a mapping from a decode-space addr to a song-space PCM sample.
    // This will build the initial libav frame_seek for the first packet.  All subsequent
    // packets reference that first packet.
    int64_t init_map_pcm;
    int64_t init_map_decode;
    
    zdj_decode_packet_t * first_packet;
    zdj_decode_packet_t * last_packet;

    // Layer currently has a packet containing song's end
    // bool has_eof;
    // bool has_sof;

    // Discon state
    zdj_decode_layer_discon_t fwd_discon;
    zdj_decode_layer_discon_t back_discon;

    // Core samples
    int64_t earliest_core_sample;
    int64_t latest_core_sample;

    // Lead in/out samples
    int64_t earliest_lead_in_sample;
    int64_t latest_lead_out_sample;

    struct zdj_decode_layer_t * next;
    struct zdj_decode_layer_t * prev;
} zdj_decode_layer_t;

// Anatomy of a layer
// 

// Multiple packets in a single layer - long/no discontinuity or beat skip
//      |------------|--------------------------------------------------|------------|
//      |  lead_in   |                    core                          |  lead_out  |
//      |____________|__________________________________________________|____________|
//      |xxxxxxxxx xfade xxxxxxxxx|                        |xxxxxxxxx xfade xxxxxxxxx| 
// |--------||--------||--------||--------||--------||--------||--------||--------||--------|
// | packet || packet || packet || packet || packet || packet || packet || packet || packet |
// |--------||--------||--------||--------||--------||--------||--------||--------||--------|

// Single packet covers a single layer - short discontinuity
//          |----------|------------------------------------|----------|
//          | lead_in  |                core                | lead_out |
//          |__________|____________________________________|__________|
//          |xxxxxxx xfade xxxxxxx|              |xxxxxxx xfade xxxxxxx| 
// |------------------------------------------------------------------------------------|
// |                               packet                                               |
// |------------------------------------------------------------------------------------|

// Special case: first/last layer in a song - no lead-in/lead-out
//      |----------------------//          //----------------|
//      |         core                            core       |
//      |______________________//          //----------------|
//                                            
//      |--------||--------||--//          //-------||------||
//      | packet || packet ||                packet || packe|| 
//      |--------||--------||--//          //-------||------||

// NOTE packet_layer core min width = 2


// Narrow loop case
// =======================================================================//
// decode_space
//        -10       0  3      10        20        30        40        50 53
//   decode window -> [^                                                 ^]
//                    .                                                   .
// layer 0 ...-*----|-lo-|                                                 .
// layer 1     |-li-|----*-cor-*----|-lo-| (28 v)                         .
// layer 2            .        |-li-|----*-cor-*----|-lo-|                .
// layer 3            .                        |-li-|----*-cor-*----|-lo-|.
// layer 4            .                                        |-li-|----*-...

// NOTE how packet layer cores are contiguous, but lead-in/out overlap.

// Broad loop/Beat skip case
// =======================================================================//
// decode_space
//        -10       0  3      10        20        30        40        50 53
//   decode window -> [^                                                 ^]
//                    .                                                   .
// layer 0 --------cor-------------*----|-lo-|                            .
// layer 1            .            |-li-|----*--------------cor-------------- 



// Laminar case
// =======================================================================//
// decode_space
//        -10       0  3      10        20        30        40        50 53
//   decode window -> [^                                                 ^]
//                    .                                                   .
// layer 0 --------------------------cor-----------------------------------


// Mapping from decode space to out_buffer space
// =====================================================//
//  0         10        20        30        40        50
// [ out_buffer index space                            ]
// 
// out_buffer index = decode_space coord - decode_window start



// Song packet types + layout
// =====================================================//
// 
// [>>>>>>][......][~~~][~~~]  ...  [~~~][~~.][......][<<<<<<]
//  bumper  inert         song packets         inert   bumper
// 
// [>>>>>>] <- start_bumper packet
//  - Start bumper signals to the deck controls that the head is before the playable
//    address space of the original audio file.
// [......] <- inert packet
//  - Inert packets are added to the beginning and end of a continuous decode layer
//    to allow scrubbing/scratching across the first/last samples of a song.
//    They are made of silence and behave exactly like song packets.
//    They prevent start/end_bumper behavior from governing the immediate
//    boundaries of a song audio stream.
// [~~~] <- normal packet
//  - Normal packets are... normal.  They just contain decoded audio samples.
// [<<<<<<] <- end bumper packet
//  - End bumper signals to the deck controls that the head is after the playable 
//    address space of the original file.  It triggers slightly different control
//    behavior than start bumpers.
// 
// Start-Bumper behavior
//  - Update slipmat sim
//  - Accept Play/Pause control events
//  - Accept Fwd Jog control events
//  - !loop
//      - Reject Back Jog control events
//  - loop
//      - Accept Back Jog control events (normal discontinuity behavior)
// 
//  Start-Bumper Loop Case
//       .......................................
//       |>                                   <|
// ... >>>>>>>>>>>>>>>][......][~~~][~~~][~~~][~~~][~~~][~~~] ...
//  - Note that loops can extend into the start bumpers.  As long as the loop end 
//    remains inside the playable address space of the song, the start of the loop
//    is allowed to extend infinitely into start bumper space.
//  - Playback controls ignore bumper space behavior while inside 
//    an active loop's bounds.
//  - Disabling a loop while the head is in bumper space re-enables bumper space
//    behavior and leaves the head in place.
// 
// 
// End-Bumper behavior
//  - Update slipmap sim
//  - Accept Back Jog control events
//  - !loop
//      - Reject Play/Pause control events
//      - Reject Fwd Jog control events
//      - Set motor.set_val = 0.0;
//  - loop
//      - Accept Play/Pause control events
//      - Accept Fwd Jog control events (normal discontinuity behavior)
// 
//  End-Bumper Loop Case
//                       ................................
//                       |>                            <|
//  ... [~~~][~~~][~~~][~~~][~~.][......][<<<<<<<<<<<<<<<<<<<< ...
//  - End bumper behaves similary to Start bumper.  Bumper playback control
//    behavior is ignored, but re-activated if loop is disabled.

typedef struct {
    zdj_library_song_t * song;
    zdj_decode_node_status_t status;

    // Indexes of out_buffer define an address space: ZDJ_PIPELINE_ADDRESS_DECODE_WIN.
    // It is the mapping link between ZDJ_PIPELINE_ADDRESS_FILE_PCM and
    // higher-level address spaces, like ZDJ_PIPELINE_ADDRESS_GLOBAL_PCM.
    float * out_buffer;

    // Discon list
    // zdj_decode_discon_t * first_discon;
    // zdj_decode_discon_t * last_discon;
    
    // packet_layer joins together 2 purposes:
    // 1 - hold decoded sample data from libav decode process.
    // 2 - serve as the address data source while copying discontinuities (loops/skips)
    //     from the originial decoded sample data to the out_buffer.
    zdj_decode_layer_t * first_layer;
    zdj_decode_layer_t * last_layer;
    // It's a linked-list of containers for sets of decoded sample packets/frames.
    // Starting at first packet_layer, samples are copied from the layer's packets
    // into the out_buffer.  If we reach the end of the layer's packets before
    // we reach the end of the out_buffer, we advance to the next layer, etc. 

    // In a loop state which is shorter than decode_win, filling the
    // decode buffer requires multiple copies of the same loop data, with
    // smooth fades across their in/out points.

    // packet_layers are ordered chronologically in decode_space time from earliest to latest.
    // The first layer contains the earliest samples to be mapped to the decode buffer,
    // the last layer contains the latest samples to be mapped to the decode buffer.

    // This complexity supports a window that can move both forward and backward
    // by an arbitrary number of samples.
    // AND will support forward and backward playback of loops of arbitrary length.
 
    // FFMpeg/libav contexts
    AVFormatContext * fmt_ctx;
    AVCodecContext * codec_ctx;
    AVCodecParserContext * parser_ctx;

    
    // The address in decode_space under the node's 'head'.
    int64_t head_decode_addr; 
    int64_t head_win_start;
    int64_t head_win_end;

    
    // The sample number of the original decoded audio stream where the 'head' sits.
    int64_t head_pcm_addr;
    // This maps down thru the packet_layer stack to find the layer whose 'core' will
    // render the PCM sample under the node's 'head'.
    // Remember that when discontinuites are present, source pcm addresses of the samples
    // in the node's out_buffer will not increment monotonically.
    // You can't assume that 5 samples forward in the out_buffer is 5 samples forward in 
    // source pcm space.

    // Accumulator functions vary by codecID
    // Link the accumulator function during init.
    int ( *accum )( zdj_pipeline_node_t *, struct zdj_decode_packet_t * );

    // Add packet behavior varies by encoding type.  
    // Ex. MP3s only read 47 samples after a seek.
    // Check the song codecID and assign these at init.
    void ( *layer_can_add_packet_before ) (
        zdj_pipeline_node_t *, 
        zdj_decode_layer_t *,
        int64_t address
    );
    void ( *add_packet_before_layer ) (
        zdj_pipeline_node_t *, 
        zdj_decode_layer_t * 
    );
    void ( *layer_can_add_packet_after ) (
        zdj_pipeline_node_t *, 
        zdj_decode_layer_t *,
        int64_t address
    );
    void ( *add_packet_after_layer ) (
        zdj_pipeline_node_t *, 
        zdj_decode_layer_t * 
    );
    void ( *add_packet_to_empty_layer ) (
        zdj_pipeline_node_t *, 
        zdj_decode_layer_t * 
    );

    // Core samples
    int64_t earliest_core_sample;
    int64_t latest_core_sample;

    // Lead in/out samples
    int64_t earliest_lead_in_sample;
    int64_t latest_lead_out_sample;


    // Total count of stereo sample frames in song
    double song_pcm_duration; 

    float head_percent; // 0 -> 1, percent of head position in file
    double head_sec; // PCM head position in seconds
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
    // NOTE these are in decode_buf space, not decode, or source_pcm.
    // They only refer to indexes in the node's out_buffer
    int win_head_sample;
    size_t win_back_sample_count;
    size_t win_fwd_sample_count;
    size_t win_sample_count;
    int win_fwd_valid_sample;
    int win_back_valid_sample;

    int debug_layer_count;
    int debug_packet_count;
    int debug_accum_tally;
    int debug_accum_src_st_samp;
    int debug_accum_dest_st_samp;
    int debug_accum_dest_en_samp;
} zdj_decode_node_state_t;

// Node
zdj_pipeline_node_t * zdj_new_decode_node( 
    zdj_library_song_t * song,
    uint64_t address,
    size_t back_sample_count,
    size_t fwd_sample_count
);

// Layer
zdj_decode_layer_t * zdj_new_decode_layer( int64_t pcm_address, int64_t decode_address );
zdj_decode_layer_t * zdj_decode_add_loop_layer( 
    zdj_pipeline_node_t * node, 
    int64_t layer_start_decode_addr, 
    int64_t layer_start_pcm_addr, 
    int64_t loop_start_decode_addr, 
    int64_t loop_start_pcm_addr, 
    int64_t loop_len
);
zdj_decode_layer_t * zdj_decode_add_skip_layer( 
    zdj_pipeline_node_t * node, 
    int64_t start_decode_addr, 
    int64_t depart_pcm_addr, 
    int64_t dest_pcm_addr 
);
void zdj_decode_fill_layer( 
    zdj_pipeline_node_t * node,
    zdj_decode_layer_t * layer
);
void zdj_decode_truncate_layer( 
    zdj_decode_layer_t * layer, 
    int64_t clip_decode_addr,
    zdj_decode_discon_type_t discon_type
);
zdj_decode_layer_t * zdj_decode_get_layer_under_head( zdj_pipeline_node_t * node );
void zdj_decode_layer_reset_discon( zdj_decode_layer_t * layer, int64_t end_pcm_addr );
int64_t zdj_decode_discon_loop_length( zdj_decode_layer_discon_t * discon );
int64_t zdj_decode_discon_skip_length( zdj_decode_layer_discon_t * discon );
void zdj_decode_deinit_layer( zdj_decode_layer_t * layer );

// Packet
int zdj_decode_packet( 
    zdj_pipeline_node_t * node, 
    zdj_decode_packet_t * packet,
    AVPacket * av_packet, 
    AVFrame * av_frame
);
zdj_decode_packet_t * zdj_decode_get_packet_under_head( 
    zdj_pipeline_node_t * node, 
    zdj_decode_layer_t * layer 
);
bool zdj_decode_packet_contains_decode_addr( zdj_decode_packet_t * packet, int64_t decode_addr );
void zdj_decode_deinit_packet( zdj_decode_packet_t * packet );
void zdj_decode_make_admin_packet( 
    zdj_decode_layer_t * layer,
    zdj_decode_packet_t * packet, 
    zdj_decode_packet_t * linked_packet, 
    zdj_decode_packet_type_t type,
    zdj_decode_node_packet_direction_t direction,
    int len 
);
void zdj_decode_flush_packets( zdj_pipeline_node_t * node );

// Addresses
bool zdj_decode_address_intersects_packet( 
    int64_t address, 
    zdj_decode_packet_t * packet
);

bool zdj_pcm_address_intersects_packet( 
    int64_t address, 
    zdj_decode_packet_t * packet
);
int64_t zdj_decode_get_pcm_addr_for_decode_addr( zdj_pipeline_node_t * node, int64_t decode_address );

void zdj_decode_add_packet_before_pcm_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer );
void zdj_decode_add_packet_to_empty_pcm_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer );
void zdj_decode_add_packet_after_pcm_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer );

void zdj_decode_add_packet_before_mp3_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer );
void zdj_decode_add_packet_to_empty_mp3_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer );
void zdj_decode_add_packet_after_mp3_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer );

// // void zdj_decode_flac_layer_can_add_packet_before( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer, int64_t address );
// void zdj_decode_add_packet_before_flac_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer );
// void zdj_decode_add_packet_to_empty_flac_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer );
// // void zdj_decode_flac_layer_can_add_packet_after( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer, int64_t address );
// void zdj_decode_add_packet_after_flac_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer );

// // void zdj_decode_aac_layer_can_add_packet_before( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer, int64_t address );
// void zdj_decode_add_packet_before_aac_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer );
// void zdj_decode_add_packet_to_empty_aac_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer );
// // void zdj_decode_aac_layer_can_add_packet_after( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer, int64_t address );
// void zdj_decode_add_packet_after_aac_layer( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer );


// Accumulators
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
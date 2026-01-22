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

#include <stdint.h>

#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavutil/samplefmt.h>
#include <libavcodec/codec_id.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>

typedef enum {
    ZDJ_ADDR_COORD_ORIGIN,
    ZDJ_ADDR_COORD_TRANSPORT,
    ZDJ_ADDR_COORD_OUT_BUF,
} zdj_decode_addr_coord_t;

typedef struct zdj_decode_addr_t {
    int64_t transport_i; // Addresses in transport stream sample coords
    double transport_d;
    double transport_bg; // Address in transport stream beatgrid coords

    int64_t origin_i; // Addresses in original PCM stream sample coords
    double origin_d;
    double origin_bg; // Address in original beatgrid coords
    bool has_valid_origin;
    
    int buf_i; // Addresses in soundcard buffer sample coords
    double buf_d;
    bool has_valid_buf;

    void ( *copy )( struct zdj_decode_addr_t *, struct zdj_decode_addr_t * );
    bool ( *equal )( struct zdj_decode_addr_t *, struct zdj_decode_addr_t *, zdj_decode_addr_coord_t );
    bool ( *less_than )( struct zdj_decode_addr_t *, struct zdj_decode_addr_t *, zdj_decode_addr_coord_t );
    bool ( *greater_than )( struct zdj_decode_addr_t *, struct zdj_decode_addr_t *, zdj_decode_addr_coord_t );
    void ( *update_buf_i_for_node_head )( struct zdj_decode_addr_t *, zdj_pipeline_node_t * );
} zdj_decode_addr_t;

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
    ZDJ_DECODE_JUSTIFY_LEFT,
    ZDJ_DECODE_JUSTIFY_RIGHT
} zdj_decode_packet_justify_t;

typedef enum {
    ZDJ_DECODE_DIR_FWD,
    ZDJ_DECODE_DIR_BACK,
} zdj_decode_dir_t;

typedef struct zdj_decode_packet_t {
    int64_t id;
    zdj_decode_packet_type_t type;
    zdj_decode_packet_justify_t justify;

    AVPacket * av_packet; // libav's holder for a chunk of raw, undecoded data
    AVFrame * av_frame; // libav's holder for a chunk of decoded PCM data

    bool has_decode_error;

    // Discon extents
    // These declare a packet to be the end of a single discon's decoded layer.
    // One packet can be both, if for ex. a short loop inside a single packet.
    bool is_back_extent;
    bool is_fwd_extent;
    bool is_eof;
    bool is_sof;

    zdj_decode_addr_t start_addr;
    zdj_decode_addr_t end_addr;
    int64_t sample_count;

    bool ( *intersects_out_buf )( struct zdj_decode_packet_t *, zdj_pipeline_node_t * );
    void ( *render_to_out_buf )( struct zdj_decode_packet_t *, void *, zdj_pipeline_node_t * );
    double ( *xfade_coeff_for_transport_d_coord )( struct zdj_decode_packet_t *, void *, double );
    void ( *deinit )( struct zdj_decode_packet_t * );

    struct zdj_decode_packet_t * next;
    struct zdj_decode_packet_t * prev;
} zdj_decode_packet_t;

typedef enum {
    ZDJ_DECODE_DISCON_NONE,
    ZDJ_DECODE_DISCON_SKIP,
    ZDJ_DECODE_DISCON_LOOP
} zdj_decode_discon_type_t;

// typedef struct {
//     zdj_decode_discon_type_t type;
//     // int64_t depart_pcm_addr;
//     // int64_t depart_decode_addr;
//     // int64_t dest_pcm_addr;
//     // int64_t dest_decode_addr;
//     // zdj_decode_addr_t depart_addr;
//     // zdj_decode_addr_t dest_addr;
//     int64_t length;
// } zdj_decode_layer_discon_t;

typedef struct zdj_decode_layer_t {
    // The address given when initializing any type of layer.
    // Serves as layer's reference mapping between origin <-> transport coords.
    // Also used as a starting origin coord for a continuous layer.
    zdj_decode_addr_t init_addr;

    // If a layer is continuous, core+lead transport coords will all be 0, and
    // origin coords will all be @ init_addr.origin_x.
    // Discontinuous layers have start/end addrs set to loop/skip coords.
    zdj_decode_addr_t core_start;
    zdj_decode_addr_t core_end;
    int64_t core_sample_count;

    zdj_decode_addr_t lead_in_start;
    zdj_decode_addr_t lead_in_end;
    int64_t lead_in_sample_count;

    zdj_decode_addr_t lead_out_start;
    zdj_decode_addr_t lead_out_end;
    int64_t lead_out_sample_count;

    // Layer address API
    bool ( *contains_core_addr )( struct zdj_decode_layer_t *, zdj_decode_addr_t *, zdj_decode_addr_coord_t );
    bool ( *contains_addr )( struct zdj_decode_layer_t *, zdj_decode_addr_t *, zdj_decode_addr_coord_t );
    void ( *update_buf_coords_for_head )( struct zdj_decode_layer_t *, zdj_pipeline_node_t * );
    double ( *origin_d_coord_for_transport_d_coord )( struct zdj_decode_layer_t *, double );

    // Discon state
    void * _loop_state;
    void * _skip_state;
    zdj_decode_discon_type_t fwd_discon_type;
    zdj_decode_discon_type_t back_discon_type;
    void ( *truncate )( zdj_pipeline_node_t *, struct zdj_decode_layer_t *, void *, double, zdj_decode_discon_type_t, void * );
    void ( *untruncate )( zdj_pipeline_node_t *, struct zdj_decode_layer_t * );

    // Packet decoding
    void ( *fill )( struct zdj_decode_layer_t *, zdj_pipeline_node_t * );
    bool ( *can_fill )( struct zdj_decode_layer_t *, zdj_pipeline_node_t *, zdj_decode_dir_t );
    int64_t ( *fill_packet_count )( struct zdj_decode_layer_t *, zdj_pipeline_node_t *, zdj_decode_dir_t );
    int64_t ( *seek_target )( struct zdj_decode_layer_t *, zdj_pipeline_node_t *, int, zdj_decode_dir_t );
    void ( *seek_and_decode )( 
        struct zdj_decode_layer_t *, 
        zdj_pipeline_node_t *, 
        int64_t, 
        int, 
        zdj_decode_dir_t, 
        zdj_decode_packet_justify_t,
        int, 
        bool 
    );

    // Packet stack management
    bool ( *is_empty )( struct zdj_decode_layer_t* );
    void ( *prepend_packet )( struct zdj_decode_layer_t *, zdj_decode_packet_t * );
    void ( *append_packet )( struct zdj_decode_layer_t *, zdj_decode_packet_t * );
    void ( *insert_packet_after )( struct zdj_decode_layer_t *, zdj_decode_packet_t *, zdj_decode_packet_t * );
    void ( *insert_packet_before )( struct zdj_decode_layer_t *, zdj_decode_packet_t *, zdj_decode_packet_t * );
    bool ( *can_remove_packet )( struct zdj_decode_layer_t *, zdj_pipeline_node_t *, zdj_decode_packet_t * );
    void ( *remove_packet )( struct zdj_decode_layer_t *, zdj_decode_packet_t * );

    // Packet address API
    void ( *get_first_packet_addr )( struct zdj_decode_layer_t *, zdj_decode_addr_t * );
    void ( *get_last_packet_addr )( struct zdj_decode_layer_t *, zdj_decode_addr_t * );
    zdj_decode_packet_t * ( *get_packet_containing_addr )( struct zdj_decode_layer_t *, zdj_decode_addr_t *, zdj_decode_addr_coord_t );

    // Accumulator
    enum AVSampleFormat accum_fmt;
    void ( *accum ) ( struct zdj_decode_layer_t *, zdj_pipeline_node_t * );

    // Lifecycle
    void ( *deinit )( struct zdj_decode_layer_t * );

    // Child linkage
    zdj_decode_packet_t * first_packet;
    zdj_decode_packet_t * last_packet;
    
    // Sibling Linkage 
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
    // Song details
    zdj_library_song_t * song;
    double song_pcm_duration; // Empirical value - calculated during library import
    int channel_count;
    int64_t layer_fade_len;

    zdj_decode_node_status_t status;

    // FFMpeg/libav contexts
    AVFormatContext * fmt_ctx;
    AVCodecContext * codec_ctx;
    AVCodecParserContext * parser_ctx;

    // Indexes of out_buffer define an address space: ZDJ_PIPELINE_ADDRESS_DECODE_WIN.
    // It is the mapping link between ZDJ_PIPELINE_ADDRESS_FILE_PCM and
    // higher-level address spaces, like ZDJ_PIPELINE_ADDRESS_GLOBAL_PCM.
    float * out_buffer;
    void ( *clear_out_buffer )( zdj_pipeline_node_t * ); 

    // enum AVSampleFormat accum_fmt;
    // void ( *accum ) ( zdj_pipeline_node_t *, zdj_decode_layer_t * );

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
    void ( *prepend_layer ) ( zdj_pipeline_node_t *, zdj_decode_layer_t * );
    void ( *append_layer ) ( zdj_pipeline_node_t *, zdj_decode_layer_t * );
    bool ( *can_remove_layer )( zdj_decode_layer_t * );
    void ( *remove_layer )( zdj_pipeline_node_t *, zdj_decode_layer_t * );


    //////////////////////////
    // External Address API //
    //////////////////////////

    zdj_decode_addr_t head;

    // Calculate and copy the window start addr into the given addr
    void ( *get_win_start_addr )( zdj_pipeline_node_t *, zdj_decode_addr_t * );
    // Calculate and copy the window end addr into the given addr
    void ( *get_win_end_addr )( zdj_pipeline_node_t *, zdj_decode_addr_t * );
    bool ( *win_contains_addr )( zdj_pipeline_node_t *, zdj_decode_addr_t *, zdj_decode_addr_coord_t);
    
    float ( *get_head_percent )( zdj_pipeline_node_t * ); // 0 -> 1, percent of head position in file
    double ( *get_head_sec )( zdj_pipeline_node_t * ); // PCM head position in seconds
    double ( *get_quantized_head_origin_bg )( zdj_pipeline_node_t *, double ); // Head addr. quantized to nearest beatgrid based on quantize value
    
    // Update an addr's coords based on a given coord
    void ( *set_addr_transport_i_coord )( zdj_pipeline_node_t *, zdj_decode_addr_t *, int64_t );
    void ( *set_addr_transport_d_coord )( zdj_pipeline_node_t *, zdj_decode_addr_t *, double );
    void ( *set_addr_transport_bg_coord )( zdj_pipeline_node_t *, zdj_decode_addr_t *, double );

    // Move all of an addr's coords by a given offset
    void ( *offset_addr_by_transport_i_coord )( zdj_pipeline_node_t *, zdj_decode_addr_t *, int64_t );
    void ( *offset_addr_by_transport_d_coord )( zdj_pipeline_node_t *, zdj_decode_addr_t *, double );
    void ( *offset_addr_by_transport_bg_coord )( zdj_pipeline_node_t *, zdj_decode_addr_t *, double );

    void ( *addr_for_transport_i_coord )( zdj_pipeline_node_t *, zdj_decode_addr_t *, int64_t );
    void ( *addr_for_transport_d_coord )( zdj_pipeline_node_t *, zdj_decode_addr_t *, double );
    void ( *addr_for_transport_bg_coord )( zdj_pipeline_node_t *, zdj_decode_addr_t *, double );

    void ( *addr_for_origin_i_coord_in_layer )( 
        zdj_pipeline_node_t *, zdj_decode_layer_t *, zdj_decode_addr_t *, int64_t 
    );
    void ( *addr_for_origin_d_coord_in_layer )( 
        zdj_pipeline_node_t *, zdj_decode_layer_t *, zdj_decode_addr_t *, double 
    );
    void ( *addr_for_origin_bg_coord_in_layer )( 
        zdj_pipeline_node_t *, zdj_decode_layer_t *, zdj_decode_addr_t *, double 
    );

    // For beat skip discon - alter the origin coords only
    void ( *offset_addr_origin_by_skip_bg )( zdj_pipeline_node_t *, zdj_decode_addr_t *, double );


    //////////////////////////
    // Internal Address API //
    //////////////////////////

    void ( *get_earliest_core_addr )( zdj_pipeline_node_t *, zdj_decode_addr_t * );
    void ( *get_latest_core_addr )( zdj_pipeline_node_t *, zdj_decode_addr_t * );
    void ( *get_earliest_lead_in_addr )( zdj_pipeline_node_t *, zdj_decode_addr_t * );
    void ( *get_latest_lead_out_addr )( zdj_pipeline_node_t *, zdj_decode_addr_t * );
    bool requires_garbage;
    int av_timebase_factor;
    // Best guess at number of samples per packet.
    // This is constant on MP3/WAV, but variable on FLAC/AAC.
    double estimated_packet_sample_count;

    zdj_decode_layer_t * ( *get_layer_containing_addr )( 
        zdj_pipeline_node_t *, zdj_decode_addr_t *, zdj_decode_addr_coord_t 
    );
    double ( *get_beatgrid_coord_for_d_coord )( zdj_pipeline_node_t *, double );
    double ( *get_beatgrid_dist_between_addrs )( 
        zdj_pipeline_node_t *, zdj_decode_addr_t *, zdj_decode_addr_t * 
    );
    double ( *get_d_offset_for_beatgrid_dist )( zdj_pipeline_node_t *, double );
    
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
    size_t win_back_sample_count;
    size_t win_fwd_sample_count;
    size_t win_sample_count;

    ///////////////////////
    // Discontinuity API //
    ///////////////////////

    bool discon_is_active;
    void ( *enable_loop_discon ) ( zdj_pipeline_node_t *, void * );
    void ( *release_loop_discon ) ( zdj_pipeline_node_t *, void * );
    void ( *move_loop_discon ) ( zdj_pipeline_node_t *, void * );
    bool ( *move_loop_discon_will_move_head ) ( zdj_pipeline_node_t *, void * );
    void ( *resize_loop_discon ) ( zdj_pipeline_node_t *, void * );
    void ( *refresh_loop_discon_layers ) ( zdj_pipeline_node_t *, void * );
    void ( *add_skip_discon ) ( zdj_pipeline_node_t *, void * );
    void ( *add_hyperscrub_discon ) ( zdj_pipeline_node_t *, void * );
} zdj_decode_node_state_t;

// Node
zdj_pipeline_node_t * zdj_new_decode_node( 
    zdj_library_song_t * song,
    int64_t address,
    size_t back_sample_count,
    size_t fwd_sample_count
);

// Layer
void zdj_decode_layer_init_addr_api( zdj_decode_layer_t * layer );
void zdj_decode_layer_init_fill_api( zdj_decode_layer_t * layer );
void zdj_decode_layer_init_packet_api( zdj_decode_layer_t * layer );
zdj_decode_layer_t * zdj_new_decode_continuous_layer( 
    zdj_pipeline_node_t * node, 
    zdj_decode_addr_t * init_addr 
);
zdj_decode_layer_t * zdj_new_decode_loop_layer( 
    zdj_pipeline_node_t * node, 
    zdj_decode_addr_t * loop_start_addr, 
    void * _loop_state
);
zdj_decode_layer_t * zdj_new_decode_skip_layer( 
    zdj_pipeline_node_t * node,  
    zdj_decode_addr_t * depart_addr
);
zdj_decode_layer_t * zdj_new_decode_hyperscrub_layer( 
    zdj_pipeline_node_t * node,  
    zdj_decode_addr_t * depart_addr,
    zdj_decode_dir_t dir
);


// Discon
void zdj_decode_init_node_discon_api( zdj_pipeline_node_t * node );

// Packet
zdj_decode_packet_t * zdj_decode_packet( 
    zdj_pipeline_node_t * node,
    zdj_decode_layer_t * layer,
    AVFormatContext * fmt_ctx,
    AVCodecContext * codec_ctx,
    int av_timebase_factor,
    zdj_decode_packet_justify_t justify
);
void zdj_decode_garbage_packet( 
    zdj_pipeline_node_t * node,
    zdj_decode_layer_t * layer,
    AVFormatContext * fmt_ctx,
    AVCodecContext * codec_ctx
);
void zdj_decode_flush_packets( zdj_pipeline_node_t * node );

// Addresses
void zdj_decode_init_node_addr_api( zdj_pipeline_node_t * node );
void zdj_decode_init_addr( zdj_decode_addr_t * addr );

// Accumulators
// void zdj_decode_init_accum( zdj_pipeline_node_t * node, zdj_decode_layer_t * layer );

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
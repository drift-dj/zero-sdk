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

#ifndef ZDJ_DECODE_RAW_NODE_H
#define ZDJ_DECODE_RAW_NODE_H

#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavcodec/codec_id.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>

typedef struct zdj_decode_raw_data_frame_t {
    uint32_t song_samp_addr;
    void * buffer;
    size_t buffer_len;
    size_t buffer_size;
    uint32_t buf_start_index;
    uint32_t buf_end_index;
    bool discard;
    bool is_decoded;
    struct zdj_decode_raw_data_frame_t * next;
    struct zdj_decode_raw_data_frame_t * prev;
} zdj_decode_raw_data_frame_t;

typedef struct zdj_decode_raw_node_state_t {
    zdj_library_song_t * song;
    zdj_decode_raw_data_frame_t * frames;
    zdj_decode_raw_data_frame_t * first_frame;
    zdj_decode_raw_data_frame_t * last_frame;
    zdj_pipeline_node_t * file_node;
    bool req_active;
    uint32_t req_song_samp_addr;
    size_t req_samp_count;
    size_t req_samp_size;
    int req_ch_count;
    void * req_dest_addr;
    AVCodec * codec;
    AVCodecParserContext * parser;
    AVCodecContext * codec_context;
    zdj_error_type_t ( *decode )( zdj_decode_raw_data_frame_t* );
    struct zdj_decode_raw_node_state_t * next;
    struct zdj_decode_raw_node_state_t * prev;
} zdj_decode_raw_node_state_t;

zdj_pipeline_node_t * zdj_new_decode_raw_node( 
    zdj_library_song_t * song, 
    uint32_t address, 
    size_t frame_count 
);

zdj_error_type_t zdj_decode_raw_node_request_samples( 
    zdj_pipeline_node_t * node,
    uint32_t req_song_samp_addr,
    size_t req_samp_count,
    size_t req_samp_size,
    int req_ch_count,
    void * req_dest_addr
);

#endif
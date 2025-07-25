// // Copyright (c) 2025 Drift DJ Industries

// // Permission is hereby granted, free of charge, to any person obtaining a copy
// // of this software and associated documentation files (the "Software"), to deal
// // in the Software without restriction, including without limitation the rights
// // to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// // copies of the Software, and to permit persons to whom the Software is
// // furnished to do so, subject to the following conditions:

// // The above copyright notice and this permission notice shall be included in all
// // copies or substantial portions of the Software.

// // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// // IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// // FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// // AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// // LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// // OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// // SOFTWARE.

// #ifndef ZDJ_PLAYBACK_DECODE_NODE_H
// #define ZDJ_PLAYBACK_DECODE_NODE_H

// #include <zerodj/library/zdj_library.h>
// #include <zerodj/signal/pipeline/zdj_pipeline.h>
// #include <zerodj/signal/pipeline/node/audio/zdj_audio_node.h>

// #define DECOD_GRAPH_LOOKAHEAD 32
// #define DECOD_GRAPH_CULL_BOUNDARY 20
// #define DECOD_NODE_FRAMES   1024 // Number of sample *frames* in each decode block
// #define DECOD_NODE_LEN    DECOD_NODE_FRAMES * 2 // Number of *samples* in each decode block frame

// #define DECOD_BLK_FRMS   1024 // Number of sample *frames* in each decode block
// #define DECOD_BLK_LEN    DECOD_BLK_FRMS * 2 // Number of *samples* in each decode block

// void init_decod_graph( zdj_playback_decode_node_state_t * ch_state );
// void update_decod_graph( zdj_playback_decode_node_state_t * ch_state );
// void get_decod_sample_at_offset( 
//     zdj_playback_decode_node_state_t * ch_state, 
//     decod_sample_float_t * frame, 
//     double offset,
//     bool interpolate 
// );
// int get_current_decod_sample( zdj_playback_decode_node_state_t * ch_state );
// decod_sample_float_t * new_decod_sample_float( void );
// void free_decod_sample_float( decod_sample_float_t * frame );
// void reset_decod_to_sample( zdj_playback_decode_node_state_t * ch_state, int frame );


// void init_mp3_decod( zdj_playback_decode_node_state_t * state );
// void mp3_jump_to_hotcue( zdj_playback_decode_node_state_t * state );
// void mp3_reset_decod_to_sample( zdj_playback_decode_node_state_t * state, int sample );
// void update_mp3_decod( zdj_playback_decode_node_state_t * state );
// decod_frame_t * create_mp3_node_for_source_sample( zdj_playback_decode_node_state_t * state, int datum_sample );
// void free_mp3_node( zdj_playback_decode_node_state_t * state, decod_frame_t * node );
// void get_mp3_decod_sample_at_offset( 
//     zdj_playback_decode_node_state_t * state, 
//     decod_sample_float_t * sample, 
//     double offset,
//     bool interpolate
// );
// void offset_mp3_decod_address( zdj_playback_decode_node_state_t * state, double offset );
// int get_mp3_decod_current_address( zdj_playback_decode_node_state_t * state );

// #endif
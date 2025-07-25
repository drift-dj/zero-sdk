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

// #ifndef ZDJ_AUDIO_NODE_H
// #define ZDJ_AUDIO_NODE_H

// #include <mpg123.h>

// #include <zerodj/library/zdj_library.h>
// #include <zerodj/signal/pipeline/zdj_pipeline.h>
// #include <zerodj/signal/pipeline/node/audio/zdj_audio_node.h>

// typedef enum {
//     ZDJ_FAST_DECODE_INIT,
//     ZDJ_FAST_DECODE_READY,
//     ZDJ_FAST_DECODE_RUNNING,
//     ZDJ_FAST_DECODE_DONE,
//     ZDJ_FAST_DECODE_IDLE
// } zdj_fast_decode_node_status_t;

// typedef struct {
//     zdj_library_song_t * song_dto;
//     zdj_fast_decode_node_status_t status;
//     sem_t * wait_sem;
//     sem_t * ready_sem;
//     pthread_t thread;
// } zdj_fast_decode_node_state_t;


// // Indices here are defined in samples, NOT bytes.
// typedef struct {
//     int id;
//     int res_start;
//     int dcod_start;
//     bool decoded;
//     bool in_leader;
//     bool out_leader;
// } fs_decod_buf_page;

// typedef enum {
//     DECODE_NODE_TYPE_GARBAGE,
//     DECODE_NODE_TYPE_LEADER,
//     DECODE_NODE_TYPE_ERROR,
//     DECODE_NODE_TYPE_NORMAL,
//     DECODE_NODE_TYPE_LOOP_START,
//     DECODE_NODE_TYPE_LOOP_END,
//     DECODE_NODE_TYPE_BEAT_SKIP,
// } decod_frame_type_t;

// typedef struct {
//     int index; // the decode node within sample-space (start sample/decode buf samples)
//     decod_frame_type_t type;
//     int source_datum_sample; // data[i] == source_pcm_data[source_datum_sample + i]
//     int jump_out_index; // sample in buffer where we jump to the other end of the discontinuity
//     int jump_in_index; // sample in buffer where we arrive when jumping from a discontinuity start
//     int16_t * data;
//     bool decoded;
//     void * next;
//     void * prev;
// } decod_frame_t;

// typedef struct {
//     int channel_count;
//     float * pcm_values;
// } decod_sample_float_t;

// typedef struct {
//     int channel_count;
//     int * pcm_values;
// } decod_sample_int;

// typedef struct {
//     decod_frame_t * node;
//     int int_index; // int value suitable for array addressing
//     float float_index; // precise value which can handle non-integer address advances
// } decod_address_t;

// typedef struct {
//     int start_sample;
//     int end_sample;
//     int start_bar;
//     int start_beat;
//     int start_milibeat;
//     int end_bar;
//     int end_beat;
//     int end_milibeat;
//     bool quantize;
// } playback_loop_t;

// typedef struct {
//     zdj_library_song_t * song_dto;
//     zdj_fast_decode_node_status_t status;
//     sem_t * wait_sem;
//     sem_t * ready_sem;
//     pthread_t thread;

//     playback_loop_t * loop;

//     decod_address_t * decod_address;
//     decod_frame_t * decod_graph_start_frame;
//     decod_frame_t * decod_graph_end_frame;
//     mpg123_handle * m_play;
//     int mpg123_encosize;

// } zdj_playback_decode_node_state_t;

// zdj_pipeline_node_t * zdj_new_fast_decode_node( zdj_library_song_t * song );
// zdj_pipeline_node_t * zdj_new_playback_decode_node( zdj_library_song_t * song );

// zdj_pipeline_node_t * zdj_new_bpm_detect_node( void );

// #endif
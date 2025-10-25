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

#ifndef ZDJ_AUDIO_RECORD_NODE_H
#define ZDJ_AUDIO_RECORD_NODE_H

#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavcodec/codec_id.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>

#define ZDJ_RECORDING_DIR "/media/internal/recordings"
#define ZDJ_RECORDING_TEMP_DIR "/media/internal/recordings/tmp"

typedef struct {
	char chunk_id[4];
	uint32_t chunk_size;
	char format[4];
	char subchunk_1_id[4];
	uint32_t subchunk_1_size;
	uint16_t audio_format;
	uint16_t num_channels;
	uint32_t sample_rate;
	uint32_t byte_rate;
	uint16_t block_align;
	uint16_t bits_per_sample;
	char subchunk_2_id[4];
	uint32_t subchunk_2_size;
} record_wav_header_t;

typedef enum {
    ZDJ_AUDIO_RECORD_INIT,
    ZDJ_AUDIO_RECORD_BEGIN,
    ZDJ_AUDIO_RECORD_ACTIVE,
    ZDJ_AUDIO_RECORD_INACTIVE,
    ZDJ_AUDIO_RECORD_FINISH,
    ZDJ_AUDIO_RECORD_PROCESSING,
    ZDJ_AUDIO_RECORD_DONE
} zdj_audio_record_node_status_t;

typedef struct {
    zdj_audio_record_node_status_t status;
    zdj_soundcard_node_t * soundcard_node;
    char tmp_filepath[ 256 ];
    char filename[ 256 ];
    FILE * tmp_fp;

} zdj_audio_record_node_state_t;

zdj_pipeline_node_t * zdj_new_audio_record_node( zdj_soundcard_node_t * soundcard_node );

void zdj_new_audio_record_capture( zdj_pipeline_node_t * record_node );
void zdj_finish_audio_record_capture( zdj_pipeline_node_t * record_node, bool save );


#endif
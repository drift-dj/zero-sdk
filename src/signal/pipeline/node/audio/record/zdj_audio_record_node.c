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
#include <zerodj/signal/pipeline/node/audio/buffer/zdj_audio_buffer_node.h>
#include <zerodj/signal/pipeline/node/audio/record/zdj_audio_record_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/system/fs/zdj_fs.h>

static void _update_wait( zdj_pipeline_node_t * node );
static void _deinit_state( zdj_pipeline_node_t * node );

static void * _zdj_audio_record_processing_thread_main( void * arg );

zdj_pipeline_node_t * zdj_new_audio_record_node( zdj_soundcard_node_t * soundcard_node ) {
    printf( "zdj_new_audio_record_node\n" );
    // Make node
    zdj_pipeline_node_t * node = zdj_new_pipeline_node( );
    node->deinit_state = &_deinit_state;
    node->update_wait = &_update_wait;

    // Add state
    zdj_audio_record_node_state_t * state = calloc( 1, sizeof( zdj_audio_record_node_state_t ) );
    node->state = state;
    state->soundcard_node = soundcard_node;
   
    return node;
}

// Note that we're on the audio fast update cycle here.
// Don't spend too much time on anything in this func.
static void _update_wait( zdj_pipeline_node_t * node ) {
    zdj_audio_record_node_state_t * record_state = (zdj_audio_record_node_state_t*)node->state;
    
    if ( record_state->status == ZDJ_AUDIO_RECORD_ACTIVE ) {
        // printf( "ZDJ_AUDIO_RECORD_ACTIVE\n" );
        // Write samples in soundcard_node (recording buffer) to tmp data file
        zdj_pipeline_node_t * pipe = record_state->soundcard_node->data_pipe;
        float * buf = pipe->get_data( pipe );
        zdj_audio_buffer_node_state_t * pipe_state = (zdj_audio_buffer_node_state_t *)pipe->state;
        
        int bw = 0;
        int16_t samp;
        for( int i=0; i<pipe_state->sample_count; i++ ) {
            samp = (int16_t)(buf[ i*pipe_state->stereo_mode ] * (float)INT16_MAX ); 
            bw += fwrite( buf, sizeof( int16_t ), 1, record_state->tmp_fp );
            if( pipe_state->stereo_mode > 1 ) {
                samp = (int16_t)(buf[ (i*pipe_state->stereo_mode)+1 ] * (float)INT16_MAX ); 
                bw += fwrite( buf, sizeof( int16_t ), 1, record_state->tmp_fp );
            }
        }
        
        // printf( "Wrote %d samples\n", bw );
    
    } else if ( record_state->status == ZDJ_AUDIO_RECORD_FINISH ) {
        // printf( "ZDJ_AUDIO_RECORD_FINISH\n" );
        // Close tmp data file and prep for handoff to the post-processing thread.
        fclose( record_state->tmp_fp );
        record_state->status = ZDJ_AUDIO_RECORD_PROCESSING;

    } else if ( record_state->status == ZDJ_AUDIO_RECORD_BEGIN ) {
        // printf( "ZDJ_AUDIO_RECORD_BEGIN\n" );
        zdj_fs_mkdir_p( ZDJ_RECORDING_TEMP_DIR );
        // Initialize the tmp wav file.
        char uuid[ 256 ];
        zdj_library_put_uuid( uuid );
        snprintf( record_state->tmp_filepath, sizeof( record_state->tmp_filepath ), "%s/%s.wav", 
            ZDJ_RECORDING_TEMP_DIR, uuid 
        );
        record_state->tmp_fp = fopen( record_state->tmp_filepath, "w" );
        
        record_wav_header_t * hdr = calloc( 1, sizeof( record_wav_header_t ) );
        hdr->chunk_id[ 0 ] = 'R';
        hdr->chunk_id[ 1 ] = 'I';
        hdr->chunk_id[ 2 ] = 'F';
        hdr->chunk_id[ 3 ] = 'F';
        hdr->chunk_size = sizeof( record_wav_header_t );
        hdr->format[ 0 ] = 'W';
        hdr->format[ 1 ] = 'A';
        hdr->format[ 2 ] = 'V';
        hdr->format[ 3 ] = 'E';
        hdr->subchunk_1_id[ 0 ] = 'f';
        hdr->subchunk_1_id[ 1 ] = 'm';
        hdr->subchunk_1_id[ 2 ] = 't';
        hdr->subchunk_1_id[ 3 ] = 0x20;
        hdr->subchunk_1_size = 16;
        hdr->audio_format = 1;
        hdr->num_channels = 2;
        hdr->sample_rate = 44100;
        hdr->bits_per_sample = 16;
        hdr->block_align = 4;
        hdr->byte_rate = ( hdr->sample_rate * hdr->bits_per_sample * hdr->num_channels ) / 8;
        hdr->subchunk_2_id[ 0 ] = 'd';
        hdr->subchunk_2_id[ 1 ] = 'a';
        hdr->subchunk_2_id[ 2 ] = 't';
        hdr->subchunk_2_id[ 3 ] = 'a';

        fwrite( hdr, sizeof( record_wav_header_t ), 1, record_state->tmp_fp );

        // Start up in inactive state
        record_state->status = ZDJ_AUDIO_RECORD_INACTIVE;
    }
}

static void _deinit_state( zdj_pipeline_node_t * node ) {
    zdj_audio_record_node_state_t * state = (zdj_audio_record_node_state_t*)node->state;
    if( state ) { node->state = NULL; free( state );  }
}

// The following commands come from the UI thread.  All audio processing must be done
// on the audio fast-cycle thread.
void zdj_new_audio_record_capture( zdj_pipeline_node_t * record_node ) {
    zdj_audio_record_node_state_t * record_state = (zdj_audio_record_node_state_t*)record_node->state;
    // Create tmp file to store sample data.
    record_state->status = ZDJ_AUDIO_RECORD_BEGIN;
}

void zdj_enable_audio_record_capture( zdj_pipeline_node_t * record_node ) {
    zdj_audio_record_node_state_t * record_state = (zdj_audio_record_node_state_t*)record_node->state;
    // Begin capturing samples from soundcard to tmp file.
    record_state->status = ZDJ_AUDIO_RECORD_ACTIVE;
}

void zdj_disable_audio_record_capture( zdj_pipeline_node_t * record_node ) {
    zdj_audio_record_node_state_t * record_state = (zdj_audio_record_node_state_t*)record_node->state;
    // Pause capturing samples from soundcard to tmp file.
    record_state->status = ZDJ_AUDIO_RECORD_INACTIVE;
}

void zdj_finish_audio_record_capture( zdj_pipeline_node_t * record_node, bool save ) {
    zdj_audio_record_node_state_t * record_state = (zdj_audio_record_node_state_t*)record_node->state;
    // Teardown the tmp capture system and transition to post-processing.
    record_state->status = ZDJ_AUDIO_RECORD_FINISH;

    // Launch post-processing thread
    printf( "launching record post-processing thread\n" );
    zdj_thread_launch_record_post_proc_cycle( &_zdj_audio_record_processing_thread_main, record_node );
}

static void * _zdj_audio_record_processing_thread_main( void * arg ) {
    zdj_pipeline_node_t * record_node = (zdj_pipeline_node_t*)arg;
    zdj_audio_record_node_state_t * record_state = (zdj_audio_record_node_state_t*)record_node->state;
    
    char out_filename[ 256 ];
    snprintf( out_filename, sizeof( out_filename ), "%s/%s", 
        ZDJ_RECORDING_DIR, basename( record_state->tmp_filepath ) 
    );

    

    // Populate tags from current state.
    // Write tmp data thru AVContext to output file.


    // SAMPLE CODE
    // --------------------------------------------------------------
    
    // const char *in_filename = argv[1];
    // const char *out_filename = argv[2];

    AVFormatContext *in_fmt_ctx = NULL;
    AVFormatContext *out_fmt_ctx = NULL;
    AVPacket pkt;

    int ret;
    int stream_index = -1;
    AVStream *in_stream, *out_stream;

    // Open the input WAV file
    if ((ret = avformat_open_input(&in_fmt_ctx, record_state->tmp_filepath, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return NULL;
    }

    if ((ret = avformat_find_stream_info(in_fmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        avformat_close_input(&in_fmt_ctx);
        return NULL;
    }

    // Find the audio stream
    for (int i = 0; i < in_fmt_ctx->nb_streams; i++) {
        if (in_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            stream_index = i;
            break;
        }
    }
    if (stream_index == -1) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find an audio stream\n");
        avformat_close_input(&in_fmt_ctx);
        return NULL;
    }

    // Create the output context for the new WAV file
    avformat_alloc_output_context2(&out_fmt_ctx, NULL, NULL, out_filename);
    if (!out_fmt_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
        avformat_close_input(&in_fmt_ctx);
        return NULL;
    }

    in_stream = in_fmt_ctx->streams[stream_index];
    out_stream = avformat_new_stream(out_fmt_ctx, NULL);
    if (!out_stream) {
        av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
        avformat_close_input(&in_fmt_ctx);
        avformat_free_context(out_fmt_ctx);
        return NULL;
    }

    // Copy the codec parameters from the input to the output stream
    ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to copy codec parameters\n");
        avformat_close_input(&in_fmt_ctx);
        avformat_free_context(out_fmt_ctx);
        return NULL;
    }

    // Set WAV-compatible metadata (RIFF INFO tags)
    av_dict_set(&out_fmt_ctx->metadata, "artist", "Zero", 0);
    av_dict_set(&out_fmt_ctx->metadata, "title", "Zero Recording", 0);
    // Use other compatible RIFF INFO keys as needed

    // Open the output file
    if (!(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if ((ret = avio_open(&out_fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE)) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'\n", out_filename);
            avformat_close_input(&in_fmt_ctx);
            avformat_free_context(out_fmt_ctx);
            return NULL;
        }
    }

    // Write the stream headers
    if ((ret = avformat_write_header(out_fmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error occurred when writing header\n");
        avio_closep(&out_fmt_ctx->pb);
        avformat_close_input(&in_fmt_ctx);
        avformat_free_context(out_fmt_ctx);
        return NULL;
    }

    // Read frames from input and write to output
    while (av_read_frame(in_fmt_ctx, &pkt) >= 0) {
        if (pkt.stream_index == stream_index) {
            // printf( "write %ld: %d -> %d\n", pkt.pts, in_stream->time_base.den, out_stream->time_base.den );
            // Adjust timestamps if needed
            av_packet_rescale_ts(&pkt, in_stream->time_base, out_stream->time_base);
            // log_packet(out_fmt_ctx, &pkt, "out");

            // Write the packet
            if ((ret = av_interleaved_write_frame(out_fmt_ctx, &pkt)) < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error writing packet\n");
                break;
            }
        }
        av_packet_unref(&pkt);
    }

    // Write the trailer
    av_write_trailer(out_fmt_ctx);

    // Clean up
    if (out_fmt_ctx && !(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&out_fmt_ctx->pb);
    }
    avformat_close_input(&in_fmt_ctx);
    avformat_free_context(out_fmt_ctx);

    sync( );

    printf( "record_node done writing wav\n" );
    return NULL;
}
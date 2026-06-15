#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/syscall.h>
#include <math.h>

#include <libavformat/avformat.h>
#include <libavcodec/packet.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavcodec/codec_id.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/deck/zdj_deck_manager.h>
#include <zerodj/signal/math/zdj_signal_math.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/analysis/waveform/zdj_waveform.h>
#include <zerodj/signal/pipeline/node/audio/buffer/zdj_audio_buffer_node.h>
#include <zerodj/signal/pipeline/node/audio/library_decode/zdj_library_decode_node.h>
#include <zerodj/signal/pipeline/node/audio/record/zdj_audio_record_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/system/settings/zdj_settings.h>
#include <zerodj/ui/widget/notify/zdj_notify_widget.h>

static void _update_wait( zdj_pipeline_node_t * node );
static void _deinit_state( zdj_pipeline_node_t * node );
static void _hms_string_for_pcm_sample( char * str, double sample, uint32_t sample_rate );
static void _ms_string_for_pcm_sample( char * str, double sample, uint32_t sample_rate );

static void _write_recording( zdj_pipeline_node_t * record_node );

pthread_t _zdj_thread_record_proc;
void * _zdj_record_proc_thread_main( void * arg );

zdj_pipeline_node_t * zdj_new_audio_record_node( zdj_soundcard_node_t * soundcard_node ) {
    // printf( "zdj_new_audio_record_node\n" );
    // Make node
    zdj_pipeline_node_t * node = zdj_new_pipeline_node( );
    node->deinit_state = &_deinit_state;
    node->update_wait = &_update_wait;

    // Add state
    zdj_audio_record_node_state_t * state = calloc( 1, sizeof( zdj_audio_record_node_state_t ) );
    node->state = state;
    state->soundcard_node = soundcard_node;
    state->status = ZDJ_AUDIO_RECORD_INIT;
    state->sample_count = 0;
    state->buf_0 = calloc( 1, ZDJ_RECORDING_BUF_SIZE );
    state->buf_1 = calloc( 1, ZDJ_RECORDING_BUF_SIZE );
    state->front_buf = state->buf_0;
    state->back_buf = state->buf_1;

    // Launch the processing thread
    pthread_create( &_zdj_thread_record_proc, NULL, &_zdj_record_proc_thread_main, node );
   
    return node;
}

// Note that we're on the audio fast update cycle here.
// Don't spend too much time on anything in this func.
static void _update_wait( zdj_pipeline_node_t * node ) {
    zdj_audio_record_node_state_t * record_state = (zdj_audio_record_node_state_t*)node->state;
    // printf( "record _update_wait: %p - %d\n", record_state, record_state->status );
    
    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_RECORD_UPDATE;

    if ( record_state->status == ZDJ_AUDIO_RECORD_ACTIVE ) {
        // printf( "ZDJ_AUDIO_RECORD_ACTIVE: %p - %d\n", record_state, record_state->status );
        // Write samples in soundcard_node (recording buffer) to tmp data file
        zdj_pipeline_node_t * pipe = record_state->soundcard_node->data_pipe;

        float * buf = pipe->get_data( pipe );
        zdj_audio_buffer_node_state_t * pipe_state = (zdj_audio_buffer_node_state_t *)pipe->state;

        record_state->sample_count += pipe_state->sample_count;

        // Append samples to buf
        int buf_index = 0;
        float samp;
        float * out_buf;
        if( record_state->cur_buf == 0 ) { out_buf = record_state->buf_0; }
        else { out_buf = record_state->buf_1; }

        for( int i=0; i<pipe_state->sample_count; i++ ) {
            samp = buf[ i*pipe_state->stereo_mode ]; 
            buf_index = (ZDJ_SOUNDCARD_BUF_LEN*record_state->buf_count*pipe_state->stereo_mode) + (i*pipe_state->stereo_mode);
            out_buf[ buf_index ] = samp;
            if( pipe_state->stereo_mode > 1 ) {
                samp = buf[ (i*pipe_state->stereo_mode)+1 ]; 
                buf_index = (ZDJ_SOUNDCARD_BUF_LEN*record_state->buf_count*pipe_state->stereo_mode) + (i*pipe_state->stereo_mode) + 1;
                out_buf[ buf_index ] = samp;
            }
        }

        // If we've filled the front buf, swap buffs and command a flush on the slow-thread
        record_state->buf_count++;
        if( record_state->buf_count >= ZDJ_RECORDING_BUF_COUNT ) {
            // if( record_state->front_buf == record_state->buf_0 ) {
            //     printf( "swapping to back buf\n" );
            //     record_state->front_buf = record_state->buf_1;
            // } else {
            //     printf( "swapping to front buf\n" );
            //     record_state->front_buf = record_state->buf_0;
            // }
            if( record_state->cur_buf == 0 ) {
                record_state->cur_buf = 1;
            } else {
                record_state->cur_buf = 0;
            }

            record_state->buf_flush_cmd = true;
            record_state->buf_count = 0;
        }
    }

    zdj_error_state( )->marker = ZDJ_ERROR_MARKER_UNCLAIMED;
    // printf( "record _update_wait done\n" );
}

static void _deinit_state( zdj_pipeline_node_t * node ) {
    zdj_audio_record_node_state_t * state = (zdj_audio_record_node_state_t*)node->state;
    if( state ) { node->state = NULL; free( state );  }
}

// The following commands come from the UI thread.  All audio processing must be done
// on the audio fast-cycle thread.
void zdj_new_audio_record_capture( zdj_pipeline_node_t * record_node, bool notify ) {
    printf( "zdj_new_audio_record_capture\n" );
    zdj_audio_record_node_state_t * record_state = (zdj_audio_record_node_state_t*)record_node->state;
    record_state->save_on_finish = false;

    // Create tmp file to store sample data.
    record_state->status = ZDJ_AUDIO_RECORD_BEGIN;

    // Show note w/filename
    if( notify ) {
        char recording_name[ 32 ];
        // recording num will update during finalize process
        int recording_num = zdj_setting_get( ZDJ_SETTING_RECORDING_COUNTER )->i_val + 1;
        snprintf( recording_name, sizeof( recording_name ), "recording_%03d.wav", recording_num );
        zdj_show_notify_widget( recording_name, NULL, NULL );
    }
}

void zdj_finish_audio_record_capture( zdj_pipeline_node_t * record_node, bool save, bool notify ) {
    printf( "zdj_finish_audio_record_capture\n" );
    zdj_audio_record_node_state_t * record_state = (zdj_audio_record_node_state_t*)record_node->state;
    
    // Show note w/filename
    if( notify ) {
        char recording_name[ 32 ];
        // recording num will update during finalize process
        int recording_num = zdj_setting_get( ZDJ_SETTING_RECORDING_COUNTER )->i_val + 1;
        snprintf( recording_name, sizeof( recording_name ), "recording_%03d.wav", recording_num );
        zdj_show_notify_widget( recording_name, NULL, NULL );
    }
    
    // Signal to all threads that we're done capturing data.
    record_state->sample_count = 0;
    record_state->save_on_finish = save;
    record_state->status = ZDJ_AUDIO_RECORD_FINISH;
}

void zdj_audio_record_put_capture_time( zdj_pipeline_node_t * record_node, char * str ) {
    zdj_audio_record_node_state_t * state = (zdj_audio_record_node_state_t*)record_node->state;
    _hms_string_for_pcm_sample( str, state->sample_count, 44100 );
}

// Slow-thread for writing captured samples to the drive.
// Runs on UI core - should never step on soundcard thread.
void * _zdj_record_proc_thread_main( void * arg ) {
    zdj_pipeline_node_t * record_node = (zdj_pipeline_node_t*)arg;
    zdj_audio_record_node_state_t * record_state = (zdj_audio_record_node_state_t*)record_node->state;

    // Set core affinity to Core #1;
    cpu_set_t cpuset;
	CPU_ZERO( &cpuset );
	CPU_SET( 0,&cpuset );
	int err = sched_setaffinity( syscall(SYS_gettid), sizeof(cpu_set_t), &cpuset );
    if( err != 0 ) {
        perror( "set affinity failed" );
    }

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
    hdr->audio_format = 3; //32-bit float
    hdr->num_channels = 2;
    hdr->sample_rate = 44100;
    hdr->bits_per_sample = 32;
    hdr->block_align = ( hdr->bits_per_sample * hdr->num_channels ) / 8;
    hdr->byte_rate = ( hdr->sample_rate * hdr->bits_per_sample * hdr->num_channels ) / 8;
    hdr->subchunk_2_id[ 0 ] = 'd';
    hdr->subchunk_2_id[ 1 ] = 'a';
    hdr->subchunk_2_id[ 2 ] = 't';
    hdr->subchunk_2_id[ 3 ] = 'a';

    struct timespec wait_sleep = { 0, 100000000 };
    
    // Loop while record is running
    while( 1 ) {
        wait_sleep.tv_nsec = 100000000; // Sleep length when not active
        
        if( record_state->status == ZDJ_AUDIO_RECORD_ACTIVE ) {
            // printf( "-" );
            // Check for swap cmd from soundcard-fast thread
            if( record_state->buf_flush_cmd ) {
                // printf( "o\n" );
                record_state->buf_flush_cmd = false;
                float * buf = (record_state->cur_buf == 0 ) ? record_state->buf_1 : record_state->buf_0;
                // Append the buf to the temp data file
                if( record_state->channel_count == 1 ) {
                    fwrite( 
                        buf, 
                        sizeof( float ), 
                        ZDJ_RECORDING_BUF_LEN / 2, 
                        record_state->tmp_fp 
                    );
                } else if( record_state->channel_count == 2 ) {
                    fwrite( 
                        buf, 
                        sizeof( float ), 
                        ZDJ_RECORDING_BUF_LEN, 
                        record_state->tmp_fp 
                    );
                }
            }
            // Shorter sleep when active
            wait_sleep.tv_nsec = 7000000;

        } else if( record_state->status == ZDJ_AUDIO_RECORD_BEGIN ) {
            printf( "ZDJ_AUDIO_RECORD_BEGIN: %p - %d\n", record_state, record_state->status );
            
            zdj_pipeline_node_t * pipe = record_state->soundcard_node->data_pipe;
            zdj_audio_buffer_node_state_t * pipe_state = (zdj_audio_buffer_node_state_t *)pipe->state;
            
            // Rewind the tmp file + write a wav header
            record_state->tmp_fp = fopen( ZDJ_RECORDING_TEMP_FILE, "w" );
            fseek( record_state->tmp_fp, 0, SEEK_SET );
            fwrite( hdr, sizeof( record_wav_header_t ), 1, record_state->tmp_fp );

            record_state->channel_count = pipe_state->stereo_mode;
            record_state->sample_count = 0; // Zero the sample counter for the UI timer
            record_state->save_on_finish = false;
            record_state->front_buf = record_state->buf_0;
            record_state->back_buf = record_state->buf_1;

            // Clear the buffers
            memset( record_state->buf_0, 0, ZDJ_RECORDING_BUF_SIZE );
            memset( record_state->buf_1, 0, ZDJ_RECORDING_BUF_SIZE );

            record_state->status = ZDJ_AUDIO_RECORD_ACTIVE;

        } else if( record_state->status == ZDJ_AUDIO_RECORD_FINISH ) {
            
            record_state->status = ZDJ_AUDIO_RECORD_PROCESSING;

        } else if( record_state->status == ZDJ_AUDIO_RECORD_PROCESSING ) {
            // Close the tmp file
            fclose( record_state->tmp_fp );
            // Finalize the recording file
            _write_recording( record_node );
            // Move to inert state
            record_state->status = ZDJ_AUDIO_RECORD_INACTIVE;
        
        } else if( record_state->status == ZDJ_AUDIO_RECORD_INACTIVE ) {
            // printf( "inact\n" );
        }

        // sleep for a bit
        nanosleep( &wait_sleep, NULL );
    }

    return NULL;
}


static void _write_recording( zdj_pipeline_node_t * record_node ) {
    zdj_audio_record_node_state_t * record_state = (zdj_audio_record_node_state_t*)record_node->state;

    
    char out_filename[ 256 ];
    int recording_num = zdj_setting_increment_int( ZDJ_SETTING_RECORDING_COUNTER );
    snprintf( out_filename, sizeof( out_filename ), "%s/recording_%03d.wav", 
        ZDJ_RECORDING_DIR, 
        recording_num 
    );

    // Create song graph for storage
    zdj_library_song_t * song = zdj_library_create_file_import_song_graph( out_filename, zdj_library_db );
    strcpy( song->catalog->artist, ZDJ_RECORDING_ARTIST );
    char rec_title[ 64 ];
    sprintf( rec_title, "%s%03d", ZDJ_RECORDING_PREFIX, recording_num );
    strcpy( song->catalog->title, rec_title );
    if( zdj_deck_manager( )->sync.active ) {
        song->performance->has_beat_grid = true;
        song->performance->bpm = zdj_deck_manager( )->sync.set_bpm;
    }

    // Write tmp data thru AVContext to output file.

    AVFormatContext *in_fmt_ctx = NULL;
    AVFormatContext *out_fmt_ctx = NULL;
    AVPacket pkt;

    int ret;
    int stream_index = -1;
    AVStream *in_stream, *out_stream;

    // printf( "0\n" );

    // Open the input WAV file
    if ((ret = avformat_open_input(&in_fmt_ctx, ZDJ_RECORDING_TEMP_FILE, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        char err_str[ 64 ];
        sprintf( err_str, "Error saving:%s", out_filename );
        // zdj_announce_mini( err_str );
        return;
    }

    // printf( "0.1\n" );

    if ((ret = avformat_find_stream_info(in_fmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        avformat_close_input(&in_fmt_ctx);
        return;
    }

    // Store AVCodecID, stream index, and song duration for later use.
    for (int i = 0; i < in_fmt_ctx->nb_streams; i++) {
        AVCodecParameters * params = in_fmt_ctx->streams[ i ]->codecpar;
        if ( params->codec_type == AVMEDIA_TYPE_AUDIO) {
            song->audio->av_codec_id = params->codec_id;
            song->audio->av_stream_index = i;
            song->audio->av_sample_rate = params->sample_rate;
            song->audio->av_sample_format = params->format;
            song->audio->av_channel_count = params->channels;
            double tb = (double)(in_fmt_ctx->streams[ i ]->time_base.num) / (double)(in_fmt_ctx->streams[ i ]->time_base.den);
            song->audio->duration_sec = (double)in_fmt_ctx->streams[ i ]->duration * tb;
            song->audio->timebase = (double)in_fmt_ctx->streams[ i ]->time_base.den;
        }
    }


    // printf( "1\n" );

    // Find the audio stream
    for (int i = 0; i < in_fmt_ctx->nb_streams; i++) {
        AVCodecParameters * params = in_fmt_ctx->streams[ i ]->codecpar;
        if ( params->codec_type == AVMEDIA_TYPE_AUDIO) {
        // if (in_fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            stream_index = i;

            // Capture the stream info in the library DTO
            song->audio->av_codec_id = params->codec_id;
            song->audio->av_stream_index = i;
            song->audio->av_sample_rate = params->sample_rate;
            song->audio->av_sample_format = params->format;
            song->audio->av_channel_count = params->channels;
            double tb = (double)(in_fmt_ctx->streams[ i ]->time_base.num) / (double)(in_fmt_ctx->streams[ i ]->time_base.den);
            song->audio->duration_sec = (double)in_fmt_ctx->streams[ i ]->duration * tb;
            song->audio->timebase = (double)in_fmt_ctx->streams[ i ]->time_base.den;

            break;
        }
    }

    if (stream_index == -1) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find an audio stream\n");
        avformat_close_input(&in_fmt_ctx);
        return;
    }

    // printf( "2\n" );

    // Create the output context for the new WAV file
    avformat_alloc_output_context2(&out_fmt_ctx, NULL, NULL, out_filename);
    if (!out_fmt_ctx) {
        av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
        avformat_close_input(&in_fmt_ctx);
        return;
    }

    in_stream = in_fmt_ctx->streams[stream_index];
    out_stream = avformat_new_stream(out_fmt_ctx, NULL);
    if (!out_stream) {
        av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
        avformat_close_input(&in_fmt_ctx);
        avformat_free_context(out_fmt_ctx);
        return;
    }
    
    // printf( "3\n" );

    // Copy the codec parameters from the input to the output stream
    ret = avcodec_parameters_copy(out_stream->codecpar, in_stream->codecpar);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to copy codec parameters\n");
        avformat_close_input(&in_fmt_ctx);
        avformat_free_context(out_fmt_ctx);
        return;
    }

    // Open the output file
    if (!(out_fmt_ctx->oformat->flags & AVFMT_NOFILE)) {
        if ((ret = avio_open(&out_fmt_ctx->pb, out_filename, AVIO_FLAG_WRITE)) < 0) {
            av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'\n", out_filename);
            avformat_close_input(&in_fmt_ctx);
            avformat_free_context(out_fmt_ctx);
            return;
        }
    }

    // Write the stream headers
    if ((ret = avformat_write_header(out_fmt_ctx, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Error occurred when writing header\n");
        avio_closep(&out_fmt_ctx->pb);
        avformat_close_input(&in_fmt_ctx);
        avformat_free_context(out_fmt_ctx);
        return;
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

    // printf( "9\n" );

    sync( );

    printf( "Wrote file: %s\n", out_filename );


    /////////////////////////////////////
    // Import new recording to Library //
    /////////////////////////////////////
    song->analysis_state = ZDJ_LIBRARY_ANALYSIS_STATE_RUNNING;

    // Make waveforms
    zdj_pipeline_node_t * decode_node = zdj_new_library_decode_node( song );
    zdj_library_decode_node_state_t * decode_state = (zdj_library_decode_node_state_t*)decode_node->state;

    zdj_fs_mkdir_p( ZDJ_LIBRARY_PLAYBACK_WAVEFORM_TEMP_DIR );
    zdj_fs_mkdir_p( ZDJ_LIBRARY_THUMB_WAVEFORM_TEMP_DIR );

    char tmp_waveform_path[ 256 ];
    char tmp_thumb_path[ 256 ];

    snprintf( tmp_waveform_path, sizeof( tmp_waveform_path ), "%s/%s", 
        ZDJ_LIBRARY_PLAYBACK_WAVEFORM_TEMP_DIR, song->entity_id
    );
    zdj_pipeline_node_t * playback_waveform_maker = zdj_new_waveform_maker( 
        decode_node,
        tmp_waveform_path,
        ZDJ_PLAYBACK_WAVEFORM_SAMPLE_STRIDE,
        6500
    );
    if( !playback_waveform_maker ) { printf( "playback waveform maker failed\n" ); }

    // Make Thumb waveform node
    snprintf( tmp_thumb_path, sizeof( tmp_thumb_path ), "%s/%s", ZDJ_LIBRARY_THUMB_WAVEFORM_TEMP_DIR, song->entity_id );
    // Take a rough guess art duration to figure out thumb waveform stride.  Doesn't have to be accurate.
    int64_t est_duration = song->audio->duration_sec * song->audio->av_sample_rate;
    zdj_pipeline_node_t * thumb_waveform_maker = zdj_new_waveform_maker( 
        decode_node,
        tmp_thumb_path,
        fmin( ZDJ_THUMB_WAVEFORM_SAMPLE_STRIDE, est_duration / 300 ),
        20
    );
    if( !thumb_waveform_maker ) { printf( "thumb waveform maker failed\n" ); }

    while( song->analysis_state == ZDJ_LIBRARY_ANALYSIS_STATE_RUNNING ) {
        // printf( "updating waveform makers\n" );
        decode_node->update_wait( decode_node );
        playback_waveform_maker->update_wait( playback_waveform_maker );
        thumb_waveform_maker->update_wait( thumb_waveform_maker );
    }

    decode_node->deinit( decode_node );
    zdj_close_waveform_maker( playback_waveform_maker );
    playback_waveform_maker->deinit( playback_waveform_maker );
    zdj_close_waveform_maker( thumb_waveform_maker );
    thumb_waveform_maker->deinit( thumb_waveform_maker );

    // Store graph
    zdj_library_store_song_graph( song, zdj_library_db );
    // Add song entity_id to library's song links
    zdj_library_add_song_link( zdj_library_config_get_current_library_id( ), song, zdj_library_db );

    // Copy waveforms from temp to final dirs
    char waveform_path[ 256 ];
    char thumb_path[ 256 ];

    snprintf( waveform_path, sizeof( waveform_path ), "%s/%s",  ZDJ_LIBRARY_PLAYBACK_WAVEFORM_DIR, song->entity_id );
    zdj_fs_copy_file( tmp_waveform_path, waveform_path, true );
    
    snprintf( thumb_path, sizeof( thumb_path ), "%s/%s",  ZDJ_LIBRARY_THUMB_WAVEFORM_DIR, song->entity_id );
    zdj_fs_copy_file( tmp_thumb_path, thumb_path, true );
    
    zdj_library_db_flush( );
    zdj_library_free_song_graph( song );
    // return NULL;
}

static void _hms_string_for_pcm_sample( char * str, double sample, uint32_t sample_rate ) {
    int hrs = sample / sample_rate / 60 / 60;
    int mins = sample / sample_rate / 60;
    int secs = (sample / sample_rate) - (mins * 60);
    double secf = ((double)sample / (double)sample_rate);
    int msec = (int)((secf - secs) * 1000.0);

    snprintf( str, -1, "%d:%02d:%02d", 
        hrs,
        mins,
        secs
    );
}

static void _ms_string_for_pcm_sample( char * str, double sample, uint32_t sample_rate ) {
    int mins = sample / sample_rate / 60;
    int secs = (sample / sample_rate) - (mins * 60);
    double secf = ((double)sample / (double)sample_rate);
    int msec = (int)((secf - secs) * 10.0);

    snprintf( str, -1, ".%d", msec );
}
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavcodec/codec_id.h>

#include <zerodj/library/zdj_library.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/decode/zdj_fast_decode_node.h>

#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

static bool _zdj_av_codec_id_is_pcm( enum AVCodecID id );
static void * _zdj_fast_pcm_decode_node_thread( void * arg );
static void * _zdj_fast_lossy_decode_node_thread( void * arg );

// Basic FFMPEG PCM decode from file to buffer.
// For analysis only.  Not suitable for playback.
zdj_pipeline_node_t * zdj_new_fast_decode_node( zdj_library_song_t * song ) {
    zdj_pipeline_node_t * node = zdj_new_pipeline_node( );
    zdj_fast_decode_node_state_t * state = calloc( 1, sizeof( zdj_fast_decode_node_state_t ) );
    state->song_dto = song;
    state->status = ZDJ_FAST_DECODE_INIT;
    node->state = (void*)state;

    #define MAX_DATA_SIZE 8
    #define MAX_CHANNEL_COUNT 2
    #define MAX_SAMPLE_COUNT 2000

    // Init the in/out buffers.
    // node->out_buf = zdj_new_pipeline_buffer( 
    //     MAX_SAMPLE_COUNT,
    //     song->audio->av_channel_count
    // );

    // Set up the wait/ready semaphores.
    node->wait_sem = calloc( 1, sizeof( sem_t ) );
    sem_init( node->wait_sem, 0, 0 );
    node->ready_sem = calloc( 1, sizeof( sem_t ) );
    sem_init( node->ready_sem, 0, 0 );

    // Set up and launch the decode thread.  
    // Will immediately sleep waiting for run sem.
    if( _zdj_av_codec_id_is_pcm( song->audio->av_codec_id ) ) {
        pthread_create( &state->thread, NULL, _zdj_fast_pcm_decode_node_thread, node );
    } else {
        pthread_create( &state->thread, NULL, _zdj_fast_lossy_decode_node_thread, node );
    }

    return node;
}

// Check CodecID for anything in range of PCM encodings (WAV/AIF, etc.).
bool _zdj_av_codec_id_is_pcm( enum AVCodecID id ) {
    return ( id >= AV_CODEC_ID_PCM_S16LE && id <= AV_CODEC_ID_PCM_SGA );
}

unsigned long decode( AVCodecContext *dec_ctx, AVPacket *pkt, AVFrame *frame, zdj_pipeline_node_t * node ) {
    int i, ch;
    int ret, data_size;

    /* send the packet with the compressed data to the decoder */
    ret = avcodec_send_packet( dec_ctx, pkt );
    if ( ret < 0 ) { return 0; }

    long unsigned int decode_count = 0;

    /* read all the output frames (in general there may be any number of them */
    while ( ret >= 0 ) {
        ret = avcodec_receive_frame( dec_ctx, frame );
        if ( ret == AVERROR(EAGAIN) || ret == AVERROR_EOF || ret < 0 ) { 
            // printf( "returning on ret: %d\n", ret );
            // printf( "decode_count: %lu\n", decode_count );
            return decode_count; 
        }
        // else if (ret < 0) {
        //     fprintf(stderr, "Error during decoding\n");
        //     return;
        // }
        data_size = av_get_bytes_per_sample( dec_ctx->sample_fmt );
        if ( data_size < 0 ) { 
            printf( "returning on sample format\n" );
            return 0; 
        }

        // Tally decoded sample count.
        decode_count += frame->nb_samples;

        // Push decoded frame's samples into node's output buffer
        for ( i = 0; i < frame->nb_samples; i++ ) {
            for ( ch = 0; ch < 2; ch++ ) {
                float l, r;

                memcpy( &l, frame->data[ch] + data_size*i, data_size );
                memcpy( &r, frame->data[ch] + data_size*i + data_size, data_size );

                // printf( "putting sample %d: %1.2f, %1.2f\n", i, l, r );
                // zdj_pipeline_buffer_put_sample( node->out_buf, l, r );
            }
        }
    }
}

// Decode a lossy (mp3, aac, etc.) file.
// This func interacts with a controlling thread.
// We setup decode resources, then wait for controlling thread
// to tell us to decode a block of samples.
// Then we wait for controlling thread to do something with
// that block of samples, and tell us to decode the next block.
// When we run out of samples to decode, we tell the controlling
// thread that we're done, tear down the decode resources, and end the thread.
void * _zdj_fast_lossy_decode_node_thread( void * arg ) {
    zdj_pipeline_node_t * node = (zdj_pipeline_node_t*)arg;
    zdj_fast_decode_node_state_t * node_state = (zdj_fast_decode_node_state_t*)node->state;

    // Don't attempt to import anything which has already errored out
    if( node_state->song_dto->has_error ) { pthread_exit( NULL ); }

    // Hash filepath
    node_state->song_dto->audio->file_checksum = zdj_library_audio_file_crc( node_state->song_dto->audio->filepath );

    // Find codec
    const AVCodec * codec = avcodec_find_decoder( node_state->song_dto->audio->av_codec_id );
    if ( !codec ) { 
        node_state->status = ZDJ_FAST_DECODE_DONE;
        node_state->song_dto->has_error = true;
        pthread_exit( NULL ); 
    }

    // Set up parser
    AVCodecParserContext * parser = av_parser_init( codec->id );
    if ( !parser ) {
        node_state->status = ZDJ_FAST_DECODE_DONE;
        node_state->song_dto->has_error = true;
        pthread_exit( NULL );
    }

    // Set up codec context
    AVCodecContext * c = avcodec_alloc_context3( codec );
    if ( !c ) {
        av_parser_close( parser );
        node_state->status = ZDJ_FAST_DECODE_DONE;
        node_state->song_dto->has_error = true;
        pthread_exit( NULL );
    }

    // Open the codec
    if ( avcodec_open2( c, codec, NULL ) < 0 ) {
        avcodec_free_context( &c );
        av_parser_close( parser );
        node_state->status = ZDJ_FAST_DECODE_DONE;
        node_state->song_dto->has_error = true;
        pthread_exit( NULL );
    }

    // Open the source file
    FILE * f = fopen( node_state->song_dto->audio->filepath, "rb" );
    if ( !f ) {
        avcodec_free_context( &c );
        av_parser_close( parser );
        node_state->status = ZDJ_FAST_DECODE_DONE;
        node_state->song_dto->has_error = true;
        pthread_exit( NULL );
    }

    // Set up the decode resources.
    int len, ret;
    uint8_t inbuf[ AUDIO_INBUF_SIZE + AV_INPUT_BUFFER_PADDING_SIZE ];
    uint8_t * data;
    size_t data_size;
    data = inbuf;
    data_size = fread( inbuf, 1, AUDIO_INBUF_SIZE, f );

    AVFrame * frame = av_frame_alloc( );
    AVPacket * pkt = av_packet_alloc( );
    unsigned long decode_count = 0;
    
    while ( data_size > 0 ) {
        // Wait until the consuming thread asks for a new buffer.
        sem_wait( node->wait_sem );

        ret = av_parser_parse2( 
            parser, c, &pkt->data, &pkt->size, data, 
            data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0 
        );
        if ( ret < 0 ) { pthread_exit( NULL ); }
        data += ret;
        data_size -= ret;

        if ( pkt->size ) { 
            decode_count = decode( c, pkt, frame, node );
        } else {
            decode_count = 0;
        }

        // Build a unit-value representing progress through file's data.
        node_state->song_dto->audio->decoded_sample_count += decode_count;
        double sec = (double)node_state->song_dto->audio->decoded_sample_count / (double)node_state->song_dto->audio->av_sample_rate;
        node_state->song_dto->analysis_progress = sec / node_state->song_dto->audio->duration;

        // Shift more file data into buffer if needed.
        if ( data_size < AUDIO_REFILL_THRESH ) {
            memmove( inbuf, data, data_size );
            data = inbuf;
            len = fread( data + data_size, 1, AUDIO_INBUF_SIZE - data_size, f );
            if ( len > 0 ) { data_size += len; }
        }

        // When we're done, set state before posting the semaphore.
        if( data_size == 0 ) { node_state->status = ZDJ_FAST_DECODE_DONE; }
        
        // Tell the consuming thread there's a buffer available.
        sem_post( node->ready_sem );
    }

    // Flush out the decoder when we're done.
    pkt->data = NULL;
    pkt->size = 0;
    decode( c, pkt, frame, node );

    // Close out and free resources.
    fclose( f );
    avcodec_free_context( &c );
    av_parser_close( parser );
    av_frame_free( &frame );
    av_packet_free( &pkt );

    pthread_exit( NULL );
}

// Decode a PCM file.
void * _zdj_fast_pcm_decode_node_thread( void * arg ) {
    zdj_pipeline_node_t * node = (zdj_pipeline_node_t*)arg;
    zdj_fast_decode_node_state_t * node_state = (zdj_fast_decode_node_state_t*)node->state;

    // Don't attempt to import anything which has already errored out
    if( node_state->song_dto->has_error ) { pthread_exit( NULL ); }

    // Hash filepath
    node_state->song_dto->audio->file_checksum = zdj_library_audio_file_crc( node_state->song_dto->audio->filepath );

    // Get a file type from the file extension.


    // Read the header and seek to data start.

    // Set up the decode resources.

    // Wait until the consuming thread asks for a new buffer.
    sem_wait( node->wait_sem );

    node_state->status = ZDJ_FAST_DECODE_DONE;

    // Tell the consuming thread there's a buffer available.
    sem_post( node->ready_sem );

    // Tear down resources.

    pthread_exit( NULL );
}
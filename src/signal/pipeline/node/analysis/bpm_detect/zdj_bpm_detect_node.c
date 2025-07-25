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

zdj_pipeline_node_t * zdj_new_bpm_detect_node( void ) {
    // zdj_pipeline_node_t * node = zdj_new_pipeline_node( ZDJ_SIGNAL_TYPE_AUDIO );
    // return node;
}

// float _scan_bpm( int samp_start, int block_count ) {
//     soundtouch::BPMDetect bpm ( 2, 44100 );
//     mpg123_seek( m, samp_start, SEEK_SET );
//     for( int n=0; n<block_count; n++ ) {
//         if( mpg123_read( m, decode_buf, BUFFER_LEN * 2 * encosize, &decode_count ) != MPG123_OK ) {
//             fprintf( stderr, "Cannot read: %s\n", mpg123_strerror( m ) );
//             return 0;
//         }
//         for ( int i=0; i<BUFFER_LEN*2; i++) {
//             decode_float_buf[ i ] = normalize( decode_buf[ i ] );
//         }
//         bpm.inputSamples( decode_float_buf, decode_count / 4 );
//     }
//     int beats = bpm.getBeats( NULL, NULL, 100 );
//     float * beatarr = ( float * ) malloc( sizeof( float ) * beats );
//     memset( beatarr, 0, sizeof(float) * beats );
//     float * posarr = ( float * ) malloc( sizeof( float ) * beats );
//     memset( posarr, 0, sizeof(float) * beats );
//     bpm.getBeats( posarr, beatarr, beats );
//     return( bpm.getBpm( ) );
// }
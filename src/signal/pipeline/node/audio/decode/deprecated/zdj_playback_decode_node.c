// #include <stdlib.h>
// #include <stdio.h>
// #include <stdbool.h>
// #include <unistd.h>
// #include <pthread.h>
// #include <semaphore.h>

// #include <libavformat/avformat.h>
// #include <libavutil/dict.h>
// #include <libavutil/error.h>
// #include <libavcodec/codec_id.h>

// #include <zerodj/library/zdj_library.h>
// #include <zerodj/signal/pipeline/zdj_pipeline.h>
// #include <zerodj/signal/pipeline/node/audio/zdj_audio_node.h>
// #include <zerodj/signal/pipeline/node/audio/decode/zdj_playback_decode_node.h>

// static void * _zdj_playback_pcm_decode_node_thread( void * arg );
// static void * _zdj_playback_lossy_decode_node_thread( void * arg );

// // Basic FFMPEG PCM decode from file to buffer.
// // For analysis only.  Not suitable for playback.
// zdj_pipeline_node_t * zdj_new_playback_decode_node( zdj_library_song_t * song ) {
//     zdj_pipeline_node_t * node = zdj_new_pipeline_node( ZDJ_SIGNAL_TYPE_FILE );
//     zdj_playback_decode_node_state_t * state = calloc( 1, sizeof( zdj_playback_decode_node_state_t ) );
//     state->song_dto = song;
//     state->status = ZDJ_FAST_DECODE_INIT;
//     node->state = (void*)state;

//     // Set up the wait/ready semaphores.
//     node->wait_sem = calloc( 1, sizeof( sem_t ) );
//     sem_init( node->wait_sem, 0, 0 );
//     node->ready_sem = calloc( 1, sizeof( sem_t ) );
//     sem_init( node->ready_sem, 0, 0 );

//     init_decod_graph( state );

//     return node;
// }


// void init_decod_graph( zdj_playback_decode_node_state_t * ch_state ) {
//     // printf( "init_decod_graph( )\n" );
//     // Create an empty read address for this channel
//     decod_address_t * read_address = malloc( sizeof( decod_address_t ) );
//     ch_state->decod_address = read_address;
    
//     // Stand up decoding system based on file type
//     // switch ( ch_state->song->file_type ) {
//     //     case SONG_FILETYPE_MP3:
//             init_mp3_decod( ch_state );
//             // break;
//     //     case SONG_FILETYPE_WAV:
//     //         init_wav_decod( ch_state );
//     //         break;
//     //     default:
//     //         break;
//     // }
// }

// void update_decod_graph( zdj_playback_decode_node_state_t * ch_state ) {
//     // switch ( ch_state->song->file_type ) {
//     //     case SONG_FILETYPE_MP3:
//             update_mp3_decod( ch_state );
//     //         break;
//     //     case SONG_FILETYPE_WAV:
//     //         update_wav_decod( ch_state );
//     //         break;
//     //     default:
//     //         break;
//     // }
// }

// void get_decod_sample_at_offset( 
//     zdj_playback_decode_node_state_t * ch_state, 
//     decod_sample_float_t * frame, 
//     double offset,
//     bool interpolate 
// ) {
//     // printf( "get_decod_sample_at_offset\n" );
//     // switch ( ch_state->song->file_type ) {
//     //     case SONG_FILETYPE_MP3:
//             // First get the next frame value
//             get_mp3_decod_sample_at_offset( ch_state, frame, offset, interpolate );
//             // Then destructively update the address
//             // offset_mp3_decod_address( ch_state, offset );
//     //         break;
//     //     case SONG_FILETYPE_WAV:
//     //         // First get the next frame value
//     //         get_wav_decod_frame_at_offset( ch_state, frame, offset, interpolate );
//     //         // Then destructively update the address
//     //         offset_wav_decod_address( ch_state, offset );
//     //         break;
//     //     default:
//     //         break;
//     // }
//     // printf( "get_decod_sample_at_offset done\n" );
// }

// // Return a sample frame suitable for seeking.
// // Only meaningful if playback isn't running.
// // Otherwise it's impossible to know where in a DAC CYCLE
// // the audio stream currently is.
// int get_current_decod_sample( zdj_playback_decode_node_state_t * ch_state ) {
//     // switch ( ch_state->song->file_type ) {
//     //     case SONG_FILETYPE_MP3:
//             // Peek the most recently retrieved address
//             return get_mp3_decod_current_address( ch_state );
//     //         break;
//     //     case SONG_FILETYPE_WAV:
//     //         break;
//     //     default:
//     //         break;
//     // }
// }

// decod_sample_float_t * new_decod_sample_float( void ) {
//     // printf( "new_decod_sample_float\n" );
//     decod_sample_float_t * frame = malloc( sizeof( decod_sample_float_t ) );
//     frame->pcm_values = malloc( sizeof( float ) * 2 );
//     // printf( "new_decod_sample_float done\n" );
//     return frame;
// }

// void free_decod_sample_float( decod_sample_float_t * frame ) {
//     // printf( "free_decod_sample_float\n" );
//     free( frame->pcm_values );
//     free( frame );
//     // printf( "free_decod_sample_float done\n" );
// }

// void reset_decod_to_sample( zdj_playback_decode_node_state_t * ch_state, int frame ) {
//     // printf( "reset_decod_to_sample\n" );
//     // switch ( ch_state->song->file_type ) {
//     //     case SONG_FILETYPE_MP3:
//             mp3_reset_decod_to_sample( ch_state, frame );
//     //         break;
//     //     case SONG_FILETYPE_WAV:
            
//     //         break;
//     //     default:
//     //         break;
//     // }
// }
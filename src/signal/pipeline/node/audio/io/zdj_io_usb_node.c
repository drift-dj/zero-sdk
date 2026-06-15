#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include <alsa/asoundlib.h>

#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/system/perf/zdj_perf.h>
#include <zerodj/system/thread/zdj_thread.h>
#include <zerodj/signal/pipeline/node/audio/buffer/zdj_audio_buffer_node.h>
#include <zerodj/signal/pipeline/node/audio/io/zdj_io_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/system/thread/zdj_thread.h>

#define ZDJ_ALSA_BUF_LEN ZDJ_SOUNDCARD_BUF_LEN * 2
#define ZDJ_ALSA_PERIOD_COUNT 2

static void _deinit_state( zdj_pipeline_node_t * node );
static void _push_to_s16_buf( float * source, int16_t * dest, int samples, int channels );
static void _push_to_s32_buf( float * source, int32_t * dest, int samples, int channels );

static void _pull_from_s16_buf( int16_t * source, int source_channels, float * dest, int dest_channels, int samples );
static void _pull_from_s32_buf( int16_t * source, int source_channels, float * dest, int dest_channels, int samples );

static void _start_log( zdj_pipeline_node_t * node ) {
    zdj_io_usb_node_state_t * node_state = (zdj_io_usb_node_state_t*)node->state;
}

static void _log( zdj_pipeline_node_t * node, char * str ) {
    zdj_io_usb_node_state_t * node_state = (zdj_io_usb_node_state_t*)node->state;
    printf( "USB IO Log: %s\n", str );
}

static void _end_log( zdj_pipeline_node_t * node ) {
    zdj_io_usb_node_state_t * node_state = (zdj_io_usb_node_state_t*)node->state;
}

static bool _discover_pcm_device( 
    zdj_pipeline_node_t * node, 
    char * device_name,
    snd_pcm_t ** disco_handle,
    snd_pcm_stream_t stream
);

static void _alsa_buffer_callback( snd_async_handler_t *pcm_callback );

zdj_pipeline_node_t * zdj_new_io_usb_node( void ) {
    // Make node
    zdj_pipeline_node_t * node = zdj_new_pipeline_node( );
    node->deinit_state = &_deinit_state;

    zdj_io_usb_node_state_t * state = calloc( 1, sizeof( zdj_io_usb_node_state_t ) );
    node->state = state;

    // This may need to be set to max buffer length supported by all conceivable devices ever.
    state->soundcard_in_buffer = zdj_new_audio_buffer_node( 
        ZDJ_ALSA_BUF_LEN, ZDJ_AUDIO_BUFFER_STEREO 
    );
    state->soundcard_out_buffer = zdj_new_audio_buffer_node( 
        ZDJ_ALSA_BUF_LEN, ZDJ_AUDIO_BUFFER_STEREO 
    );

    // Node is ready for work
    state->phase = ZDJ_IO_USB_PHASE_INIT;

    return node;
}


static void _deinit_state( zdj_pipeline_node_t * node ) {

}

zdj_error_type_t zdj_io_usb_configure( zdj_pipeline_node_t * node ) {
    zdj_io_usb_node_state_t * node_state = (zdj_io_usb_node_state_t*)node->state;
    
    return ZDJ_ERROR_OKAY;
}

static void _alsa_buffer_callback( snd_async_handler_t *pcm_callback ) {
	// Buffer ready from ALSA
    // Move the next batch of samples into/out of ALSA's buffers
    // printf( "alsa buffer cb\n" );
    zdj_pipeline_node_t * usb_io_node = zdj_soundcard->usb_io_node;
    if( usb_io_node ) {
        zdj_io_usb_node_state_t * node_state = (zdj_io_usb_node_state_t*)usb_io_node->state;
        if( node_state->phase == ZDJ_IO_USB_PHASE_RUNNING ) {
            if( node_state->playback_available && node_state->pcm_out_handle ) {
                zdj_audio_buffer_node_state_t * out_buffer_state = (zdj_audio_buffer_node_state_t*)node_state->soundcard_out_buffer->state;
                switch( node_state->playback_fmt ) {
                        _push_to_s16_buf( 
                            out_buffer_state->buffer, 
                            (int16_t*)node_state->playback_buf, 
                            node_state->period_len,
                            node_state->playback_chan_count
                        );
                        break;
                    case SND_PCM_FORMAT_S32:
                        _push_to_s32_buf( 
                            out_buffer_state->buffer, 
                            (int32_t*)node_state->playback_buf, 
                            node_state->buffer_len,
                            node_state->playback_chan_count
                        );
                        break;
                }
                // push the next buffers into alsa
                int err = snd_pcm_writei( 
                    node_state->pcm_out_handle, 
                    node_state->playback_buf, 
                    node_state->period_len 
                );
            }
            if( node_state->capture_available && node_state->pcm_in_handle ) {
                // printf( "cap\n" );
                zdj_audio_buffer_node_state_t * in_buffer_state = (zdj_audio_buffer_node_state_t*)node_state->soundcard_in_buffer->state;
                switch( node_state->capture_fmt ) {
                    case SND_PCM_FORMAT_S16:
                        snd_pcm_readi( 
                            node_state->pcm_in_handle, 
                            (int16_t*)node_state->capture_buf, 
                            node_state->period_len 
                        );
                        break;
                    case SND_PCM_FORMAT_S32:
                        snd_pcm_readi( 
                            node_state->pcm_in_handle, 
                            (int32_t*)node_state->capture_buf, 
                            node_state->period_len 
                        );
                        break;
                }
                if( node_state->capture_buf == node_state->capture_buf_0 ) {
                    node_state->capture_buf = node_state->capture_buf_1;
                } else {
                    node_state->capture_buf = node_state->capture_buf_0;
                }
            }
        } else {
            // teardown alsa state
        }

        // printf( "_alsa_buffer_callback node:%p/%p cap_0:%p cap_1:%p cap:%p\n", usb_io_node, node_state, node_state->capture_buf_0, node_state->capture_buf_1, node_state->capture_buf );
    }
}

// Move samples from the node's mix buffer to the ALSA stage buffer
zdj_error_type_t zdj_usb_io_push_samples( zdj_pipeline_node_t * node ) {
    zdj_io_usb_node_state_t * node_state = (zdj_io_usb_node_state_t*)node->state;
    zdj_audio_buffer_node_state_t * out_buf_state = (zdj_audio_buffer_node_state_t*)node_state->soundcard_out_buffer->state;

}

zdj_error_type_t zdj_usb_io_pull_samples( zdj_pipeline_node_t * node ) {
    if( !node ){ return ZDJ_ERROR_OKAY; }
    // printf( "zdj_usb_io_pull_samples\n" );
    zdj_io_usb_node_state_t * node_state = (zdj_io_usb_node_state_t*)node->state;
    if( node_state && node_state->capture_buf ) {
        // printf( "pull_samples node:%p/%p cap_0:%p cap_1:%p cap:%p\n", node, node_state, node_state->capture_buf_0, node_state->capture_buf_1, node_state->capture_buf );
        zdj_audio_buffer_node_state_t * in_buffer_state = (zdj_audio_buffer_node_state_t*)node_state->soundcard_in_buffer->state;
        switch( node_state->capture_fmt ) {
            case SND_PCM_FORMAT_S16:
                _pull_from_s16_buf( 
                    (int16_t*)node_state->capture_buf,
                    node_state->capture_chan_count,
                    in_buffer_state->buffer,
                    2, // soundcard usb in is stereo for now
                    node_state->period_len
                );
                break;
            case SND_PCM_FORMAT_S32:
                _pull_from_s32_buf( 
                    (int32_t*)node_state->capture_buf, 
                    node_state->capture_chan_count,
                    in_buffer_state->buffer, 
                    2,
                    node_state->period_len
                );
                break;
        }
    }
}

zdj_error_type_t zdj_io_usb_alsa_bringup( zdj_pipeline_node_t * node ) {

}

zdj_error_type_t zdj_io_usb_alsa_teardown( zdj_pipeline_node_t * node ) {
    
}

zdj_error_type_t zdj_io_usb_alsa_start( zdj_pipeline_node_t * node ) {
    printf( "zdj_io_usb_alsa_start\n" );
    zdj_io_usb_node_state_t * node_state = (zdj_io_usb_node_state_t*)node->state;
    node_state->phase = ZDJ_IO_USB_PHASE_RUNNING;
    if( node_state->playback_available && node_state->pcm_out_handle ) {
        printf( "starting out_handle\n" );
        int err = snd_pcm_prepare( node_state->pcm_out_handle );
        printf( "prepare result: %s\n", snd_strerror( err ) );

        err = snd_pcm_writei( node_state->pcm_out_handle, (void*)node_state->playback_buf, node_state->buffer_len );

        // Disabling start, since writing buffer_len samples starts it automatically
        // err = snd_pcm_start( node_state->pcm_out_handle );
        // printf( "start result: %s\n", snd_strerror( err ) );
    }
    if( node_state->capture_available && node_state->pcm_in_handle ) {
        node_state->capture_buf_0 = calloc( 
            node_state->capture_chan_count * node_state->buffer_len,
            sizeof( float ) // this is inaccurate, but won't break for smaller sizes
        );
        node_state->capture_buf_1 = calloc( 
            node_state->capture_chan_count * node_state->buffer_len,
            sizeof( float ) // this is inaccurate, but won't break for smaller sizes
        );
        node_state->capture_buf = node_state->capture_buf_0;
        printf( "cap_0:%p cap_1:%p cap:%p\n", node_state->capture_buf_0, node_state->capture_buf_1, node_state->capture_buf );

        printf( "starting in_handle\n" );
        int err = snd_pcm_prepare( node_state->pcm_in_handle );
        printf( "prepare result: %s\n", snd_strerror( err ) );

        // err = snd_pcm_readi( node_state->pcm_in_handle, (void*)node_state->capture_buf, node_state->buffer_len );
        // printf( "start read result: %s\n", snd_strerror( err ) );
        // if( err < 0 ) {
        //     err = snd_pcm_recover( node_state->pcm_in_handle, err, false );
        //     printf( "recover result: %s\n", snd_strerror( err ) );
        // }

        int pcmreturn;
        while ( (pcmreturn = snd_pcm_readi( node_state->pcm_in_handle, (void*)node_state->capture_buf, node_state->period_len )) < 0 ) {
            snd_pcm_prepare( node_state->pcm_in_handle );
            printf( "pcmreturn: %s In buffer overrun\n", snd_strerror(pcmreturn) );
        }

        // err = snd_pcm_start( node_state->pcm_in_handle );
        // printf( "start result: %s\n", snd_strerror( err ) );
    }
}

// Only call this from the USB Admin thread.
// This is potentially slow since it has to discover the best format/sample rate options.
// Avoid calling on the Fast Soundcard thread.
zdj_error_type_t zdj_io_usb_discover_hwparams( zdj_pipeline_node_t * node, zdj_usb_device_t * device ) {
    _start_log( node );
    char log_str[ 256 ];
    sprintf( log_str, "## zdj_io_usb_discover_hwparams ##" );
    _log( node, log_str );

    // Create ALSA channels based on device config
    zdj_io_usb_node_state_t * node_state = (zdj_io_usb_node_state_t*)node->state;
    node_state->capture_available = false;
    node_state->playback_available = false;
    // if( device->has_audio_out ) {
    //     _log( node, "Reading Playback Device" );
    //     node_state->playback_available = _discover_pcm_device( 
    //         node, 
    //         device->snd_card_playback_name,
    //         &node_state->pcm_out_handle,
    //         SND_PCM_STREAM_PLAYBACK 
    //     );
    // }
    if( device->has_audio_in ) {
        _log( node, "Reading Capture Device" );
        node_state->capture_available = _discover_pcm_device( 
            node, 
            device->snd_card_capture_name,
            &node_state->pcm_in_handle,
            SND_PCM_STREAM_CAPTURE
        );
    }
    // Prep async buffer callback - use either available handle, preferring playback over capture
    if( node_state->playback_available && node_state->pcm_out_handle ) {
        sprintf( log_str, "Adding PCM callback to Playback Device" );
        _log( node, log_str );
        // alloc the alsa playback buffer
        node_state->playback_buf = calloc( 
            node_state->playback_chan_count * node_state->buffer_len,
            sizeof( float ) // this is inaccurate, but won't break for smaller sizes
        );
        // printf( "alloc playback buf: %d * %d = %d floats: %p\n", 
        //     node_state->playback_chan_count,
        //     node_state->buffer_len, 
        //     node_state->playback_chan_count * node_state->buffer_len, 
        //     node_state->playback_buf
        // );
        snd_async_add_pcm_handler( 
            &node_state->pcm_callback, node_state->pcm_out_handle, _alsa_buffer_callback, NULL 
        );
    } else if( node_state->capture_available && node_state->pcm_in_handle ) {
        _log( node, "Adding PCM callback to Capture Device" );
        
        // node_state->capture_buf = calloc( 
        //     node_state->capture_chan_count * node_state->buffer_len,
        //     sizeof( float ) // this is inaccurate, but won't break for smaller sizes
        // );

        snd_async_add_pcm_handler( &node_state->pcm_callback, node_state->pcm_in_handle, _alsa_buffer_callback, NULL );

        // snd_pcm_sw_params_t *sw_params;
        // snd_pcm_sw_params_malloc( &sw_params );
        // snd_pcm_sw_params_current( node_state->pcm_in_handle, sw_params );
        // snd_pcm_sw_params_set_start_threshold( node_state->pcm_in_handle, sw_params, node_state->period_len );
        // snd_pcm_sw_params_set_avail_min( node_state->pcm_in_handle, sw_params, node_state->period_len );
        // // snd_pcm_sw_params_set_stop_threshold( node_state->pcm_in_handle, sw_params, node_state->buffer_len * 4 );
        // snd_pcm_sw_params( node_state->pcm_in_handle, sw_params );
        // snd_pcm_sw_params_free( sw_params );

        // int err = snd_pcm_prepare( node_state->pcm_in_handle );
        // printf( "prepare result: %s\n", snd_strerror( err ) );

        // err = snd_pcm_start( node_state->pcm_in_handle );
        // printf( "start result: %s\n", snd_strerror( err ) );
    }

    if( device->has_midi_out ) { 
        // Find rawmidi stuff here?
    }

    _end_log( node );

    // Node is ready for snd_pcm_start( )
    node_state->phase = ZDJ_IO_USB_PHASE_READY;
}


// Do a dry-run of standing up ALSA hw_params for a device/stream combo.
// Store the results so we can quickly init this device on the soundcard fast thread
// without having to test all the possible combinations of format/sample rate/etc.

static bool _discover_pcm_device( 
    zdj_pipeline_node_t * node, 
    char * device_name, 
    snd_pcm_t ** pcm_handle,
    snd_pcm_stream_t stream 
) {
    zdj_io_usb_node_state_t * node_state = (zdj_io_usb_node_state_t*)node->state;

	int err;
    char err_str[ 256 ];
    char plughw_name[ 32 ];
    sprintf( plughw_name, "plug%s", device_name );
	snd_pcm_hw_params_t * hw_params;
	// Open PCM device - return false if it fails
    snd_pcm_t * disco_handle;
    printf( "pcm pre: %p\n", disco_handle );
    err = snd_pcm_open( &disco_handle, plughw_name, stream, 0 );
    if( err ) { 
        _log( node, "snd_pcm_open failed" );
        return false; 
    }

    printf( "pcm post: %p\n", disco_handle );

    // Init hardware params
    snd_pcm_hw_params_malloc( &hw_params );
    err = snd_pcm_hw_params_any( disco_handle, hw_params );
	err = snd_pcm_hw_params_set_access(
        disco_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED
    );
    if( err ) { _log( node, "snd_pcm_hw_params_set_access failed" ); return false; }

    // Discover available sample format
    // err = snd_pcm_hw_params_set_format( disco_handle, hw_params, SND_PCM_FORMAT_S32_LE );
    // if( !err ) {
    //     if( stream == SND_PCM_STREAM_PLAYBACK ) { 
    //         node_state->playback_fmt = SND_PCM_FORMAT_S32_LE; 
    //     } else if( stream == SND_PCM_STREAM_CAPTURE ) { 
    //         node_state->capture_fmt = SND_PCM_FORMAT_S32_LE; 
    //     }
    //     _log( node, "snd_pcm_hw_params_set_format: SND_PCM_FORMAT_S32_LE" );
    // } else { 
        err = snd_pcm_hw_params_set_format( disco_handle, hw_params, SND_PCM_FORMAT_S16_LE );
        if( !err ) {
            if( stream == SND_PCM_STREAM_PLAYBACK ) { 
                node_state->playback_fmt = SND_PCM_FORMAT_S16_LE; 
            } else if( stream == SND_PCM_STREAM_CAPTURE ) { 
                node_state->capture_fmt = SND_PCM_FORMAT_S16_LE; 
            }
            _log( node, "snd_pcm_hw_params_set_format: SND_PCM_FORMAT_S16_LE" );
        } else {
            // No supported sample format available
            _log( node, "snd_pcm_hw_params_set_format failed" );
            return false;
        }
    // }

    // Check channel count
    // err = snd_pcm_hw_params_set_channels( disco_handle, hw_params, 2 );
    // if( !err ) {
    //     if( stream == SND_PCM_STREAM_PLAYBACK ) { 
    //         node_state->playback_chan_count = 2; 
    //     } else if( stream == SND_PCM_STREAM_CAPTURE ) { 
    //         node_state->capture_chan_count = 2; 
    //     }
    //     _log( node, "snd_pcm_hw_params_set_channels: 2" );
    // } else { 
        err = snd_pcm_hw_params_set_channels( disco_handle, hw_params, 1 );
        if( !err ) {
            if( stream == SND_PCM_STREAM_PLAYBACK ) { 
                node_state->playback_chan_count = 1; 
            } else if( stream == SND_PCM_STREAM_CAPTURE ) { 
                node_state->capture_chan_count = 1;
            }
            _log( node, "snd_pcm_hw_params_set_channels: 1" );
        } else {
            // No supported sample format available
            _log( node, "snd_pcm_hw_params_set_channels failed" );
            return false;
        }
    // }

    // Check sample rate
    int rate = 44100;
    err = snd_pcm_hw_params_set_rate( disco_handle, hw_params, 44100, 0);
    // TODO: implement ALSA resampling?
    if( err ){ 
        // Unsupported sample rate
        _log( node, "snd_pcm_hw_params_set_rate failed" );
        return false; 
    }

    // Check period/buffer sizes
    node_state->buffer_len = ZDJ_ALSA_BUF_LEN;
	err = snd_pcm_hw_params_set_buffer_size( disco_handle, hw_params, node_state->buffer_len );
    if( !err ) {
        sprintf( err_str, "snd_pcm_hw_params_set_buffer_size: %d", node_state->buffer_len );
        _log( node, err_str );
    } else {
        _log( node, "snd_pcm_hw_params_set_buffer_size failed" );
        return false;
    }

    // Try a bunch of different period sizes in case some don't work
	node_state->period_count = ZDJ_ALSA_PERIOD_COUNT;
    node_state->period_len = node_state->buffer_len / node_state->period_count;
    err = snd_pcm_hw_params_set_period_size( disco_handle, hw_params, node_state->period_len, -1 );
    if( !err ) {
        sprintf( err_str, "snd_pcm_hw_params_set_period_size: %d", node_state->period_len );
        _log( node, err_str );
    } else {
        node_state->period_count = ZDJ_ALSA_PERIOD_COUNT * 2;
        node_state->period_len = node_state->buffer_len / node_state->period_count;
        err = snd_pcm_hw_params_set_period_size( disco_handle, hw_params, node_state->period_len, 0 );
        if( !err ) {
            sprintf( err_str, "snd_pcm_hw_params_set_period_size: %d", node_state->period_len );
            _log( node, err_str );
        } else {
            node_state->period_count = ZDJ_ALSA_PERIOD_COUNT * 4;
            node_state->period_len = node_state->buffer_len / node_state->period_count;
            err = snd_pcm_hw_params_set_period_size( disco_handle, hw_params, node_state->period_len, 0 );
            if( !err ) {
                sprintf( err_str, "snd_pcm_hw_params_set_period_size: %d", node_state->period_len );
                _log( node, err_str );
            } else {
                node_state->period_count = ZDJ_ALSA_PERIOD_COUNT * 8;
                node_state->period_len = node_state->buffer_len / node_state->period_count;
                err = snd_pcm_hw_params_set_period_size( disco_handle, hw_params, node_state->period_len, 0 );
                if( !err ) {
                    sprintf( err_str, "snd_pcm_hw_params_set_period_size: %d", node_state->period_len );
                    _log( node, err_str );
                } else {
                    _log( node, "snd_pcm_hw_params_set_period_size failed" );
                    return false;
                }
            }
        }
    }

    
    err = snd_pcm_hw_params( disco_handle, hw_params);
	if( err ) {
        _log( node, "snd_pcm_hw_params failed" );
        return false;
    }
    
    snd_pcm_hw_params_free( hw_params );

    *pcm_handle = disco_handle;

    // Success
    return true;
}

static void _push_to_s16_buf( float * source, int16_t * dest, int samples, int channels ) {
    // printf( "_push_to_s16_buf\n" );
    for( int i=0; i<samples; i++ ) {
        dest[ i * channels ] = source[ i * channels ] * INT16_MAX;
        if( channels == 2 ) {
            dest[ (i * channels)+1 ] = source[ (i * channels)+1 ] * INT16_MAX;
        }
    }
}

static void _push_to_s32_buf( float * source, int32_t * dest, int samples, int channels ) {
    
}


static void _pull_from_s16_buf( int16_t * source, int source_channels, float * dest, int dest_channels, int samples ) {
    // printf( "_pull_from_s16_buf: %p %p %d %d\n", source, dest, samples, channels );
    for( int i=0; i<samples; i++ ) {

        if( source_channels == 1 ) {
            if( dest_channels == 1 ) {
                dest[ i ] = source[ i ] / (float)INT16_MAX;
            } else if( dest_channels == 2 ) {
                dest[ i * 2 ] = source[ i ] / (float)INT16_MAX;
                dest[ (i*2) + 1 ] = source[ i ] / (float)INT16_MAX;
            }
            
        } else if( source_channels == 2 ) {
            if( dest_channels == 1 ) {
                dest[ i ] = source[ i * 2 ] / (float)INT16_MAX;
            } else if( dest_channels == 2 ) {
                dest[ i * 2 ] = source[ i * 2 ] / (float)INT16_MAX;
                dest[ (i*2) + 1 ] = source[ (i*2) + 1 ] / (float)INT16_MAX;
            }
        }
    }
}

static void _pull_from_s32_buf( int16_t * source, int source_channels, float * dest, int dest_channels, int samples ) {
    for( int i=0; i<samples; i++ ) {

        if( source_channels == 1 ) {
            if( dest_channels == 1 ) {
                dest[ i ] = source[ i ] / (float)INT32_MAX;
            } else if( dest_channels == 2 ) {
                dest[ i * 2 ] = source[ i ] / (float)INT32_MAX;
                dest[ (i*2) + 1 ] = source[ i ] / (float)INT32_MAX;
            }
            
        } else if( source_channels == 2 ) {
            if( dest_channels == 1 ) {
                dest[ i ] = source[ i * 2 ] / (float)INT32_MAX;
            } else if( dest_channels == 2 ) {
                dest[ i * 2 ] = source[ i * 2 ] / (float)INT32_MAX;
                dest[ (i*2) + 1 ] = source[ (i*2) + 1 ] / (float)INT32_MAX;
            }
        }
    }
}
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/syscall.h>

#include <SDL2/SDL2_gfxPrimitives.h>

#include <zerodj/system/m7/zdj_m7.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/system/perf/zdj_perf.h>
#include <zerodj/system/thread/zdj_thread.h>
#include <zerodj/signal/pipeline/node/audio/buffer/zdj_audio_buffer_node.h>
#include <zerodj/signal/pipeline/node/audio/io/zdj_io_node.h>
#include <zerodj/signal/soundcard/zdj_soundcard.h>
#include <zerodj/system/thread/zdj_thread.h>

static void _zdj_io_analog_node_deinit_state( zdj_pipeline_node_t * node );
void * _zdj_io_analog_fast_cycle_thread_main( void * arg );

zdj_pipeline_node_t * zdj_new_io_analog_node( void ) {
    // Make node
    zdj_pipeline_node_t * node = zdj_new_pipeline_node( );
    node->deinit_state = &_zdj_io_analog_node_deinit_state;

    zdj_io_analog_node_state_t * state = calloc( 1, sizeof( zdj_io_analog_node_state_t ) );
    node->state = state;

    int mem_fd = open( "/dev/mem", O_RDWR );
    state->shared_audio_state = (zdj_shared_audio_state_t*)mmap(0, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, ZDJ_SHARED_AUDIO_STATE_ADDR);
    state->shared_adc_buffer = (volatile int32_t*)mmap(0, 0x8000, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, ZDJ_SHARED_ADC_BUF);
    state->shared_dac_buffer = (int32_t*)mmap(0, 0x8000, PROT_READ|PROT_WRITE, MAP_SHARED, mem_fd, ZDJ_SHARED_DAC_BUF);
    close( mem_fd );

    state->in_1_buffer = zdj_new_audio_buffer_node( 
        ZDJ_SOUNDCARD_BUF_LEN, ZDJ_AUDIO_BUFFER_STEREO 
    );
    state->in_2_buffer = zdj_new_audio_buffer_node( 
        ZDJ_SOUNDCARD_BUF_LEN, ZDJ_AUDIO_BUFFER_STEREO 
    );
    state->out_1_buffer = zdj_new_audio_buffer_node( 
        ZDJ_SOUNDCARD_BUF_LEN, ZDJ_AUDIO_BUFFER_STEREO 
    );
    state->out_2_buffer = zdj_new_audio_buffer_node( 
        ZDJ_SOUNDCARD_BUF_LEN, ZDJ_AUDIO_BUFFER_STEREO 
    );
    
    return node;
}

zdj_error_type_t zdj_io_analog_configure( zdj_pipeline_node_t * node ) {
    zdj_io_analog_node_state_t * node_state = (zdj_io_analog_node_state_t*)node->state;
    printf( "zdj_io_analog_configure\n" );
    // Write inputs to shared buffer_state struct
    node_state->shared_audio_state->buffer_len = ZDJ_SOUNDCARD_BUF_LEN;

    node_state->shared_audio_state->dac_cfg_gain_boost_0 = 0;
    node_state->shared_audio_state->dac_cfg_gain_boost_1 = 0;
    node_state->shared_audio_state->dac_cfg_gain_boost_2 = 0;
    node_state->shared_audio_state->dac_cfg_gain_boost_3 = 0;
    node_state->shared_audio_state->dac_cfg_volume_0 = 0x30;
    node_state->shared_audio_state->dac_cfg_volume_1 = 0x30;
    node_state->shared_audio_state->dac_cfg_volume_2 = 0x30;
    node_state->shared_audio_state->dac_cfg_volume_3 = 0x30;

    // Write cfg_dac request to shared buffer_state struct
    zdj_m7_shared_msg_buffer( )->update_audio_cfg_req = true;

    // Resize window based on buffer_len
    // zdj_pipeline_window_state_resize( 
    //     node->window_state,
    //     0,
    //     node_state->shared_audio_state->buffer_len
    // );

    return ZDJ_ERROR_OKAY;
}

zdj_error_type_t zdj_io_analog_run( zdj_pipeline_node_t * node ) {
    zdj_io_analog_node_state_t * state = (zdj_io_analog_node_state_t *)node->state;
    state->running = true;
    zdj_m7_shared_msg_buffer( )->activate_audio_req = true;

    // Start update thread
    zdj_thread_launch_audio_buf_cycle( &_zdj_io_analog_fast_cycle_thread_main, node );

}

zdj_error_type_t zdj_io_analog_stop( zdj_pipeline_node_t * node ) {
    zdj_io_analog_node_state_t * state = (zdj_io_analog_node_state_t *)node->state;
    state->running = false;
    zdj_m7_shared_msg_buffer( )->deactivate_audio_req = true;
}

zdj_error_type_t zdj_io_analog_silence( zdj_pipeline_node_t * node ) {
    zdj_io_analog_node_state_t * state = (zdj_io_analog_node_state_t *)node->state;
    for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
        state->shared_dac_buffer[ i*4+0 ] = 0;
        state->shared_dac_buffer[ i*4+1 ] = 0;
        state->shared_dac_buffer[ i*4+2 ] = 0;
        state->shared_dac_buffer[ i*4+3 ] = 0;
    }
}

// Transform samples from soundcard audio buffers to shared M7 buffer
zdj_error_type_t zdj_analog_io_push_samples( zdj_pipeline_node_t * node ) {
    zdj_io_analog_node_state_t * state = (zdj_io_analog_node_state_t *)node->state;
    zdj_audio_buffer_node_state_t * out_1_state = (zdj_audio_buffer_node_state_t*)state->out_1_buffer->state;
    zdj_audio_buffer_node_state_t * out_2_state = (zdj_audio_buffer_node_state_t*)state->out_2_buffer->state;

    double xfrm = 0;
    for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
        xfrm = (double)out_1_state->buffer[ i*2+0 ] * INT32_MAX;
        state->shared_dac_buffer[ i*4+0 ] = (int32_t)xfrm;
        xfrm = (double)out_1_state->buffer[ i*2+1 ] * INT32_MAX;
        state->shared_dac_buffer[ i*4+1 ] = (int32_t)xfrm;
        xfrm = (double)out_2_state->buffer[ i*2+0 ] * INT32_MAX;
        state->shared_dac_buffer[ i*4+2 ] = (int32_t)xfrm;
        xfrm = (double)out_2_state->buffer[ i*2+1 ] * INT32_MAX;
        state->shared_dac_buffer[ i*4+3 ] = (int32_t)xfrm;
    }
}

// Transform samples from shared M7 buffer to soundcard audio buffers
zdj_error_type_t zdj_analog_io_pull_samples( zdj_pipeline_node_t * node ) {
    zdj_io_analog_node_state_t * state = (zdj_io_analog_node_state_t *)node->state;
    zdj_audio_buffer_node_state_t * in_1_state = (zdj_audio_buffer_node_state_t*)state->in_1_buffer->state;
    zdj_audio_buffer_node_state_t * in_2_state = (zdj_audio_buffer_node_state_t*)state->in_2_buffer->state;

    // Add system here to pull data into individual mono channels based on node linkage.
    double INT24_MAX = 8388607;
    double xfrm = 0;
    for( int i=0; i<ZDJ_SOUNDCARD_BUF_LEN; i++ ) {
        xfrm = (double)((int32_t)state->shared_adc_buffer[ i*8+1 ] >> 8) / INT24_MAX;
        in_1_state->buffer[ i*2+0 ] = (float)xfrm;
        xfrm = (double)((int32_t)state->shared_adc_buffer[ i*8+2 ] >> 8) / INT24_MAX;
        in_1_state->buffer[ i*2+1 ] = (float)xfrm;
        xfrm = (double)((int32_t)state->shared_adc_buffer[ i*8+3 ] >> 8) / INT24_MAX;
        in_2_state->buffer[ i*2+0 ] = (float)xfrm;
        xfrm = (double)((int32_t)state->shared_adc_buffer[ i*8+4 ] >> 8) / INT24_MAX;
        in_2_state->buffer[ i*2+1 ] = (float)xfrm;
    }

    // printf( "zdj_analog_io_pull_samples: %f\n", in_1_state->buffer[ 2 ] );
}

void _zdj_io_analog_node_deinit_state( zdj_pipeline_node_t * node ) {

}

////////////////////////////
// Move this to soundcard //
////////////////////////////

// This thread checks for the cycle_ready flag in the shared audio state.
// It sleeps, then periodically wakes to check flag then goes back to sleep.
// The sleep cycle is asycnhronous in relation to the M7 cores ADC/DAC update cycles.

// Ensure that you check cycle_ready often enough that you can fill
// the shared ADC/DAC back buffers before the M7 core needs them.
void * _zdj_io_analog_fast_cycle_thread_main( void * arg ) {
    zdj_pipeline_node_t * node = (zdj_pipeline_node_t *)arg;
    // printf( "_zdj_io_analog_fast_cycle_thread_main: %p\n", node );
    zdj_io_analog_node_state_t * node_state = (zdj_io_analog_node_state_t *)node->state;


    // Set up scheduling
    int prio = sched_get_priority_max( SCHED_RR );
	struct sched_param param;
	param.sched_priority = prio;
	sched_setscheduler( syscall(SYS_gettid), SCHED_RR, &param );

    // Give realtime scheduler access to 100% of core time
	system( "echo -1 >/proc/sys/kernel/sched_rt_runtime_us" );

    // Set core affinity to Core #1;
    cpu_set_t cpuset;
	CPU_ZERO( &cpuset );
	CPU_SET( 1,&cpuset ); // Fast cycle dedicated to CPU #1
	int err = sched_setaffinity( syscall(SYS_gettid), sizeof(cpu_set_t), &cpuset );
    if( err != 0 ) {
        perror( "set affinity failed" );
    }

    


    float cycle_sec = (float)(node_state->shared_audio_state->buffer_len) / 44100.0f;
    long cycle_nano = (long)(cycle_sec * 1000000000);

    // Check for cycle_ready ~ 8 times per cycle
    // Note that this is async w/M7 core so actual timing will be arbitrary.
    long cycle_time = cycle_nano / 8.0; 
    // struct timespec cycle_delay = { 0, cycle_time };
    struct timespec cycle_delay = { 0, 50 };


    while( 1 ) {
        // Check for cycle_ready
        // cycle_ready indicates that there is a buffer of ADC samples available, 
        // and a buffer of DAC samples is needed.
        // On cycle_ready
        if( node_state->shared_audio_state->cycle_ready ) {
            // De-assert cycle_ready to let M7 core know we got it.
            node_state->shared_audio_state->cycle_ready = false;

            // tag a cycle catch
            node_state->shared_audio_state->miss_count--;

            // Don't spend a ton of time in the CB.  
            // You want to be done before the next cycle_ready assert.
            if( node->update_cb ) { 
                if( zdj_perf_enabled( ) ) {
                    zdj_perf_tag_t * tag = zdj_new_perf_tag_for_thread( ZDJ_SYSTEM_THREAD_AUDIO_BUF );
                    tag->name = ZDJ_PERF_TAG_AUDIO_BUF_CYCLE;
                    tag->start = zdj_perf_time( );
                    node->update_cb( node );
                    tag->end = zdj_perf_time( );
                } else {
                    node->update_cb( node );
                }
            }
        } 

        // Sleep thread until next check
        // nanosleep( &cycle_delay, NULL );

        // Exit thread on command
        if( node_state && node_state->running == false ) { 
            // printf( "exiting thread\n" );
            return NULL; 
        }
    }

    return NULL;
}
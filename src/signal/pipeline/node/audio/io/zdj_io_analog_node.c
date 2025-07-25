#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>

#include <zerodj/m7/zdj_m7.h>
#include <zerodj/signal/pipeline/zdj_pipeline.h>
#include <zerodj/signal/pipeline/node/audio/io/zdj_io_node.h>

static void _zdj_io_analog_node_deinit_state( zdj_pipeline_node_t * node );
void * _zdj_io_analog_thread_main( void * arg );


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
    
    return node;
}

zdj_error_type_t zdj_io_analog_configure( zdj_pipeline_node_t * node ) {
    zdj_io_analog_node_state_t * node_state = (zdj_io_analog_node_state_t*)node->state;
    // printf( "zdj_io_analog_configure\n" );
    // Write inputs to shared buffer_state struct
    node_state->shared_audio_state->buffer_len = 256;

    node_state->shared_audio_state->dac_cfg_gain_boost_0 = 0;
    node_state->shared_audio_state->dac_cfg_gain_boost_1 = 0;
    node_state->shared_audio_state->dac_cfg_gain_boost_2 = 0;
    node_state->shared_audio_state->dac_cfg_gain_boost_3 = 0;
    node_state->shared_audio_state->dac_cfg_volume_0 = 0x30;
    node_state->shared_audio_state->dac_cfg_volume_1 = 0x30;
    node_state->shared_audio_state->dac_cfg_volume_2 = 0x30;
    node_state->shared_audio_state->dac_cfg_volume_3 = 0x30;

    // printf( "shared audio state: %d %d %lu %u %d %d\n",
    //     node_state->shared_audio_state->cycle_ready,
    //     node_state->shared_audio_state->buffer_len,
    //     node_state->shared_audio_state->cycle_count,
    //     node_state->shared_audio_state->miss_count,
    //     node_state->shared_audio_state->dac_cfg_gain_boost_0,
    //     node_state->shared_audio_state->dac_cfg_gain_boost_1
    // );

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

    printf( "zdj_io_analog_run\n" );
    zdj_m7_shared_msg_buffer( )->activate_audio_req = true;

    // Start update thread
    node->thread = malloc( sizeof( pthread_t ) );
    pthread_create( node->thread, NULL, _zdj_io_analog_thread_main, node );
}

zdj_error_type_t zdj_io_analog_stop( zdj_pipeline_node_t * node ) {
    zdj_io_analog_node_state_t * state = (zdj_io_analog_node_state_t *)node->state;

    printf( "zdj_io_analog_stop\n" );
    zdj_m7_shared_msg_buffer( )->deactivate_audio_req = true;
}

void _zdj_io_analog_node_deinit_state( zdj_pipeline_node_t * node ) {

}

// This thread checks for the cycle_ready flag in the shared audio state.
// It sleeps, then periodically wakes to check flag then goes back to sleep.
// The sleep cycle is asycnhronous in relation to the M7 cores ADC/DAC update cycles.

// Ensure that you check cycle_ready often enough that you can fill
// the shared ADC/DAC back buffers before the M7 core needs them.
void * _zdj_io_analog_thread_main( void * arg ) {
    zdj_pipeline_node_t * node = (zdj_pipeline_node_t *)arg;
    // printf( "_zdj_io_analog_thread_main: %p\n", node );
    zdj_io_analog_node_state_t * node_state = (zdj_io_analog_node_state_t *)node->state;


    float cycle_sec = (float)(node_state->shared_audio_state->buffer_len) / 44100.0f;
    long cycle_nano = (long)(cycle_sec * 1000000000);

    // Check for cycle_ready ~ 8 times per cycle
    // Note that this is async w/M7 core so actual timing will be arbitrary.
    long cycle_time = cycle_nano / 8.0; 
    struct timespec cycle_delay = { 0, cycle_time };

    while( 1 ) {
        // Check for cycle_ready
        // cycle_ready indicates that there is a buffer of ADC samples available, 
        // and a buffer of DAC samples is needed.
        // On cycle_ready
        if( node_state->shared_audio_state->cycle_ready ) {
            // De-assert cycle_ready to let M7 core know we got it.
            node_state->shared_audio_state->cycle_ready = false;

            // tag a cycle catch
            if( node_state->shared_audio_state->miss_count > 0 ) {
                node_state->shared_audio_state->miss_count--;
            }

            // Don't spend a ton of time in the CB.  
            // You want to be done before the next cycle_ready assert.
            if( node->update_cb ) { node->update_cb( node ); }
        } 

        // Sleep thread until next check
        nanosleep( &cycle_delay, NULL );

        // Exit thread on command
        // if( !node_state->shared_audio_state->running ) { return NULL; }
    }

    return NULL;
}
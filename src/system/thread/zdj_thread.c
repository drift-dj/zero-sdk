#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sched.h>
#include <pthread.h>

#include <zerodj/system/error/zdj_error.h>
#include <zerodj/system/thread/zdj_thread.h>

pthread_t _zdj_thread_control;
pthread_t _zdj_thread_audio_buf;
pthread_t _zdj_thread_ui;
pthread_t _zdj_thread_deck_manager;

pthread_t _zdj_thread_deck_station_1;
bool zdj_thread_deck_1_station_available = true;

pthread_t _zdj_thread_deck_station_2;
bool zdj_thread_deck_2_station_available = true;

pthread_t _zdj_thread_deck_station_ext;
bool zdj_thread_deck_ext_station_available = true;

pthread_t _zdj_thread_record_post_proc;

zdj_error_type_t zdj_thread_launch_control_cycle( zdj_thread_main main, void * arg ) {
    pthread_create( &_zdj_thread_control, NULL, main, arg );
}

zdj_error_type_t zdj_thread_launch_ui_cycle( zdj_thread_main main, void * arg ) {
    pthread_create( &_zdj_thread_ui, NULL, main, arg );
}

zdj_error_type_t zdj_thread_launch_audio_buf_cycle( zdj_thread_main main, void * arg ) {
    pthread_create( &_zdj_thread_audio_buf, NULL, main, arg );

    // // Set up scheduling
    // int prio = sched_get_priority_max( SCHED_FIFO );
	// struct sched_param param;
	// param.sched_priority = prio;
	// sched_setscheduler( _zdj_thread_audio_buf, SCHED_FIFO, &param );

    // // Give realtime scheduler access to 100% of core time
	// // system( "echo -1 >/proc/sys/kernel/sched_rt_runtime_us" );

    // // Set core affinity to Core #1;
    // cpu_set_t cpuset;
	// CPU_ZERO( &cpuset );
	// CPU_SET( 1,&cpuset ); // Fast cycle dedicated to CPU #1
	// int err = sched_setaffinity( _zdj_thread_audio_buf, sizeof(cpu_set_t), &cpuset );
    // if( err != 0 ) {
    //     perror( "set affinity failed" );
    // }

    // syscall(SYS_gettid)
}

zdj_error_type_t zdj_thread_launch_deck_manager_cycle( zdj_thread_main main, void * arg ) {
    pthread_create( &_zdj_thread_deck_manager, NULL, main, arg );
}

zdj_error_type_t zdj_thread_launch_deck_station_1_cycle( zdj_thread_main main, void * arg ) {
    pthread_create( &_zdj_thread_deck_station_1, NULL, main, arg );
}

zdj_error_type_t zdj_thread_launch_deck_station_2_cycle( zdj_thread_main main, void * arg ) {
    pthread_create( &_zdj_thread_deck_station_2, NULL, main, arg );
}

zdj_error_type_t zdj_thread_launch_deck_station_ext_cycle( zdj_thread_main main, void * arg ) {
    pthread_create( &_zdj_thread_deck_station_ext, NULL, main, arg );
}

zdj_error_type_t zdj_thread_launch_record_post_proc_cycle( zdj_thread_main main, void * arg ) {
    pthread_create( &_zdj_thread_record_post_proc, NULL, main, arg );
}
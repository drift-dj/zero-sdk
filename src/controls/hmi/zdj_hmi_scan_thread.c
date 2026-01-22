#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>

#include <zerodj/controls/hmi/zdj_hmi_input.h>
#include <zerodj/controls/hmi/zdj_hmi_m7_state_model.h>
#include <zerodj/system/perf/zdj_perf.h>

typedef struct {
    int32_t cursam;
    int32_t presam;
    int32_t upval; // increment/decrement vals are collected here until cleared during flush
} zdj_hmi_enco_scan_model_t;

// volatile bool hmi_scan_thread_flush;

static int a0_fd;
static int a1_fd;
static int a2_fd;
static int a_fd;
static int b_fd;
static int sw_fd;

static char a_val[1];
static char b_val[1];
static char sw_val[1];
static int a_vals[8];
static int b_vals[8];
static int sw_vals[8];
static int quad_mat [16] = {0,-1,1,0,1,0,0,-1,-1,0,0,1,0,1,-1,0};

static zdj_hmi_enco_scan_model_t * enco_1_vol;
static zdj_hmi_enco_scan_model_t * enco_2_jog;
static zdj_hmi_enco_scan_model_t * enco_3_t1;
static zdj_hmi_enco_scan_model_t * enco_4_t2;
static zdj_hmi_enco_scan_model_t * enco_5_t3;

void zdj_control_prepare_hmi_input_scan( void ) {
    // Setup GPIO bitbang for encoders
    int fd = open( "/sys/class/gpio/export", O_WRONLY );
    if ( fd == -1 ) { printf( "Unable to open GPIO export sysfs\n" ); }
    write( fd, "12", 2 );
    write( fd, "8", 2 );
    write( fd, "10", 2 );
    write( fd, "9", 2 );
    write( fd, "6", 2 );
    write( fd, "15", 2 );
    close( fd );

    fd = open( "/sys/class/gpio/gpio12/direction", O_WRONLY );
    write( fd, "out", 3 );
    close( fd );
    fd = open( "/sys/class/gpio/gpio8/direction", O_WRONLY );
    write( fd, "out", 3 );
    close( fd );
    fd = open( "/sys/class/gpio/gpio10/direction", O_WRONLY );
    write( fd, "out", 3 );
    close( fd );
    fd = open( "/sys/class/gpio/gpio9/direction", O_WRONLY );
    write( fd, "in", 2 );
    close( fd );
    fd = open( "/sys/class/gpio/gpio6/direction", O_WRONLY );
    write( fd, "in", 2 );
    close( fd );
    fd = open( "/sys/class/gpio/gpio15/direction", O_WRONLY );
    write( fd, "in", 2 );
    close( fd );

    a0_fd = open( "/sys/class/gpio/gpio12/value", O_WRONLY );
    a1_fd = open( "/sys/class/gpio/gpio8/value", O_WRONLY );
    a2_fd = open( "/sys/class/gpio/gpio10/value", O_WRONLY );
    a_fd = open( "/sys/class/gpio/gpio9/value", O_RDONLY );
    b_fd = open( "/sys/class/gpio/gpio6/value", O_RDONLY );
    sw_fd = open( "/sys/class/gpio/gpio15/value", O_RDONLY );

    enco_1_vol = calloc( 1, sizeof( zdj_hmi_enco_scan_model_t ) );
    enco_2_jog = calloc( 1, sizeof( zdj_hmi_enco_scan_model_t ) );
    enco_3_t1 = calloc( 1, sizeof( zdj_hmi_enco_scan_model_t ) );
    enco_4_t2 = calloc( 1, sizeof( zdj_hmi_enco_scan_model_t ) );
    enco_5_t3 = calloc( 1, sizeof( zdj_hmi_enco_scan_model_t ) );
}

// Scan HMI encoders in the background
// void * hmi_input_scan_thread_main( void * arg ) {
void zdj_control_scan_hmi_input( void ) {
    struct timespec settle_sleep = { 0, 1 }; // ~100 µsec

    write( a0_fd, "0", 1 );
    write( a1_fd, "0", 1 );
    write( a2_fd, "0", 1 );

    nanosleep( &settle_sleep, NULL );

    lseek(a_fd, 0, SEEK_SET);
    read(a_fd, &a_val, 1);
    lseek(b_fd, 0, SEEK_SET);
    read(b_fd, &b_val, 1);
    lseek(sw_fd, 0, SEEK_SET);
    read(sw_fd, &sw_val, 1);

    enco_5_t3->cursam = (a_val[0]-48) * 2 + (b_val[0]-48);
    enco_5_t3->upval += quad_mat[ enco_5_t3->presam * 4 + enco_5_t3->cursam ];
    enco_5_t3->presam = enco_5_t3->cursam;
    zdj_hmi_m7_state_model->tone_3_pb_state = !(sw_val[0]-48);

    // Address 1 - 001
    write( a0_fd, "1", 1 );
    write( a1_fd, "0", 1 );
    write( a2_fd, "0", 1 );

    // Sleep while GPIO settles
    nanosleep( &settle_sleep, NULL );

    lseek(a_fd, 0, SEEK_SET);
    read(a_fd, &a_val, 1);
    lseek(b_fd, 0, SEEK_SET);
    read(b_fd, &b_val, 1);
    lseek(sw_fd, 0, SEEK_SET);
    read(sw_fd, &sw_val, 1);

    enco_2_jog->cursam = (a_val[0]-48) * 2 + (b_val[0]-48);
    enco_2_jog->upval += quad_mat[ enco_2_jog->presam * 4 + enco_2_jog->cursam ];
    enco_2_jog->presam = enco_2_jog->cursam;
    zdj_hmi_m7_state_model->jog_pb_state = !(sw_val[0]-48);

    // Address 2 - 010
    write( a0_fd, "0", 1 );
    write( a1_fd, "1", 1 );
    write( a2_fd, "0", 1 );

    // Sleep while GPIO settles
    nanosleep( &settle_sleep, NULL );

    lseek(a_fd, 0, SEEK_SET);
    read(a_fd, &a_val, 1);
    lseek(b_fd, 0, SEEK_SET);
    read(b_fd, &b_val, 1);
    lseek(sw_fd, 0, SEEK_SET);
    read(sw_fd, &sw_val, 1);

    enco_1_vol->cursam = (a_val[0]-48) * 2 + (b_val[0]-48);
    enco_1_vol->upval += quad_mat[ enco_1_vol->presam * 4 + enco_1_vol->cursam ];
    enco_1_vol->presam = enco_1_vol->cursam;
    zdj_hmi_m7_state_model->out_pb_state = !(sw_val[0]-48);


    // Address 3 - 011
    write( a0_fd, "1", 1 );
    write( a1_fd, "1", 1 );
    write( a2_fd, "0", 1 );

    // Sleep while GPIO settles
    nanosleep( &settle_sleep, NULL );

    lseek(a_fd, 0, SEEK_SET);
    read(a_fd, &a_val, 1);
    lseek(b_fd, 0, SEEK_SET);
    read(b_fd, &b_val, 1);
    lseek(sw_fd, 0, SEEK_SET);
    read(sw_fd, &sw_val, 1);

    zdj_hmi_m7_state_model->fn_1_pb_state = !(a_val[0]-48);
    zdj_hmi_m7_state_model->fn_2_pb_state = !(b_val[0]-48);
    zdj_hmi_m7_state_model->fn_3_pb_state = !(sw_val[0]-48);


    // Address 4 - 100
    write( a0_fd, "0", 1 );
    write( a1_fd, "0", 1 );
    write( a2_fd, "1", 1 );

    // Sleep while GPIO settles
    nanosleep( &settle_sleep, NULL );

    lseek(a_fd, 0, SEEK_SET);
    read(a_fd, &a_val, 1);
    lseek(b_fd, 0, SEEK_SET);
    read(b_fd, &b_val, 1);
    lseek(sw_fd, 0, SEEK_SET);
    read(sw_fd, &sw_val, 1);

    enco_3_t1->cursam = (a_val[0]-48) * 2 + (b_val[0]-48);
    enco_3_t1->upval += quad_mat[ enco_3_t1->presam * 4 + enco_3_t1->cursam ];
    enco_3_t1->presam = enco_3_t1->cursam;
    zdj_hmi_m7_state_model->tone_1_pb_state = !(sw_val[0]-48);


    // Address 6 - 110
    write( a0_fd, "0", 1 );
    write( a1_fd, "1", 1 );
    write( a2_fd, "1", 1 );

    // Sleep while GPIO settles
    nanosleep( &settle_sleep, NULL );

    lseek(a_fd, 0, SEEK_SET);
    read(a_fd, &a_val, 1);
    lseek(b_fd, 0, SEEK_SET);
    read(b_fd, &b_val, 1);
    lseek(sw_fd, 0, SEEK_SET);
    read(sw_fd, &sw_val, 1);

    enco_4_t2->cursam = (a_val[0]-48) * 2 + (b_val[0]-48);
    enco_4_t2->upval += quad_mat[ enco_4_t2->presam * 4 + enco_4_t2->cursam ];
    enco_4_t2->presam = enco_4_t2->cursam;
    zdj_hmi_m7_state_model->tone_2_pb_state = !(sw_val[0]-48);

    // Address 7 - 111
    write( a0_fd, "1", 1 );
    write( a1_fd, "1", 1 );
    write( a2_fd, "1", 1 );

    // Sleep while GPIO settles
    nanosleep( &settle_sleep, NULL );

    lseek(a_fd, 0, SEEK_SET);
    read(a_fd, &a_val, 1);
    lseek(b_fd, 0, SEEK_SET);
    read(b_fd, &b_val, 1);
    lseek(sw_fd, 0, SEEK_SET);
    read(sw_fd, &sw_val, 1);
    
    zdj_hmi_m7_state_model->hotcue_pb_state = !(a_val[0]-48);
    zdj_hmi_m7_state_model->play_pb_state = !(b_val[0]-48);
    zdj_hmi_m7_state_model->nav_pb_state = !(sw_val[0]-48);

    // printf( "%d, %d, %d\n",
    //     zdj_hmi_m7_state_model->hotcue_pb_state,
    //     zdj_hmi_m7_state_model->play_pb_state,
    //     zdj_hmi_m7_state_model->nav_pb_state
    // );

    zdj_hmi_m7_state_model->out_state = enco_1_vol->upval;
    zdj_hmi_m7_state_model->jog_state = enco_2_jog->upval;
    zdj_hmi_m7_state_model->tone_1_state = enco_3_t1->upval;
    zdj_hmi_m7_state_model->tone_2_state = enco_4_t2->upval;
    zdj_hmi_m7_state_model->tone_3_state = enco_5_t3->upval;
    enco_1_vol->upval = 0;
    enco_2_jog->upval = 0;
    enco_3_t1->upval = 0;
    enco_4_t2->upval = 0;
    enco_5_t3->upval = 0;
}
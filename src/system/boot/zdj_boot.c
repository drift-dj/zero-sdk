#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/mman.h>

#include <zerodj/system/boot/zdj_boot.h>
#include <zerodj/health/zdj_health_type.h>
#include <zerodj/controls/hmi/zdj_hmi.h>
#include <zerodj/controls/hmi/zdj_hmi_input.h>
#include <zerodj/controls/hmi/zdj_hmi_m7_state_model.h>

void zdj_boot_scan_hmi( void ) {

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

    int a0_fd = open( "/sys/class/gpio/gpio12/value", O_WRONLY );
    int a1_fd = open( "/sys/class/gpio/gpio8/value", O_WRONLY );
    int a2_fd = open( "/sys/class/gpio/gpio10/value", O_WRONLY );
    int a_fd = open( "/sys/class/gpio/gpio9/value", O_RDONLY );
    int b_fd = open( "/sys/class/gpio/gpio6/value", O_RDONLY );
    int sw_fd = open( "/sys/class/gpio/gpio15/value", O_RDONLY );

    char a_val[1];
    char b_val[1];
    char sw_val[1];
    int a_vals[8];
    int b_vals[8];
    int sw_vals[8];

    // Sleep after writing hi/lo vals to GPIO address lines.
    // Required to give the OS+IMX8M enough time to (de)assert, 
    // and to give the mux chip enough time to react.
    struct timespec settle_sleep = { 0, 100000 }; // ~100 µsec


    // Bitbang the GPIO address lines and read values from the GPIO input lines
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


    zdj_hmi_m7_state_model->fn_1_pb_state = a_val[0]-48;
    zdj_hmi_m7_state_model->fn_2_pb_state = b_val[0]-48;
    zdj_hmi_m7_state_model->fn_3_pb_state = sw_val[0]-48;

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
    
    zdj_hmi_m7_state_model->hotcue_pb_state = a_val[0]-48;
    zdj_hmi_m7_state_model->play_pb_state = b_val[0]-48;
    zdj_hmi_m7_state_model->nav_pb_state = sw_val[0]-48;


    close( a0_fd );
    close( a1_fd );
    close( a2_fd );
    close( a_fd );
    close( b_fd );
    close( sw_fd );
}

zdj_boot_mode_t zdj_boot_get_normal_override( void ) {
    zdj_boot_mode_t mode = BOOT_MODE_NORMAL;
    if( access( "/etc/registry/boot/normal_override", F_OK ) == 0 ) {
        zdj_normal_boot_override_t normal_mode_override;
        FILE * fd = fopen( "/etc/registry/boot/normal_override", "r" );
        fread( &normal_mode_override, sizeof( zdj_normal_boot_override_t ), 1, fd );
        fclose( fd );
        mode = normal_mode_override.mode;
    }
    return mode;
}

zdj_health_status_t zdj_boot_set_normal_override( zdj_boot_mode_t mode ) {
    zdj_normal_boot_override_t normal_mode_override = { mode };
    FILE * fd = fopen( "/etc/registry/boot/normal_override", "w" );
    fwrite( &normal_mode_override, sizeof( zdj_normal_boot_override_t ), 1, fd );
    fclose( fd );
    return ZDJ_HEALTH_STATUS_OKAY;
}
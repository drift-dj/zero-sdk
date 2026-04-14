#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include <zerodj/system/battery/zdj_battery.h>

static void * _update_battery( void * arg );

pthread_t batt_update_thread;
zdj_battery_state_t * _zdj_battery_state = NULL;

zdj_battery_state_t * zdj_battery_state( void ) {
    if( !_zdj_battery_state ) { 

        // Lazily bringup the battery module
        // modprobe max17040_battery
        system( "modprobe max17040_battery" );
        // echo -n "max17040" 0x36 > /sys/bus/i2c/devices/i2c-0/new_device
        system( "echo -n \"max17040\" 0x36 > /sys/bus/i2c/devices/i2c-0/new_device" );

        _zdj_battery_state = calloc( 1, sizeof( zdj_battery_state_t ) );
        _zdj_battery_state->update_thread = malloc( sizeof( pthread_t ) );
        pthread_create( 
            _zdj_battery_state->update_thread, NULL, _update_battery, _zdj_battery_state 
        );
    }
    return _zdj_battery_state;
}

static void * _update_battery( void * arg ) {
    if( !arg ){ return NULL; }
    zdj_battery_state_t * state = (zdj_battery_state_t*)arg;

    char str[ 8 ];
    state->fd = fopen("/sys/class/power_supply/battery/capacity", "r");

    while( 1 ) {
        if( state->fd ) {
            fseek( state->fd, 0, SEEK_SET );
            fread( str, 8, 1, state->fd );
            state->valid = true;
            state->charge_val = atoi( str );
            state->charge_pct = (double)state->charge_val / 100.0f;
            // printf( "batt_update: %d %1.1f\n", state->charge_val, state->charge_pct );
        } else {
            state->fd = fopen("/sys/class/power_supply/battery/capacity", "r");
        }
        // Sleep thread until next sample
        sleep( 20 );
    }
}
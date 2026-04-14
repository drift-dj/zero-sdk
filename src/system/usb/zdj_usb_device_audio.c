#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <uuid.h>

#include <alsa/asoundlib.h>

#include <zerodj/system/usb/zdj_usb.h>


void zdj_usb_scan_attached_alsa_devices( zdj_usb_state_t * state ) {

    // Read subdirs of /sys/class/sound/ to get:
    // output (name ends in 'p')
    // input (name ends in 'c')
    // midi (name begins with 'midi')




    int card = -1;
    snd_card_next( &card ); // Get first card
    while ( card >= 0 ) {
        char ctl_name[ 32 ];
        sprintf( ctl_name, "hw:%d", card );
        snd_ctl_t *ctl;
        
        if ( snd_ctl_open( &ctl, ctl_name, 0 ) >= 0 ) {
            int device = -1;
            while ( snd_ctl_rawmidi_next_device( ctl, &device ) >= 0 && device >= 0 ) {
                snd_rawmidi_info_t *info;
                snd_rawmidi_info_alloca( &info );
                snd_rawmidi_info_set_device( info, device );
                
                // Check both input and output streams
                snd_rawmidi_info_set_stream( info, SND_RAWMIDI_STREAM_OUTPUT );
                if ( snd_ctl_rawmidi_info( ctl, info ) >= 0) {
                    printf( "Card %d, Device %d: %s (%s)\n", 
                           card, device, snd_rawmidi_info_get_name( info ), ctl_name );
                }
            }
            snd_ctl_close( ctl );
        }
        snd_card_next( &card ); // Next card
    }
}
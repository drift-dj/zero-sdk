#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <uuid.h>

#include <alsa/asoundlib.h>

#include <zerodj/system/usb/zdj_usb.h>

void zdj_usb_update_alsa_profiles( zdj_usb_state_t * state, zdj_usb_device_t * device ) {

    printf( "zdj_usb_scan_attached_alsa_devices\n" );
    // Read subdirs of /sys/class/sound/ to get:
    // output (name ends in 'p')
    // input (name ends in 'c')
    // midi (name begins with 'midi')




    int card = -1;
    snd_card_next( &card ); // Get first card
    while ( card >= 0 ) {
        char ctl_name[ 32 ];
        sprintf( ctl_name, "hw:%d", card );
        snd_ctl_t * ctl;
        snd_ctl_card_info_t * info;
        
        printf( "checking %s\n", ctl_name );
        if ( snd_ctl_open( &ctl, ctl_name, 0 ) >= 0 ) {
            int device_num = -1;
            printf( "checking for midi...\n" );
            while ( snd_ctl_rawmidi_next_device( ctl, &device_num ) >= 0 && device_num >= 0 ) {
                printf( "rawmidi device\n" );
                snd_rawmidi_info_t *info;
                snd_rawmidi_info_alloca( &info );
                snd_rawmidi_info_set_device( info, device_num );
                
                // Check both input and output streams
                snd_rawmidi_info_set_stream( info, SND_RAWMIDI_STREAM_OUTPUT );
                if ( snd_ctl_rawmidi_info( ctl, info ) >= 0) {
                    device->has_midi = true;
                    device->has_midi_out = true;
                    printf( "Midi Out Card %d, Device %d: %s (%s)\n", 
                           card, device_num, snd_rawmidi_info_get_name( info ), ctl_name );
                }

                snd_rawmidi_info_set_stream( info, SND_RAWMIDI_STREAM_INPUT );
                if ( snd_ctl_rawmidi_info( ctl, info ) >= 0) {
                    device->has_midi = true;
                    device->has_midi_in = true;
                    printf( "Midi In Card %d, Device %d: %s (%s)\n", 
                           card, device_num, snd_rawmidi_info_get_name( info ), ctl_name );
                }
            }


            while ( snd_ctl_pcm_next_device( ctl, &device_num ) >= 0 && device_num >= 0 ) {
                printf( "pcm device\n" );
                snd_pcm_info_t *info;
                snd_pcm_info_alloca( &info );
                snd_pcm_info_set_device( info, device_num );
                
                // Check both input and output streams
                snd_pcm_info_set_stream( info, SND_PCM_STREAM_PLAYBACK );
                if ( snd_ctl_pcm_info( ctl, info ) >= 0 ) {
                    device->has_audio = true;
                    device->has_audio_out = true;
                    printf( "PCM Playback Card %d, Device %d: %s (%s)\n", 
                           card, device_num, snd_pcm_info_get_name( info ), ctl_name );
                    sprintf( device->snd_card_playback_name, "hw:%d,%d", card, device_num );
                }
                snd_pcm_info_set_stream( info, SND_PCM_STREAM_CAPTURE );
                if ( snd_ctl_pcm_info( ctl, info ) >= 0 ) {
                    device->has_audio = true;
                    device->has_audio_in = true;
                    printf( "PCM Capture Card %d, Device %d: %s (%s)\n", 
                           card, device_num, snd_pcm_info_get_name( info ), ctl_name );
                    sprintf( device->snd_card_capture_name, "hw:%d,%d", card, device_num );
                }
            }

            snd_ctl_close( ctl );
        } else {
            printf( "snd_ctl_open failed\n" );
        }
        snd_card_next( &card ); // Next card
    }
}
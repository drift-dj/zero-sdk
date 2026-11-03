#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <unistd.h>

#include <zerodj/system/fs/zdj_fs.h>
#include <zerodj/system/log/zdj_log.h>
#include <zerodj/system/settings/zdj_settings.h>
#include <zerodj/ui/widget/log/zdj_log_widget.h>

#define ZDJ_LOG_BUF_LEN 8196

zdj_log_settings_t * zdj_log_settings;

static char _log_buf[ ZDJ_LOG_BUF_LEN ];
static char _log_out[ ZDJ_LOG_BUF_LEN ];

static FILE * activity_log;
static FILE * usb_log;

void zdj_log_init( void ) {
    // printf( "zdj_log_init\n" );
    zdj_log_settings = calloc( 1, sizeof( zdj_log_settings_t ) );

    zdj_log_refresh_settings( );
}

void zdj_log_refresh_settings( void ) {
    // printf( "zdj_log_refresh_settings\n" );
    // Read stored settings
    zdj_log_settings->log_level = zdj_setting_get( ZDJ_SETTING_LOG_LEVEL )->i_val;
    zdj_log_settings->log_printf = zdj_setting_get( ZDJ_SETTING_LOG_PRINTF )->b_val;
    zdj_log_settings->log_to_file = zdj_setting_get( ZDJ_SETTING_LOG_TO_FILE )->b_val;

    // Build subject mask:
    zdj_log_settings->subject_masks = 0x0;
    zdj_log_settings->subject_masks |= ((int)zdj_setting_get( ZDJ_SETTING_LOG_CRASH )->b_val);
    zdj_log_settings->subject_masks |= (((int)zdj_setting_get( ZDJ_SETTING_LOG_PLAYBACK )->b_val) << ZDJ_LOG_PLAYBACK);
    zdj_log_settings->subject_masks |= (((int)zdj_setting_get( ZDJ_SETTING_LOG_UI )->b_val) << ZDJ_LOG_UI);
    zdj_log_settings->subject_masks |= (((int)zdj_setting_get( ZDJ_SETTING_LOG_USB )->b_val) << ZDJ_LOG_USB);
    zdj_log_settings->subject_masks |= (((int)zdj_setting_get( ZDJ_SETTING_LOG_LIBRARY )->b_val) << ZDJ_LOG_LIBRARY);
    zdj_log_settings->subject_masks |= (((int)zdj_setting_get( ZDJ_SETTING_LOG_INIT )->b_val) << ZDJ_LOG_INIT);
    zdj_log_settings->subject_masks |= (((int)zdj_setting_get( ZDJ_SETTING_LOG_FS )->b_val) << ZDJ_LOG_FS);
    zdj_log_settings->subject_masks |= (((int)zdj_setting_get( ZDJ_SETTING_LOG_RECORD )->b_val) << ZDJ_LOG_RECORDING);
    zdj_log_settings->subject_masks |= (((int)zdj_setting_get( ZDJ_SETTING_LOG_MIXER )->b_val) << ZDJ_LOG_MIXER);
    zdj_log_settings->subject_masks |= (((int)zdj_setting_get( ZDJ_SETTING_LOG_SYSTEM )->b_val) << ZDJ_LOG_SYSTEM);
    zdj_log_settings->subject_masks |= (((int)zdj_setting_get( ZDJ_SETTING_LOG_CRASH )->b_val) << ZDJ_LOG_CRASH);
}

void zdj_log( zdj_log_subject_t subject, zdj_log_level_t level, const char* fmt, ... ) {
    // printf( "zdj_log %d/%d/0x%x / [%d/0x%x]: %s\n", 
    //     level, subject, zdj_log_subject_mask[ subject ],
    //     zdj_log_settings->log_level, zdj_log_settings->subject_masks, 
    //     fmt 
    // );
    if( (level >= zdj_log_settings->log_level) && 
        (zdj_log_settings->subject_masks & zdj_log_subject_mask[ subject ]) 
    ) {
        va_list args;
        va_start( args, fmt );
        vsnprintf( _log_buf, ZDJ_LOG_BUF_LEN, fmt, args );
        va_end( args );
        snprintf( _log_out, ZDJ_LOG_BUF_LEN, "%s %s", zdj_log_prefix[subject], _log_buf );

        zdj_log_widget_append_line( _log_out );

        if( zdj_log_settings->log_printf ) { printf( "%s\n", _log_out ); }

        if( zdj_log_settings->log_to_file ) { 
            zdj_log_file_append_line( subject, _log_out );
        }
    }
}

int zdj_cur_log_num( zdj_log_subject_t subject ) {
    char log_dir[ 512 ];
    char count_path[ 512 ];
    switch ( subject ) {
        case ZDJ_LOG_USB:
            snprintf( log_dir, 512, "%s", ZDJ_USB_LOG_DIR );
            snprintf( count_path, 512, "%s", ZDJ_USB_LOG_COUNT );
            break;
        case ZDJ_LOG_CRASH_REPORT:
            snprintf( log_dir, 512, "%s", ZDJ_CRASH_LOG_DIR );
            snprintf( count_path, 512, "%s", ZDJ_CRASH_LOG_COUNT );
            break;
        default: 
            snprintf( log_dir, 512, "%s", ZDJ_ACTIVITY_LOG_DIR );
            snprintf( count_path, 512, "%s", ZDJ_ACTIVITY_LOG_COUNT );
            break;
    }

    // Create the logs dir if it's missing
    if( access( log_dir, F_OK ) != 0 ) { zdj_fs_mkdir_p( log_dir ); }
    // Open the log counter, create if missing
    int num = 0;
    FILE * count_fp = fopen( count_path, "r" );
    if( count_fp ) {
        fread( &num, sizeof( int ), 1, count_fp );
        fclose( count_fp );
    }
    return num;
}

static int _inc_current_log_num( zdj_log_subject_t subject ) {
    char log_dir[ 512 ];
    char count_path[ 512 ];
    switch ( subject ) {
        case ZDJ_LOG_USB:
            snprintf( log_dir, 512, "%s", ZDJ_USB_LOG_DIR );
            snprintf( count_path, 512, "%s", ZDJ_USB_LOG_COUNT );
            break;
        case ZDJ_LOG_CRASH_REPORT:
            snprintf( log_dir, 512, "%s", ZDJ_CRASH_LOG_DIR );
            snprintf( count_path, 512, "%s", ZDJ_CRASH_LOG_COUNT );
            break;
        default: 
            snprintf( log_dir, 512, "%s", ZDJ_ACTIVITY_LOG_DIR );
            snprintf( count_path, 512, "%s", ZDJ_ACTIVITY_LOG_COUNT );
            break;
    }

    // Create the logs dir if it's missing, fail to 0 
    if( access( log_dir, F_OK ) != 0 ) { zdj_fs_mkdir_p( log_dir ); }
    if( access( log_dir, F_OK ) != 0 ) { printf( "FAILED TO CREATE LOG DIR: %s\n", log_dir ); return 0; }
    // Open the log counter, create if missing
    int num = 0;
    FILE * count_fp = fopen( count_path, "r" );
    if( count_fp ) {
        fread( &num, sizeof( int ), 1, count_fp );
        fclose( count_fp );
    }
    // Increment and write the new num to counter
    num++;
    count_fp = fopen( count_path, "w" );
    if( count_fp ) {
        fwrite( &num, sizeof( int ), 1, count_fp );
        fclose( count_fp );
    }
    return num;
}

int zdj_new_log_num( zdj_log_subject_t subject ) {
    return _inc_current_log_num( subject );
}

void zdj_put_cur_log( zdj_log_subject_t subject, char * str_1, char * str_2, char * str_3, char * str_4 ) {
    int cur_log_num = zdj_cur_log_num( subject );

    char log_path[ 512 ];
    switch ( subject ) {
        case ZDJ_LOG_USB:
            snprintf( log_path, sizeof( log_path ), "%s/usb_log_%03d.txt", ZDJ_USB_LOG_DIR, cur_log_num );
            break;
        case ZDJ_LOG_CRASH_REPORT:
            snprintf( log_path, sizeof( log_path ), "%s/crash_log_%03d.txt", ZDJ_CRASH_LOG_DIR, cur_log_num );
            break;
        default: return;
    }

    if( access( log_path, F_OK ) != 0 ) { return; }
    FILE * log = fopen( log_path, "r" );
    if( !log ) { return; }
    
    // printf( "reading log: %s\n", log_path );
    char line[ 512 ];
    if( str_1 && fgets( line, 512, log ) ) {
        snprintf( str_1, 512, "%s", line );
    }
    if( str_2 && fgets( line, 512, log ) ) {
        snprintf( str_2, 512, "%s", line );
    }
    if( str_3 && fgets( line, 512, log ) ) {
        snprintf( str_3, 512, "%s", line );
    }
    if( str_4 && fgets( line, 512, log ) ) {
        snprintf( str_4, 512, "%s", line );
    }

    fclose( log );
}

void zdj_log_file_append_line( zdj_log_subject_t subject, char* line ) {
    FILE * log_fp;
    char log_path[ 512 ];
    switch ( subject ) {
        case ZDJ_LOG_INIT:
        case ZDJ_LOG_UI:
        case ZDJ_LOG_RECORDING:
        case ZDJ_LOG_MIXER:
        case ZDJ_LOG_SYSTEM:
        case ZDJ_LOG_LIBRARY:
        case ZDJ_LOG_PLAYBACK:
            if( !activity_log ) { 
                snprintf( 
                    log_path, 
                    sizeof( log_path ), 
                    "%s/activity_log_%03d.txt", 
                    ZDJ_ACTIVITY_LOG_DIR, 
                    zdj_new_log_num( ZDJ_LOG_ANY ) 
                );
                activity_log = fopen( log_path, "w" ); 
            }
            if( activity_log ) { log_fp = activity_log; }
            else { return; }
            break;
        case ZDJ_LOG_USB:
            if( !usb_log ) { 
                snprintf( 
                    log_path, 
                    sizeof( log_path ), 
                    "%s/usb_log_%03d.txt", 
                    ZDJ_USB_LOG_DIR, 
                    zdj_new_log_num( ZDJ_LOG_USB ) 
                );
                usb_log = fopen( log_path, "w" ); 
            }
            if( usb_log ) { log_fp = usb_log; }
            else { return; }
            break;
        default: return;
    }

    // Write log line and flush buffer
    fprintf( log_fp, "%s\n", line );
    fflush( log_fp );
    
    // Sync log to disk file
    int fd = fileno( log_fp );
    if ( fd != -1 ) { fsync( fd ); }
}
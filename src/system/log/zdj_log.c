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

static bool _put_remap_line_for_addr( char * addr, char * remap_line, FILE * remap_file );

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

        // if( zdj_log_settings->log_to_file ) { }
    }
}


int zdj_cur_log_num( zdj_log_subject_t subject ) {
    char log_dir[ 512 ];
    char count_path[ 512 ];
    switch ( subject ) {
        case ZDJ_LOG_USB:
            strcpy( log_dir, ZDJ_USB_LOG_DIR );
            strcpy( count_path, ZDJ_USB_LOG_COUNT );
            break;
        case ZDJ_LOG_LIBRARY:
            strcpy( log_dir, ZDJ_ACTIVITY_LOG_DIR );
            strcpy( count_path, ZDJ_ACTIVITY_LOG_COUNT );
            break;
        default: return 0;
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
            strcpy( log_dir, ZDJ_USB_LOG_DIR );
            strcpy( count_path, ZDJ_USB_LOG_COUNT );
            break;
        case ZDJ_LOG_LIBRARY:
            strcpy( log_dir, ZDJ_ACTIVITY_LOG_DIR );
            strcpy( count_path, ZDJ_ACTIVITY_LOG_COUNT );
            break;
        default: return 0;
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
        
        // Increment and write the new num to counter
        num++;
        count_fp = fopen( count_path, "w" );
        if( count_fp ) {
            fwrite( &num, sizeof( int ), 1, count_fp );
            fclose( count_fp );
        }
    }
    return num;
}

int zdj_new_log_num( zdj_log_subject_t subject ) {
    return _inc_current_log_num( subject );
}

void zdj_put_cur_log( zdj_log_subject_t subject, char * str_1, char * str_2, char * str_3 ) {
    int cur_log_num = zdj_cur_log_num( subject );

    char log_path[ 512 ];
    switch ( subject ) {
        case ZDJ_LOG_USB:
            sprintf( log_path, "%s/usb_log_%03d.txt", ZDJ_USB_LOG_DIR, cur_log_num );
            break;
        case ZDJ_LOG_LIBRARY:
            sprintf( log_path, "%s/lib_log_%03d.txt", ZDJ_CRASH_LOG_DIR, cur_log_num );
            break;
        default: return;
    }

    if( access( log_path, F_OK ) != 0 ) { return; }
    FILE * log = fopen( log_path, "r" );
    if( !log ) { return; }
    
    printf( "reading log: %s\n", log_path );
    char line[ 512 ];
    if( str_1 && fgets( line, 512, log ) ) {
        strcpy( str_1, line );
    }
    if( str_2 && fgets( line, 512, log ) ) {
        strcpy( str_2, line );
    }
    if( str_3 && fgets( line, 512, log ) ) {
        strcpy( str_3, line );
    }

    fclose( log );
}


// Get the most recent crash log and reformat to human-readable
void zdj_process_latest_crash_log_file( void ) {
    
    // Find crash log
    int log_num = zdj_cur_log_num( ZDJ_LOG_CRASH );
    char log_filepath[ 512 ];
    sprintf( log_filepath, "%s/crash_log_%03d.txt", ZDJ_CRASH_LOG_DIR, log_num );

    printf( "_process_latest_crash_log_file: %s\n", log_filepath );

    FILE * log_file = fopen( log_filepath, "r" );
    if ( !log_file ) { printf( "FAILED TO OPEN CRASH LOG!!!\n" ); return; }

    FILE * remap_file = fopen( "/usr/bin/zero-dj/zero-dj.remap", "r" );
    if ( !remap_file ) { printf( "FAILED TO OPEN REMAP!!!\n" ); return; }

    // If crash log isn't raw, it's already been processed, bug out
    // char first_line[ 256 ];
    // if( fgets( first_line, sizeof( first_line ), log_file ) ) {
    //     if( strncmp( first_line, "###", 3 ) == 0 ) { printf( "Processing a non-raw file!!!\n" ); return; }
    // };
    rewind( log_file );

    // Create an array of lines from the log file
    // Max out at 10 lines
    #define MAX_LINES 10
    char lines[ MAX_LINES ][ 256 ];
    char remap_lines[ MAX_LINES ][ 256 ];
    int line_counter = 0;
    
    while ( fgets( lines[ line_counter ], sizeof( lines[ line_counter ] ), log_file ) ) { 
        line_counter++; 
        if( line_counter >= MAX_LINES ){ break; }
    }

    // Rewrite each line using data from the remap file
    int output_line = 0;
    char addr_str[ 64 ];
    for( int i=0; i<line_counter; i++ ) {
        // Get address from log line
        if ( sscanf( lines[ i ], "%*[^(](%*[+]%31[^)])", addr_str ) == 1 ) {
            // Rewrite log line with function name and source file
            if( _put_remap_line_for_addr( addr_str, remap_lines[ output_line ], remap_file ) ) {
                output_line++;
            }
        }
    }

    // Close log file and re-open to overwrite
    fclose( log_file );
    log_file = fopen( log_filepath, "w" );
    if ( !log_file ) { printf( "FAILED TO OPEN CRASH LOG!!!\n" ); return; }

    // Write re-formatted lines to log
    for( int o=0; o<output_line; o++ ){  
        // printf( "%s\n", remap_lines[ o ] );
        fprintf( log_file, "%s\n", remap_lines[ o ] );
    }
}

static bool _put_remap_line_for_addr( char * addr, char * remap_line, FILE * remap_file ) {
    bool found_remap_line = false;
    // Make int from addr
    long result = strtol( addr, NULL, 16 );
    // Scan remap file until address is captured
    char line[ 512 ];
    char source_file[ 128 ];
    char function_name[ 128 ];
    long vma = 0;
    char prev_source_file[ 128 ];
    char prev_function_name[ 128 ];
    long prev_vma = 0;
    // Find first address/func in remap file
    rewind( remap_file );
    fgets( line, sizeof( line ), remap_file );
    int res = sscanf( line, "%lx %s %s", &vma, (char*)&source_file, (char*)&function_name );
    if( res == 3 ) {
        strcpy( prev_function_name, function_name );
        strcpy( prev_source_file, source_file );
        prev_vma = vma;
    }

    // Start full scan of remap file
    rewind( remap_file );
    while ( fgets( line, sizeof( line ), remap_file ) ) {
        int res = sscanf( line, "%lx %s %s", &vma, &source_file, &function_name );
        if( res == 3 ) {
            if( result > prev_vma && result < vma &&
                strncmp( prev_function_name, "_zdj_error_sig", 12 ) != 0
            ) { 
                // printf( "found line: %llx %llx %s %s\n", result, vma, source_file, function_name );
                snprintf( remap_line, 256, "%s( )    [%s/0x%lx (0x%lx + 0x%lx)]",
                    prev_function_name,
                    prev_source_file,
                    result,
                    prev_vma,
                    result - prev_vma
                );
                printf( "%s\n", remap_line );
                found_remap_line = true;
                // Exit the loop early when we find the line
                break;
            }
            strcpy( prev_function_name, function_name );
            strcpy( prev_source_file, source_file );
            prev_vma = vma;
        }
    }
    return found_remap_line;
}